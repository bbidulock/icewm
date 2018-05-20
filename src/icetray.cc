#include "config.h"
#include "intl.h"
#include "ylib.h"
#include "ylocale.h"
#include "yxapp.h"
#include "yxtray.h"
#include "base.h"
#include "debug.h"
#include "sysdep.h"
#include "ypaths.h"
#include "yprefs.h"
#include "yconfig.h"
#include "ypointer.h"

char const *ApplicationName = "icewmtray";

XSV(const char *, clrDefaultTaskBar, "rgb:C0/C0/C0")
XIV(bool,         trayDrawBevel,     false)

YColorName taskBarBg(&clrDefaultTaskBar);
ref<YPixmap> taskbackPixmap;

#ifdef CONFIG_EXTERNAL_TRAY
class SysTray: public YWindow, public YXTrayNotifier {
public:
    SysTray();
    void requestDock();

    void handleUnmap(const XUnmapEvent &) {
        MSG(("SysTray::handleUnmap hide %s", boolstr(visible())));
        if (visible())
            hide();
    }

    void trayChanged();
    int count() const { return fTray2->countClients(); }

private:
    YAtom icewm_internal_tray;
    YAtom _NET_SYSTEM_TRAY_OPCODE;
    osmart<YXTray> fTray2;

    void show() {
        YWindow::show();
        XMapWindow(xapp->display(), handle());
    }
    void hide() {
        YWindow::hide();
        XUnmapWindow(xapp->display(), handle());
    }
};

class SysTrayApp: public YXApplication {
public:
    SysTrayApp(int *argc, char ***argv,
               const char* configFile = 0,
               const char* overrideTheme = 0);
    ~SysTrayApp();

    void loadConfig();
    virtual bool filterEvent(const XEvent &xev);
    virtual void handleSignal(int sig);

private:
    YAtom _ICEWM_ACTION;
    YAtom icewm_internal_tray;
    YAtom manager;
    osmart<SysTray> tray;
    const char* configFile;
    const char* overrideTheme;
};

static int handler(Display *display, XErrorEvent *xev) {
    XDBG {
        char message[80], req[80], number[80];

        snprintf(number, sizeof number, "%d", xev->request_code);
        XGetErrorDatabaseText(display, "XRequest", number, "", req, sizeof req);
        if (req[0] == 0)
            snprintf(req, sizeof req, "[request_code=%d]", xev->request_code);

        if (XGetErrorText(display, xev->error_code, message, sizeof message))
            *message = '\0';

        tlog("X error %s(0x%lX): %s", req, xev->resourceid, message);
    }
    return 0;
}

SysTrayApp::SysTrayApp(int *argc, char ***argv,
        const char* _configFile, const char* _overrideTheme):
    YXApplication(argc, argv),
    _ICEWM_ACTION("_ICEWM_ACTION"),
    icewm_internal_tray("_ICEWM_INTTRAY_S", true),
    manager("MANAGER"),
    tray(0), configFile(_configFile), overrideTheme(_overrideTheme)
{
    desktop->setStyle(YWindow::wsDesktopAware);
    catchSignal(SIGINT);
    catchSignal(SIGTERM);
    catchSignal(SIGHUP);
    loadConfig();

    XSetErrorHandler(handler);
    tray = new SysTray();
}

void SysTrayApp::loadConfig() {
    static cfoption tray_prefs[] = {
        OSV("ColorDefaultTaskBar", &clrDefaultTaskBar, "Background of the taskbar"),
        OBV("TrayDrawBevel",       &trayDrawBevel,     "Surround the tray with plastic border"),
        OK0()
    };

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
    YConfig::findLoadConfigFile(this, tray_prefs, configFile);
    if (themeName != 0) {
        MSG(("themeName=%s", themeName));

        YConfig::findLoadThemeFile(
            this,
            tray_prefs,
            upath("themes").child(themeName));
    }
    YConfig::findLoadConfigFile(this, tray_prefs, "prefoverride");
    taskbackPixmap = YResourcePaths::loadPixmapFile("taskbarbg.xpm");
}

SysTrayApp::~SysTrayApp() {
}

bool SysTrayApp::filterEvent(const XEvent &xev) {
#ifdef DEBUG
    extern void logEvent(const XEvent &xev);
    logEvent(xev);
#endif
    if (xev.type == ClientMessage) {
        if (xev.xclient.message_type == _ICEWM_ACTION) {
            MSG(("loadConfig"));
            loadConfig();
            tray->trayChanged();
            return true;
        }
        else if (xev.xclient.message_type == manager &&
          (Atom) xev.xclient.data.l[1] == icewm_internal_tray)
        {
            if (tray->count() > 0)
                tray->requestDock();
            else
                tray = new SysTray;
            return true;
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
        this->exit(0);
        return;
    }
    YXApplication::handleSignal(sig);
}

SysTray::SysTray():
    icewm_internal_tray("_ICEWM_INTTRAY_S", true),
    _NET_SYSTEM_TRAY_OPCODE("_NET_SYSTEM_TRAY_OPCODE")
{
    setTitle("SysTray");
    setParentRelative();
    desktop->setStyle(YWindow::wsDesktopAware);

    YAtom trayatom("_NET_SYSTEM_TRAY_S", true);
    fTray2 = new YXTray(this, false, trayatom, this);
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
        XClientMessageEvent xev = {};
        xev.type = ClientMessage;
        xev.window = w;
        xev.message_type = _NET_SYSTEM_TRAY_OPCODE;
        xev.format = 32;
        xev.data.l[0] = CurrentTime;
        xev.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
        xev.data.l[2] = handle(); //fTray2->handle();
        xapp->send(xev, w, StructureNotifyMask);
    }
}

static const char* get_help_text() {
    return _(
    "  -n, --notify        Notify parent process by sending signal USR1.\n"
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
            if (is_switch(*arg, "n", "notify")) {
                notifyParent = true;
            }
            else if (GetArgument(value, "c", "config", arg, argv+argc)) {
                configFile = value;
            }
            else if (GetArgument(value, "t", "theme", arg, argv+argc)) {
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
#else /*CONFIG_EXTERNAL_TRAY*/
int main() {
    return 0;
}
#endif /*CONFIG_EXTERNAL_TRAY*/

// vim: set sw=4 ts=4 et:
