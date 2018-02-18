#ifndef __WMSTATUS_H
#define __WMSTATUS_H

#include "ywindow.h"
#include "ytimer.h"

class YFrameWindow;

class YWindowManagerStatus: public YWindow {
public:
    YWindowManagerStatus(YWindow *aParent, const ustring &sampleString);
    virtual ~YWindowManagerStatus();

    virtual void paint(Graphics &g, const YRect &r);

    void begin();
    void end() { hide(); }

    virtual ustring getStatus() = 0;

protected:
    static YColorName statusFg;
    static YColorName statusBg;
    static ref<YFont> statusFont;
};

class MoveSizeStatus: public YWindowManagerStatus {
public:
    MoveSizeStatus(YWindow *aParent);
    virtual ~MoveSizeStatus();

    virtual ustring getStatus();

    void begin(YFrameWindow *frame);
    void setStatus(YFrameWindow *frame, const YRect &r);
    void setStatus(YFrameWindow *frame);
private:
    int fX, fY, fW, fH;
};

class WorkspaceStatus: public YWindowManagerStatus, public YTimerListener {
public:
    static WorkspaceStatus * createInstance(YWindow *aParent);
    virtual ~WorkspaceStatus();

    virtual ustring getStatus();
    void begin(long workspace);
    virtual void setStatus(long workspace);
    virtual bool handleTimer(YTimer *timer);
private:
    WorkspaceStatus(YWindow *aParent, ustring templateString);
    static ustring getStatus(const char* name);

    long workspace;
    YTimer timer;
};

extern MoveSizeStatus *statusMoveSize;
extern WorkspaceStatus *statusWorkspace;

#endif

// vim: set sw=4 ts=4 et:
