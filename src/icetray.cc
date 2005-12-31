#include "config.h"
#include "ylib.h"
#include "ylocale.h"
#include "yxapp.h"
#include "yxtray.h"
#include "base.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yprefs.h"

extern void logEvent(const XEvent &xev);

char const *ApplicationName = "icewmtray";

#include "yconfig.h"

#ifdef CONFIG_TASKBAR
YColor *taskBarBg;

XSV(const char *, clrDefaultTaskBar,            "rgb:C0/C0/C0")

cfoption icewmbg_prefs[] = {
    OSV("ColorDefaultTaskBar",                  &clrDefaultTaskBar,             "Background of the taskbar"),
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
    Atom _NET_SYSTEM_TRAY_OPCODE;
    YXTray *fTray2;
};

class SysTrayApp: public YXApplication {
public:
    SysTrayApp(int *argc, char ***argv, const char *displayName = 0);
    ~SysTrayApp();

    bool filterEvent(const XEvent &xev);
    void handleSignal(int sig);

private:
    SysTray *tray;
};

SysTrayApp::SysTrayApp(int *argc, char ***argv, const char *displayName):
    YXApplication(argc, argv, displayName)
{
    desktop->setStyle(YWindow::wsDesktopAware);
    catchSignal(SIGINT);
    catchSignal(SIGTERM);

#ifdef CONFIG_TASKBAR
#ifndef NO_CONFIGURE
    {
        cfoption theme_prefs[] = {
            OSV("Theme", &themeName, "Theme name"),
            OK0()
        };

        YConfig::findLoadConfigFile(theme_prefs, "preferences");
        YConfig::findLoadConfigFile(theme_prefs, "theme");
    }
    YConfig::findLoadConfigFile(icewmbg_prefs, "preferences");
    if (themeName != 0) {
        MSG(("themeName=%s", themeName));

        YConfig::findLoadConfigFile(icewmbg_prefs,
                                 upath("themes").child(themeName));
    }
    YConfig::findLoadConfigFile(icewmbg_prefs, "prefoverride");
#endif
#endif

#ifdef CONFIG_TASKBAR
    if (taskBarBg == 0)
        taskBarBg = new YColor(clrDefaultTaskBar);
#endif

    tray = new SysTray();
}

SysTrayApp::~SysTrayApp() {

}

bool SysTrayApp::filterEvent(const XEvent &xev) {
#ifdef DEBUG
    logEvent(xev);
#endif
    if (xev.type == ClientMessage) {
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
    fTray2->relayout();
    setSize(fTray2->width(),
            fTray2->height());
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
    if (message.message_type == icewm_internal_tray) {
        MSG(("requestDock"));
        requestDock();
    }
    return true;
}

int main(int argc, char **argv) {
    YLocale locale;

    SysTrayApp stapp(&argc, &argv);

    return app->mainLoop();
}
