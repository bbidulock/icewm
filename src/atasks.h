#ifndef ATASKS_H_
#define ATASKS_H_

#include "ywindow.h"
#include "wmclient.h"
#include "ytimer.h"

class TaskBarApp;

class TaskBarApp: public YWindow, public YTimerListener {
public:
    TaskBarApp(ClientData *frame, YWindow *aParent);
    virtual ~TaskBarApp();

    virtual bool isFocusTraversable();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);

    ClientData *getFrame() const { return fFrame; }

    void setShown(bool show);
    bool getShown() const { return fShown; }
    
    TaskBarApp *getNext() const { return fNext; }
    TaskBarApp *getPrev() const { return fPrev; }
    void setNext(TaskBarApp *next) { fNext = next; }
    void setPrev(TaskBarApp *prev) { fPrev = prev; }

private:
    ClientData *fFrame;
    TaskBarApp *fPrev, *fNext;
    bool fShown;
    int selected;
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
    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
private:
    TaskBarApp *fFirst, *fLast;
    int fCount;
    bool fNeedRelayout;
};

#endif
