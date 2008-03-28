#include "config.h"
#include "ylib.h"
#include "yxembed.h"
#include "yapp.h"

YXEmbed::YXEmbed(YWindow *aParent):
    YWindow(aParent)
{
}

YXEmbed::~YXEmbed() {
}

YXEmbedClient::YXEmbedClient(YXEmbed *embedder, YWindow *aParent, Window win):
    YWindow(aParent, win)
{
    fEmbedder = embedder;
}

YXEmbedClient::~YXEmbedClient() {
}

void YXEmbedClient::handleDestroyWindow(const XDestroyWindowEvent &destroyWindow) {
    MSG(("embed client destroy"));

    fEmbedder->destroyedClient(handle());

    YWindow::handleDestroyWindow(destroyWindow);
}

void YXEmbedClient::handleUnmap(const XUnmapEvent &/*unmap*/) {
//    YWindow::handleUnmap(unmap);
    fEmbedder->handleClientUnmap(handle());
}
