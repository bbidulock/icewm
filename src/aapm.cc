/*
 * IceWM
 *
 * Copyright (C) 1999 Andreas Hinz
 *
 * Mostly derrived from aclock.cc and aclock.h by Marko Macek
 *
 * Apm status
 */

#define NEED_TIME_H

#include "config.h"
#include "aapm.h"

#ifdef CONFIG_APPLET_APM

#include "ylib.h"
#include "sysdep.h"

#include "aclock.h"

#include "yapp.h"
#include "prefs.h"
#include "intl.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#ifdef __FreeBSD__
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <machine/apm_bios.h>
#endif

#ifdef __NetBSD__
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <machine/apmvar.h>
#endif

YColor *YApm::apmBg = 0;
YColor *YApm::apmFg = 0;
ref<YFont> YApm::apmFont;

extern ref<YPixmap> taskbackPixmap;
static YColor *taskBarBg = 0;


#define AC_UNKNOWN      0
#define AC_ONLINE       1
#define AC_OFFLINE      2

#define BAT_ABSENT      0
#define BAT_PRESENT     1

#define BAT_UNKNOWN     0
#define BAT_CHARGING    1
#define BAT_DISCHARGING 2


void ApmStr(char *s, bool Tool) {
#ifdef __FreeBSD__
    struct apm_info ai;
#elif defined __NetBSD__
    struct apm_power_info ai;
#else
    char buf[80];
#endif
    int len, i, fd = open(APMDEV, O_RDONLY);
    char driver[16];
    char apmver[16];
    int apmflags;
    int ACstatus=0;
    int BATstatus=0;
    int BATflag=0;
    int BATlife=1;
    int BATtime;
    char units[16];

    if (fd == -1) {
        static int error = 0;
        if (!error)
            perror("Can't open the apm device");
        error = 1;
        return ;
    }
#ifdef __FreeBSD__
    if (ioctl(fd,APMIO_GETINFO, &ai) == -1)
    {
        static int error = 0;
        if (!error)
            perror("Can't ioctl the apm device");
        error = 1;
        close(fd);
        return;
    }
    close(fd);

    sprintf(apmver, "%u.%u", ai.ai_major, ai.ai_minor);
    ACstatus = ai.ai_acline;
    BATflag = ai.ai_batt_stat == 3 ? 8 : 0;
    BATlife = ai.ai_batt_life;
    BATtime = ai.ai_batt_time == 0 ? -1 : ai.ai_batt_time;
    strcpy(units, "sec");
#elif defined __NetBSD__
    memset(&ai, 0, sizeof(ai));
    if (ioctl(fd, APM_IOC_GETPOWER, &ai) == -1)
    {
	perror("Cannot ioctl the apm device");
        close(fd);
        return;
    }
    close(fd);

    strcpy(apmver, "?.?");
    ACstatus = (ai.ac_state == APM_AC_ON) ? 1 : 0;
    BATflag = (ai.battery_state == APM_BATT_CHARGING) ? 8 : 0;
    BATlife = ai.battery_life;
    BATtime = (ai.minutes_left == 0) ? -1 : ai.minutes_left;
    strcpy(units, "min");
#else
    len = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    buf[len] = 0;

    if ((i = sscanf(buf, "%s %s 0x%x 0x%x 0x%x 0x%x %d%% %d %s",
                    driver, apmver, &apmflags,
                    &ACstatus, &BATstatus, &BATflag, &BATlife,
                    &BATtime, units)) != 9)
    {
        static int error = 1;
        if (error) {
            error = 0;
            warn(_("%s - unknown format (%d)"), APMDEV, i);
        }
        return ;
    }
#endif
    if (BATlife == -1)
        BATlife = 0;

    if (strcmp(units, "min") != 0 && BATtime != -1)
        BATtime /= 60;

    if (!Tool) {
        if (taskBarShowApmTime) { // mschy
            if (BATtime == -1) {
                // -1 indicates that apm-bios can't
                // calculate time for akku
                // no wonder -> we're plugged!
                sprintf(s, "%3d%%", BATlife);
            } else {
                sprintf(s, "%d:%02d", BATtime/60, (BATtime)%60);
            }
        } else
            sprintf(s, "%3d%%", BATlife);
#if 0
        while ((i < 3) && (buf[28 + i] != '%'))
            i++;
        for (len = 2; i; i--, len--) {
            *(s + len) = buf[28 + i - 1];
        }
#endif
    } else {
        sprintf(s, "%d%%", BATlife);
    }



    if (ACstatus == 0x1)
        if (Tool)
            strcat(s, _(" - Power"));
        else
            strcat(s, _("P"));
    if ((BATflag & 8))
        if (Tool)
            strcat(s, _(" - Charging"));
        else
            strcat(s, _("C"));
}

int ignore_directory_entry(struct dirent *de) {
    return
        !strcmp(de->d_name, ".") || \
        !strcmp(de->d_name, "..");
}

void strcat3(char* dest,
             const char* src1,
             const char* src2,
             const char* src3,
             size_t n)
{
    if (!dest) return;
    int len = n;
    *dest = '\0';
    len -= 1;
    if (len < 0) return;
    strncat(dest, src1, len);
    len -= strlen(src1);
    if (len < 0) return;
    strncat(dest, src2, len);
    len -= strlen(src2);
    if (len < 0) return;
    strncat(dest, src3, len);
}

int YApm::ignore_directory_bat_entry(struct dirent *de) {
    return
        ignore_directory_entry(de) || \
        (acpiIgnoreBatteries &&
         strstr(acpiIgnoreBatteries, de->d_name));
}

void YApm::AcpiStr(char *s, bool Tool) {
    char buf[80], buf2[80], bat_info[250];
    FILE *fd;
    //name of the battery
    char *BATname;
    //battery is present or absent
    int BATpresent;
    //battery status charging/discharging/unknown
    int BATstatus;
    //maximal battery capacity (mWh)
    int BATcapacity_full;
    //design capacity (mAh)
    int BATcapacity_design;
    //current battery capacity (mWh)
    int BATcapacity_remain;
    //how much energy is used atm (mW)
    int BATrate;
    //time until the battery is discharged (min)
    int BATtime_remain;
    //status of ac-adapter online/offline
    int ACstatus;
    int i;

    *s='\0';

    //assign some default values, in case
    //the file in /proc/acpi will contain unexpected values
    ACstatus = -1;
    if (acpiACName && acpiACName[0] != 0) {
        strcat3(buf, "/proc/acpi/ac_adapter/", acpiACName, "/state", sizeof(buf));
        fd = fopen(buf, "r");
        if (fd == NULL) {
            //try older /proc/acpi format
            strcat3(buf, "/proc/acpi/ac_adapter/", acpiACName, "/status", sizeof(buf));
            fd = fopen(buf, "r");
        }
        if (fd != NULL) {
            while (fgets(buf, sizeof(buf), fd)) {
                if ((strncasecmp(buf, "state:", 6) == 0 &&
                     sscanf(buf + 6, "%s", buf2) > 0) ||
                    //older /proc/acpi format
                    (strncasecmp(buf, "Status:", 7) == 0 &&
                     sscanf(buf + 7, "%s", buf2) > 0)) {
                    if (strncasecmp(buf2, "on-line", 7) == 0) {
                        ACstatus = AC_ONLINE;
                    }
                    else if (strncasecmp(buf2, "off-line", 8) == 0) {
                        ACstatus = AC_OFFLINE;
                    }
                    else {
                        ACstatus = AC_UNKNOWN;
                    }
                }
            }
            fclose(fd);
        }
    }

    int n = 0;
    for (i = 0; i < batteryNum; i++) {
        BATname = acpiBatteries[i]->name;
        //assign some default values, in case
        //the files in /proc/acpi will contain unexpected values
        BATpresent = -1;
        BATstatus = -1;
        BATcapacity_full = -1;
        BATcapacity_design = -1;
        BATcapacity_remain = -1;
        BATrate = -1;
        BATtime_remain = -1;

        strcat3(buf, "/proc/acpi/battery/", BATname, "/state", sizeof(buf));
        fd = fopen(buf, "r");
        if (fd == NULL) {
            //try older /proc/acpi format
            strcat3(buf, "/proc/acpi/battery/", BATname, "/status", sizeof(buf));
            fd = fopen(buf, "r");
        }
        if (fd != NULL) {
            while (fgets(buf, sizeof(buf), fd)) {
                if ((strncasecmp(buf, "charging state:", 15) == 0 &&
                     sscanf(buf + 15, "%s", buf2) > 0) ||
                    //older /proc/acpi format
                    (strncasecmp(buf, "State:", 6) == 0 &&
                     sscanf(buf + 6, "%s", buf2) > 0)) {
                    if (strncasecmp(buf2, "charging", 8) == 0) {
                        BATstatus = BAT_CHARGING;
                    }
                    else if (strncasecmp(buf2, "discharging", 11) == 0) {
                        BATstatus = BAT_DISCHARGING;
                    }
                    else {
                        BATstatus = BAT_UNKNOWN;
                    }
                }
                else if (strncasecmp(buf,"present rate:", 13) == 0) {
                    //may contain non-numeric value
                    if (sscanf(buf+13,"%d", &BATrate) <= 0) {
                        BATrate = -1;
                    }
                }
                else if (strncasecmp(buf,"remaining capacity:", 19) == 0) {
                    //may contain non-numeric value
                    if (sscanf(buf+19,"%d", &BATcapacity_remain) <= 0) {
                        BATcapacity_remain = -1;
                    }
                }
                else if (strncasecmp(buf, "present:", 8) == 0) {
                    sscanf(buf + 8, "%s", buf2);
                    if (strncasecmp(buf2, "yes", 3) == 0) {
                        BATpresent = BAT_PRESENT;
                    }
                    else {
                        BATpresent = BAT_ABSENT;
                    }
                }
            }
            fclose(fd);
        }

        if (BATpresent == BAT_PRESENT) { //battery is present now
            if (acpiBatteries[i]->present == BAT_ABSENT) { //and previously was absent
                //read full-capacity value
                strcat3(buf, "/proc/acpi/battery/", BATname, "/info", sizeof(buf));
                fd = fopen(buf, "r");
                if (fd != NULL) {
                    while (fgets(buf, sizeof(buf), fd)) {
                        if (strncasecmp(buf, "design capacity:", 16) == 0) {
                            //may contain non-numeric value
                            if (sscanf(buf, "%*[^0-9]%d", &BATcapacity_design)<=0) {
                                BATcapacity_design = -1;
                            }
                        }
                        if (strncasecmp(buf, "last full capacity:", 19) == 0) {
                            //may contain non-numeric value
                            if (sscanf(buf, "%*[^0-9]%d", &BATcapacity_full)<=0) {
                                BATcapacity_full = -1;
                            }
                        }
                    }
                    fclose(fd);
                    if (BATcapacity_remain > BATcapacity_full && BATcapacity_design > 0)
                        BATcapacity_full = BATcapacity_design;
                }
                acpiBatteries[i]->capacity_full = BATcapacity_full;
            }
            else {
                BATcapacity_full = acpiBatteries[i]->capacity_full;
            }
        }
        acpiBatteries[i]->present = BATpresent;

        if (!Tool &&
            taskBarShowApmTime &&
            BATpresent == BAT_PRESENT &&
            //bios calculates remaining time, only while discharging
            BATstatus == BAT_DISCHARGING &&
            //did we parse the needed values successfully?
            BATcapacity_full >= 0 && BATcapacity_remain >= 0 && BATrate > 0) {
            BATtime_remain = (int) (60 * (double)(BATcapacity_remain) / BATrate);
            sprintf(bat_info, "%d:%02d", BATtime_remain / 60, BATtime_remain % 60);
        }
        else if (BATpresent == BAT_PRESENT &&
                 //did we parse the needed values successfully?
                 BATcapacity_remain >= 0 && BATcapacity_full >= 0)
        {
            sprintf(bat_info, "%3.0f%%",
                    100 * (double)BATcapacity_remain / BATcapacity_full);
        }
        else {
            //battery is absent or we didn't parse some needed values
            bat_info[0] = 0;
        }

        if (BATstatus == BAT_CHARGING) {
            if (Tool)
                strcat(bat_info, _(" - Charging"));
            else
                strcat(bat_info, _("C"));
        }

        if ((n > 0) && (*bat_info)) {
            if (Tool)
                strcat(s, " / ");
            else
                strcat(s, "/");
            n++;
        }
        strcat(s, bat_info);
    }

    if (ACstatus == AC_ONLINE) {
        if (Tool)
            strcat(s,_(" - Power"));
        else {
///            if (!prettyClock) strcat(s, " ");
            strcat(s,_("P"));
        }
    }

}

void YApm::PmuStr(char *s, const bool tool_tip)
{
   #warning Why the string s is "  "?

   FILE *fd = fopen("/proc/pmu/info", "r");
   if (fd == NULL) {
      strcpy(s, "Err");
      // this is somewhat difficult, because pmu support seams to be gone
      return;
   }
   
   char line[80];
   int power_present;
   while ( fgets(line, 80, fd) != NULL )
     if (strncmp("AC Power", line, strlen("AC Power")) == 0) {
	sscanf(strchr(line, ':')+2, "%d", &power_present);
	break;
     }
   fclose(fd);

   char* s_end = s;
   for (int i=0; i < batteryNum; ++i) {
      char file_name[20];
      snprintf(file_name, 20, "/proc/pmu/battery_%d", i);
      fd = fopen(file_name, "r");
      if (fd == NULL) {
	 strcpy(s_end, "Err");
	 s_end += 3;
	 continue;
      }

      int flags=0, rem_time=-1, charge=0, max_charge=0, voltage=0;
      while ( fgets(line, 80, fd) != NULL )
	if (strncmp("flags", line, strlen("flags")) == 0)
	  sscanf(strchr(line, ':')+2, "%x", &flags);
        else if (strncmp("time rem.", line, strlen("time rem.")) == 0)
	  sscanf(strchr(line, ':')+2, "%d", &rem_time);
        else if (strncmp("charge", line, strlen("charge")) == 0)
	  sscanf(strchr(line, ':')+2, "%d", &charge);
        else if (strncmp("max_charge", line, strlen("max_charge")) == 0)
	  sscanf(strchr(line, ':')+2, "%d", &max_charge);
        else if (strncmp("voltage", line, strlen("voltage")) == 0)
	  sscanf(strchr(line, ':')+2, "%d", &voltage);
      fclose(fd);

      const bool battery_present = flags & 0x1,
	battery_charging = (flags & 0x2),
	battery_full = !battery_charging && power_present,
	time_in_min= rem_time>600;

      if (time_in_min)
	rem_time /= 60;
      
      if (tool_tip) {
	 if (i > 0) {
	    strcpy(s_end, " / ");
	    s_end += 3;
	 }

	 if (battery_present) {
	    if ( !battery_full )
	      s_end += sprintf(s_end, " %d%c%02d%s", rem_time/60,
			       time_in_min?':':'.', rem_time%60,
			       time_in_min?"h":"m");
	    
	    s_end += sprintf(s_end, " %.0f%% %d/%dmAh %.3fV", 100.0*charge/max_charge,
		     charge, max_charge, voltage/1e3);

	    if (battery_charging)
	      s_end += sprintf(s_end, _(" - Charging"));
	 } else {
	   // Battery not present
	    strcpy(s_end, "--");
	    s_end += 2;
	 }
      } else {  // Taskbar
	 if (i > 0)
	   strcpy(s_end++, "/");

	 if (! battery_present ) {
	    strcpy(s_end, "--");
	    s_end += 2;
	 } else if (taskBarShowApmTime && !battery_full)
	   s_end += sprintf(s_end, "%d%c%02d", rem_time/60,
			    time_in_min?':':'.', rem_time%60);
	 else
	   s_end += sprintf(s_end, "%3.0f%%", 100.0*charge/max_charge);

	 if (battery_charging)
	    strcpy(s_end++, "C");
      }
   }

   if (power_present) {
      if (tool_tip)
	strcpy(s_end, _(" - Power"));
      else
	strcpy(s_end, "P");
   }
}

YApm::YApm(YWindow *aParent): YWindow(aParent) {
    struct dirent **de;
    int n, i;
    FILE *pmu_info;

    batteryNum = 0;
    acpiACName = 0;
    fCurrentState = 0;

    //search for acpi info first
    n = scandir("/proc/acpi/battery", &de, 0, alphasort);
    if (n > 0) {
        //use acpi info
        mode = ACPI;

        //scan for batteries
        i = 0;
        while (i < n && batteryNum < MAX_ACPI_BATTERY_NUM) {
            if (!ignore_directory_bat_entry(de[i])) {
                //found a battery
                acpiBatteries[batteryNum] =
                    (bat_info*)malloc(sizeof(bat_info));
                acpiBatteries[batteryNum]->name =
                    (char*)calloc(strlen(de[i]->d_name) + 1, sizeof(char));
                strcpy(acpiBatteries[batteryNum]->name, de[i]->d_name);
                //initially set as absent, to force reading of
                //full-capacity value
                acpiBatteries[batteryNum]->present = BAT_ABSENT;
                acpiBatteries[batteryNum]->capacity_full = -1;
                batteryNum++;
            }
            free(de[i]);
            i++;
        }
        free(de);

        //scan for first ac_adapter
        n = scandir("/proc/acpi/ac_adapter", &de, 0, alphasort);
        if (n > 0) {
            i = 0;
            while (i < n) {
                if (!ignore_directory_entry(de[i])) {
                    //found an ac_adapter
                    acpiACName = (char*)calloc(strlen(de[i]->d_name) + 1, sizeof(char));
                    strcpy(acpiACName, de[i]->d_name);
                    break;
                }
                free(de[i]);
                i++;
            }
            free(de);
        }
        if (!acpiACName) {
            //no ac_adapter was found
            acpiACName = (char*)malloc(sizeof(char));
            *acpiACName = '\0';
        }

    } else if ( (pmu_info = fopen("/proc/pmu/info", "r")) != NULL) {
       mode = PMU;
       char line[80];
       while ( fgets(line, 80, pmu_info) != NULL )
	 if (strncmp("Battery count", line, strlen("Battery count")) == 0)
           sscanf(strchr(line, ':')+2, "%d", &batteryNum);

       fclose(pmu_info);
    } else {
       //use apm info
       mode = APM;
       batteryNum = 1;
    }

    if (apmBg == 0 && *clrApm) apmBg = new YColor(clrApm);
    if (apmFg == 0) apmFg = new YColor(clrApmText);
    if (apmFont == null) apmFont = YFont::getFont(XFA(apmFontName));
    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }
    updateState();

    apmTimer = new YTimer(1000 * batteryPollingPeriod);
    apmTimer->setTimerListener(this);
    apmTimer->startTimer();
    setSize(calcInitialWidth(), 20);
    updateToolTip();
    // setDND(true);
}

YApm::~YApm() {
    int i;
    delete apmTimer; apmTimer = 0;
    if (ACPI == mode) {
        for (i=0; i<batteryNum; i++) {
            free(acpiBatteries[i]->name);
            free(acpiBatteries[i]);
            acpiBatteries[i] = 0;
        }
        batteryNum = 0;
        delete acpiACName; acpiACName = 0;
    }
}

void YApm::updateToolTip() {
    setToolTip(fCurrentState);
}

int YApm::calcInitialWidth() {
    char buf[80] = { 0 };
    int i;
    int n = 0;

    //estimate applet's size
    for (i = 0; i < batteryNum; i++) {
        if (acpiBatteries[i]->present == BAT_ABSENT)
            continue;
        if (taskBarShowApmTime)
            strcat(buf, "0:00");
        else
            strcat(buf, "100%");
        strcat(buf, "C");
        if (n > 0)
            strcat(buf, "/");
        n++;
    }
///    if (!prettyClock) strcat(buf, " ");
    strcat(buf, "P");

    return calcWidth(buf, strlen(buf));
}

void YApm::updateState() {
    char s[64] = {' ', ' ', ' ', 0, 0, 0, 0, 0};

    switch (mode) {
    case ACPI:
        AcpiStr(s,0);
        break;
    case APM:
        ApmStr(s,0);
        break;
    case PMU:
        PmuStr(s, 0);
        break;
    }

    if (fCurrentState != 0)
        free(fCurrentState);
    fCurrentState = strdup(s);
}

void YApm::paint(Graphics &g, const YRect &/*r*/) {
    unsigned int x = 0;
    int len, i;
   
    len = fCurrentState ? strlen(fCurrentState) : 0;

    //clean background of current size first, so that
    //it is possible to use transparent apm-background
#ifdef CONFIG_GRADIENTS
    ref<YPixbuf> gradient(parent()->getGradient());

    if (gradient != null) {
        g.copyPixbuf(*gradient, this->x(), this->y(), width(), height(), 0, 0);
    }
    else
#endif
    if (taskbackPixmap != null) {
        g.fillPixmap(taskbackPixmap,
                     0, 0, width(), height(),
                     this->x(), this->y());
    }
    else {
        g.setColor(taskBarBg);
        g.fillRect(0, 0, width(), height());
    }

    unsigned int new_width = width(); //calcWidth(s, len);
///    unsigned int old_width = width();

#if 0
    //if enlarging then resize immediately, to avoid
    //of a delay until the added rectangle is drawed
    if (new_width>old_width) setSize(new_width, 20);
#endif

    //draw foreground
    if (prettyClock) {
        ref<YPixmap> p;

        for (i = 0; x < new_width; i++) {
            if (i < len) {
                p = getPixmap(fCurrentState[i]);
            } else
                p = PixSpace;

            if (p != null) {
                g.drawPixmap(p, x, 0);
                x += p->width();
            } else if (i >= len) {
                g.setColor(apmBg);
                g.fillRect(x, 0, new_width - x, height());
                break;
            }
        }
    } else if (fCurrentState) {
        if (apmBg) {
            g.setColor(apmBg);
            g.fillRect(0, 0, new_width, height());
        }

        int y = (height() - 1 - apmFont->height()) / 2 + apmFont->ascent();
        g.setColor(apmFg);
        g.setFont(apmFont);
        g.drawChars(fCurrentState, 0, len, 2, y);
    }

    //if diminishing then resize only at the end, to avoid
    //of a delay until the removed rectangle is cleared
#if 0
    if (new_width < old_width)
        setSize(new_width, 20);
#endif
}

bool YApm::handleTimer(YTimer *t) {
    if (t != apmTimer) return false;

    updateState();

    if (toolTipVisible())
        updateToolTip();
    repaint();
    return true;
}

ref<YPixmap> YApm::getPixmap(char c) {
    ref<YPixmap> pix;
    switch (c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': pix = PixNum[c - '0']; break;
    case ' ': pix = PixSpace; break;
    case ':': pix = PixColon; break;
    case '.': pix = PixDot; break;
    case 'P': pix = PixP; break;
    case 'M': pix = PixM; break;
    case '/': pix = PixSlash; break;
    case '%': pix = PixPercent; break;
    }

    return pix;
}

int YApm::calcWidth(const char *s, int count) {
    if (!prettyClock)
        //leave 2px space on both sides
        return (apmFont != null ? apmFont->textWidth(s, count) : 0) + 4;
    else {
        int len = 0;

        while (count--) {
            ref<YPixmap> p = getPixmap(*s++);
            if (p != null)
                len += p->width();
        }
        return len;
    }
}
#endif
