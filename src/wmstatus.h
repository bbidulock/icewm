#ifndef __WMSTATUS_H
#define __WMSTATUS_H

#ifndef LITE

#include "ywindow.h"

class YFrameWindow;

class MoveSizeStatus: public YWindow {
public:
    MoveSizeStatus(YWindow *aParent);
    virtual ~MoveSizeStatus();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void setStatus(YFrameWindow *frame, int x, int y, int width, int height);
    void setStatus(YFrameWindow *frame);
    void begin(YFrameWindow *frame);
    void end() { hide(); }
private:
    int fX, fY, fW, fH;

    static YColor *statusFg;
    static YColor *statusBg;
    static YFont *statusFont;
};

extern MoveSizeStatus *statusMoveSize;

#endif

#endif
