
#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)

#include "ywindow.h"
#include "ytimer.h"

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define APMDEV "/dev/apm"
#else
#define APMDEV "/proc/apm"
#endif

//assume there is no laptop with more
//than 3 batteries
#define MAX_ACPI_BATTERY_NUM 3

struct Battery {
  //(file)name of battery
  char *name;
  bool present;
  int capacity_full;
  Battery(const char* batName) : name(newstr(batName)),
          present(false), capacity_full(-1) { }
  ~Battery() { delete[] name; name = 0; }
};


class YApm: public YWindow, public YTimerListener {
public:
        // autodetect==true means becoming dormant if no battery was detected correctly
    YApm(YWindow *aParent = 0, bool autodetect=false);
    virtual ~YApm();

    virtual void paint(Graphics &g, const YRect &r);

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

    // On line status and charge persent
    bool     acIsOnLine;
    // current and maximum charge, large enough for hundred 40Wh batteries
    unsigned    energyNow, energyFull;

    void updateState();
};
#endif

// vim: set sw=4 ts=4 et:
