#ifndef ATASKS_H_
#define ATASKS_H_

#include "ywindow.h"
//#include "wmclient.h"
#include "ytimer.h"

class TaskBarApp;
class YWindowManager;
class WindowInfo;
class DesktopInfo;

class TaskBarApp: public YWindow, public YTimerListener {
public:
    TaskBarApp(WindowInfo *frame, YWindow *aParent);
    virtual ~TaskBarApp();

    virtual bool isFocusTraversable();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);

    WindowInfo *getFrame() const { return fFrame; }

    void setShown(bool show);
    bool getShown() const { return fShown; }
    
    TaskBarApp *getNext() const { return fNext; }
    TaskBarApp *getPrev() const { return fPrev; }
    void setNext(TaskBarApp *next) { fNext = next; }
    void setPrev(TaskBarApp *prev) { fPrev = prev; }

private:
    WindowInfo *fFrame;
    TaskBarApp *fPrev, *fNext;
    bool fShown;
    int selected;
    static YTimer *fRaiseTimer;
};

class TaskPane: public YWindow {
public:
    TaskPane(DesktopInfo *root, YWindow *parent);
    ~TaskPane();

    TaskBarApp *addApp(WindowInfo *frame);
    void removeApp(WindowInfo *frame);
    TaskBarApp *getFirst() const { return fFirst; }
    TaskBarApp *getLast() const { return fLast; }

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
private:
    void insert(TaskBarApp *tapp);
    void remove(TaskBarApp *tapp);

    DesktopInfo *fRoot;
    TaskBarApp *fFirst, *fLast;
    int fCount;
    bool fNeedRelayout;
};

#endif
