#ifndef WMTITLE_H
#define WMTITLE_H

class YFrameButton;
class YFrameWindow;
class YFrameClient;

class YFrameTitleBar: public YWindow, private YTimerListener {
public:
    YFrameTitleBar(YFrameWindow *frame);
    virtual ~YFrameTitleBar();

    void activate();
    void deactivate();
    void renderShape(Graphics& g);

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual bool handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleEndDrag(const XButtonEvent& down, const XButtonEvent& up);
    virtual void handleDrag(const XButtonEvent& down, const XMotionEvent& motion);
    virtual void handleVisibility(const XVisibilityEvent &visibility);
    virtual void handleExpose(XExposeEvent const& expose) {}
    virtual bool handleTimer(YTimer* timer);
    virtual void configure(const YRect2& r);
    virtual void repaint();

    YFrameWindow* getFrame() const { return fFrame; }
    YFrameClient* getClient() const { return fFrame->client(); }
    YFrameButton* menuButton() const { return fButtons[6]; }
    YFrameButton* rollupButton() const { return fButtons[5]; }
    YFrameButton* maximizeButton() const { return fButtons[4]; }

    void layoutButtons();
    void relayout();
    void refresh();

    static YColor background(bool active);
    static bool isRight(const YFrameButton* b);
    static bool isRight(char c);
    static bool supported(char c);

    enum {
        Depth = 'd',
        Hide  = 'h',
        Mini  = 'i',
        Maxi  = 'm',
        Roll  = 'r',
        Menu  = 's',
        Close = 'x',
    };

private:
    static void initTitleColorsFonts();

    unsigned decors() const { return getFrame()->frameDecors(); }
    bool focused() const { return getFrame()->focused(); }
    static bool hasMask(unsigned state, unsigned bits,
                        unsigned ignore = ControlMask);

    YFrameButton* getButton(char c);
    YFrameTitleBar* findPartner();
    bool hasRoom() const { return 50 < fRoom && visible(); }
    bool isPartner(YFrameTitleBar* other);
    void setPartner(YFrameTitleBar* partner);

    YFrameWindow* fFrame;
    YFrameTitleBar* fPartner;
    lazy<YTimer> fTimer;
    int fDragX, fDragY;
    int fRoom;
    bool wasCanRaise;
    bool fVisible;
    bool fToggle;

    enum { Count = 8, };
    YFrameButton* fButtons[Count];
    static bool swapTitleButtons;
};

#endif

// vim: set sw=4 ts=4 et:
