#ifndef __WMTITLE_H
#define __WMTITLE_H

#include "ywindow.h"

class YFrameWindow;

class YFrameTitleBar: public YWindow {
public:
    YFrameTitleBar(YWindow *parent, YFrameWindow *frame);
    virtual ~YFrameTitleBar();

    void activate();
    void deactivate();

    int titleLen();

    virtual void paint(Graphics &g, const YRect &r);

#ifdef CONFIG_SHAPED_DECORATION
    void renderShape(Pixmap shape);
#endif

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);

    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);

    YFrameWindow *getFrame() const { return fFrame; };
private:
    YFrameWindow *fFrame;
    int buttonDownX, buttonDownY;
    int movingWindow;
    bool wasCanRaise;
};

#endif
