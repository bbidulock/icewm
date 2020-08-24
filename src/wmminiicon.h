#ifndef WMMINIICON_H
#define WMMINIICON_H

#include "ywindow.h"

class YFrameWindow;
class YIcon;

class MiniIcon: public YWindow {
public:
    MiniIcon(YFrameWindow* frame);
    virtual ~MiniIcon();

    virtual bool handleKey(const XKeyEvent& key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleBeginDrag(const XButtonEvent& d, const XMotionEvent& m);
    virtual void handleEndDrag(const XButtonEvent& d, const XButtonEvent& u);
    virtual void handleExpose(const XExposeEvent& expose);
    virtual void paint(Graphics &g, const YRect &r);
    virtual void repaint();

    void show();
    void updateIcon();
    void updatePosition();
    YFrameWindow *getFrame() const { return fFrame; };

private:
    YFrameWindow *fFrame;
};


#endif

// vim: set sw=4 ts=4 et:
