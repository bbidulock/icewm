#ifndef __WMSTATUS_H
#define __WMSTATUS_H

#include "ywindow.h"
#include "ytimer.h"

class YFrameWindow;

class YWindowManagerStatus: public YWindow {
public:
    YWindowManagerStatus();
    virtual ~YWindowManagerStatus();

    virtual void configure(const YRect2& r2);
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void paint(Graphics &g, const YRect &r);
    virtual void repaint() { repaintSync(); }
    virtual void repaintSync();

    void begin();
    virtual void end() { hide(); }

    virtual ustring getStatus() = 0;
    virtual ustring longestStatus() = 0;

protected:
    static YColorName statusFg;
    static YColorName statusBg;
    static ref<YFont> statusFont;

private:
    void configureStatus();
    bool fConfigured;
};

class MoveSizeStatus: public YWindowManagerStatus {
    typedef YWindowManagerStatus super;

public:
    MoveSizeStatus();
    virtual ~MoveSizeStatus();

    virtual void end();
    virtual ustring getStatus();
    virtual ustring longestStatus();

    void begin(YFrameWindow *frame);
    void setStatus(YFrameWindow *frame, const YRect &r);
    void setStatus(YFrameWindow *frame);
private:

    int fX, fY, fW, fH;
};

class WorkspaceStatus: public YWindowManagerStatus, public YTimerListener {
    typedef YWindowManagerStatus super;

public:
    WorkspaceStatus();
    virtual ~WorkspaceStatus();

    virtual void end();
    virtual ustring getStatus();
    virtual ustring longestStatus();

    void begin(long workspace);
    virtual void setStatus(long workspace);
    virtual bool handleTimer(YTimer *timer);
private:
    WorkspaceStatus(YWindow *aParent, ustring templateString);
    static ustring getStatus(const char* name);

    long workspace;
    YTimer timer;
};

extern lazy<MoveSizeStatus> statusMoveSize;
extern lazy<WorkspaceStatus> statusWorkspace;

#endif

// vim: set sw=4 ts=4 et:
