#ifndef __WMTITLE_H
#define __WMTITLE_H

#include "ywindow.h"
#include "wmbutton.h"

class YFrameWindow;

class YFrameTitleBar: public YWindow {
public:
    YFrameTitleBar(YWindow *parent, YFrameWindow *frame);
    virtual ~YFrameTitleBar();

    static void initTitleColorsFonts();

    void activate();
    void deactivate();

    int titleLen();

    virtual void paint(Graphics &g, const YRect &r);

#ifdef CONFIG_SHAPE
    void renderShape(Pixmap shape);
#endif

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);

    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);

    YFrameWindow *getFrame() const { return fFrame; };

    YFrameButton* menuButton();
    YFrameButton* closeButton();
    YFrameButton* minimizeButton();
    YFrameButton* maximizeButton();
    YFrameButton* hideButton();
    YFrameButton* rollupButton();
    YFrameButton* depthButton();

    YFrameButton* getButton(char c);
    void positionButton(YFrameButton *b, int &xPos, bool onRight);
    bool isButton(char c);
    void layoutButtons();
    void raiseButtons();

private:
    YFrameWindow *fFrame;
    bool wasCanRaise;

    YFrameButton* fCloseButton;
    YFrameButton* fMenuButton;
    YFrameButton* fMaximizeButton;
    YFrameButton* fMinimizeButton;
    YFrameButton* fHideButton;
    YFrameButton* fRollupButton;
    YFrameButton* fDepthButton;
};

#endif

// vim: set sw=4 ts=4 et:
