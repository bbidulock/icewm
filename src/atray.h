/*
 *  IceWM - Interface of the window tray
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef ATRAY_H_
#define ATRAY_H_

#ifdef CONFIG_TRAY

#include "ywindow.h"
#include "wmclient.h"
#include "ytimer.h"

class TrayApp: public YWindow, public YTimerListener {
public:
    TrayApp(ClientData *frame, YWindow *aParent);
    virtual ~TrayApp();

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
    bool getShown() const { return fShown; }
    
    TrayApp *getNext() const { return fNext; }
    TrayApp *getPrev() const { return fPrev; }
    void setNext(TrayApp *next) { fNext = next; }
    void setPrev(TrayApp *prev) { fPrev = prev; }

private:
    ClientData *fFrame;
    TrayApp *fPrev, *fNext;
    bool fShown;
    int selected;
    static YTimer *fRaiseTimer;
    
#ifdef CONFIG_GRADIENTS
    static ref<YImage> taskMinimizedGradient;
    static ref<YImage> taskActiveGradient;
    static ref<YImage> taskNormalGradient;
#endif    
};

class IAppletContainer;

class TrayPane: public YWindow {
public:
    TrayPane(IAppletContainer *taskBar, YWindow *parent);
    ~TrayPane();

    void insert(TrayApp *tapp);
    void remove(TrayApp *tapp);
    TrayApp *addApp(YFrameWindow *frame);
    void removeApp(YFrameWindow *frame);
    TrayApp *getFirst() const { return fFirst; }
    TrayApp *getLast() const { return fLast; }
    int getRequiredWidth();

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics &g, const YRect &r);
private:
    IAppletContainer *fTaskBar;
    TrayApp *fFirst, *fLast;
    int fCount;
    bool fNeedRelayout;
};

#endif

#endif
