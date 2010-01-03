#ifndef __YXTRAY_H
#define __YXTRAY_H

#include "yxembed.h"

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

class YXTrayProxy;
class YXTray;

class YXTrayNotifier {
public:
    virtual void trayChanged() = 0;
protected:
    virtual ~YXTrayNotifier() {};
};

class YXTrayEmbedder: public YXEmbed {
public:
    YXTrayEmbedder(YXTray *tray, Window win);
    ~YXTrayEmbedder();
    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual void destroyedClient(Window win);
    void detach();
    virtual void configure(const YRect &r);

    Window client_handle() { return fDocked->handle(); }
    YXEmbedClient *client() { return fDocked; }
    void handleClientUnmap(Window win);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);

    bool fVisible;
private:
    YXTray *fTray;
    YXEmbedClient *fDocked;
};

class YXTray: public YWindow {
public:
    YXTray(YXTrayNotifier *notifier, bool internal, const char *atom, YWindow *aParent = 0);
    virtual ~YXTray();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void configure(const YRect &r);
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);

    void backgroundChanged();
    void relayout();

    void trayRequestDock(Window win);
    void detachTray();

    void showClient(Window win, bool show);
    bool kdeRequestDock(Window win);

    void destroyedClient(Window win);
private:
    YXTrayProxy *fTrayProxy;
    YObjectArray<YXTrayEmbedder> fDocked;
    YXTrayNotifier *fNotifier;
    bool fInternal;
};

#endif
