#ifndef __WMSTATUS_H
#define __WMSTATUS_H

#ifndef LITE

#include "ywindow.h"
#include "yconfig.h"

class YFrameWindow;
class YWindowManager;

class MoveSizeStatus: public YWindow {
public:
    MoveSizeStatus(YWindowManager *root, YWindow *aParent);
    virtual ~MoveSizeStatus();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void setStatus(YFrameWindow *frame, int x, int y, int width, int height);
    void setStatus(YFrameWindow *frame);
    void begin(YFrameWindow *frame);
    void end() { hide(); }
private:
    YWindowManager *fRoot;
    int fX, fY, fW, fH;

    static YBoolPrefProperty gShowMoveSizeStatus;

    static YFontPrefProperty gStatusFont;
    static YColorPrefProperty gStatusBg;
    static YColorPrefProperty gStatusFg;
private: // not-used
    MoveSizeStatus(const MoveSizeStatus &);
    MoveSizeStatus &operator=(const MoveSizeStatus &);
};

#endif

#endif
