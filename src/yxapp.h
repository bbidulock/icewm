#ifndef __YXAPP_H
#define __YXAPP_H

#include "yapp.h"

#include "ywindow.h"
#include "ycursor.h"

class YXApplication: public YApplication {
public:
    YXApplication(int *argc, char ***argv, const char *displayName = 0);
    virtual ~YXApplication();

    Display * display() const { return fDisplay; }
    int screen() { return DefaultScreen (display()); }
    Visual * visual() { return DefaultVisual(display(), screen()); }
    Colormap colormap() { return DefaultColormap(display(), screen()); }
    unsigned depth() { return DefaultDepth(display(), screen()); }

    bool hasColormap();

    void saveEventTime(const XEvent &xev);
    Time getEventTime(const char *debug) const;

    int grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask, int grabMouse = 1, int grabKeyboard = 1, int grabTree = 0);
    int releaseEvents();
    void handleGrabEvent(YWindow *win, XEvent &xev);
    void handleWindowEvent(Window xwindow, XEvent &xev);

    void replayEvent();

    void captureGrabEvents(YWindow *win);
    void releaseGrabEvents(YWindow *win);

    virtual bool filterEvent(const XEvent &xev);

    void dispatchEvent(YWindow *win, XEvent &e);
    virtual void afterWindowEvent(XEvent &xev);

    YPopupWindow *popup() const { return fPopup; }
    bool popup(YWindow *forWindow, YPopupWindow *popup);
    void popdown(YPopupWindow *popdown);

    YWindow *grabWindow() const { return fGrabWindow; }

    void initModifiers();

    void alert();

    void setClipboardText(char *data, int len);

    static YCursor leftPointer;
    static YCursor rightPointer;
    static YCursor movePointer;

    unsigned int AltMask;
    unsigned int MetaMask;
    unsigned int NumLockMask;
    unsigned int ScrollLockMask;
    unsigned int SuperMask;
    unsigned int HyperMask;
    unsigned int ModeSwitchMask;
    unsigned int WinMask;
    unsigned int Win_L;
    unsigned int Win_R;
    unsigned int KeyMask;
    unsigned int ButtonMask;
    unsigned int ButtonKeyMask;
private:
    Display *fDisplay;
    Time lastEventTime;
    YPopupWindow *fPopup;

    int fGrabTree;
    YWindow *fXGrabWindow;
    int fGrabMouse;
    YWindow *fGrabWindow;

    YClipboard *fClip;
    bool fReplayEvent;

    virtual bool handleXEvents();
    virtual int readFDCheckX();
    virtual void flushXEvents();
};

extern YXApplication *xapp;

#endif
