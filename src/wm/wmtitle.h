#ifndef __WMTITLE_H
#define __WMTITLE_H

#include "ywindow.h"
#include "yconfig.h"

class YFrameWindow;
class YFrameButton;

class YFrameTitleBar: public YWindow {
public:
    YFrameTitleBar(YWindow *parent, YFrameWindow *frame);
    virtual ~YFrameTitleBar();

    void activate();
    void deactivate();

    int titleLen();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);

    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);

    YFrameWindow *getFrame() const { return fFrame; };

    YFrameButton *menuButton() const { return fMenuButton; }
    YFrameButton *closeButton() const { return fCloseButton; }
    YFrameButton *minimizeButton() const { return fMinimizeButton; }
    YFrameButton *maximizeButton() const { return fMaximizeButton; }
    YFrameButton *hideButton() const { return fHideButton; }
    YFrameButton *rollupButton() const { return fRollupButton; }
    YFrameButton *depthButton() const { return fDepthButton; }
    YFrameButton *getButton(char c);
    void positionButton(YFrameButton *b, int &xPos, bool onRight);
    bool isButton(char c);
    void layoutButtons();
private:
    YFrameWindow *fFrame;
    int buttonDownX, buttonDownY;

    YFrameButton *fCloseButton;
    YFrameButton *fMenuButton;
    YFrameButton *fMaximizeButton;
    YFrameButton *fMinimizeButton;
    YFrameButton *fHideButton;
    YFrameButton *fRollupButton;
    YFrameButton *fDepthButton;

    static YStrPrefProperty gTitleButtonsSupported;
    static YStrPrefProperty gTitleButtonsLeft;
    static YStrPrefProperty gTitleButtonsRight;
    static YNumPrefProperty gTitleMaximizeButton;
    static YNumPrefProperty gTitleRollupButton;
    static YBoolPrefProperty gTitleBarCentered;
    static YBoolPrefProperty gRaiseOnClickTitleBar;

    static YPixmapPrefProperty gTitleAL;
    static YPixmapPrefProperty gTitleAS;
    static YPixmapPrefProperty gTitleAP;
    static YPixmapPrefProperty gTitleAT;
    static YPixmapPrefProperty gTitleAM;
    static YPixmapPrefProperty gTitleAB;
    static YPixmapPrefProperty gTitleAR;

    static YPixmapPrefProperty gTitleIL;
    static YPixmapPrefProperty gTitleIS;
    static YPixmapPrefProperty gTitleIP;
    static YPixmapPrefProperty gTitleIT;
    static YPixmapPrefProperty gTitleIM;
    static YPixmapPrefProperty gTitleIB;
    static YPixmapPrefProperty gTitleIR;
};

#endif
