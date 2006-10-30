#include "config.h"
#include "ylib.h"
#include "yxtray.h"
#include "yrect.h"
#include "yxapp.h"
#include "prefs.h"
#include "yprefs.h"
#include "wmtaskbar.h"
#include "sysdep.h"

extern YColor *taskBarBg;

// make this configurable
#define TICON_H_MAX 24
#define TICON_W_MAX 30

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

    _NET_SYSTEM_TRAY_OPCODE = XInternAtom(xapp->display(),
                                          "_NET_SYSTEM_TRAY_OPCODE",
                                          False);
    _NET_SYSTEM_TRAY_MESSAGE_DATA =
        XInternAtom(xapp->display(),
                    "_NET_SYSTEM_TRAY_MESSAGE_DATA",
                    False);
    _NET_SYSTEM_TRAY_S0 = XInternAtom(xapp->display(),
                                      atom,
                                      False);

    XSetSelectionOwner(xapp->display(),
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

    XSendEvent(xapp->display(), desktop->handle(), False, StructureNotifyMask, (XEvent *) &xev);
}

YXTrayProxy::~YXTrayProxy() {
    XSetSelectionOwner(xapp->display(),
                       _NET_SYSTEM_TRAY_S0,
                       None,
                       CurrentTime);
}

void YXTrayProxy::handleClientMessage(const XClientMessageEvent &message) {
/// TODO #warning "implement systray notifications"
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
               bool internal,
               const char *atom, 
               YWindow *aParent): 
    YXEmbed(aParent) 
{
    setStyle(wsManager);
#ifndef LITE
    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }
#endif

    fNotifier = notifier;
    fInternal = internal;
    fTrayProxy = new YXTrayProxy(atom, this);
    show();
#ifndef LITE
    XSetWindowBackground(xapp->display(), handle(), taskBarBg->pixel());
#endif
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

    destroyedClient(win);
    YXEmbedClient *client = new YXEmbedClient(this, this, win);

    MSG(("size %d %d", client->width(), client->height()));

    XSetWindowBorderWidth(xapp->display(),
                          client->handle(),
                          0);

    if (!fInternal) {
        int ww = client->width();
        int hh = client->height();

        // !!! hack, hack
        if (ww < 16 || ww > 8 * TICON_W_MAX)
            ww = TICON_W_MAX;
        if (hh < 16 || hh > TICON_H_MAX)
            hh = TICON_H_MAX;

        client->setSize(ww, hh);
    }
         
    XAddToSaveSet(xapp->display(), client->handle());

    client->reparent(this, 0, 0);
//    client->show();

    fDocked.append(client);
    relayout();
}

void YXTray::destroyedClient(Window win) {
///    MSG(("undock %d", fDocked.getCount()));
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
///        msg("win %lX %lX", ec->handle(), win);
        if (ec->handle() == win) {
///            msg("removing %d %lX", i, win);
            fDocked.remove(i);
            break;
        }
    }
    relayout();
}

void YXTray::handleConfigureRequest(const XConfigureRequestEvent &configureRequest)
{
    MSG(("tray configureRequest w=%d h=%d", configureRequest.width, configureRequest.height));
    bool changed = false;
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
        if (ec->handle() == configureRequest.window) {
            int w = configureRequest.width;
            int h = configureRequest.height;
            if (h != TICON_H_MAX) {
                w = w * h / TICON_H_MAX; //MCM FIX
                h = TICON_H_MAX;
            }
            if (w != ec->width() || h != ec->height())
                changed = true;
            ec->setSize(w/*configureRequest.width*/, h);
        }
    }
    if (changed)
        relayout(); 
}

void YXTray::detachTray() {
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];

        XAddToSaveSet(xapp->display(), ec->handle());

        ec->reparent(desktop, 0, 0);
        ec->hide();
        XRemoveFromSaveSet(xapp->display(), ec->handle());
    }
    fDocked.clear();
}


void YXTray::paint(Graphics &g, const YRect &/*r*/) {
    if (fInternal) 
        return;
#ifdef CONFIG_TASKBAR
    if (taskBarBg)
        g.setColor(taskBarBg);
#endif
    g.fillRect(0, 0, width(), height());
    if (trayDrawBevel && fDocked.getCount())
        g.draw3DRect(0, 0, width() - 1, height() - 1, false);
}

void YXTray::configure(const YRect &r, const bool resized) {
    YXEmbed::configure(r, resized);
    if (resized)
        relayout();
}
void YXTray::backgroundChanged() {
    if (fInternal)
        return;
#ifdef CONFIG_TASKBAR
    XSetWindowBackground(xapp->display(),handle(), taskBarBg->pixel());
#endif
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
#ifdef CONFIG_TASKBAR
        XSetWindowBackground(xapp->display(), ec->handle(), taskBarBg->pixel());
#endif
	ec->repaint();
    }
    relayout();
    repaint();
}

void YXTray::relayout() {
    int aw = 0;
    int h  = TICON_H_MAX;
    if (!fInternal && trayDrawBevel)
        aw+=1;
    
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
        int eh(h), ew=ec->width(), ay(0);
        if (!fInternal) {
	   ew=min(TICON_W_MAX,ec->width());
           if (trayDrawBevel) {
               eh-=2; ay=1;
           }	
    }
        ec->setGeometry(YRect(aw,ay,ew,eh));
        aw += ew;
    }
    if (!fInternal && trayDrawBevel)
        aw+=1;

    int w = aw;
    if (!fInternal) {
        if (w < 1)
            w = 1;
    } else {
        if (w < 2)
            w = 0;
    }

    MSG(("relayout %d %d : %d %d", w, h, width(), height()));
    if (w != width() || h != height()) {
        setSize(w, h);
        /// messy, but works
        if (fNotifier)
            fNotifier->trayChanged();
    }
    for (unsigned int i = 0; i < fDocked.getCount(); i++) {
        YXEmbedClient *ec = fDocked[i];
        ec->show();
    }

    MSG(("clients %d width: %d", fDocked.getCount(), width()));
}

bool YXTray::kdeRequestDock(Window win) {
    if (fDocked.getCount() == 0)
        return false;
    puts("trying to dock");
    char trayatom[64];
    sprintf(trayatom, "_NET_SYSTEM_TRAY_S%d", xapp->screen());
    Atom tray = XInternAtom(xapp->display(), trayatom, False);
    Atom opcode = XInternAtom(xapp->display(), "_NET_SYSTEM_TRAY_OPCODE", False);
    Window w = XGetSelectionOwner(xapp->display(), tray);

    if (w && w != handle()) {
        XClientMessageEvent xev;
        memset(&xev, 0, sizeof(xev));

        xev.type = ClientMessage;
        xev.window = w;
        xev.message_type = opcode; //_NET_SYSTEM_TRAY_OPCODE;
        xev.format = 32;
        xev.data.l[0] = CurrentTime;
        xev.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
        xev.data.l[2] = win; //fTray2->handle();

        XSendEvent(xapp->display(), w, False, StructureNotifyMask, (XEvent *) &xev);
        return true;
    }
    return false;
}
