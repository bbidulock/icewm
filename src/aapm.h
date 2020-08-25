#ifndef AAPM_H
#define AAPM_H

#ifndef __sun__

#include "ytimer.h"

#ifdef __linux__
#define APMDEV "/proc/apm"
#else
#define APMDEV "/dev/apm"
#endif
#if __FreeBSD__ || __FreeBSD_kernel__
#define ACPIDEV "/dev/acpi"
#endif

// assume at most 3 batteries
#define MAX_ACPI_BATTERY_NUM 3

struct Battery {
    mstring name;
    bool present;
    int capacity_full;

    Battery(const char* batName) : name(batName),
            present(false), capacity_full(-1) { }
};


class YApm:
    public IApplet,
    private Picturer,
    private YTimerListener
{
public:
    // if autodetect==true and no battery then go to sleep
    YApm(YWindow *aParent = nullptr, bool autodetect = false);
    virtual ~YApm();

    virtual void updateToolTip();
    virtual bool handleTimer(YTimer *t);
    inline bool hasBatteries() { return batteryNum; }

private:
    lazy<YTimer> apmTimer;

    ref<YPixmap> getPixmap(char ch);
    int calcInitialWidth();
    int calcWidth(const char *s, int count);

    void AcpiStr(char *s, bool Tool);
    void SysStr(char *s, bool Tool);
    void PmuStr(char *, const bool);
    void ApmStr(char *s, bool Tool);
    bool ignore_directory_bat_entry(const char* name);
    bool ignore_directory_ac_entry(const char* name);

    YColorName apmBg;
    YColorName apmFg;
    ref<YFont> apmFont;

    YColorName apmColorOnLine;
    YColorName apmColorBattery;
    YColorName apmColorGraphBg;

    // inspection mode: legacy APM, legacy ACPI (procfs), PMU (Mac), ACPI (sysfs)
    enum { APM, ACPI, PMU, SYSFS } mode;
    //number of batteries (for apm == 1)
    int batteryNum;

    //list of batteries (static info)
    Battery *acpiBatteries[MAX_ACPI_BATTERY_NUM];
    //(file)name of ac adapter
    char *acpiACName;
    char *fCurrentState;
    bool fStatusChanged;

    // On line status and charge persent
    bool     acIsOnLine;
    // current and maximum charge, large enough for hundred 40Wh batteries
    unsigned    energyNow, energyFull;

    bool updateState();
    bool picture();
    void draw(Graphics& g);
};
#endif

#endif

// vim: set sw=4 ts=4 et:
