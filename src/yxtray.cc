#include "config.h"
#include "ylib.h"
#include "yxtray.h"
#include "yrect.h"
#include "yapp.h"
#include "wmtaskbar.h"

extern YColor *taskBarBg;

class YXTrayProxy: public YWindow {
public:
    YXTrayProxy(const char *atom, YXTray *tray, YWindow *aParent = 0);
    ~YXTrayProxy();

    virtual void handleClientMessage(const XClientMessageEvent &message);
private:
    Atom _NET_SYSTEM_TRAY_OPCODE;
    Atom _NET_SYSTEM_TRAY_MESSAGE_DATA;
    Atom _NET_SYSTEM_TRAY_S0; /// FIXME
    YXTray *fTray;
};

YXTrayProxy::YXTrayProxy(const char *atom, YXTray *tray, YWindow *aParent):
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
                                      atom,
                                      False);

    XSetSelectionOwner(app->display(),
                       _NET_SYSTEM_TRAY_S0,
                       handle(),
                       CurrentTime);

    XClientMessageEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.window = desktop->handle();
    xev.message_type = _NET_SYSTEM_TRAY_S0;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = handle();

    XSendEvent(app->display(), desktop->handle(), False, StructureNotifyMask, (XEvent *) &xev);
}

YXTrayProxy::~YXTrayProxy() {
    XSetSelectionOwner(app->display(),
                       _NET_SYSTEM_TRAY_S0,
                       None,
                       CurrentTime);
}

void YXTrayProxy::handleClientMessage(const XClientMessageEvent &message) {
#warning "implement systray notifications"
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

YXTray::YXTray(YXTrayNotifier *notifier, 
               const char *atom, 
               YWindow *aParent): 
    YXEmbed(aParent) 
{
    setStyle(wsManager);

    fNotifier = notifier;
    fTrayProxy = new YXTrayProxy(atom, this);
}

YXTray::~YXTray() {
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];

        ec->hide();
        ec->reparent(desktop, 0, 0);
    }
    delete fTrayProxy; fTrayProxy = 0;
}

void YXTray::trayRequestDock(Window win) {
    MSG(("trayRequestDock"));

    YXEmbedClient *client = new YXEmbedClient(this, this, win);

    MSG(("size %d %d", client->width(), client->height()));

    XSetWindowBorderWidth(app->display(),
                          client->handle(),
                          0);
 
    if (client->width() <= 1 && client->height() <= 1) 
        client->setSize(24, 24);
         
    XAddToSaveSet(app->display(), client->handle());

    client->reparent(this, 0, 0);
    client->show();

    fDocked.append(client);
    relayout();
}

void YXTray::destroyedClient(Window win) {
    MSG(("undock"));
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
        if (ec->handle() == win) {
            fDocked.remove(i);
            break;
        }
    }
    relayout();
}

void YXTray::handleConfigureRequest(const XConfigureRequestEvent &configureRequest)
{
    MSG(("tray configureRequest"));
    bool changed = false;
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
        if (ec->handle() == configureRequest.window) {
            if (configureRequest.width != ec->width())
                changed = true;
            ec->setSize(configureRequest.width, 24);
        }
    }
    if (changed)
        relayout(); 
}

void YXTray::detachTray() {
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];

        XAddToSaveSet(app->display(), ec->handle());

        ec->reparent(desktop, 0, 0);
        ec->hide();
        XRemoveFromSaveSet(app->display(), ec->handle());
    }
    fDocked.clear();
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
//    int w = BORDER * 2 + fDocked.getCount() * aw;
    
    int x = BORDER;
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
        ec->setGeometry(YRect(x, BORDER, ec->width(), ah));
        x += ec->width();
    }
    int w = x + BORDER;
    if (w < 1)
        w = 1;
    if (h < 24)
        h = 24;
    MSG(("relayout %d %d : %d %d", w, h, width(), height()));
    if (w != width() || h != height()) {
        setSize(w, h);
        /// messy, but works
        if (fNotifier)
            fNotifier->trayChanged();
    }
}
