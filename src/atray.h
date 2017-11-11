/*
 *  IceWM - Interface of the window tray
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef ATRAY_H_
#define ATRAY_H_

#include "ywindow.h"
#include "ypointer.h"
#include "ytimer.h"

class ClientData;

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

private:
    ClientData *fFrame;
    bool fShown;
    int selected;
    osmart<YTimer> fRaiseTimer;

    static ref<YImage> taskMinimizedGradient;
    static ref<YImage> taskActiveGradient;
    static ref<YImage> taskNormalGradient;
};

class IAppletContainer;

class TrayPane: public YWindow {
public:
    TrayPane(IAppletContainer *taskBar, YWindow *parent);
    ~TrayPane();

    TrayApp *addApp(YFrameWindow *frame);
    void removeApp(YFrameWindow *frame);
    int getRequiredWidth();

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics &g, const YRect &r);

private:
    IAppletContainer *fTaskBar;
    bool fNeedRelayout;

    typedef YObjectArray<TrayApp> AppsType;
    typedef AppsType::IterType IterType;
    AppsType fApps;
};

#endif

// vim: set sw=4 ts=4 et:
