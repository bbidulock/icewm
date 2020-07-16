#ifndef __WMMINIICON_H
#define __WMMINIICON_H

#include "ywindow.h"

class YFrameWindow;

class MiniIcon: public YWindow {
public:
    MiniIcon(YWindow *aParent, YFrameWindow *frame);
    virtual ~MiniIcon();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleExpose(const XExposeEvent& expose);
    virtual void repaint();

    YFrameWindow *getFrame() const { return fFrame; };

private:
    void setSelected(int state);

    YFrameWindow *fFrame;
    int selected;
};


#endif

// vim: set sw=4 ts=4 et:
