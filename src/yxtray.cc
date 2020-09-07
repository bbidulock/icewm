#include "config.h"
#include "yxtray.h"
#include "prefs.h"
#include "wmoption.h"
#include "ytimer.h"
#include <X11/Xutil.h>
#include <X11/Xproto.h>

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
        if (bytes == nullptr) {
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

static bool windowDestroyed(Window win) {
    XWindowAttributes attr;
    return None == XGetWindowAttributes(xapp->display(), win, &attr);
}

static int getOrder(mstring title) {
    WindowOption opt(title);
    if (hintOptions)
        hintOptions->mergeWindowOption(opt, title, true);
    if (defOptions)
        defOptions->mergeWindowOption(opt, title, false);
    return opt.order;
}

struct DockRequest {
    Window window;
    lazy<YTimer> timer;
    mstring title;
    DockRequest(Window w, YTimer* t, mstring s):
        window(w), timer(t), title(s)
    { }
};

class YXTrayProxy: public YWindow, private YTimerListener {
public:
    YXTrayProxy(const YAtom& atom, YXTray *tray);
    virtual ~YXTrayProxy();

    virtual void handleClientMessage(const XClientMessageEvent &message);
private:
    YAtom _NET_SYSTEM_TRAY_S0;
    YXTray *fTray;
    lazy<YTimer> fUpdateTimer;
    YObjectArray<DockRequest> fDockRequests;
    typedef YObjectArray<DockRequest>::IterType DockIter;
    mstring toolTip;
    unsigned char error_code;
    unsigned char request_code;
    static YXTrayProxy* singleton;

    bool requestDock(Window win);
    bool enableBackingStore(Window win);
    void handleError(XErrorEvent *xerr);
    mstring fetchTitle(Window win);
    static int dockError(Display *disp, XErrorEvent *xerr);

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
    bool trace() const {
        return fTray->trace();
    }

    virtual bool handleTimer(YTimer *timer);
    virtual void updateToolTip();
};
YXTrayProxy* YXTrayProxy::singleton;

YXTrayProxy::YXTrayProxy(const YAtom& atom, YXTray *tray):
    _NET_SYSTEM_TRAY_S0(atom),
    fTray(tray),
    error_code(0),
    request_code(0)
{
    singleton = this;
    addStyle(wsNoExpose);
    setTitle("YXTrayProxy");
    if (isExternal()) {
        long orientation = SYSTEM_TRAY_ORIENTATION_HORZ;
        setProperty(_XA_NET_SYSTEM_TRAY_ORIENTATION, XA_CARDINAL, orientation);

        unsigned long visualid = xapp->visual()->visualid;
        setProperty(_XA_NET_SYSTEM_TRAY_VISUAL, XA_VISUALID, visualid);
    }

    XSetSelectionOwner(xapp->display(), _NET_SYSTEM_TRAY_S0,
                       handle(), CurrentTime);

    XClientMessageEvent xev = {};
    xev.type = ClientMessage;
    xev.window = desktop->handle();
    xev.message_type = _XA_MANAGER;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = _NET_SYSTEM_TRAY_S0;
    xev.data.l[2] = handle();

    xapp->send(xev, desktop->handle(), StructureNotifyMask);
}

YXTrayProxy::~YXTrayProxy() {
    Window w = XGetSelectionOwner(xapp->display(), _NET_SYSTEM_TRAY_S0);
    if (w == handle()) {
        XSetSelectionOwner(xapp->display(), _NET_SYSTEM_TRAY_S0,
                           None, CurrentTime);
    }
}

void YXTrayProxy::beginMessage(Window win, long msec, long size, long ident) {
    if (trace())
        tlog("systray begin message 0x%08lx: ms=%ld, sz=%ld, id=%ld",
                win, msec, size, ident);
    if (messages.getCount() < MaxTrayNumMessages) {
        messages.append(new TrayMessage(win, msec, size, ident));
    }
}

void YXTrayProxy::cancelMessage(Window win, long ident) {
    if (trace())
        tlog("systray cancel message 0x%08lx: id=%ld", win, ident);
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
    if (trace())
        tlog("systray message data 0x%08lx: len=%ld", win, len);
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
    if (messages.nonempty()) {
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

    DockIter dock = fDockRequests.iterator();
    while (++dock && timer != dock->timer);
    if (dock) {
        fTray->trayRequestDock(dock->window, dock->title);
        dock.remove();
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

bool YXTrayProxy::requestDock(Window win) {
    mstring title(fetchTitle(win));
    MSG(("systemTrayRequestDock 0x%lX, title \"%s\"", win, title.c_str()));

    if (title == null && (error_code == BadWindow || windowDestroyed(win))) {
        MSG(("Ignoring tray request for unknown window 0x%08lX", win));
        return false;
    }

    /*
     * Some tray apps (GTK) sometimes fail to respond to expose events.
     * As an experiment, enable backing store for icons to mitigate that.
     */
    if (enableBackingStore(win) == false) {
        MSG(("Cannot get attributes for dock window 0x%08lX", win));
        return false;
    }

    if (title == "Pidgin") {
        long delay = 200L + 25L * fDockRequests.getCount();
        YTimer* tm = new YTimer(delay, this, true, true);
        fDockRequests.append(new DockRequest(win, tm, title));
        return true;
    }

    return fTray->trayRequestDock(win, title);
}

mstring YXTrayProxy::fetchTitle(Window win) {
    mstring title;
    xsmart<char> name;
    error_code = request_code = 0;
    if (XFetchName(xapp->display(), win, &name)) {
        title = (char *) name;
    }
    else if (error_code == BadWindow) {
        return null;
    }
    if (title.isEmpty()) {
        XTextProperty text = {};
        if (XGetTextProperty(xapp->display(), win, &text, _XA_NET_WM_NAME)) {
            title = reinterpret_cast<char *>(text.value);
            XFree(text.value);
        }
        else if (error_code == BadWindow) {
            return null;
        }
    }
    if (title.isEmpty()) {
        XClassHint hint;
        if (XGetClassHint(xapp->display(), win, &hint)) {
            title = hint.res_name;
            XFree(hint.res_name);
            XFree(hint.res_class);
        }
        else if (error_code == BadWindow) {
            return null;
        }
    }
    return title;
}

void YXTrayProxy::handleError(XErrorEvent* e) {
    request_code = e->request_code;
    error_code = e->error_code;

    if (trace()) {
        Display* display = xapp->display();
        char message[80], req[80], number[80];

        snprintf(number, 80, "%d", e->request_code);
        XGetErrorDatabaseText(display, "XRequest",
                              number, "",
                              req, sizeof(req));
        if (req[0] == 0)
            snprintf(req, 80, "[request_code=%d]", e->request_code);

        if (XGetErrorText(display, e->error_code, message, 80) != Success)
            *message = '\0';

        tlog("systray %s(0x%08lx): %s", req, e->resourceid, message);
    }
}

int YXTrayProxy::dockError(Display *disp, XErrorEvent* e) {
    if (singleton) {
        singleton->handleError(e);
    }
    // Ignore bad dock attempts.
    return Success;
}

void YXTrayProxy::handleClientMessage(const XClientMessageEvent &message) {
    const Window window = message.window;
    unsigned long type = message.message_type;

    if (type == _XA_NET_SYSTEM_TRAY_OPCODE) {
        const long opcode = message.data.l[1];
        if (opcode == SYSTEM_TRAY_REQUEST_DOCK) {
            const Window dock = message.data.l[2];
            XErrorHandler old = XSetErrorHandler(dockError);
            error_code = request_code = 0;
            requestDock(dock);
            XSetErrorHandler(old);
        }
        else if (opcode == SYSTEM_TRAY_BEGIN_MESSAGE) {
            const long milli = message.data.l[2];
            const long bytes = message.data.l[3];
            const long ident = message.data.l[4];
            beginMessage(window, milli, bytes, ident);
        }
        else if (opcode == SYSTEM_TRAY_CANCEL_MESSAGE) {
            const long ident = message.data.l[2];
            cancelMessage(window, ident);
        }
        else if (trace()) {
            tlog("systray ignoring unknown opcode %ld from 0x%08lx",
                 opcode, window);
        }
    }
    else if (type == _XA_NET_SYSTEM_TRAY_MESSAGE_DATA) {
        const long length = long(sizeof message.data.b);
        messageData(window, message.data.b, length);
    }
    else if (trace()) {
        tlog("systray unknown client message type %ld from 0x%08lx",
             type, window);
    }
}

YXTrayEmbedder::YXTrayEmbedder(YXTray *tray, Window win, Window ldr, mstring title):
    YWindow(tray),
    fVisible(false),
    fTray(tray),
    fClient(new YXEmbedClient(this, this, win)),
    fLeader(Elvis(ldr, win)),
    fTitle(title),
    fDamage(None),
    fComposing(xapp->alpha() && composite.supported &&
               xapp->format() && damage.supported),
    fOrder(getOrder(fTitle))
{
    if (fClient->destroyed())
        return;

    setStyle(wsManager | wsNoExpose);
    setParentRelative();
    setTitle("YXTrayEmbedder");

    fClient->setBorderWidth(0);
    XAddToSaveSet(xapp->display(), win);
    fClient->reparent(this, 0, 0);

    if (composing()) {
        fDamage = XDamageCreate(xapp->display(), win, XDamageReportNonEmpty);
        XCompositeRedirectWindow(xapp->display(), win, CompositeRedirectManual);
    }

    if (xapp->alpha() == false)
        fClient->setParentRelative();
}

void YXTrayEmbedder::realise() {
    client()->show();
    client()->infoMapped();
    client()->sendNotify();
    client()->sendActivate();
}

YXTrayEmbedder::~YXTrayEmbedder() {
    if (false == fClient->destroyed()) {
        if (fDamage) {
            XDamageDestroy(xapp->display(), fDamage);
            fDamage = None;
        }
        fClient->hide();
        fClient->reparent(desktop, 0, 0);
    }
    delete fClient;
}

bool YXTrayEmbedder::trace() const {
    return fTray->trace();
}

void YXTrayEmbedder::damagedClient() {
    if (trace())
        tlog("systray damage repaint 0x%08lx", fClient->handle());
    repaint();
}

void YXTrayEmbedder::detach() {
    if (false == fClient->destroyed()) {
        if (fDamage) {
            XDamageDestroy(xapp->display(), fDamage);
            fDamage = None;
        }
        XAddToSaveSet(xapp->display(), fClient->handle());
        fClient->reparent(desktop, 0, 0);
        fClient->hide();
        XRemoveFromSaveSet(xapp->display(), fClient->handle());
        if (trace())
            tlog("systray detached dock window 0x%08lx", fClient->handle());
    }
}

bool YXTrayEmbedder::destroyedClient(Window win) {
    fDamage = None;
    return fTray->destroyedClient(win);
}

void YXTrayEmbedder::handleClientUnmap(Window win) {
    if (trace())
        tlog("systray client unmaps  0x%08lx when %s",
                win, fVisible ? "visible" : "unmapped");
    if (fVisible)
        fTray->showClient(win, false);
    else if (client()->testDestroyed())
        fTray->destroyedClient(client_handle());
}

void YXTrayEmbedder::handleClientMap(Window win) {
    if (trace())
        tlog("systray client map window 0x%08lx", win);
    fClient->show();
    fTray->showClient(win, true);
}

void YXTrayEmbedder::paint(Graphics &g, const YRect& r) {
    extern ref<YPixmap> taskbackPixmap;
    if (taskbackPixmap != null) {
        g.fillPixmap(taskbackPixmap,
                     r.x(), r.y(), r.width(), r.height(),
                     x() + r.x(), y() + r.y() + parent()->y());
    }
    else {
        g.setColor(taskBarBg);
        g.fillRect(r.x(), r.y(), r.width(), r.height());
    }

    if (composing()) {
        XDamageSubtract(xapp->display(), fDamage, None, None);
        Picture source = fClient->createPicture();
        Picture target = g.picture();
        if (source && target) {
            XRenderComposite(xapp->display(), PictOpOver,
                             source, None, target,
                             0, 0, 0, 0, 0, 0,
                             width(), height());
        }
        if (source)
            XRenderFreePicture(xapp->display(), source);
    }
}

void YXTrayEmbedder::configure(const YRect2 &r) {
    if (r.resized()) {
        repaint();
        fClient->setGeometry(YRect(0, 0, r.width(), r.height()));
    }
}

void YXTrayEmbedder::repaint() {
    GraphicsBuffer(this).paint();
}

void YXTrayEmbedder::handleConfigureRequest(const XConfigureRequestEvent &configureRequest)
{
    fTray->handleConfigureRequest(configureRequest);
}

void YXTrayEmbedder::handleClientMessage(const XClientMessageEvent& message)
{
    if (trace())
        logClientMessage(message);
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
    fTrayProxy(nullptr),
    fNotifier(notifier),
    fLocked(false),
    fRunProxy(internal == false),
    fTrace(YTrace::traces("systray")),
    fDrawBevel(drawBevel)
{
    DBG { fTrace = true; }
    addStyle(wsNoExpose);
    setTitle("YXTray");
    setParentRelative();
    fTrayProxy = new YXTrayProxy(atom, this);
    regainTrayWindows();
}

YXTray::~YXTray() {
    delete fTrayProxy; fTrayProxy = nullptr;
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
    YProperty prop(win, _XA_WM_CLIENT_LEADER, F32, 1L, XA_WINDOW);
    return prop ? *prop : None;
}

bool YXTray::trayRequestDock(Window win, mstring title) {
    if (trace())
        tlog("systray dock requested 0x%08lx \"%s\"", win, title.c_str());

    if (destroyedClient(win)) {
        if (trace())
            tlog("systray ignoring destroyed window 0x%08lx", win);
        return false;
    }

    Window leader = getLeader(win);
    if (leader == None && windowDestroyed(win)) {
        if (trace())
            tlog("systray ignoring destroyed window 0x%08lx", win);
        return false;
    }

    YXTrayEmbedder *embed = new YXTrayEmbedder(this, win, leader, title);

    unsigned ww = embed->client()->width();
    unsigned hh = embed->client()->height();

    if (trace())
        tlog("systray docking window 0x%08lx size %d %d", win, ww, hh);

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
    embed->realise();
    updateTrayWindows();

    if (trace())
        tlog("systray docked window  0x%08lx at %d", win, iter.where());

    unsigned w = max(width(), 2U * fDrawBevel) + ww;
    unsigned h = max(height(), hh + fDrawBevel);
    trayUpdateGeometry(w, h, true);
    relayout(true);
    return true;
}

bool YXTray::destroyedClient(Window win) {
    bool change = false;
    for (IterType ec = fDocked.reverseIterator(); ++ec; ) {
        if (ec->client_handle() == win) {
            ec.remove();
            if (trace())
                tlog("systray removed window 0x%08lx from %d",
                     win, ec.where());
            change = true;
        }
    }
    if (change) {
        updateTrayWindows();
        relayout(true);
    }
    return change;
}

void YXTray::handleConfigureRequest(const XConfigureRequestEvent &request)
{
    if (trace())
        tlog("systrace configure req 0x%08lx to w=%d h=%d",
            request.window, request.width, request.height);
    bool changed = false;
    for (IterType ec = fDocked.iterator(); ++ec; ) {
        if (ec->client_handle() == request.window) {
            unsigned ww = request.width;
            unsigned hh = request.height;
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
    fDocked.clear();
}

void YXTray::paint(Graphics &g, const YRect &/*r*/) {
    if (!fDrawBevel)
        return;
    g.setColor(taskBarBg);
    if (trayDrawBevel && fDocked.getCount())
        g.draw3DRect(0, 0, width() - 1, height() - 1, false);
}

void YXTray::configure(const YRect2& rect) {
    bool enforce = (fGeometry != rect);
    fGeometry = rect;
    if (rect.resized() || enforce) {
        repaint();
        relayout(true);
    } else {
        relayout(false);
    }
}

void YXTray::repaint() {
    GraphicsBuffer(this).paint();
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
            if (trace())
                tlog("systray destroyed %08lx", ec->client_handle());
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
            if (trace())
                tlog("systray sanity remove %08lx", ec->client_handle());
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

void YXTray::updateTrayWindows() {
    const int count = fDocked.getCount();
    Window windows[count];

    for (IterType ec = fDocked.iterator(); ++ec; )
        windows[ec.where()] = ec->leader();

    desktop->setProperty(_XA_KDE_NET_SYSTEM_TRAY_WINDOWS, XA_WINDOW,
                         windows, count);
}

void YXTray::regainTrayWindows() {
    YProperty prop(desktop, _XA_KDE_NET_SYSTEM_TRAY_WINDOWS,
                   F32, 123L, XA_WINDOW, True);
    fRegained.clear();
    for (int i = 0; i < int(prop.size()); ++i) {
        fRegained.append(prop[i]);
    }
}

// vim: set sw=4 ts=4 et:
