#include "yxembed.h"

YXEmbed::YXEmbed(YWindow *aParent):
    YWindow(aParent)
{
    setParentRelative();
}

YXEmbed::~YXEmbed() {
}

YXEmbedClient::YXEmbedClient(YXEmbed *embedder, YWindow *aParent, Window win):
    YWindow(aParent, win)
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

void YXEmbedClient::handleUnmap(const XUnmapEvent &/*unmap*/) {
//    YWindow::handleUnmap(unmap);
    fEmbedder->handleClientUnmap(handle());
}

// vim: set sw=4 ts=4 et:
