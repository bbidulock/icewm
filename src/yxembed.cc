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
    _XEMBED_INFO("_XEMBED_INFO")
{
    fEmbedder = embedder;
    setParentRelative();
}

YXEmbedClient::~YXEmbedClient() {
}

void YXEmbedClient::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    MSG(("embed client destroy"));

    if (destroyWindow.window == handle()) {
        if (false == fEmbedder->destroyedClient(handle())) {
            YWindow::handleDestroyWindow(destroyWindow);
        }
    }
}

void YXEmbedClient::handleReparentNotify(const XReparentEvent &reparent) {
    MSG(("embed client reparent"));

    if (reparent.window == handle() &&
            reparent.parent != fEmbedder->handle()) {
        if (false == fEmbedder->destroyedClient(handle())) {
            YWindow::handleReparentNotify(reparent);
        }
    }
}

void YXEmbedClient::handleProperty(const XPropertyEvent &property) {
    MSG(("embed client property"));

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

void YXEmbedClient::handleUnmap(const XUnmapEvent &/*unmap*/) {
//    YWindow::handleUnmap(unmap);
    fEmbedder->handleClientUnmap(handle());
}

// vim: set sw=4 ts=4 et:
