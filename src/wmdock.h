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

    operator bool() const { return docks.nonempty(); }
    bool dock(YFrameClient* client);
    bool undock(YFrameClient* client);
    void adapt();
    int layer() const { return layered; }

private:
    void handleButton(const XButtonEvent& button) override;
    void handleClick(const XButtonEvent& button, int count) override;
    void actionPerformed(YAction action, unsigned modifiers) override;
    void handlePopDown(YPopupWindow *popup) override;
    bool handleTimer(YTimer* timer) override;
    lazy<YTimer> timer;
    lazy<YMenu> menu;

    struct docking {
        Window window;
        YFrameClient* client;
        docking(Window w, YFrameClient* c) : window(w), client(c) { }
    };
    YArray<docking> docks;
    void undock(int index);
    bool isChild(Window window);
    bool setup();
    void checks();
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
    bool isRight;
};

#endif
