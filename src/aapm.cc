/*
 * IceWM
 *
 * Copyright (C) 1999 Andreas Hinz
 *
 * Mostly derived from aclock.cc and aclock.h by Marko Macek
 *
 * Apm status
 */

#define NEED_TIME_H

#include "config.h"
#include "aapm.h"

#include "sysdep.h"
#include "wpixmaps.h"
#include "prefs.h"
#include "intl.h"
#include "udir.h"

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#ifdef i386
#include <machine/apm_bios.h>
#endif
#include <sys/sysctl.h>
#include <dev/acpica/acpiio.h>
#endif

#ifdef __NetBSD__
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <machine/apmvar.h>
#endif

#include <math.h>

extern YColorName taskBarBg;

#define AC_UNKNOWN      0
#define AC_ONLINE       1
#define AC_OFFLINE      2

#define BAT_ABSENT      false
#define BAT_PRESENT     true

#define BAT_UNKNOWN     0
#define BAT_CHARGING    1
#define BAT_DISCHARGING 2
#define BAT_FULL        3

#define SYS_STR_SIZE    64
#define APM_LINE_LEN    80

void YApm::ApmStr(char *s, bool Tool) {
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && defined(i386)
    struct apm_info ai;
#elif defined __NetBSD__
    struct apm_power_info ai;
#else
    char buf[APM_LINE_LEN];
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
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && defined(i386)
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

    snprintf(apmver, sizeof apmver, "%u.%u", ai.ai_major, ai.ai_minor);
    ACstatus = ai.ai_acline;
    BATflag = ai.ai_batt_stat == 3 ? 8 : 0;
    BATlife = ai.ai_batt_life;
    BATtime = ai.ai_batt_time == 0 ? -1 : ai.ai_batt_time;
    strlcpy(units, "sec", sizeof units);
#elif defined __NetBSD__
    memset(&ai, 0, sizeof(ai));
    if (ioctl(fd, APM_IOC_GETPOWER, &ai) == -1)
    {
        perror("Cannot ioctl the apm device");
        close(fd);
        return;
    }
    close(fd);

    strlcpy(apmver, "?.?", sizeof apmver);
    ACstatus = (ai.ac_state == APM_AC_ON) ? 1 : 0;
    BATflag = (ai.battery_state == APM_BATT_CHARGING) ? 8 : 0;
    BATlife = ai.battery_life;
    BATtime = (ai.minutes_left == 0) ? -1 : ai.minutes_left;
    strlcpy(units, "min", sizeof units);
#else
    len = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    buf[len] = 0;

    acIsOnLine     = (ACstatus == 0x1);
    energyFull = energyNow = 0;

    if ((i = sscanf(buf, "%s %s 0x%x 0x%x 0x%x 0x%x %d%% %d %s",
                    driver, apmver, &apmflags,
                    &ACstatus, &BATstatus, &BATflag, &BATlife,
                    &BATtime, units)) != 9)
    {
        static int error = 1;
        if (error) {
            error = 0;
            warn("%s - unknown format (%d)", APMDEV, i);
        }
        return ;
    }
#endif
    if (BATlife == -1)
        BATlife = 0;

    if (strcmp(units, "min") != 0 && BATtime != -1)
        BATtime /= 60;

    energyNow += BATlife;
    energyFull += 100;

    if (!Tool) {
        if (taskBarShowApmTime) { // mschy
            if (BATtime == -1) {
                // -1 indicates that apm-bios can't
                // calculate time for akku
                // no wonder -> we're plugged!
                snprintf(s, SYS_STR_SIZE, "%3d%%", BATlife);
            } else {
                snprintf(s, SYS_STR_SIZE, "%d:%02d", BATtime/60, (BATtime)%60);
            }
        } else
            snprintf(s, SYS_STR_SIZE, "%3d%%", BATlife);
#if 0
        while ((i < 3) && (buf[28 + i] != '%'))
            i++;
        for (len = 2; i; i--, len--) {
            *(s + len) = buf[28 + i - 1];
        }
#endif
    } else {
        snprintf(s, SYS_STR_SIZE, "%d%%", BATlife);
    }



    if (ACstatus == 0x1) {
        if (Tool)
            strlcat(s, _(" - Power"), SYS_STR_SIZE);
        else
            strlcat(s, _("P"), SYS_STR_SIZE);
    }
    if ((BATflag & 8)) {
        if (Tool)
            strlcat(s, _(" - Charging"), SYS_STR_SIZE);
        else
            strlcat(s, _("C"), SYS_STR_SIZE);
    }
}

static void strcat3(char* dest,
             const char* src1,
             const char* src2,
             const char* src3,
             size_t n)
{
    if (dest) snprintf(dest, n, "%s%s%s", src1, src2, src3);
}

bool YApm::ignore_directory_bat_entry(const char* name) {
    return strstr(name, "AC") ||
        (acpiIgnoreBatteries && strstr(acpiIgnoreBatteries, name));
}

bool YApm::ignore_directory_ac_entry(const char* name) {
    return strstr(name, "BAT") ||
        (acpiIgnoreBatteries && strstr(acpiIgnoreBatteries, name));
}

void YApm::AcpiStr(char *s, bool Tool) {
    char buf[255], bat_info[250];

    *s='\0';

    //assign some default values, in case
    //the file in /proc/acpi will contain unexpected values
    int ACstatus = -1;
#if !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)
    char buf2[APM_LINE_LEN];
    if (acpiACName && acpiACName[0] != 0) {
        strcat3(buf, "/proc/acpi/ac_adapter/", acpiACName, "/state", sizeof(buf));
        FILE* fd = fopen(buf, "r");
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
#else // some FreeBSD kernel
    int i;
    size_t len = sizeof(i);
    if (sysctlbyname("hw.acpi.acline", &i, &len, NULL, 0) >= 0) {
            switch(i) {
                    case 0: ACstatus = AC_OFFLINE; break;
                    case 1: ACstatus = AC_ONLINE; break;
                    default: ACstatus = AC_UNKNOWN; break;
            }
    }
#endif

    acIsOnLine     = (ACstatus == AC_ONLINE);
    energyFull = energyNow = 0;
    int batCount = 0;

    int n = 0;
    for (int i = 0; i < batteryNum; i++) {
        //assign some default values, in case
        //the files in /proc/acpi will contain unexpected values
        bool BATpresent = BAT_ABSENT;
        int BATstatus = -1;
        int BATcapacity_full = -1;
        int BATcapacity_design = -1;
        int BATcapacity_remain = -1;
        int BATrate = -1;
        int BATtime_remain = -1;

#if !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)
        const char* BATname = acpiBatteries[i]->name;
        strcat3(buf, "/proc/acpi/battery/", BATname, "/state", sizeof(buf));
        FILE* fd = fopen(buf, "r");
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
                }
            }
            fclose(fd);
        }
#else // some FreeBSD kernel
#define ACPIDEV         "/dev/acpi"
        int acpifd = open(ACPIDEV, O_RDONLY);
        if (acpifd != -1) {
            union acpi_battery_ioctl_arg battio;

            battio.unit = i;
            if (ioctl(acpifd, ACPIIO_BATT_GET_BATTINFO, &battio) != -1) {
                if (battio.battinfo.state != ACPI_BATT_STAT_NOT_PRESENT) {
                    BATpresent = BAT_PRESENT;
                    if (battio.battinfo.state == 0)
                        BATstatus = BAT_FULL;
                    else if (battio.battinfo.state & ACPI_BATT_STAT_CHARGING)
                        BATstatus = BAT_CHARGING;
                    else if (battio.battinfo.state & ACPI_BATT_STAT_DISCHARG)
                        BATstatus = BAT_DISCHARGING;
                    else
                        BATstatus = BAT_UNKNOWN;
                    if (battio.battinfo.cap != -1 && acpiBatteries[i]->capacity_full != -1)
                        BATcapacity_remain = acpiBatteries[i]->capacity_full *
                            battio.battinfo.cap / 100;
                    if (battio.battinfo.min != -1)
                        BATtime_remain = battio.battinfo.min;
                    if (battio.battinfo.rate != -1)
                        BATrate = battio.battinfo.rate;
                }
            }
        }
#endif

        if (BATpresent == BAT_PRESENT) { //battery is present now
            if (acpiBatteries[i]->present == BAT_ABSENT) { //and previously was absent
                //read full-capacity value
#if !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)
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
#else
                union acpi_battery_ioctl_arg battio;
#define UNKNOWN_CAP 0xffffffff
#define UNKNOWN_VOLTAGE 0xffffffff

                battio.unit = i;
                if (ioctl(acpifd, ACPIIO_BATT_GET_BIF, &battio) != -1) {
                    if (battio.bif.dcap != UNKNOWN_CAP)
                        BATcapacity_design = battio.bif.dcap;
                    if (battio.bif.lfcap != UNKNOWN_CAP)
                        BATcapacity_full = battio.bif.lfcap;
                    if (BATcapacity_remain > BATcapacity_full && BATcapacity_design > 0)
                        BATcapacity_full = BATcapacity_design;
                }
#endif
                acpiBatteries[i]->capacity_full = BATcapacity_full;
            }
            else {
                BATcapacity_full = acpiBatteries[i]->capacity_full;
            }
        }
        acpiBatteries[i]->present = BATpresent;

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
        close(acpifd);
#endif

        if (BATpresent == BAT_PRESENT &&
            //did we parse the needed values successfully?
            BATcapacity_remain >= 0 && BATcapacity_full >= 0)
        {
           energyFull += BATcapacity_full;
           energyNow += BATcapacity_remain;
           batCount++;
        }

        bat_info[0] = 0;
        if (!Tool &&
            taskBarShowApmTime &&
            BATpresent == BAT_PRESENT &&
            //bios calculates remaining time, only while discharging
            BATstatus == BAT_DISCHARGING &&
            //did we parse the needed values successfully?
            BATcapacity_full >= 0 && BATcapacity_remain >= 0 && BATrate > 0) {
            if (BATtime_remain == -1)
                BATtime_remain = (int) (60 * (double)(BATcapacity_remain) / BATrate);
            snprintf(bat_info, sizeof bat_info, "%d:%02d",
                     BATtime_remain / 60, BATtime_remain % 60);
        }
        else if (BATpresent == BAT_PRESENT &&
                 //did we parse the needed values successfully?
                 BATcapacity_remain >= 0 && BATcapacity_full >= 0)
        {
            snprintf(bat_info, sizeof bat_info, "%3.0f%%",
                    100 * (double)BATcapacity_remain / BATcapacity_full);
        }

        if (BATstatus == BAT_CHARGING) {
            if (Tool)
                strlcat(bat_info, _(" - Charging"), sizeof bat_info);
            else
                strlcat(bat_info, _("C"), sizeof bat_info);
        } else if (BATstatus == BAT_FULL && Tool)
                strlcat(bat_info, _(" - Full"), sizeof bat_info);

        if (Tool && BATrate > 0) {
            snprintf(buf, sizeof buf, " %d", BATrate);
            strlcat(bat_info, buf, sizeof bat_info);
        }

        if ((n > 0) && (*bat_info)) {
            if (Tool)
                strlcat(s, " / ", SYS_STR_SIZE);
            else
                strlcat(s, "/", SYS_STR_SIZE);
        }
        n++;
        strlcat(s, bat_info, SYS_STR_SIZE);
    }

    if (ACstatus == AC_ONLINE) {
        if (Tool)
            strlcat(s,_(" - Power"), SYS_STR_SIZE);
        else {
///            if (!prettyClock) strlcat(s, " ", SYS_STR_SIZE);
            strlcat(s,_("P"), SYS_STR_SIZE);
        }
    }

}

void YApm::SysStr(char *s, bool Tool) {
    char buf[255], bat_info[250];

    *s='\0';

    //assign some default values, in case
    //the file in /sys/class/power_supply will contain unexpected values
    int ACstatus = -1;
    if (acpiACName && acpiACName[0] != 0) {
        strcat3(buf, "/sys/class/power_supply/", acpiACName, "/online", sizeof(buf));
        FILE* fd = fopen(buf, "r");
        if (fd != NULL) {
            while (fgets(buf, sizeof(buf), fd)) {
                    if (strncmp(buf, "1", 1) == 0) {
                        ACstatus = AC_ONLINE;
                    }
                    else if (strncmp(buf, "0", 1) == 0) {
                        ACstatus = AC_OFFLINE;
                    }
                    else {
                        ACstatus = AC_UNKNOWN;
                    }
            }
            fclose(fd);
        }
    }

    acIsOnLine     = (ACstatus == AC_ONLINE);
    energyFull = energyNow = 0;

    int n = 0;
    for (int i = 0; i < batteryNum; i++) {
        const char* BATname = acpiBatteries[i]->name;
        //assign some default values, in case
        //the files in /sys/class/power_supply will contain unexpected values
        bool BATpresent = BAT_ABSENT;
        int BATstatus = -1;
        int BATcapacity_full = -1;
        int BATcapacity_design = -1;
        int BATcapacity_remain = -1;
        int BATrate = -1;
        int BATtime_remain = -1;

        strcat3(buf, "/sys/class/power_supply/", BATname, "/status", sizeof(buf));
        FILE* fd = fopen(buf, "r");
        if (fd == NULL) {
                strcat3(buf, "/sys/class/power_supply/", BATname, "/power_now", sizeof(buf));
                fd = fopen(buf, "r");
        }

        if (fd != NULL) {
            if (fgets(buf, sizeof(buf), fd)) {
                if (strncasecmp(buf, "charging", 8) == 0) {
                        BATstatus = BAT_CHARGING;
                    }
                    else if (strncasecmp(buf, "discharging", 11) == 0) {
                        BATstatus = BAT_DISCHARGING;
                    }
                    else if (strncasecmp(buf, "full", 4) == 0) {
                        BATstatus = BAT_FULL;
                    }
                    else {
                        BATstatus = BAT_UNKNOWN;
                    }
            }
            fclose(fd);
        }

        // XXX: investigate, if current_now is missing, can we stop polling it? For all or just for this battery?
        strcat3(buf, "/sys/class/power_supply/", BATname, "/current_now", sizeof(buf));
        fd = fopen(buf, "r");
        if (fd == NULL) {
            strcat3(buf, "/sys/class/power_supply/", BATname, "/power_now", sizeof(buf));
            fd = fopen(buf, "r");
        }
        if (fd != NULL) {
            if (fgets(buf, sizeof(buf), fd)) {
                //In case it contains non-numeric value
                if (sscanf(buf,"%d", &BATrate) <= 0) {
                    BATrate = -1;
                }
            }
            fclose(fd);
        }

        strcat3(buf, "/sys/class/power_supply/", BATname, "/energy_now", sizeof(buf));
        fd = fopen(buf, "r");
        if (fd == NULL) {
            strcat3(buf, "/sys/class/power_supply/", BATname, "/charge_now", sizeof(buf));
            fd = fopen(buf, "r");
        }
        if (fd != NULL) {
            if (fgets(buf, sizeof(buf), fd)) {
                //In case it contains non-numeric value
                if (sscanf(buf,"%d", &BATcapacity_remain) <= 0) {
                    BATcapacity_remain = -1;
                }
            }
            fclose(fd);
        }

        strcat3(buf, "/sys/class/power_supply/", BATname, "/present", sizeof(buf));
        fd = fopen(buf, "r");
        if (fd != NULL) {
            if (fgets(buf, sizeof(buf), fd)) {
                if (strncmp(buf, "1", 1) == 0) {
                    BATpresent = BAT_PRESENT;
                }
                else {
                    BATpresent = BAT_ABSENT;
                }
            }
            fclose(fd);
        }

        if (BATpresent == BAT_PRESENT) { //battery is present now
            if (acpiBatteries[i]->present == BAT_ABSENT) { //and previously was absent
                //read full-capacity value
                strcat3(buf, "/sys/class/power_supply/", BATname, "/energy_full_design", sizeof(buf));
                fd = fopen(buf, "r");
                if (fd == NULL) {
                    strcat3(buf, "/sys/class/power_supply/", BATname, "/charge_full_design", sizeof(buf));
                    fd = fopen(buf, "r");
                }
                if (fd != NULL) {
                    if (fgets(buf, sizeof(buf), fd)) {
                            //in case it contains non-numeric value
                            if (sscanf(buf, "%d", &BATcapacity_design)<=0) {
                                BATcapacity_design = -1;
                            }
                    }
                    fclose(fd);
                }
                strcat3(buf, "/sys/class/power_supply/", BATname, "/energy_full", sizeof(buf));
                fd = fopen(buf, "r");
                if (fd == NULL) {
                    strcat3(buf, "/sys/class/power_supply/", BATname, "/charge_full", sizeof(buf));
                    fd = fopen(buf, "r");
                }
                if (fd != NULL) {
                    if (fgets(buf, sizeof(buf), fd)) {
                            //in case it contains non-numeric value
                            if (sscanf(buf, "%d", &BATcapacity_full)<=0) {
                                BATcapacity_full = -1;
                            }
                    }
                    fclose(fd);
                }
                if (BATcapacity_remain > BATcapacity_full && BATcapacity_design > 0)
                        BATcapacity_full = BATcapacity_design;
                acpiBatteries[i]->capacity_full = BATcapacity_full;
            }
            else {
                BATcapacity_full = acpiBatteries[i]->capacity_full;
            }
        }
        acpiBatteries[i]->present = BATpresent;

        // the code above caches BATcapacity_full when battery is installed;
        // however, this value and _remain can increase slightly while the battery is charging,
        // so set a limit to not display resulting value over 100% to the user
        if (BATcapacity_remain > BATcapacity_full) BATcapacity_remain = BATcapacity_full;

        if (BATpresent == BAT_PRESENT &&
            //did we parse the needed values successfully?
            BATcapacity_remain >= 0 && BATcapacity_full >= 0)
        {
           energyFull += BATcapacity_full;
           energyNow += BATcapacity_remain;
        }
        if (Tool &&
            taskBarShowApmTime &&
            BATpresent == BAT_PRESENT &&
            //bios calculates remaining time, only while discharging
            BATstatus == BAT_DISCHARGING &&
            //did we parse the needed values successfully?
            BATcapacity_full >= 0 && BATcapacity_remain >= 0 && BATrate > 0) {
            BATtime_remain = (int) (60 * (double)(BATcapacity_remain) / BATrate);
            snprintf(bat_info, sizeof bat_info, "%d:%02d (%3.0f%%)",
                    BATtime_remain / 60, BATtime_remain % 60,
                    round(double(100) * BATcapacity_remain / BATcapacity_full));
        }
        else if (BATpresent == BAT_PRESENT &&
                 //did we parse the needed values successfully?
                 BATcapacity_remain >= 0 && BATcapacity_full >= 0)
        {
            snprintf(bat_info, sizeof bat_info, "%3.0f%%",
                     round(double(100) * BATcapacity_remain / BATcapacity_full));
        }
        else {
            //battery is absent or we didn't parse some needed values
            bat_info[0] = 0;
        }

        if (BATstatus == BAT_CHARGING) {
            if (Tool)
                strlcat(bat_info, _(" - Charging"), sizeof bat_info);
            else
                strlcat(bat_info, _("C"), sizeof bat_info);
        }

        if ((n > 0) && (*bat_info)) {
            if (Tool)
                strlcat(s, " / ", SYS_STR_SIZE);
            else
                strlcat(s, "/", SYS_STR_SIZE);
        }
        n++;
        strlcat(s, bat_info, SYS_STR_SIZE);
    }

    if (ACstatus == AC_ONLINE) {
        if (Tool)
            strlcat(s, _(" - Power"), SYS_STR_SIZE);
        else {
///            if (!prettyClock) strlcat(s, " ", SYS_STR_SIZE);
            strlcat(s, _("P"), SYS_STR_SIZE);
        }
    }

}

void YApm::PmuStr(char *s, const bool tool_tip)
{
   FILE *fd = fopen("/proc/pmu/info", "r");
   if (fd == NULL) {
      strlcpy(s, "Err", SYS_STR_SIZE);
      // this is somewhat difficult, because pmu support seams to be gone
      return;
   }

   char line[APM_LINE_LEN];
   int power_present(0);
   while ( fgets(line, APM_LINE_LEN, fd) != NULL )
   {
      if (strncmp("AC Power", line, strlen("AC Power")) == 0) {
         sscanf(strchr(line, ':')+2, "%d", &power_present);
         break;
      }
   }
   fclose(fd);

   acIsOnLine     = (power_present != 0);
   int batCount = 0;

   char* s_end = s;
   for (int i=0; i < batteryNum; ++i) {
      char file_name[SYS_STR_SIZE];
      snprintf(file_name, SYS_STR_SIZE, "/proc/pmu/battery_%d", i);
      fd = fopen(file_name, "r");
      if (fd == NULL) {
         strlcpy(s_end, "Err", SYS_STR_SIZE - (s_end - s));
         s_end += 3;
         continue;
      }

      int flags=0, rem_time=-1, charge=0, max_charge=0, voltage=0;
      while ( fgets(line, ACOUNT(line), fd) != NULL )
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

      if (battery_present &&
          //did we parse the needed values successfully?
          charge >= 0 && max_charge >= 0)
      {
         energyFull += max_charge;
         energyNow += charge;
         batCount++;
      }

      if (tool_tip) {
         if (i > 0) {
            strlcpy(s_end, " / ", SYS_STR_SIZE - (s_end - s));
            s_end += 3;
         }

         if (battery_present) {
            if ( !battery_full )
              s_end += snprintf(s_end, SYS_STR_SIZE - (s_end - s),
                               " %d%c%02d%s", rem_time/60,
                               time_in_min?':':'.', rem_time%60,
                               time_in_min?"h":"m");

            s_end += snprintf(s_end, SYS_STR_SIZE - (s_end - s),
                     " %.0f%% %d/%dmAh %.3fV", 100.0*charge/max_charge,
                     charge, max_charge, voltage/1e3);

            if (battery_charging)
              s_end += snprintf(s_end, SYS_STR_SIZE - (s_end - s),
                                "%s", _(" - Charging"));
         } else {
           // Battery not present
            strlcpy(s_end, "--", SYS_STR_SIZE - (s_end - s));
            s_end += 2;
         }
      } else {  // Taskbar
         if (i > 0) {
           strlcpy(s_end, "/", SYS_STR_SIZE - (s_end - s));
           s_end += 1;
         }

         if (! battery_present ) {
            strlcpy(s_end, "--", SYS_STR_SIZE - (s_end - s));
            s_end += 2;
         } else if (taskBarShowApmTime && !battery_full)
           s_end += snprintf(s_end, SYS_STR_SIZE - (s_end - s),
                             "%d%c%02d", rem_time/60,
                            time_in_min?':':'.', rem_time%60);
         else
           s_end += snprintf(s_end, SYS_STR_SIZE - (s_end - s),
                             "%3.0f%%", 100.0*charge/max_charge);

         if (battery_charging) {
            strlcpy(s_end, "C", SYS_STR_SIZE - (s_end - s));
            s_end += 1;
         }
      }
   }

   if (power_present) {
      if (tool_tip)
        strlcpy(s_end, _(" - Power"), SYS_STR_SIZE - (s_end - s));
      else
        strlcpy(s_end, "P", SYS_STR_SIZE - (s_end - s));
   }
}

YApm::YApm(YWindow *aParent, bool autodetect):
    YWindow(aParent), YTimerListener(),
    apmTimer(0), apmBg(&clrApm), apmFg(&clrApmText),
    apmFont(YFont::getFont(XFA(apmFontName))),
    apmColorOnLine(&clrApmLine),
    apmColorBattery(&clrApmBat),
    apmColorGraphBg(&clrApmGraphBg),
    mode(APM), batteryNum(0), acpiACName(0), fCurrentState(0),
    acIsOnLine(false), energyNow(0), energyFull(0)
{
    FILE *pmu_info;
    char buf[300];

    //search for acpi info first
#if !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)
    adir dir("/sys/class/power_supply");
    if (dir.isOpen() && dir.count() > 0)
        mode = SYSFS;
    else if (dir.open("/proc/acpi/battery") && dir.count() > 0)
        mode = ACPI;
    if (mode == SYSFS || mode == ACPI) {
        //scan for batteries
        while (dir.next()) {
            if (mode == SYSFS) {
                strcat3(buf, "/sys/class/power_supply/",
                        dir.entry(), "/online", sizeof(buf));
                if (access(buf, R_OK) == 0) {
                    if (acpiACName == 0)
                        acpiACName = newstr(dir.entry());
                    continue;
                }
            }
            else if (mode == ACPI && acpiACName == 0) {
                if (!ignore_directory_ac_entry(dir.entry())) {
                    //found an ac_adapter
                    acpiACName = newstr(dir.entry());
                    continue;
                }
            }
            if (!ignore_directory_bat_entry(dir.entry())) {
                if (batteryNum < MAX_ACPI_BATTERY_NUM) {
                    //found a battery
                    acpiBatteries[batteryNum++] = new Battery(dir.entry());
                }
            }
        }
        if (!acpiACName) {
            //no ac_adapter was found
            acpiACName = newstr("");
        }
        dir.close();
    }
#else
    int acpifd = open(ACPIDEV, O_RDONLY);
    if (acpifd != -1) {
        mode = ACPI;

        //scan for batteries
        for (int i = 0; i < 64 && batteryNum < MAX_ACPI_BATTERY_NUM; ++i) {
            union acpi_battery_ioctl_arg battio;

            battio.unit = i;
            if (ioctl(acpifd, ACPIIO_BATT_GET_BATTINFO, &battio) != -1) {
                snprintf(buf, sizeof buf, "Battery%d", i);
                acpiBatteries[batteryNum++] = new Battery(buf);
            }
        }

        acpiACName = newstr("AC1");
        close(acpifd);
    }
#endif
    else if ( (pmu_info = fopen("/proc/pmu/info", "r")) != NULL) {
       mode = PMU;
       char line[APM_LINE_LEN];
       while ( fgets(line, APM_LINE_LEN, pmu_info) != NULL )
         if (strncmp("Battery count", line, strlen("Battery count")) == 0)
           sscanf(strchr(line, ':')+2, "%d", &batteryNum);

       fclose(pmu_info);
    } else if (!autodetect) {
       //use apm info
       mode = APM;
       batteryNum = 1;
    }

    if (autodetect && 0 == batteryNum)
        return;

    updateState();

    apmTimer->setTimer(1000 * batteryPollingPeriod, this, true);

    if (taskBarShowApmGraph)
       setSize(taskBarApmGraphWidth, taskBarGraphHeight);
    else
       setSize(calcInitialWidth(), taskBarGraphHeight);
    updateToolTip();
    // setDND(true);
}

YApm::~YApm() {
    if (ACPI == mode || mode == SYSFS) {
        for (int i = batteryNum; --i >= 0; --batteryNum) {
            delete acpiBatteries[i]; acpiBatteries[i] = 0;
        }
        delete[] acpiACName; acpiACName = 0;
    }
    delete[] fCurrentState; fCurrentState = 0;
}

void YApm::updateToolTip() {
    char s[SYS_STR_SIZE] = {' ', ' ', ' ', 0, 0, 0, 0, 0};

    switch (mode) {
    case ACPI:
        AcpiStr(s, 1);
        break;
    case SYSFS:
        SysStr(s, 1);
        break;
    case APM:
        ApmStr(s, 1);
        break;
    case PMU:
        PmuStr(s, 1);
        break;
    }

    setToolTip(s);
}

int YApm::calcInitialWidth() {
    char buf[APM_LINE_LEN] = { 0 };
    int i;
    int n = 0;

    //estimate applet's size
    for (i = 0; i < batteryNum; i++) {
        if ((mode == ACPI || mode == SYSFS) && acpiBatteries[i]->present == BAT_ABSENT)
            continue;
        if (taskBarShowApmTime)
            strlcat(buf, "0:00 0.0W", sizeof buf);
        else
            strlcat(buf, "100%", sizeof buf);
        strlcat(buf, "C", sizeof buf);
        if (n > 0)
            strlcat(buf, "/", sizeof buf);
        n++;
    }
///    if (!prettyClock) strlcat(buf, " ", sizeof buf);
    strlcat(buf, "P", sizeof buf);

    return calcWidth(buf, strlen(buf));
}

void YApm::updateState() {
    char s[SYS_STR_SIZE] = {' ', ' ', ' ', 0, 0, 0, 0, 0};

    switch (mode) {
    case ACPI:
        AcpiStr(s, 0);
        break;
    case SYSFS:
        SysStr(s, 0);
        break;
    case APM:
        ApmStr(s, 0);
        break;
    case PMU:
        PmuStr(s, 0);
        break;
    }
    MSG((_("power:\t%s"), s));

    if (fCurrentState != 0)
        delete[] fCurrentState;
    fCurrentState = newstr(s);
}

void YApm::paint(Graphics &g, const YRect &/*r*/) {
    unsigned int x = 0;
    int len, i;

    len = fCurrentState ? strlen(fCurrentState) : 0;

    //clean background of current size first, so that
    //it is possible to use transparent apm-background
    ref<YImage> gradient(parent()->getGradient());

    if (gradient != null) {
        g.drawImage(gradient, this->x(), this->y(), width(), height(), 0, 0);
    }
    else
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
    if (new_width>old_width) setSize(new_width, taskBarGraphHeight);
#endif

    //draw foreground
    if (taskBarShowApmGraph) {
       // Draw graph indicator
       g.setColor(apmColorGraphBg);
       g.fillRect(0, 0, taskBarApmGraphWidth, height());

       int new_h = (int) round((double(energyNow)/double(energyFull)) * height());
       g.setColor(acIsOnLine ? apmColorOnLine : apmColorBattery);
       g.fillRect(0, height() - new_h, taskBarApmGraphWidth, new_h);
    } else if (prettyClock) {
        ref<YPixmap> p;

        for (i = 0; x < new_width; i++) {
            if (i < len) {
                p = getPixmap(fCurrentState[i]);
            } else
                p = ledPixSpace;

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
        setSize(new_width, taskBarGraphHeight);
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
    case '5': case '6': case '7': case '8': case '9':
        pix = ledPixNum[c - '0']; break;
    case ' ': pix = ledPixSpace; break;
    case ':': pix = ledPixColon; break;
    case '.': pix = ledPixDot; break;
    case 'P': pix = ledPixP; break;
    case 'M': pix = ledPixM; break;
    case '/': pix = ledPixSlash; break;
    case '%': pix = ledPixPercent; break;
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

// vim: set sw=4 ts=4 et:
