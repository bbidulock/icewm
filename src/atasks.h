#ifndef ATASKS_H_
#define ATASKS_H_

#include "ywindow.h"
#include "ytimer.h"

class TaskPane;
class TaskButton;
class TaskBarApp;
class IAppletContainer;
class ClientData;

class TaskBarApp {
public:
    TaskBarApp(ClientData* frame, TaskButton* button);
    virtual ~TaskBarApp();

    void activate() const;
    ClientData* getFrame() const { return fFrame; }
    TaskButton* button() const { return fButton; }

    void setShown(bool show);
    bool getShown() const;

    int getOrder() const;
    void setFlash(bool urgent);
    void setToolTip(mstring tip);
    void repaint();

private:
    ClientData* fFrame;
    TaskButton* fButton;
    bool fShown;
};

class TaskButton: public YWindow, private YTimerListener {
public:
    TaskButton(TaskPane* taskPane);
    virtual ~TaskButton();

    void setShown(TaskBarApp* app, bool show);
    bool getShown() const;
    virtual bool isFocusTraversable();

    virtual void paint(Graphics& g, const YRect& r);
    virtual void handleButton(const XButtonEvent& button);
    virtual void handleClick(const XButtonEvent& up, int count);
    virtual void handleCrossing(const XCrossingEvent& crossing);
    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual void handleBeginDrag(const XButtonEvent& down, const XMotionEvent& motion);
    virtual void handleExpose(const XExposeEvent& expose);
    virtual void configure(const YRect2& r);
    virtual void repaint();

    void activate() const;
    void addApp(TaskBarApp* app);
    void remove(TaskBarApp* tapp);
    void setFlash(bool urgent);
    bool flashing() const { return fFlashing; }
    int getOrder() const;
    int getCount() const;

    TaskBarApp* getActive() const { return fActive; }
    ClientData* getFrame() const { return fActive->getFrame(); }
    TaskPane* taskPane() const { return fTaskPane; }
    static unsigned maxHeight();

private:
    TaskPane* fTaskPane;
    TaskBarApp* fActive;
    bool fRepainted;
    bool fShown;
    bool fFlashing;
    bool fFlashOn;
    timeval fFlashStart;
    int selected;
    lazy<YTimer> fFlashTimer;
    lazy<YTimer> fRaiseTimer;

    virtual bool handleTimer(YTimer* t);
    ref<YFont> getFont();
    static ref<YFont> getNormalFont();
    static ref<YFont> getActiveFont();
};

class TaskPane: public YWindow, private YTimerListener {
public:
    TaskPane(IAppletContainer* taskBar, YWindow* parent);
    ~TaskPane();

    void insert(TaskBarApp* tapp);
    void remove(TaskBarApp* tapp);
    void insert(TaskButton* task);
    void remove(TaskButton* task);
    TaskBarApp* addApp(ClientData* frame);
    TaskBarApp* findApp(ClientData* frame);
    TaskBarApp* getActiveApp();
    TaskBarApp* predecessor(TaskBarApp* tapp);
    TaskBarApp* successor(TaskBarApp* tapp);
    TaskButton* getActiveButton();

    static unsigned maxHeight();
    void relayout(bool force = false);
    void relayoutNow(bool force = false);

    virtual void configure(const YRect2& r);
    virtual void handleClick(const XButtonEvent& up, int count);
    virtual void handleMotion(const XMotionEvent& motion);
    virtual void handleButton(const XButtonEvent& button);
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void paint(Graphics& g, const YRect& r);

    void startDrag(TaskButton* drag, int byMouse, int sx, int sy);
    void processDrag(int mx, int my);
    void endDrag();
    TaskButton* dragging() const { return fDragging; }

    void switchToPrev();
    void switchToNext();
    void movePrev();
    void moveNext();
private:
    IAppletContainer* fTaskBar;

    typedef YObjectArray<TaskBarApp> AppsType;
    typedef AppsType::IterType IterApps;
    AppsType fApps;
    typedef YObjectArray<TaskButton> TaskType;
    typedef TaskType::IterType IterTask;
    TaskType fTasks;

    lazy<YTimer> fRelayoutTimer;
    virtual bool handleTimer(YTimer* t);

    TaskButton* fDragging;
    int fDragX;
    int fDragY;

    bool fNeedRelayout;
    bool fForceImmediate;
};

#endif

// vim: set sw=4 ts=4 et:
