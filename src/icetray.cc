#include "config.h"
#include "ylib.h"
#include "ylocale.h"
#include "yapp.h"
#include "yxtray.h"
#include "base.h"
#include "debug.h"

#include <stdlib.h>
#include <string.h>

extern void logEvent(const XEvent &xev);

char const *ApplicationName = "icewmtray";

YColor *taskBarBg;

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

class SysTrayApp: public YApplication {
public:
    SysTrayApp(int *argc, char ***argv, const char *displayName = 0);
    ~SysTrayApp();

    bool filterEvent(const XEvent &xev);
private:
    SysTray *tray;
};

SysTrayApp::SysTrayApp(int *argc, char ***argv, const char *displayName):
    YApplication(argc, argv, displayName)
{
    desktop->setStyle(YWindow::wsDesktopAware);
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

SysTray::SysTray(): YWindow(0) {
    desktop->setStyle(YWindow::wsDesktopAware);
    fTray2 = new YXTray(this, "_NET_SYSTEM_TRAY_S0", this);

    icewm_internal_tray =
        XInternAtom(app->display(), "_ICEWM_INTTRAY", False);

    _NET_SYSTEM_TRAY_OPCODE =
        XInternAtom(app->display(),
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
    Window w = XGetSelectionOwner(app->display(), icewm_internal_tray);

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

        XSendEvent(app->display(), w, False, StructureNotifyMask, (XEvent *) &xev);
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

    SysTrayApp app(&argc, &argv);

    taskBarBg = new YColor("#C0C0C0"); /// FIXME

    return app.mainLoop();
}
