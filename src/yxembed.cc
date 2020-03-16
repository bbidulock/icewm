#include "config.h"
#include "yxembed.h"

YXEmbed::~YXEmbed() {
}

YXEmbedClient::YXEmbedClient(YXEmbed *embedder, YWindow *aParent, Window win):
    YWindow(aParent, win),
    fEmbedder(embedder)
{
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
            reparent.parent != fEmbedder->getHandle()) {
        if (false == fEmbedder->destroyedClient(handle())) {
            YWindow::handleReparentNotify(reparent);
        }
    }
}

void YXEmbedClient::handleConfigure(const XConfigureEvent& event) {
    MSG(("embed client configure: evt 0x%lX, win 0x%lX, width %d, height %d",
          event.event, event.window, event.width, event.height));

    YWindow::handleConfigure(event);

    int ww = (int) fEmbedder->getWidth();
    int hh = (int) fEmbedder->getHeight();
    if (ww >= 16 && hh >= 16 &&
        (!inrange(event.width,  ww - 4, ww + 8) ||
         !inrange(event.height, hh - 4, hh + 1)))
    {
        MSG(("embed client configure: setSize ww %u, hh %u", ww, hh));
        setSize(ww, hh);
    }
}

void YXEmbedClient::handleProperty(const XPropertyEvent &property) {
    // logProperty((const XEvent&) property);

    if (property.atom == _XA_XEMBED_INFO) {
        YProperty prop(this, _XA_XEMBED_INFO, F32, 2L);
        if (prop && prop.size() == 2L) {
            long vers = prop[0];
            long flag = prop[1];
            if (flag & XEMBED_MAPPED) {
                if (vers > XEMBED_PROTOCOL_VERSION) {
                    TLOG(("embed 0x%lx: %ld, %ld", property.window, vers, flag));
                    infoMapped(true);
                    sendNotify();
                    sendActivate();
                }
//              fEmbedder->handleClientMap(handle());
            } else {
//              fEmbedder->handleClientUnmap(handle());
            }
        }
    }
}

void YXEmbedClient::handleUnmap(const XUnmapEvent& unmap) {
    MSG(("embed client unmap: evt 0x%lX, win 0x%lX, handle 0x%lX",
                unmap.event, unmap.window, handle()));

    fEmbedder->handleClientUnmap(handle());
}

void YXEmbedClient::infoMapped(bool map) {
    Atom mapped = map ? XEMBED_MAPPED : Atom(0);
    Atom info[] = { XEMBED_PROTOCOL_VERSION, mapped };
    setProperty(_XA_XEMBED_INFO, XA_CARDINAL, info, 2);
}

void YXEmbedClient::sendNotify() {
    sendMessage(XEMBED_EMBEDDED_NOTIFY, 0L, handle(), XEMBED_PROTOCOL_VERSION);
}

void YXEmbedClient::sendActivate() {
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
