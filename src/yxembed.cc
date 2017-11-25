#include "yxembed.h"

YXEmbed::YXEmbed(YWindow *aParent):
    YWindow(aParent)
{
    setParentRelative();
}

YXEmbed::~YXEmbed() {
}

YXEmbedClient::YXEmbedClient(YXEmbed *embedder, YWindow *aParent, Window win):
    YWindow(aParent, win),
    fEmbedder(embedder),
    _XEMBED_INFO("_XEMBED_INFO")
{
    setParentRelative();
}

YXEmbedClient::~YXEmbedClient() {
}

void YXEmbedClient::handleDestroyWindow(const XDestroyWindowEvent& destroy) {
    MSG(("embed client destroy: evt 0x%lX, win 0x%lX",
                destroy.event, destroy.window));

    if (destroy.window == handle()) {
        YWindow::handleDestroyWindow(destroy);
        fEmbedder->destroyedClient(handle());
    }
}

void YXEmbedClient::handleReparentNotify(const XReparentEvent &reparent) {
    MSG(("embed client reparent: win 0x%lX, par 0x%lX",
                reparent.window, reparent.parent));

    if (reparent.window == handle() &&
            reparent.parent != fEmbedder->handle()) {
        if (false == fEmbedder->destroyedClient(handle())) {
            YWindow::handleReparentNotify(reparent);
        }
    }
}

void YXEmbedClient::handleConfigure(const XConfigureEvent& event) {
    MSG(("embed client configure: evt 0x%lX, win 0x%lX, width %d, height %d",
          event.event, event.window, event.width, event.height));

    YWindow::handleConfigure(event);

    int ww = (int) fEmbedder->width();
    int hh = (int) fEmbedder->height();
    if (ww >= 16 && hh >= 16 &&
        (!inrange(event.width,  ww - 4, ww + 8) ||
         !inrange(event.height, hh - 4, hh + 1)))
    {
        MSG(("embed client configure: setSize ww %u, hh %u", ww, hh));
        setSize(ww, hh);
    }
}

void YXEmbedClient::handleProperty(const XPropertyEvent &property) {
    MSG(("embed client property: win 0x%lX, atom %ld",
                property.window, property.atom));

    if ((property.window == handle()) &&
            property.atom == _XEMBED_INFO) {
        Atom type;
        int format;
        unsigned long nitems, lbytes;
        unsigned char *prop(0);

        if (XGetWindowProperty(xapp->display(), handle(),
                    _XEMBED_INFO, 0L, 2L, False, XA_CARDINAL,
                    &type, &format, &nitems, &lbytes, &prop) == Success && prop)
        {
            if (format == 32 && nitems >= 2) {
                if (prop[1] & XEMBED_MAPPED) {
//                    fEmbedder->handleClientMap(handle());
                } else {
//                    fEmbedder->handleClientUnmap(handle());
                }
            }
            XFree(prop);
        }
    }
}

void YXEmbedClient::handleUnmap(const XUnmapEvent& unmap) {
    MSG(("embed client unmap: evt 0x%lX, win 0x%lX", unmap.event, unmap.window));

    fEmbedder->handleClientUnmap(handle());
}

// vim: set sw=4 ts=4 et:
