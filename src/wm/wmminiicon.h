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

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);

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
