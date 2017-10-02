#include "config.h"
#include "yxtray.h"
#include "yrect.h"
#include "yxapp.h"
#include "prefs.h"
#include "wmtaskbar.h"
#include "ypointer.h"
#include <X11/Xatom.h>

static const long MaxTrayMessageSize = 256;
static const long MaxTrayNumMessages = 10;
static const long MaxTrayTooltipSize = 1024;
static const long MaxTrayTooltipTime = 10*1000;

class TrayMessage {
public:
    TrayMessage(Window win, long msec, long len, long ident) :
        window(win),
        milli(clamp(msec, 0L, MaxTrayTooltipTime)),
        length(clamp(len, 0L, MaxTrayMessageSize)),
        ident(ident),
        offset(0L),
        finish(monotime() + millitime(milli))
    {
    }

    const Window window;
    const long milli;
    const long length;
    const long ident;
    csmart bytes;
    long offset;
    const timeval finish;

    bool append(const char* data, long size);
};

bool TrayMessage::append(const char* data, long size) {
    MSG(("TrayMessage::append"));
    if (offset < length) {
        if (bytes == NULL) {
            bytes = new char[1 + length];
        }
        long extra = min(length - offset, size);
        memcpy(bytes + offset, data, extra);
        bytes[offset + extra] = 0;
        offset += extra;
        return true;
    }
    return false;
}

class YXTrayProxy: public YWindow, private YTimerListener {
public:
    YXTrayProxy(const YAtom& atom, YXTray *tray, YWindow *aParent = 0);
    virtual ~YXTrayProxy();

    virtual void handleClientMessage(const XClientMessageEvent &message);
private:
    YAtom _NET_SYSTEM_TRAY_OPCODE;
    YAtom _NET_SYSTEM_TRAY_MESSAGE_DATA;
    YAtom _NET_SYSTEM_TRAY_S0;
    YXTray *fTray;
    osmart<YTimer> fUpdateTimer;
    mstring toolTip;

    typedef YObjectArray<TrayMessage> MessageListType;
    typedef MessageListType::IterType IterType;
    MessageListType messages;

    void beginMessage(Window win, long msec, long size, long ident);
    void cancelMessage(Window win, long ident);
    void messageData(Window win, const char* bytes, long maxlen);
    void expireMessages();

    bool isExternal() const {
        static const char s0[] = "_NET_SYSTEM_TRAY_S";
        return 0 == strncmp(_NET_SYSTEM_TRAY_S0.str(), s0, sizeof s0 - 1);
    }

    virtual bool handleTimer(YTimer *timer);
    virtual void updateToolTip();
};

YXTrayProxy::YXTrayProxy(const YAtom& atom, YXTray *tray, YWindow *aParent):
    YWindow(aParent),
    _NET_SYSTEM_TRAY_OPCODE("_NET_SYSTEM_TRAY_OPCODE"),
    _NET_SYSTEM_TRAY_MESSAGE_DATA("_NET_SYSTEM_TRAY_MESSAGE_DATA"),
    _NET_SYSTEM_TRAY_S0(atom),
    fTray(tray)
{
    if (isExternal()) {
        long orientation = SYSTEM_TRAY_ORIENTATION_HORZ;
        XChangeProperty(xapp->display(), handle(),
                        YAtom("_NET_SYSTEM_TRAY_ORIENTATION"),
                        XA_CARDINAL, 32, PropModeReplace,
                        (unsigned char *) &orientation, 1);

    /** Visual not needed as long as we always use the default.
        unsigned long visualid = xapp->visual()->visualid;
        XChangeProperty(xapp->display(), handle(),
                        YAtom("_NET_SYSTEM_TRAY_VISUAL"),
                        XA_VISUALID, 32, PropModeReplace,
                        (unsigned char *) &visualid, 1);
    **/
    }

    XSetSelectionOwner(xapp->display(),
                       _NET_SYSTEM_TRAY_S0,
                       handle(),
                       CurrentTime);

    XClientMessageEvent xev = {};
    xev.type = ClientMessage;
    xev.window = desktop->handle();
    xev.message_type = YAtom("MANAGER");
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = _NET_SYSTEM_TRAY_S0;
    xev.data.l[2] = handle();

    xapp->send(xev, desktop->handle(), StructureNotifyMask);
}

YXTrayProxy::~YXTrayProxy() {
    Window w = XGetSelectionOwner(xapp->display(), _NET_SYSTEM_TRAY_S0);
    if (w == handle()) {
        XSetSelectionOwner(xapp->display(),
                           _NET_SYSTEM_TRAY_S0,
                           None,
                           CurrentTime);
    }
}

void YXTrayProxy::beginMessage(Window win, long msec, long size, long ident) {
    MSG(("YXTrayProxy::beginMessage"));
    if (messages.getCount() < MaxTrayNumMessages) {
        messages.append(new TrayMessage(win, msec, size, ident));
    }
}

void YXTrayProxy::cancelMessage(Window win, long ident) {
    MSG(("YXTrayProxy::cancelMessage"));
    bool change = false;
    for (IterType iter = messages.reverseIterator(); ++iter; ) {
        if (iter->window == win && iter->ident == ident) {
            change |= (iter->offset > 0);
            iter.remove();
        }
    }
    if (change)
        updateToolTip();
}

void YXTrayProxy::messageData(Window win, const char* bytes, long len) {
    MSG(("YXTrayProxy::messageData"));
    bool change = false;
    for (IterType iter = messages.reverseIterator(); ++iter; ) {
        if (iter->window == win) {
            change = iter->append(bytes, len);
            break;
        }
    }
    if (change)
        updateToolTip();
}

void YXTrayProxy::expireMessages() {
    timeval now = monotime();
    for (IterType iter = messages.reverseIterator(); ++iter; ) {
        if (iter->finish < now) {
            iter.remove();
        }
    }
}

void YXTrayProxy::updateToolTip() {
    MSG(("YXTrayProxy::updateToolTip"));
    long size = 0;
    if (messages.getCount() > 0) {
        expireMessages();
        for (IterType iter = messages.iterator(); ++iter; ) {
            if (iter->offset > 0) {
                size += 2 + iter->offset;
                if (size > MaxTrayTooltipSize)
                    break;
            }
        }
    }
    if (size == 0) {
        if (toolTip != null) {
            toolTip = null;
            setToolTip(null);
        }
        if (fUpdateTimer)
            fUpdateTimer = NULL;
        return;
    }

    long len = 0;
    csmart text(new char[3 + size]);
    for (IterType iter = messages.iterator(); ++iter; ) {
        if (size < len + iter->offset) {
            break;
        }
        else if (iter->offset > 0) {
            memcpy(text + len, iter->bytes, iter->offset);
            len += iter->offset;
            if (text[len-1] != '\n')
                text[len++] = '\n';
            text[len++] = '\n';
        }
    }
    text[len] = '\0';

    mstring newTip(text, int(len));
    if (toolTip != newTip) {
        toolTip = newTip;
        setToolTip(newTip);
        if (fUpdateTimer == NULL) {
            const long timerInterval = 500L;
            fUpdateTimer = new YTimer(timerInterval);
            fUpdateTimer->setTimerListener(this);
        }
    }
    if (false == fUpdateTimer->isRunning()) {
        fUpdateTimer->startTimer();
    }
}

bool YXTrayProxy::handleTimer(YTimer *timer) {
    MSG(("YXTrayProxy::handleTimer"));
    if (timer == fUpdateTimer) {
        updateToolTip();
    }
    return false;
}

void YXTrayProxy::handleClientMessage(const XClientMessageEvent &message) {
    const Window window = message.window;
    unsigned long type = message.message_type;
    const long opcode = message.data.l[1];

    if (type == _NET_SYSTEM_TRAY_OPCODE) {
        if (opcode == SYSTEM_TRAY_REQUEST_DOCK) {
            MSG(("systemTrayRequestDock"));
            fTray->trayRequestDock(message.data.l[2]);
        }
        else if (opcode == SYSTEM_TRAY_BEGIN_MESSAGE) {
            const long milli = message.data.l[2];
            const long bytes = message.data.l[3];
            const long ident = message.data.l[4];
            MSG(("systemTrayBeginMessage win=%lX, mil=%ld, len=%ld, id=%ld",
                  window, milli, bytes, ident));
            beginMessage(window, milli, bytes, ident);
        }
        else if (opcode == SYSTEM_TRAY_CANCEL_MESSAGE) {
            const long ident = message.data.l[2];
            MSG(("systemTrayCancelMessage win=%lX, id=%ld", window, ident));
            cancelMessage(window, ident);
        }
        else {
            MSG(("systemTray???Message %ld ignored", opcode));
        }
    }
    else if (type == _NET_SYSTEM_TRAY_MESSAGE_DATA) {
        MSG(("systemTrayMessageData"));
        const long length = long(sizeof message.data.b);
        messageData(window, message.data.b, length);
    }
    else {
        MSG(("YXTrayProxy::handleClientMessage type %lu ignored", type));
    }
}

YXTrayEmbedder::YXTrayEmbedder(YXTray *tray, Window win): YXEmbed(tray) {
    fTray = tray;
    setStyle(wsManager);
    fDocked = new YXEmbedClient(this, this, win);

    XSetWindowBorderWidth(xapp->display(),
                          client_handle(),
                          0);

    XAddToSaveSet(xapp->display(), client_handle());

    fDocked->reparent(this, 0, 0);
    fVisible = true;
    fDocked->show();
}

YXTrayEmbedder::~YXTrayEmbedder() {
    fDocked->hide();
    fDocked->reparent(desktop, 0, 0);
    delete fDocked;
    fDocked = 0;
}

void YXTrayEmbedder::detach() {
    XAddToSaveSet(xapp->display(), fDocked->handle());

    fDocked->reparent(desktop, 0, 0);
    fDocked->hide();
    XRemoveFromSaveSet(xapp->display(), fDocked->handle());
}

bool YXTrayEmbedder::destroyedClient(Window win) {
    return fTray->destroyedClient(win);
}

void YXTrayEmbedder::handleClientUnmap(Window win) {
    fTray->showClient(win, false);
}

void YXTrayEmbedder::paint(Graphics &g, const YRect &/*r*/) {
#ifdef CONFIG_TASKBAR
    g.setColor(getTaskBarBg());
#endif
    g.fillRect(0, 0, width(), height());
}

void YXTrayEmbedder::configure(const YRect &r) {
    YXEmbed::configure(r);
    fDocked->setGeometry(YRect(0, 0, r.width(), r.height()));
}

void YXTrayEmbedder::handleConfigureRequest(const XConfigureRequestEvent &configureRequest)
{
    fTray->handleConfigureRequest(configureRequest);
}

void YXTrayEmbedder::handleMapRequest(const XMapRequestEvent &mapRequest) {
    fDocked->show();
    fTray->showClient(mapRequest.window, true);
}

YXTray::YXTray(YXTrayNotifier *notifier,
               bool internal,
               const YAtom& atom,
               YWindow *aParent):
    YWindow(aParent)
{
    fNotifier = notifier;
    fInternal = internal;
    fTrayProxy = new YXTrayProxy(atom, this);
    show();
#ifndef LITE
#ifdef CONFIG_TASKBAR
    XSetWindowBackground(xapp->display(), handle(), getTaskBarBg()->pixel());
#endif
    XClearArea(xapp->display(), handle(), 0, 0, 0, 0, True);
#endif
}

YXTray::~YXTray() {
    delete fTrayProxy; fTrayProxy = 0;
}

void YXTray::getScaleSize(int *ww, int *hh)
{
    // check if max_width / max_height < width / height. */
    if (*hh * trayIconMaxWidth < *ww * trayIconMaxHeight) {
        // icon is wide.
        if (*ww != trayIconMaxWidth) {
            *hh = (trayIconMaxWidth * *hh + (*ww / 2)) / *ww;
            *ww = trayIconMaxWidth;
        }
    } else {
        // icon is tall.
        if (*hh != trayIconMaxHeight) {
            *ww = (trayIconMaxHeight * *ww + (*hh / 2)) / *hh;
            *hh = trayIconMaxHeight;
        }
    }
}

void YXTray::trayRequestDock(Window win) {
    MSG(("trayRequestDock win %lX", win));

    if (destroyedClient(win)) {
        MSG(("docking a destroyed window"));
    }
    YXTrayEmbedder *embed = new YXTrayEmbedder(this, win);

    int ww = embed->client()->width();
    int hh = embed->client()->height();

    MSG(("docking window size %d %d", ww, hh));

    /* Workaround for GTK-Apps */
    if (ww && hh) {
        if (!fInternal) {
            // scale icons
            getScaleSize(&ww, &hh);
        }
        embed->setSize(ww, hh);
        embed->fVisible = true;
        fDocked.append(embed);
        relayout();
    }
    /* else leak embed ? */
}

bool YXTray::destroyedClient(Window win) {
    MSG(("undock N=%d, win=%lX", fDocked.getCount(), win));
    bool change = false;
    for (IterType ec = fDocked.reverseIterator(); ++ec; ) {
        MSG(("win %lX, %d handle %lX", win, ec.where(), ec->handle()));
        if (ec->client_handle() == win) {
            MSG(("removing i=%d, win=%lX", ec.where(), win));
            ec.remove();
            change = true;
        }
    }
    if (change)
        relayout();
    return change;
}

void YXTray::handleConfigureRequest(const XConfigureRequestEvent &configureRequest)
{
    MSG(("tray configureRequest w=%d h=%d internal=%s\n",
        configureRequest.width, configureRequest.height, boolstr(fInternal)));
    bool changed = false;
    for (IterType ec = fDocked.iterator(); ++ec; ) {
        if (ec->client_handle() == configureRequest.window) {
            int ww = configureRequest.width;
            int hh = configureRequest.height;
            if (!fInternal) {
                /* scale icons */
                getScaleSize(&ww, &hh);
            }
            if (ww != ec->width() || hh != ec->height())
                changed = true;
            ec->setSize(ww, hh);
        }
    }
    if (changed)
        relayout();
}

void YXTray::showClient(Window win, bool showClient) {
    for (IterType ec = fDocked.iterator(); ++ec; ) {
        if (ec->client_handle() == win) {
            if (ec->fVisible != showClient) {
                ec->fVisible = showClient;
                if (showClient)
                    ec->show();
                else
                    ec->hide();
                relayout();
            }
        }
    }
}

void YXTray::detachTray() {
    for (IterType ec = fDocked.iterator(); ++ec; ) {
        ec->detach();
    }
    fDocked.clear();
}


void YXTray::paint(Graphics &g, const YRect &/*r*/) {
    if (fInternal)
        return;
#ifdef CONFIG_TASKBAR
    g.setColor(getTaskBarBg());
#endif
    g.fillRect(0, 0, width(), height());
    if (trayDrawBevel && fDocked.getCount())
        g.draw3DRect(0, 0, width() - 1, height() - 1, false);
}

void YXTray::configure(const YRect &r) {
    YWindow::configure(r);
    relayout();
}

void YXTray::backgroundChanged() {
    if (fInternal)
        return;
#ifdef CONFIG_TASKBAR
    unsigned long bg = getTaskBarBg()->pixel();
    XSetWindowBackground(xapp->display(), handle(), bg);
#endif
    for (IterType ec = fDocked.iterator(); ++ec; ) {
#ifdef CONFIG_TASKBAR
        XSetWindowBackground(xapp->display(), ec->handle(), bg);
        XSetWindowBackground(xapp->display(), ec->client_handle(), bg);
	/* something is not clearing which background changes */
	XClearArea(xapp->display(), ec->client_handle(), 0, 0, 0, 0, True);
#endif
	ec->repaint();
    }
    relayout();
    repaint();
}

void YXTray::relayout() {
    int aw = 0;
    int h  = trayIconMaxHeight;
    if (!fInternal && trayDrawBevel)
        aw+=1;
    int cnt = 0;

    /*
       sanity check - remove already destroyed xwindows
    */
    for (IterType ec = fDocked.reverseIterator(); ++ec; ) {
        XWindowAttributes attributes;
        int status = XGetWindowAttributes(xapp->display(),
                                          ec->client()->handle(), &attributes);
        if (status == 0) {
            MSG(("relayout sanity check: removing %lX", ec->client()->handle()));
            ec.remove();
        }
    }

    for (IterType ec = fDocked.iterator(); ++ec; ) {
        if (!ec->fVisible)
            continue;
        cnt++;
        int eh(h), ew=ec->width(), ay(0);
        if (!fInternal) {
            ew = min(trayIconMaxWidth, ec->width());
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
    if (cnt == 0) {
        hide();
        w = 0;
    } else {
        show();
    }
    MSG(("relayout %d %d : %d %d", w, h, width(), height()));
    if (w != width() || h != height()) {
        setSize(w, h);
        if (fNotifier)
            fNotifier->trayChanged();
    }
    for (IterType ec = fDocked.iterator(); ++ec; ) {
        if (ec->fVisible)
            ec->show();
    }

    MSG(("clients %d width: %d, visible %s", fDocked.getCount(), width(), boolstr(visible())));
}

bool YXTray::kdeRequestDock(Window win) {
    if (fDocked.getCount() == 0)
        return false;
    puts("trying to dock");
    YAtom _NET_SYSTEM_TRAY_S0("_NET_SYSTEM_TRAY_S", true);
    YAtom opcode("_NET_SYSTEM_TRAY_OPCODE");
    Window w = XGetSelectionOwner(xapp->display(), _NET_SYSTEM_TRAY_S0);

    if (w && w != handle()) {
        XClientMessageEvent xev = {};
        xev.type = ClientMessage;
        xev.window = w;
        xev.message_type = opcode; //_NET_SYSTEM_TRAY_OPCODE;
        xev.format = 32;
        xev.data.l[0] = CurrentTime;
        xev.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
        xev.data.l[2] = win; //fTray2->handle();
        xapp->send(xev, w, StructureNotifyMask);
        return true;
    }
    return false;
}
