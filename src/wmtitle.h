#ifndef WMTITLE_H
#define WMTITLE_H

class YFrameButton;
class YFrameWindow;

class YFrameTitleBar: public YWindow {
public:
    YFrameTitleBar(YWindow *parent, YFrameWindow *frame);
    virtual ~YFrameTitleBar();

    void activate();
    void deactivate();
    void renderShape(Graphics& g);

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleVisibility(const XVisibilityEvent &visibility);
    virtual void handleExpose(XExposeEvent const& expose) {}
    virtual void configure(const YRect2& r);
    virtual void repaint();

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
    void refresh();

    static YColor background(bool active);

private:
    static void initTitleColorsFonts();

    unsigned decors() const { return getFrame()->frameDecors(); }
    bool focused() const { return getFrame()->focused(); }

    YFrameButton* getButton(char c);
    void positionButton(YFrameButton *b, int &xPos, bool onRight);
    bool isButton(char c);

    YFrameWindow *fFrame;
    bool wasCanRaise;
    bool fVisible;

    YFrameButton* fCloseButton;
    YFrameButton* fMenuButton;
    YFrameButton* fMaximizeButton;
    YFrameButton* fMinimizeButton;
    YFrameButton* fHideButton;
    YFrameButton* fRollupButton;
    YFrameButton* fDepthButton;

    enum {
        Depth = 'd',
        Hide  = 'h',
        Mini  = 'i',
        Maxi  = 'm',
        Roll  = 'r',
        Menu  = 's',
        Close = 'x',
    };
};

#endif

// vim: set sw=4 ts=4 et:
