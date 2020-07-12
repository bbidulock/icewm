#include "config.h"
#include "yxembed.h"

YXEmbed::~YXEmbed() {
}

YXEmbedClient::YXEmbedClient(YXEmbed *embedder, YWindow *aParent, Window win):
    YWindow(aParent, win),
    fEmbedder(embedder),
    fTrace(YTrace::traces("systray"))
{
    DBG { fTrace = true; }

    fInfo[0] = 0;
    fInfo[1] = 0;

    if (xapp->alpha() == false)
        setParentRelative();
}

YXEmbedClient::~YXEmbedClient() {
}

void YXEmbedClient::handleDamageNotify(const XDamageNotifyEvent& damage) {
    fEmbedder->damagedClient();
}

void YXEmbedClient::handleDestroyWindow(const XDestroyWindowEvent& destroy) {
    MSG(("embed client destroy: evt 0x%lX, win 0x%lX, handle 0x%lX",
                destroy.event, destroy.window, handle()));

    if (destroy.window == handle()) {
        YWindow::handleDestroyWindow(destroy);
        fEmbedder->destroyedClient(destroy.window);
    }
}

void YXEmbedClient::handleReparentNotify(const XReparentEvent &reparent) {
    MSG(("embed client reparent: win 0x%lX, par 0x%lX, handle 0x%lX",
                reparent.window, reparent.parent, handle()));

    if (reparent.window == handle() &&
        reparent.parent != fEmbedder->getHandle())
    {
        if (trace())
            tlog("systray reparent notify 0x%08lx from 0x%08lx to 0x%08lx",
                    reparent.window, fEmbedder->getHandle(), reparent.parent);
        fEmbedder->destroyedClient(handle());
    }
}

void YXEmbedClient::handleConfigure(const XConfigureEvent& event) {
    MSG(("embed client configure: evt 0x%lX, win 0x%lX, width %d, height %d",
          event.event, event.window, event.width, event.height));

    YWindow::handleConfigure(event);

    int ww = int(fEmbedder->getWidth());
    int hh = int(fEmbedder->getHeight());
    if (ww >= 16 && hh >= 16 &&
        (!inrange(event.width,  ww - 4, ww + 8) ||
         !inrange(event.height, hh - 4, hh + 1)))
    {
        if (trace())
            tlog("systray configure 0x%08lx from %dx%d to %dx%d",
                 handle(), event.width, event.height, ww, hh);
        setSize(ww, hh);
    }
}

void YXEmbedClient::handleProperty(const XPropertyEvent &property) {
    if (property.atom == _XA_XEMBED_INFO) {
        YProperty prop(this, _XA_XEMBED_INFO, F32, 2L);
        if (prop && prop.size() == 2L) {
            Atom vers = prop[0];
            Atom flag = prop[1];
            if (vers == fInfo[0] && flag == fInfo[1]) {
            }
            else if (flag & XEMBED_MAPPED) {
                if (vers > XEMBED_PROTOCOL_VERSION) {
                    if (trace())
                        tlog("xembed invalid version 0x%08lx version=%ld flag=%ld",
                                property.window, vers, flag);
                    infoMapped(true);
                    sendNotify();
                    sendActivate();
                }
                if (notbit(fInfo[1], XEMBED_MAPPED)) {
                    fEmbedder->handleClientMap(handle());
                }
            }
            else if (hasbit(fInfo[1], XEMBED_MAPPED)) {
                fEmbedder->handleClientUnmap(handle());
            }
        }
    }
    // else logProperty((const XEvent&) property);
}

void YXEmbedClient::handleUnmap(const XUnmapEvent& unmap) {
    MSG(("embed client unmap: evt 0x%lX, win 0x%lX, handle 0x%lX",
                unmap.event, unmap.window, handle()));

    fEmbedder->handleClientUnmap(handle());
}

void YXEmbedClient::infoMapped(bool map) {
    fInfo[0] = XEMBED_PROTOCOL_VERSION;
    fInfo[1] = map ? XEMBED_MAPPED : Atom(0);
    if (trace())
        tlog("xembed info %smapped to  0x%08lx",
             map ? "" : "un", handle());
    setProperty(_XA_XEMBED_INFO, XA_CARDINAL, fInfo, 2);
}

void YXEmbedClient::sendNotify() {
    if (trace())
        tlog("xembed send notify to  0x%08lx", handle());
    sendMessage(XEMBED_EMBEDDED_NOTIFY, None,
                fEmbedder->getHandle(), XEMBED_PROTOCOL_VERSION);
}

void YXEmbedClient::sendActivate() {
    if (trace())
        tlog("xembed send activating 0x%08lx", handle());
    sendMessage(XEMBED_WINDOW_ACTIVATE);
}

void YXEmbedClient::sendMessage(long type, long detail, long data1, long data2) {
    XClientMessageEvent xev = {};
    xev.type = ClientMessage;
    xev.window = handle();
    xev.message_type = _XA_XEMBED;
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = type;
    xev.data.l[2] = detail;
    xev.data.l[3] = data1;
    xev.data.l[4] = data2;
    xapp->send(xev, handle(), NoEventMask);
}

// vim: set sw=4 ts=4 et:
