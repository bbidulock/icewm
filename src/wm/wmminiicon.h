#ifndef __WMMINIICON_H
#define __WMMINIICON_H

#include "ywindow.h"
#include "yconfig.h"

class YFrameWindow;
class YWindowManager;

class MiniIcon: public YWindow {
public:
    MiniIcon(YWindowManager *root, YWindow *aParent, YFrameWindow *frame);
    virtual ~MiniIcon();

    virtual void paint(Graphics &g, const YRect &er);
    virtual bool eventButton(const YButtonEvent &button);
    virtual bool eventClick(const YClickEvent &up);
    virtual bool eventCrossing(const YCrossingEvent &crossing);
    virtual bool eventDrag(const YButtonEvent &down, const YMotionEvent &motion);

    YFrameWindow *getFrame() const { return fFrame; };
private:
    YWindowManager *fRoot;
    YFrameWindow *fFrame;
    int selected;

    static YFontPrefProperty gMinimizedWindowFont;
    static YColorPrefProperty gNormalBg;
    static YColorPrefProperty gNormalFg;
    static YColorPrefProperty gActiveBg;
    static YColorPrefProperty gActiveFg;

private: // not-used
    MiniIcon(const MiniIcon &);
    MiniIcon &operator=(const MiniIcon &);
};


#endif
