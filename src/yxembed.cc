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

//YXEmbedClient *YXEmbed::manage(YXEmbed *embedder, Window win) {
//    YXEmbedClient *client = new YXEmbedClient(embedder, this, win);
//    client->reparent(this, 0, 0);
//    fClients.append(client);
//    return client;
//}

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

//void YXEmbed::destroyedClient(Window /*win*/) {
//}
