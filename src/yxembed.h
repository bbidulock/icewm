#ifndef __YXEMBED_H
#define __YXEMBED_H

#include "yarray.h"
#include "ywindow.h"

class YXEmbedClient;

class YXEmbed: public YWindow {
public:
    YXEmbed(YWindow *aParent = 0);
    virtual ~YXEmbed();

//    YObjectArray<YWindow> fClients;

//    YXEmbedClient *manage(YXEmbed *embedder, Window win);
    virtual void destroyedClient(Window /*win*/) = 0;
};

class YXEmbedClient: public YWindow {
public:
    YXEmbedClient(YXEmbed *embedder, YWindow *aParent = 0, Window win = 0);
    virtual ~YXEmbedClient();

    void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
private:
    YXEmbed *fEmbedder;
};

#endif
