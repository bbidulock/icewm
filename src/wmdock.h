#ifndef WMDOCK_H
#define WMDOCK_H

#include "ywindow.h"

class YFrameClient;

class DockApp: public YWindow {
public:
    DockApp();
    ~DockApp();

    bool dock(YFrameClient* client);
    bool undock(YFrameClient* client);
    void adapt();
    bool right() const { return isRight && docks.nonempty(); }
    bool above() const { return isAbove && docks.nonempty(); }
    bool lefty() const { return isLeft  && docks.nonempty(); }
    bool below() const { return isBelow && docks.nonempty(); }

private:
    struct docking {
        Window window;
        YFrameClient* client;
        docking(Window w, YFrameClient* c) : window(w), client(c) { }
    };
    YArray<docking> docks;
    void undock(int index);
    bool setup();
    Window savewin();
    Window saveset;

    bool isRight, isLeft, isAbove, isBelow;
};

#endif
