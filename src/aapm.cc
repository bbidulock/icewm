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

YColor *YApm::apmBg = 0;
YColor *YApm::apmFg = 0;
YFont *YApm::apmFont = 0;

extern YPixmap *taskbackPixmap;
extern YColor *taskBarBg;


#define AC_UNKNOWN      0
#define AC_ONLINE       1
#define AC_OFFLINE      2

#define BAT_ABSENT      0
#define BAT_PRESENT     1

#define BAT_UNKNOWN     0
#define BAT_CHARGING    1
#define BAT_DISCHARGING 2


void ApmStr(char *s, bool Tool) {
    char buf[80];
    int len, i, fd = open("/proc/apm", O_RDONLY);
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
        return ;
    }

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
            warn(_("/proc/apm - unknown format (%d)"), i);
        }
        return ;
    }
    if (BATlife == -1)
        BATlife = 0;

    if (strcmp(units, "min") != 0)
        BATtime = BATtime / 60;

    if (!Tool) {
        if (taskBarShowApmTime) { // mschy
            if (BATtime == -1) {
                // -1 indicates that apm-bios can't
                // calculate time for akku
                // no wonder -> we're plugged!
                sprintf(s, "%d%%", BATlife);
            } else {
                sprintf(s, "%d:%02d", BATtime/60, (BATtime)%60);
            }
        } else
            sprintf(s, "%d%%", BATlife);
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
    }
    fclose(fd);

    for (i = 0; i < batteryNum; i++) {
        BATname = acpiBatteryNames[i];
        //assign some default values, in case
        //the files in /proc/acpi will contain unexpected values
        BATpresent = -1;
        BATstatus = -1;
        BATcapacity_full = -1;
        BATcapacity_remain = -1;
        BATrate = -1;
        BATtime_remain = -1;

        strcat3(buf, "/proc/acpi/battery/", BATname, "/info", sizeof(buf));
        fd = fopen(buf, "r");
        if (fd != NULL) {
            while (fgets(buf, sizeof(buf), fd)) {
                if (strncasecmp(buf, "present:", 8) == 0) {
                    sscanf(buf + 8, "%s", buf2);
                    if (strncasecmp(buf2, "yes", 3) == 0) {
                        BATpresent = BAT_PRESENT;
                    }
                    else {
                        BATpresent = BAT_ABSENT;
                    }
                }
                else if (strncasecmp(buf, "last full capacity:", 19) == 0) {
                    //may contain non-numeric value
                    if (sscanf(buf + 19, "%d", &BATcapacity_full)<=0) {
                        BATcapacity_full = -1;
                    }
                }
            }
            fclose(fd);
        }

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
            }
            fclose(fd);
        }

        if (!Tool &&
            taskBarShowApmTime &&
            BATpresent == BAT_PRESENT &&
            //bios calculates remaining time, only while discharging
            BATstatus == BAT_DISCHARGING &&
            //did we parse the needed values successfully?
            BATcapacity_full >= 0 && BATcapacity_remain >= 0 && BATrate >= 0) {
            BATtime_remain = (int) (60 * (double)(BATcapacity_remain) / BATrate);
            sprintf(bat_info, "%d:%02d", BATtime_remain / 60, BATtime_remain % 60);
        }
        else if (BATpresent == BAT_PRESENT &&
                 //did we parse the needed values successfully?
                 BATcapacity_remain >= 0 && BATcapacity_full >= 0)
        {
            sprintf(bat_info, "%.0f%%",
                    100 * (double)BATcapacity_remain / BATcapacity_full);
        }
        else {
            //battery is absent or we didn't parse some needed values
            sprintf(bat_info, "0%%");
        }

        if (BATstatus == BAT_CHARGING) {
            if (Tool)
                strcat(bat_info, _(" - Charging"));
            else
                strcat(bat_info, _("C"));
        }

        if (i>0) {
            if (Tool)
                strcat(s, " / ");
            else
                strcat(s, "/");
        }
        strcat(s, bat_info);
    }

    if (ACstatus == AC_ONLINE) {
        if (Tool)
            strcat(s,_(" - Power"));
        else {
            if (!prettyClock) strcat(s, " ");
            strcat(s,_("P"));
        }
    }

}

YApm::YApm(YWindow *aParent): YWindow(aParent) {
    struct dirent **de;
    int n, i;

    //search for acpi info first
    n = scandir("/proc/acpi/battery", &de, 0, alphasort);
    if (n > 0) {
        //use acpi info
        acpiMode = 1;

        //scan for batteries
        batteryNum = 0;
        i = 0;
        while (i < n && batteryNum < MAX_ACPI_BATTERY_NUM) {
            if (!ignore_directory_bat_entry(de[i])) {
                //found a battery
                acpiBatteryNames[batteryNum] =
                    (char*)calloc(strlen(de[i]->d_name) + 1, sizeof(char));
                strcpy(acpiBatteryNames[batteryNum], de[i]->d_name);
                batteryNum++;
            }
            free(de[i]);
            i++;
        }
        free(de);

        //scan for first ac_adapter
        n = scandir("/proc/acpi/ac_adapter", &de, 0, alphasort);
        i = 0; acpiACName = 0;
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
        if (!acpiACName) {
            //no ac_adapter was found
            acpiACName = (char*)malloc(sizeof(char));
            *acpiACName = '\0';
        }
        else {
        }
        free(de);

    }
    else {
        //use apm info
        acpiMode = 0;
        batteryNum = 1;
    }

    if (apmBg == 0 && *clrApm) apmBg = new YColor(clrApm);
    if (apmFg == 0) apmFg = new YColor(clrApmText);
    if (apmFont == 0) apmFont = YFont::getFont(apmFontName);

    apmTimer = new YTimer(2000);
    apmTimer->setTimerListener(this);
    apmTimer->startTimer();
    setSize(calcInitialWidth(), 20);
    updateToolTip();
    // setDND(true);
}

YApm::~YApm() {
    int i;
    delete apmTimer; apmTimer = 0;
    if (acpiMode) {
        for (i=0; i<batteryNum; i++) {
            free(acpiBatteryNames[i]);
            acpiBatteryNames[i] = 0;
        }
        batteryNum = 0;
        delete acpiACName; acpiACName = 0;
    }
}

void YApm::updateToolTip() {
    char s[80]={' ',' ',' ', 0, 0, 0, 0};

    if (acpiMode)
        AcpiStr(s,1);
    else
        ApmStr(s,1);

    setToolTip(s);
}

int YApm::calcInitialWidth() {
    char buf[80] = { 0};
    int i;

    //estimate applet's size
    for (i = 0; i < batteryNum; i++) {
        if (taskBarShowApmTime)
            strcat(buf, "0:00");
        else
            strcat(buf, "100%");
        strcat(buf, _("C"));
        if (i > 0)
            strcat(buf, _("/"));
    }
    if (!prettyClock) strcat(buf, " ");
    strcat(buf, "P");

    return calcWidth(buf, strlen(buf));
}

void YApm::paint(Graphics &g, const YRect &/*r*/) {
    unsigned int x = 0;
    char s[30]={' ',' ',' ',0,0,0,0,0};
    int len,i;

    if (acpiMode)
        AcpiStr(s,0);
    else
        ApmStr(s,0);
    len=strlen(s);

    //clean background of current size first, so that
    //it is possible to use transparent apm-background
#ifdef CONFIG_GRADIENTS
    class YPixbuf * gradient(parent()->getGradient());

    if (gradient) {
        g.copyPixbuf(*gradient, this->x(), this->y(), width(), height(), 0, 0);
    }
    else
#endif
    if (taskbackPixmap) {
        g.fillPixmap(taskbackPixmap,
                     0, 0, width(), height(),
                     this->x(), this->y());
    }
    else {
        g.setColor(taskBarBg);
        g.fillRect(0, 0, width(), height());
    }

    unsigned int new_width = calcWidth(s, len);
    unsigned int old_width = width();

    //if enlarging then resize immediately, to avoid
    //of a delay until the added rectangle is drawed
    if (new_width>old_width) setSize(new_width, 20);

    //draw foreground
    if (prettyClock) {
        YPixmap *p;

        for (i = 0; x < new_width; i++) {
            if (i < len) {
                p = getPixmap(s[i]);
            } else p = PixSpace;

            if (p) {
                g.drawPixmap(p, x, 0);
                x += p->width();
            } else if (i >= len) {
                g.setColor(apmBg);
                g.fillRect(x, 0, new_width - x, height());
                break;
            }
        }
    } else {
        if (apmBg) {
            g.setColor(apmBg);
            g.fillRect(0, 0, new_width, height());
        }

        int y = (height() - 1 - apmFont->height()) / 2 + apmFont->ascent();
        g.setColor(apmFg);
        g.setFont(apmFont);
        g.drawChars(s, 0, len, 2, y);
    }

    //if diminishing then resize only at the end, to avoid
    //of a delay until the removed rectangle is cleared
    if (new_width < old_width)
        setSize(new_width, 20);
}

bool YApm::handleTimer(YTimer *t) {
    if (t != apmTimer) return false;

    if (toolTipVisible())
        updateToolTip();
    repaint();
    return true;
}

YPixmap *YApm::getPixmap(char c) {
    YPixmap *pix = 0;
    switch (c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': pix = PixNum[c - '0']; break;
    case ' ': pix = PixSpace; break;
    case ':': pix = PixColon; break;
    case '.': pix = PixDot; break;
    case 'P': pix = PixP; break;
    case 'M': pix = PixM; break;
    case '/': pix = PixSlash; break;
    }

    return pix;
}

int YApm::calcWidth(const char *s, int count) {
    if (!prettyClock)
        //leave 2px space on both sides
        return apmFont->textWidth(s, count) + 4;
    else {
        int len = 0;

        while (count--) {
            YPixmap *p = getPixmap(*s++);
            if (p) len += p->width();
        }
        return len;
    }
}
#endif
