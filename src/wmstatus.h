#ifndef __WMSTATUS_H
#define __WMSTATUS_H

#ifndef LITE

#include "ywindow.h"

class YFrameWindow;

class YWindowManagerStatus: public YWindow {
public:
    YWindowManagerStatus(YWindow *aParent, const char *(*templFunc) ());
    virtual ~YWindowManagerStatus();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void begin();
    void end() { hide(); }    

    virtual const char* getStatus() = 0;

protected:
    static YColor *statusFg;
    static YColor *statusBg;
    static YFont *statusFont;
};

class MoveSizeStatus: public YWindowManagerStatus {
public:
    MoveSizeStatus(YWindow *aParent);
    virtual ~MoveSizeStatus();

    virtual const char* getStatus();
    
    void begin(YFrameWindow *frame);
    void setStatus(YFrameWindow *frame, int x, int y, int width, int height);
    void setStatus(YFrameWindow *frame);
private:
    static const char* templateFunction();

    int fX, fY, fW, fH;
};

class WorkspaceStatus: public YWindowManagerStatus {
public:
    WorkspaceStatus(YWindow *aParent);
    virtual ~WorkspaceStatus();

    virtual const char* getStatus();
    void begin(long workspace);
    virtual void setStatus(long workspace);
private:
    static const char* templateFunction();
    static const char* getStatus(const char* name);

    long workspace;    
    class YTimer *timer;
};

extern MoveSizeStatus *statusMoveSize;
extern WorkspaceStatus *statusWorkspace;

#endif

#endif
