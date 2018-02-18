#ifndef __WMTITLE_H
#define __WMTITLE_H

class YFrameButton;
class YFrameWindow;

class YFrameTitleBar: public YWindow {
public:
    YFrameTitleBar(YWindow *parent, YFrameWindow *frame);
    virtual ~YFrameTitleBar();

    void activate();
    void deactivate();

    virtual void paint(Graphics &g, const YRect &r);

#ifdef CONFIG_SHAPE
    void renderShape(Pixmap shape);
#endif

    virtual void handleButton(const XButtonEvent &button);
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

    void layoutButtons();
    void raiseButtons();

    static YColor background(bool active);

private:
    static void initTitleColorsFonts();

    unsigned decors() const { return getFrame()->frameDecors(); }
    bool focused() const { return getFrame()->focused(); }
    int titleLen() const;

    YFrameButton* getButton(char c);
    void positionButton(YFrameButton *b, int &xPos, bool onRight);
    bool isButton(char c);

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
