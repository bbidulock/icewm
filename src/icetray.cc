#include "config.h"
#include "intl.h"
#include "ylib.h"
#include "ylocale.h"
#include "yxapp.h"
#include "yxtray.h"
#include "base.h"
#include "debug.h"
#include "sysdep.h"
#include "yprefs.h"

extern void logEvent(const XEvent &xev);

char const *ApplicationName = "icewmtray";

#include "yconfig.h"

#ifdef CONFIG_TASKBAR
YColor *taskBarBg;

XSV(const char *, clrDefaultTaskBar,            "rgb:C0/C0/C0")
XIV(bool, trayDrawBevel,                        false)

cfoption icewmbg_prefs[] = {
    OSV("ColorDefaultTaskBar",                  &clrDefaultTaskBar,             "Background of the taskbar"),
    OBV("TrayDrawBevel",                        &trayDrawBevel,                 "Surround the tray with plastic border"),
    OK0()
};
#endif

class SysTray: public YWindow, public YXTrayNotifier {
public:
    SysTray();
    bool checkMessageEvent(const XClientMessageEvent &message);
    void requestDock();

    void handleUnmap(const XUnmapEvent &) {
        MSG(("hide"));
        if (visible())
            hide();
    }

    void trayChanged();
private:
    Atom icewm_internal_tray;
    Atom manager;
    Atom _NET_SYSTEM_TRAY_OPCODE;
    YXTray *fTray2;
};

class SysTrayApp: public YXApplication {
public:
    SysTrayApp(int *argc, char ***argv,
               const char* configFile = 0,
               const char* overrideTheme = 0);
    ~SysTrayApp();

    void loadConfig();
    bool filterEvent(const XEvent &xev);
    void handleSignal(int sig);

private:
    Atom _ICEWM_ACTION;
    SysTray *tray;
    const char* configFile;
    const char* overrideTheme;
};

static int handler(Display *display, XErrorEvent *xev) {
    DBG {
        char message[80], req[80], number[80];

        sprintf(number, "%d", xev->request_code);
        XGetErrorDatabaseText(display,
                              "XRequest",
                              number, "",
                              req, sizeof(req));
        if (!req[0])
            sprintf(req, "[request_code=%d]", xev->request_code);

        if (XGetErrorText(display,
                          xev->error_code,
                          message, sizeof(message)) !=
                          Success)
            *message = '\0';

        warn("X error %s(0x%lX): %s", req, xev->resourceid, message);
    }
    return 0;
}

SysTrayApp::SysTrayApp(int *argc, char ***argv,
        const char* _configFile, const char* _overrideTheme):
    YXApplication(argc, argv),
    tray(0), configFile(_configFile), overrideTheme(_overrideTheme)
{
    desktop->setStyle(YWindow::wsDesktopAware);
    catchSignal(SIGINT);
    catchSignal(SIGTERM);
    catchSignal(SIGHUP);
    loadConfig();

    XSetErrorHandler(handler);
    tray = new SysTray();

    _ICEWM_ACTION = XInternAtom(xapp->display(), "_ICEWM_ACTION", False);
}

void SysTrayApp::loadConfig() {
#ifdef CONFIG_TASKBAR
#ifndef NO_CONFIGURE
    if (configFile == 0 || *configFile == 0)
        configFile = "preferences";
    if (overrideTheme && *overrideTheme)
        themeName = overrideTheme;
    else
    {
        cfoption theme_prefs[] = {
            OSV("Theme", &themeName, "Theme name"),
            OK0()
        };

        YConfig::findLoadConfigFile(this, theme_prefs, configFile);
        YConfig::findLoadConfigFile(this, theme_prefs, "theme");
    }
    YConfig::findLoadConfigFile(this, icewmbg_prefs, configFile);
    if (themeName != 0) {
        MSG(("themeName=%s", themeName));

        YConfig::findLoadThemeFile(
            this,
            icewmbg_prefs,
            upath("themes").child(themeName));
    }
    YConfig::findLoadConfigFile(this, icewmbg_prefs, "prefoverride");
#endif
    if (taskBarBg) 
        delete taskBarBg;
    taskBarBg = new YColor(clrDefaultTaskBar);
#endif
}

SysTrayApp::~SysTrayApp() {

}

bool SysTrayApp::filterEvent(const XEvent &xev) {
#ifdef DEBUG
    logEvent(xev);
#endif
    if (xev.type == ClientMessage) {
        if (xev.xclient.message_type == _ICEWM_ACTION) {
            MSG(("loadConfig"));
            loadConfig();
            tray->trayChanged();
        } else
            tray->checkMessageEvent(xev.xclient);
        return false;
    } else if (xev.type == MappingNotify) {
            MSG(("tray mapping1"));
        if (xev.xmapping.window == tray->handle()) {
            MSG(("tray mapping"));
        }
    }
    return false;
}

void SysTrayApp::handleSignal(int sig) {
    switch (sig) {
    case SIGHUP: 
         // Reload config colors from theme file and notify tray to repaint
         loadConfig(); 
         tray->trayChanged(); 
         return; 
    case SIGINT:
    case SIGTERM:
        MSG(("exiting."));
        exit(0);
        return;
    }
    YXApplication::handleSignal(sig);
}

SysTray::SysTray(): YWindow(0) {
    desktop->setStyle(YWindow::wsDesktopAware);

    char trayatom[64];
    sprintf(trayatom, "_NET_SYSTEM_TRAY_S%d", xapp->screen());
    fTray2 = new YXTray(this, false, trayatom, this);

    char trayatom2[64];
    sprintf(trayatom2, "_ICEWM_INTTRAY_S%d", xapp->screen());
    icewm_internal_tray =
        XInternAtom(xapp->display(), trayatom2, False);
    manager =
        XInternAtom(xapp->display(), "MANAGER", False);

    _NET_SYSTEM_TRAY_OPCODE =
        XInternAtom(xapp->display(),
                    "_NET_SYSTEM_TRAY_OPCODE",
                    False);

    fTray2->relayout();
    setSize(fTray2->width(),
            fTray2->height());
    fTray2->show();
    requestDock();
}
    
void SysTray::trayChanged() {
    fTray2->backgroundChanged();
    setSize(fTray2->width(),
            fTray2->height());
    if (fTray2->visible())
        show();
    else
        hide();
}

void SysTray::requestDock() {
    Window w = XGetSelectionOwner(xapp->display(), icewm_internal_tray);

    if (w && w != handle()) {
        XClientMessageEvent xev;
        memset(&xev, 0, sizeof(xev));

        xev.type = ClientMessage;
        xev.window = w;
        xev.message_type = _NET_SYSTEM_TRAY_OPCODE;
        xev.format = 32;
        xev.data.l[0] = CurrentTime;
        xev.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
        xev.data.l[2] = handle(); //fTray2->handle();

        XSendEvent(xapp->display(), w, False, StructureNotifyMask, (XEvent *) &xev);
    }
}

bool SysTray::checkMessageEvent(const XClientMessageEvent &message) {
    if (message.message_type == manager && (Atom) message.data.l[1] == icewm_internal_tray) {
        MSG(("requestDock"));
        requestDock();
    }
    return true;
}

static const char* get_help_text() {
    return _(
    "  --notify            Notify parent process by sending signal USR1.\n"
    "  --display=NAME      Use NAME to connect to the X server.\n"
    "  --sync              Synchronize communication with X11 server.\n"
    "\n"
    "  -c, --config=FILE   Load preferences from FILE.\n"
    "  -t, --theme=FILE    Load the theme from FILE.\n"
    );
}

int main(int argc, char **argv) {
    YLocale locale;

    bool notifyParent = false;
    const char* configFile = 0;
    const char* overrideTheme = 0;

    for (char **arg = 1 + argv; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char* value(0);
            if (is_long_switch(*arg, "notify")) {
                notifyParent = true;
            }
            else if (GetLongArgument(value, "config", arg, argv+argc)
                ||  GetShortArgument(value, "c", arg, argv+argc))
            {
                configFile = value;
            }
            else if (GetLongArgument(value, "theme", arg, argv+argc)
                ||   GetShortArgument(value, "t", arg, argv+argc))
            {
                overrideTheme = value;
            }
            else if (is_help_switch(*arg)) {
                print_help_exit(get_help_text());
            }
            else if (is_version_switch(*arg)) {
                print_version_exit(VERSION);
            }
#ifdef DEBUG
            else if (is_long_switch(*arg, "debug")) {
                debug = 1;
            }
#endif
        }
    }

    SysTrayApp stapp(&argc, &argv, configFile, overrideTheme);

    if (notifyParent) {
        kill(getppid(), SIGUSR1);
    }

    return stapp.mainLoop();
}
