#ifndef WMDOCK_H
#define WMDOCK_H

#include "ywindow.h"
#include "ytimer.h"
#include "yaction.h"
#include "ypopup.h"

class YFrameClient;
class YMenu;

class DockApp:
    private YWindow,
    private YTimerListener,
    private YActionListener,
    private YPopDownListener
{
public:
    DockApp();
    ~DockApp();
    using YWindow::handle;
    using YWindow::created;
    using YWindow::visible;
    using YWindow::width;
    using YWindow::getScreen;

    operator bool() const { return docks.nonempty(); }
    bool dock(YFrameClient* client);
    bool undock(YFrameClient* client);
    void adapt();
    int layer() const { return layered; }
    bool rightside() const { return isRight; }

private:
    void handleButton(const XButtonEvent& button) override;
    void handleClick(const XButtonEvent& button, int count) override;
    void actionPerformed(YAction action, unsigned modifiers) override;
    bool handleBeginDrag(const XButtonEvent& down, const XMotionEvent& move) override;
    void handleDrag(const XButtonEvent& down, const XMotionEvent& move) override;
    void handleEndDrag(const XButtonEvent& down, const XButtonEvent& up) override;
    void handlePopDown(YPopupWindow *popup) override;
    void handleClientMessage(const XClientMessageEvent &message) override;
    void handleClose() override {}
    void gotFocus() override {}
    bool handleTimer(YTimer* timer) override;
    lazy<YTimer> timer;
    lazy<YMenu> menu;

    struct docking {
        Window window;
        YFrameClient* client;
        int order;
        docking(Window w, YFrameClient* c, int o) :
            window(w), client(c), order(o) { }
    };
    YArray<docking> docks;
    YArray<Window> recover;
    YFrameClient* dragged;

    void undock(int index);
    int ordering(YFrameClient* client, bool* startClose, bool* forced);
    bool isChild(Window window);
    bool setup();
    void grabit();
    void ungrab();
    void proper();
    void retime() { timer->setTimer(None, this, true); }
    void revoke(int k, bool kill);
    Window savewin();
    Window saveset;

    Atom intern;
    int center;
    int layered;
    int direction;
    int dragxpos;
    int dragypos;
    bool restack;
    bool isRight;

    static const char propertyName[];
};

#endif
