#include "config.h"
#include "ylib.h"
#include "yxtray.h"
#include "yrect.h"
#include "yapp.h"
#include "wmtaskbar.h"

extern YColor *taskBarBg;

class YXTrayProxy: public YWindow {
public:
    YXTrayProxy(YXTray *tray, YWindow *aParent = 0);

    virtual void handleClientMessage(const XClientMessageEvent &message);
private:
    Atom _NET_SYSTEM_TRAY_OPCODE;
    Atom _NET_SYSTEM_TRAY_MESSAGE_DATA;
    Atom _NET_SYSTEM_TRAY_S0; /// FIXME
    YXTray *fTray;
};

YXTrayProxy::YXTrayProxy(YXTray *tray, YWindow *aParent):
    YWindow(aParent)
{
    fTray = tray;
    _NET_SYSTEM_TRAY_OPCODE = XInternAtom(app->display(),
                                          "_NET_SYSTEM_TRAY_OPCODE",
                                          False);
    _NET_SYSTEM_TRAY_MESSAGE_DATA =
        XInternAtom(app->display(),
                    "_NET_SYSTEM_TRAY_MESSAGE_DATA",
                    False);
    _NET_SYSTEM_TRAY_S0 = XInternAtom(app->display(),
                                           "_NET_SYSTEM_TRAY_S0",
                                      False);

    XSetSelectionOwner(app->display(),
                       _NET_SYSTEM_TRAY_S0,
                       handle(),
                       CurrentTime);
}

void YXTrayProxy::handleClientMessage(const XClientMessageEvent &message) {
    if (message.message_type == _NET_SYSTEM_TRAY_OPCODE) {
        if (message.data.l[1] == SYSTEM_TRAY_REQUEST_DOCK)
            fTray->trayRequestDock(message.data.l[2]);
        else if (message.data.l[1] == SYSTEM_TRAY_BEGIN_MESSAGE)
            msg("systemTrayBeginMessage");
            //fTray->trayBeginMessage();
        else if (message.data.l[1] == SYSTEM_TRAY_CANCEL_MESSAGE)
            msg("systemTrayCancelMessage");
            //fTray->trayCancelMessage();
        else {
            msg("systemTray???Message");
        }
    } else if (message.message_type == _NET_SYSTEM_TRAY_MESSAGE_DATA) {
        msg("systemTrayMessageData");

    }
}

YXTray::YXTray(YWindow *aParent): YXEmbed(aParent) {
    fTrayProxy = new YXTrayProxy(this);
}

YXTray::~YXTray() {
    delete fTrayProxy; fTrayProxy = 0;
}

void YXTray::trayRequestDock(Window win) {
    msg("trayRequestDock");


    YXEmbedClient *client = new YXEmbedClient(this, this, win);

    XSetWindowBorderWidth(app->display(),
                          client->handle(),
                          0);

    XAddToSaveSet(app->display(), client->handle());

    client->reparent(this, 0, 0);
    client->show();

    fDocked.append(client);
    relayout();
}

void YXTray::destroyedClient(Window win) {
    msg("undock");
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
        if (ec->handle() == win) {
            fDocked.remove(i);
            break;
        }
    }
    relayout();
}

void YXTray::paint(Graphics &g, const YRect &/*r*/) {
    g.setColor(taskBarBg);
#define BORDER 0
    if (BORDER == 1)
        g.draw3DRect(0, 0, width() - 1, height() - 1, false);

    g.fillRect(BORDER, BORDER, width() - 2 * BORDER, height() - 2 * BORDER);
}

void YXTray::configure(const YRect &r, const bool resized) {
    YXEmbed::configure(r, resized);
    if (resized)
        relayout();
}

void YXTray::relayout() {
    int aw = 24;
    int ah = 24;
    /// FIXME
    int h = ah + BORDER * 2;
    int w = BORDER * 2 + fDocked.getCount() * aw;

    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
        ec->setGeometry(YRect(BORDER + i * aw, BORDER, aw, ah));
    }
    if (w != width() || h != height()) {
        setSize(w, h);
        /// messy, but works
        if (taskBar)
            taskBar->relayout();
    }
}
