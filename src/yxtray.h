#ifndef __YXTRAY_H
#define __YXTRAY_H

#include "yxembed.h"

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

class YXTrayProxy;
class YXTray;

class YXTrayNotifier {
public:
    virtual void trayChanged() = 0;
protected:
    virtual ~YXTrayNotifier() {}
};

class YXTrayEmbedder: public YWindow, public YXEmbed {
public:
    YXTrayEmbedder(YXTray *tray, Window win);
    ~YXTrayEmbedder();
    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual bool destroyedClient(Window win);
    virtual void handleClientUnmap(Window win);
    virtual void handleClientMap(Window win);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);
    virtual void configure(const YRect &r);
    void detach();

    Window client_handle() { return fDocked->handle(); }
    YXEmbedClient *client() { return fDocked; }

    bool fVisible;

private:
    virtual Window getHandle() { return YWindow::handle(); }
    virtual unsigned getWidth() { return YWindow::width(); }
    virtual unsigned getHeight() { return YWindow::height(); }

    YXTray *fTray;
    YXEmbedClient *fDocked;
};

class YXTray: public YWindow {
public:
    YXTray(YXTrayNotifier *notifier, bool internal,
           const class YAtom& trayatom, YWindow *aParent = 0,
           bool drawBevel = false);
    virtual ~YXTray();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void configure(const YRect &r);
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);

    void backgroundChanged();
    void relayout();
    int countClients() const { return fDocked.getCount(); }

    void trayRequestDock(Window win);
    void detachTray();

    void showClient(Window win, bool show);
    bool kdeRequestDock(Window win);

    bool destroyedClient(Window win);
private:
    static void getScaleSize(unsigned& w, unsigned& h);

    YXTrayProxy *fTrayProxy;
    typedef YObjectArray<YXTrayEmbedder> DockedType;
    typedef DockedType::IterType IterType;
    DockedType fDocked;
    YXTrayNotifier *fNotifier;
    bool fRunProxy;
    bool fDrawBevel;
};

#endif

// vim: set sw=4 ts=4 et:
