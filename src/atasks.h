#ifndef ATASKS_H_
#define ATASKS_H_

#include "ywindow.h"
#include "ytimer.h"

class TaskPane;
class TaskBarApp;
class IAppletContainer;
class ClientData;

class TaskBarApp: public YWindow, public YTimerListener {
public:
    TaskBarApp(ClientData *frame, TaskPane *taskPane, YWindow *aParent);
    virtual ~TaskBarApp();

    virtual bool isFocusTraversable();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleExpose(const XExposeEvent& expose);
    virtual void configure(const YRect2& r);
    virtual void repaint();

    void activate() const;
    ClientData *getFrame() const { return fFrame; }

    void setShown(bool show);
    bool getShown() const { return fShown || fFlashing; }

    int getOrder() const;
    void setFlash(bool urgent);
    void switchToPrev();
    void switchToNext();

    static unsigned maxHeight();
    static void freeFonts() { normalTaskBarFont = null; activeTaskBarFont = null; }

private:
    ClientData *fFrame;
    TaskPane *fTaskPane;
    Pixmap fPixmap;
    bool fRepainted;
    bool fShown;
    bool fFlashing;
    bool fFlashOn;
    timeval fFlashStart;
    int selected;
    lazy<YTimer> fFlashTimer;
    static lazy<YTimer> fRaiseTimer;

    ref<YFont> getFont();
    static ref<YFont> getNormalFont();
    static ref<YFont> getActiveFont();

    static ref<YFont> normalTaskBarFont;
    static ref<YFont> activeTaskBarFont;
};

class TaskPane: public YWindow, private YTimerListener {
public:
    TaskPane(IAppletContainer *taskBar, YWindow *parent);
    ~TaskPane();

    void insert(TaskBarApp *tapp);
    void remove(TaskBarApp *tapp);
    TaskBarApp *addApp(YFrameWindow *frame);
    TaskBarApp *findApp(YFrameWindow *frame);
    TaskBarApp *getActive();
    TaskBarApp *predecessor(TaskBarApp *tapp);
    TaskBarApp *successor(TaskBarApp *tapp);

    static unsigned maxHeight();
    void relayout(bool force = false);
    void relayoutNow(bool force = false);

    virtual void configure(const YRect2& r);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleExpose(const XExposeEvent &expose) {}
    virtual void paint(Graphics &g, const YRect &r);

    void startDrag(TaskBarApp *drag, int byMouse, int sx, int sy);
    void processDrag(int mx, int my);
    void endDrag();
    TaskBarApp* dragging() const { return fDragging; }

    void switchToPrev();
    void switchToNext();
    void movePrev();
    void moveNext();
private:
    IAppletContainer *fTaskBar;

    typedef YObjectArray<TaskBarApp> AppsType;
    typedef AppsType::IterType IterType;
    AppsType fApps;

    bool fNeedRelayout;
    bool fForceImmediate;

    TaskBarApp *fDragging;
    int fDragX;
    int fDragY;

    lazy<YTimer> fRelayoutTimer;
    virtual bool handleTimer(YTimer *t);
};

#endif

// vim: set sw=4 ts=4 et:
