#ifndef YXTRAY_H
#define YXTRAY_H

#include "yxembed.h"

#define SYSTEM_TRAY_REQUEST_DOCK     0
#define SYSTEM_TRAY_BEGIN_MESSAGE    1
#define SYSTEM_TRAY_CANCEL_MESSAGE   2

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
    YXTrayEmbedder(YXTray *tray, Window win, Window leader, mstring title);
    ~YXTrayEmbedder();
    virtual void paint(Graphics &g, const YRect &r);
    virtual void repaint();
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual void handleClientMessage(const XClientMessageEvent& message);
    virtual void damagedClient();
    virtual bool destroyedClient(Window win);
    virtual void handleClientUnmap(Window win);
    virtual void handleClientMap(Window win);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);
    virtual void configure(const YRect2 &r);
    void realise();
    void detach();

    Window client_handle() const { return fClient->handle(); }
    YXEmbedClient *client() const { return fClient; }
    Window leader() const { return fLeader; }
    mstring title() const { return fTitle; }
    int order() const { return fOrder; }
    bool trace() const;

    bool fVisible;

private:
    virtual Window getHandle() { return YWindow::handle(); }
    virtual unsigned getWidth() { return YWindow::width(); }
    virtual unsigned getHeight() { return YWindow::height(); }
    bool composing() const { return fComposing; }

    YXTray *const fTray;
    YXEmbedClient *const fClient;
    const Window fLeader;
    mstring fTitle;
    Damage fDamage;
    bool fComposing;
    const int fOrder;
};

struct Lock {
    bool *const lock;
    const bool orig;
    Lock(bool* l) : lock(l), orig(*l) { *lock = true; }
    ~Lock() { *lock = orig; }
    bool locked() const { return orig && *lock; }
};

class YXTray: public YWindow {
public:
    YXTray(YXTrayNotifier *notifier, bool internal,
           const class YAtom& trayatom, YWindow *aParent = nullptr,
           bool drawBevel = false);
    virtual ~YXTray();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void configure(const YRect2 &r);
    virtual void repaint();
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);

    void backgroundChanged();
    void relayout(bool enforce = false);
    int countClients() const { return fDocked.getCount(); }

    bool trayRequestDock(Window win, mstring title);
    void detachTray();
    void updateTrayWindows();
    void regainTrayWindows();

    void showClient(Window win, bool show);

    bool destroyedClient(Window win);
    bool trace() const { return fTrace; }

private:
    static void getScaleSize(unsigned& w, unsigned& h);
    Window getLeader(Window win);
    void trayUpdateGeometry(unsigned w, unsigned h, bool visible);

    YXTrayProxy *fTrayProxy;
    typedef YObjectArray<YXTrayEmbedder> DockedType;
    typedef DockedType::IterType IterType;
    DockedType fDocked;
    YXTrayNotifier *fNotifier;
    YArray<Window> fRegained;
    YRect fGeometry;
    bool fLocked;
    bool fRunProxy;
    bool fTrace;
    bool fDrawBevel;
};

#endif

// vim: set sw=4 ts=4 et:
