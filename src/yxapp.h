#ifndef __YXAPP_H
#define __YXAPP_H

#include "yapp.h"
#include "ywindow.h"

class YAtom {
    const char* name;
    bool screen;
    Atom atom;
    void atomize();
public:
    explicit YAtom(const char* name, bool screen = false) :
        name(name), screen(screen), atom(None) { }
    operator Atom();
    const char* str() const { return name; }
};

class YXPoll: public YPoll<class YXApplication> {
public:
    virtual void notifyRead();
    virtual void notifyWrite();
    virtual bool forRead();
    virtual bool forWrite();
};

class YXApplication: public YApplication {
public:
    YXApplication(int *argc, char ***argv, const char *displayName = 0);
    virtual ~YXApplication();

    Display * display()   const { return fDisplay; }
    int screen()          const { return fScreen; }
    Window root()         const { return fRoot; }
    Visual * visual()     const { return fVisual; }
    unsigned depth()      const { return fDepth; }
    Colormap colormap()   const { return fColormap; }
    unsigned long black() const { return fBlack; }
    unsigned long white() const { return fWhite; }
    int displayWidth() { return DisplayWidth(display(), screen()); }
    int displayHeight() { return DisplayHeight(display(), screen()); }
    Atom atom(const char* name) { return XInternAtom(display(), name, False); }
    void sync() { XSync(display(), False); }
    void send(XClientMessageEvent& ev, Window win, long mask) {
        XSendEvent(display(), win, False, mask, (XEvent*)&ev);
    }

    bool hasColormap();
    bool synchronized() const { return fArgs.runSynchronized; }

    void saveEventTime(const XEvent &xev);
    Time getEventTime(const char *debug) const;

    int grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask, int grabMouse = 1, int grabKeyboard = 1, int grabTree = 0);
    int releaseEvents();
    void handleGrabEvent(YWindow *win, XEvent &xev);
    bool handleIdle();
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

    void alert();

    void setClipboardText(const ustring &data);

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

    static const char* getHelpText();

private:
    struct AppArgs {
        const char* displayName;
        bool runSynchronized;
    } const fArgs;

    AppArgs parseArgs(int *argc, char ***argv, const char *displayName);
    Display* openDisplay();

    Display* const fDisplay;
    int const fScreen;
    Window const fRoot;
    unsigned const fDepth;
    Visual* const fVisual;
    Colormap const fColormap;
    unsigned long const fBlack;
    unsigned long const fWhite;

    Time lastEventTime;
    YPopupWindow *fPopup;
    friend class YXPoll;
    YXPoll xfd;

    int fGrabTree;
    YWindow *fXGrabWindow;
    int fGrabMouse;
    YWindow *fGrabWindow;

    YClipboard *fClip;
    bool fReplayEvent;

    virtual bool handleXEvents();
    virtual void flushXEvents();

    void initExtensions();
    void initModifiers();
    static void initAtoms();
    static void initPointers();
};

extern YXApplication *xapp;

#endif

// vim: set sw=4 ts=4 et:
