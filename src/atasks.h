#ifndef ATASKS_H_
#define ATASKS_H_

#include "ywindow.h"
#include "wmclient.h"
#include "ytimer.h"
#include <sys/time.h>

class TaskBarApp;

class TaskBarApp: public YWindow, public YTimerListener {
public:
    TaskBarApp(ClientData *frame, YWindow *aParent);
    virtual ~TaskBarApp();

    virtual bool isFocusTraversable();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);

    ClientData *getFrame() const { return fFrame; }

    void setShown(bool show);
    bool getShown() const { return fShown || fFlashing; }

    void setFlash(bool urgent);
    
    TaskBarApp *getNext() const { return fNext; }
    TaskBarApp *getPrev() const { return fPrev; }
    void setNext(TaskBarApp *next) { fNext = next; }
    void setPrev(TaskBarApp *prev) { fPrev = prev; }

private:
    ClientData *fFrame;
    TaskBarApp *fPrev, *fNext;
    bool fShown;
    bool fFlashing;
    bool fFlashOn;
    time_t fFlashStart;
    int selected;
    YTimer *fFlashTimer;
    static YTimer *fRaiseTimer;
};

class TaskPane: public YWindow {
public:
    TaskPane(YWindow *parent);
    ~TaskPane();

    void insert(TaskBarApp *tapp);
    void remove(TaskBarApp *tapp);
    TaskBarApp *addApp(YFrameWindow *frame);
    void removeApp(YFrameWindow *frame);
    TaskBarApp *getFirst() const { return fFirst; }
    TaskBarApp *getLast() const { return fLast; }

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics &g, const YRect &r);
private:
    TaskBarApp *fFirst, *fLast;
    int fCount;
    bool fNeedRelayout;
};

#endif
