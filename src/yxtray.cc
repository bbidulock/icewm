#include "config.h"
#include "yxtray.h"
#include "prefs.h"
#include "wmoption.h"
#include "ytimer.h"

extern YColorName taskBarBg;

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

bool windowDestroyed(Window win) {
    XWindowAttributes attr;
    return None == XGetWindowAttributes(xapp->display(), win, &attr);
}

int getOrder(cstring title) {
    WindowOption opt(title);
    if (defOptions)
        defOptions->mergeWindowOption(opt, title, false);
    if (hintOptions)
        hintOptions->mergeWindowOption(opt, title, true);
    return opt.order;
}

struct DockRequest {
    Window window;
    lazy<YTimer> timer;
    cstring title;
    DockRequest(Window w, YTimer* t, cstring s): window(w), timer(t), title(s) {}
};

class YXTrayProxy: public YWindow, private YTimerListener {
public:
    YXTrayProxy(const YAtom& atom, YXTray *tray);
    virtual ~YXTrayProxy();

    virtual void handleClientMessage(const XClientMessageEvent &message);
private:
    YAtom _NET_SYSTEM_TRAY_OPCODE;
    YAtom _NET_SYSTEM_TRAY_MESSAGE_DATA;
    YAtom _NET_SYSTEM_TRAY_S0;
    YAtom _NET_WM_NAME;
    YXTray *fTray;
    lazy<YTimer> fUpdateTimer;
    typedef YObjectArray<DockRequest>::IterType DockIter;
    mstring toolTip;

    void requestDock(Window win);
    cstring fetchTitle(Window win);
    bool enableBackingStore(Window win);

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

YXTrayProxy::YXTrayProxy(const YAtom& atom, YXTray *tray):
    _NET_SYSTEM_TRAY_OPCODE("_NET_SYSTEM_TRAY_OPCODE"),
    _NET_SYSTEM_TRAY_MESSAGE_DATA("_NET_SYSTEM_TRAY_MESSAGE_DATA"),
    _NET_SYSTEM_TRAY_S0(atom),
    _NET_WM_NAME("_NET_WM_NAME"),
    fTray(tray)
{
    setTitle("YXTrayProxy");
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
            fUpdateTimer = null;
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
        if ( ! fUpdateTimer)
            fUpdateTimer->setTimer(500L, this, false);
    }
    if (false == fUpdateTimer->isRunning()) {
        fUpdateTimer->startTimer();
    }
}

bool YXTrayProxy::handleTimer(YTimer *timer) {
    MSG(("YXTrayProxy::handleTimer %s %ld",
        boolstr(timer == fUpdateTimer), timer->getInterval()));

    if (timer == fUpdateTimer) {
        updateToolTip();
        return false;
    }

    return false;
}

bool YXTrayProxy::enableBackingStore(Window win) {
    XWindowAttributes attr;
    bool okay = XGetWindowAttributes(xapp->display(), win, &attr);
    if (okay && attr.backing_store == NotUseful) {
        XSetWindowAttributes xswa;
        xswa.backing_store = WhenMapped;
        XChangeWindowAttributes(xapp->display(), win, CWBackingStore, &xswa);
    }
    return okay;
}

void YXTrayProxy::requestDock(Window win) {
    cstring title(fetchTitle(win));
    MSG(("systemTrayRequestDock 0x%lX, title \"%s\"", win, title.c_str()));

    if (title == null && windowDestroyed(win)) {
        MSG(("Ignoring tray request for unknown window 0x%08lX", win));
        return;
    }

    /*
     * Some tray apps (GTK) sometimes fail to respond to expose events.
     * As an experiment, enable backing store for icons to mitigate that.
     */
    if (enableBackingStore(win) == false) {
        MSG(("Cannot get attributes for dock window 0x%08lX", win));
        return;
    }

    fTray->trayRequestDock(win, title);
}

cstring YXTrayProxy::fetchTitle(Window win) {
    xsmart<char> name;
    if (XFetchName(xapp->display(), win, &name) && name && name[0]) {
    } else {
        XTextProperty text = {};
        if (XGetTextProperty(xapp->display(), win, &text, _NET_WM_NAME))
            name = (char *) text.value;
    }
    return name && name[0] ? cstring(name) : null;
}

void YXTrayProxy::handleClientMessage(const XClientMessageEvent &message) {
    const Window window = message.window;
    unsigned long type = message.message_type;
    const long opcode = message.data.l[1];

    if (type == _NET_SYSTEM_TRAY_OPCODE) {
        if (opcode == SYSTEM_TRAY_REQUEST_DOCK) {
            const Window window = message.data.l[2];
            requestDock(window);
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

YXTrayEmbedder::YXTrayEmbedder(YXTray *tray, Window win, Window ldr, cstring title):
    YWindow(tray),
    fVisible(false),
    fTray(tray),
    fClient(new YXEmbedClient(this, this, win)),
    fLeader(Elvis(ldr, win)),
    fTitle(title),
    fRepaint(fTitle == "XXkb"), // issue #235
    fOrder(getOrder(fTitle))
{
    if (fClient->destroyed())
        return;

    setParentRelative();
    setStyle(wsManager);
    setTitle("YXTrayEmbedder");

    fClient->setBorderWidth(0);
    XAddToSaveSet(xapp->display(), win);
    fClient->reparent(this, 0, 0);

    YAtom _XEMBED("_XEMBED");
    XClientMessageEvent xev = {};

    long info[2] = { XEMBED_PROTOCOL_VERSION, XEMBED_MAPPED };

    XChangeProperty(xapp->display(), win,
            YAtom("_XEMBED_INFO"),
            XA_CARDINAL, 32, PropModeReplace,
            (unsigned char *) &info, 2);

    xev.type = ClientMessage;
    xev.window = win;
    xev.message_type = _XEMBED;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
    xev.data.l[2] = 0; // no detail
    xev.data.l[3] = handle();
    xev.data.l[4] = XEMBED_PROTOCOL_VERSION;
    xapp->send(xev, win, NoEventMask);

    xev.type = ClientMessage;
    xev.window = win;
    xev.message_type = _XEMBED;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = XEMBED_WINDOW_ACTIVATE;
    xev.data.l[2] = 0; // no detail
    xev.data.l[3] = 0; // no data1
    xev.data.l[4] = 0; // no data2
    xapp->send(xev, win, NoEventMask);

    fClient->setParentRelative();
    fVisible = true;
    fClient->show();
}

YXTrayEmbedder::~YXTrayEmbedder() {
    if (false == fClient->destroyed()) {
        fClient->hide();
        fClient->reparent(desktop, 0, 0);
    }
    delete fClient;
}

void YXTrayEmbedder::detach() {
    if (false == fClient->destroyed()) {
        XAddToSaveSet(xapp->display(), fClient->handle());
        fClient->reparent(desktop, 0, 0);
        fClient->hide();
        XRemoveFromSaveSet(xapp->display(), fClient->handle());
    }
}

bool YXTrayEmbedder::destroyedClient(Window win) {
    return fTray->destroyedClient(win);
}

void YXTrayEmbedder::handleClientUnmap(Window win) {
    fTray->showClient(win, false);
}

void YXTrayEmbedder::handleClientMap(Window win) {
    fClient->show();
    fTray->showClient(win, true);
}

void YXTrayEmbedder::paint(Graphics &g, const YRect& r) {
    if (fRepaint) {
        extern ref<YPixmap> taskbackPixmap;
        if (taskbackPixmap != null) {
            g.fillPixmap(taskbackPixmap,
                         r.x(), r.y(), r.width(), r.height(),
                         x() + r.x(), y() + r.y());
        }
        else {
            g.setColor(taskBarBg);
            g.fillRect(r.x(), r.y(), r.width(), r.height());
        }
    }
}

void YXTrayEmbedder::configure(const YRect &r) {
    YWindow::configure(r);
    fClient->setGeometry(YRect(0, 0, r.width(), r.height()));
}

void YXTrayEmbedder::handleConfigureRequest(const XConfigureRequestEvent &configureRequest)
{
    fTray->handleConfigureRequest(configureRequest);
}

void YXTrayEmbedder::handleMapRequest(const XMapRequestEvent &mapRequest) {
    fClient->show();
    fTray->showClient(mapRequest.window, true);
}

YXTray::YXTray(YXTrayNotifier *notifier,
               bool internal,
               const YAtom& atom,
               YWindow *aParent,
               bool drawBevel):
    YWindow(aParent),
    fTrayProxy(0),
    fNotifier(notifier),
    NET_TRAY_WINDOWS("_KDE_NET_SYSTEM_TRAY_WINDOWS"),
    WM_CLIENT_LEADER("WM_CLIENT_LEADER"),
    fLocked(false),
    fRunProxy(internal == false),
    fDrawBevel(drawBevel)
{
    setTitle("YXTray");
    setParentRelative();
    fTrayProxy = new YXTrayProxy(atom, this);
    regainTrayWindows();
}

YXTray::~YXTray() {
    delete fTrayProxy; fTrayProxy = 0;
}

void YXTray::getScaleSize(unsigned& w, unsigned& h)
{
    // check if max_width / max_height < width / height. */
    if (h * trayIconMaxWidth < w * trayIconMaxHeight) {
        // icon is wide.
        if (w != trayIconMaxWidth) {
            h = (trayIconMaxWidth * h + (w / 2)) / w;
            w = trayIconMaxWidth;
        }
    } else {
        // icon is tall.
        if (h != trayIconMaxHeight) {
            w = (trayIconMaxHeight * w + (h / 2)) / h;
            h = trayIconMaxHeight;
        }
    }
}

Window YXTray::getLeader(Window win) {
    Atom type = None;
    int format = None;
    const long justOne = 1L;
    unsigned long count = 0;
    unsigned long extra = 0;
    xsmart<Window> data;
    Window leader = None;
    int status =
        XGetWindowProperty(xapp->display(), win,
                           WM_CLIENT_LEADER, 0L, justOne,
                           False, XA_WINDOW,
                           &type, &format, &count, &extra,
                           (unsigned char **) &data);
    if (status == Success && data != 0 && format == 32 && count == justOne) {
        leader = data[0];
    }
    return leader;
}

void YXTray::trayRequestDock(Window win, cstring title) {
    MSG(("trayRequestDock win 0x%lX, title \"%s\"", win, title.c_str()));

    if (destroyedClient(win)) {
        MSG(("Ignoring tray request for destroyed window 0x%08lX", win));
        return;
    }

    Window leader = getLeader(win);
    if (leader == None && windowDestroyed(win)) {
        MSG(("Ignoring tray request for failing window 0x%08lX", win));
        return;
    }

    YXTrayEmbedder *embed = new YXTrayEmbedder(this, win, leader, title);

    unsigned ww = embed->client()->width();
    unsigned hh = embed->client()->height();

    MSG(("docking window size %d %d", ww, hh));

    /* Workaround for GTK-Apps */
    if (ww == 0 && hh == 0)
        ww = hh = 24;

    if (fRunProxy) {
        // scale icons
        getScaleSize(ww, hh);
    }
    embed->setSize(ww, hh);
    embed->fVisible = true;

    int found = find(fRegained, Elvis(leader, win));
    IterType iter = fDocked.iterator();
    while (++iter &&
           (iter->order() < embed->order() ||
            (iter->order() == embed->order() &&
             found >= 0 &&
             inrange(find(fRegained, iter->leader()), 0, found - 1))));

    iter.insert(embed);
    updateTrayWindows();

    unsigned w = max(width(), 2U * fDrawBevel) + ww;
    unsigned h = max(height(), hh + fDrawBevel);
    trayUpdateGeometry(w, h, true);
    relayout(true);
}

bool YXTray::destroyedClient(Window win) {
    MSG(("undock N=%d, win=%lX", fDocked.getCount(), win));
    bool change = false;
    for (IterType ec = fDocked.reverseIterator(); ++ec; ) {
        MSG(("win %lX, %d handle %lX", win, ec.where(), ec->handle()));
        if (ec->client_handle() == win) {
            MSG(("removing i=%d, win=%lX", ec.where(), win));
            ec.remove();
            updateTrayWindows();
            change = true;
        }
    }
    if (change)
        relayout(true);
    return change;
}

void YXTray::handleConfigureRequest(const XConfigureRequestEvent &configureRequest)
{
    MSG(("tray configureRequest w=%d h=%d\n",
        configureRequest.width, configureRequest.height));
    bool changed = false;
    for (IterType ec = fDocked.iterator(); ++ec; ) {
        if (ec->client_handle() == configureRequest.window) {
            unsigned ww = configureRequest.width;
            unsigned hh = configureRequest.height;
            if (fRunProxy) {
                /* scale icons */
                getScaleSize(ww, hh);
            }
            if (ww != ec->width() || hh != ec->height())
                changed = true;
            ec->setSize(ww, hh);
        }
    }
    if (changed)
        relayout(true);
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
                relayout(true);
            }
        }
    }
}

void YXTray::detachTray() {
    for (IterType ec = fDocked.reverseIterator(); ++ec; ) {
        ec->detach();
    }
    fDocked.clear();
}


void YXTray::paint(Graphics &g, const YRect &/*r*/) {
    if (!fDrawBevel)
        return;
    g.setColor(taskBarBg);
    if (trayDrawBevel && fDocked.getCount())
        g.draw3DRect(0, 0, width() - 1, height() - 1, false);
}

void YXTray::configure(const YRect& rect) {
    bool enforce = (fGeometry != rect);
    fGeometry = rect;
    YWindow::configure(rect);
    relayout(enforce);
}

void YXTray::backgroundChanged() {
    if (fDrawBevel)
        return;
    relayout(true);
    repaint();
    for (IterType ec = fDocked.iterator(); ++ec; ) {
        /* something is not clearing which background changes */
        XClearArea(xapp->display(), ec->client_handle(), 0, 0, 0, 0, True);
        ec->repaint();
    }
}

void YXTray::relayout(bool enforced) {
    if (fLocked)
        return;
    Lock lock(&fLocked);

    int aw = 0;
    int countVisible = 0;
    const unsigned h = trayIconMaxHeight + fDrawBevel;
    XWindowAttributes attr;

    for (IterType ec = fDocked.reverseIterator(); ++ec; ) {
        if (ec->client()->destroyed()) {
            ec.remove();
            updateTrayWindows();
            enforced = true;
        }
    }

    if (enforced == false)
        return;

    for (IterType ec = fDocked.iterator(); ++ec; ) {
        if (false == ec->fVisible) {
            // skip
        }
        else if (ec->client()->getWindowAttributes(&attr)) {
            int eh = h - fDrawBevel;
            int ew = ec->width();
            int ay = fDrawBevel;
            aw = max(int(fDrawBevel), aw);
            ec->setGeometry(YRect(aw, ay, ew, eh));
            ec->client()->setGeometry(YRect(0, 0, ew, eh));
            aw += ew;
            countVisible++;
        }
        else {
            MSG(("relayout sanity remove %lX", ec->client_handle()));
            ec.remove();
            updateTrayWindows();
            --ec;
        }
    }
    aw += fDrawBevel;

    unsigned w = aw;
    if (fRunProxy) {
        if (w < 1)
            w = 1;
    }
    if (fDrawBevel) {
        if (w < 4)
            w = 0;
    }
    trayUpdateGeometry(w, h, countVisible > 0);

    for (IterType ec = fDocked.iterator(); ++ec; ) {
        if (ec->fVisible)
            ec->show();
    }

    MSG(("clients %d width: %d, visible %s",
         fDocked.getCount(), width(), boolstr(visible())));
}

void YXTray::trayUpdateGeometry(unsigned w, unsigned h, bool visible) {
    if (visible == false) {
        hide();
        w = 0;
    }
    MSG(("relayout %d %d : %d %d", w, h, width(), height()));
    if (w != width() || h != height()) {
        fGeometry.setRect(x() + int(width()) - int(w), y(), w, h);
        setGeometry(fGeometry);
        if (visible)
            show();
        if (fNotifier)
            fNotifier->trayChanged();
    }
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

void YXTray::updateTrayWindows() {
    const int count = fDocked.getCount();
    Window windows[count];

    for (IterType ec = fDocked.iterator(); ++ec; )
        windows[ec.where()] = ec->leader();

    XChangeProperty(xapp->display(), xapp->root(),
                    NET_TRAY_WINDOWS,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *) windows, count);
}

void YXTray::regainTrayWindows() {
    const bool destroy = true;
    Atom type = None;
    int format = None;
    const long limit = 123L;
    unsigned long count = 0;
    unsigned long extra = 0;
    xsmart<Window> data;
    int status =
        XGetWindowProperty(xapp->display(), xapp->root(),
                           NET_TRAY_WINDOWS,
                           0L, limit, destroy, XA_WINDOW,
                           &type, &format, &count, &extra,
                           (unsigned char **) &data);

    fRegained.clear();
    if (status == Success && data != 0 && type == XA_WINDOW && format == 32) {
        for (int i = 0; i < int(count); ++i) {
            fRegained.append(data[i]);
        }
    }
}

// vim: set sw=4 ts=4 et:
