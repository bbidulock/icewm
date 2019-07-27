#ifndef __YXAPP_H
#define __YXAPP_H

#include "yapp.h"
#include "ywindow.h"
#include <X11/Xutil.h>

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

class YTextProperty : public XTextProperty {
public:
    YTextProperty(const char* str);
    ~YTextProperty();
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
        XSendEvent(display(), win, False, mask,
                   reinterpret_cast<XEvent*>(&ev));
    }

    bool hasColormap() const { return fHasColormaps; }
    bool synchronized() const { return synchronizeX11; }
    bool alpha() const { return fArgs.alpha; }
    Visual* visualForDepth(unsigned depth);
    Colormap colormapForDepth(unsigned depth);
    Colormap colormapForVisual(Visual* visual);

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
    static bool alphaBlending;
    static bool synchronizeX11;

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

protected:
    virtual int handleError(XErrorEvent* xev);

private:
    struct AppArgs {
        const char* displayName;
        bool alpha;
        Display* display;
        int screen;
        Window root;
        unsigned depth;
        Visual* visual;
        Colormap cmap;
    } const fArgs;

    AppArgs parseArgs(int *argc, char ***argv, const char *displayName);
    Display* openDisplay();

    Display* const fDisplay;
    int const fScreen;
    Window const fRoot;
    unsigned const fDepth;
    Visual* const fVisual;
    Colormap const fColormap;
    bool const fHasColormaps;
    unsigned long const fBlack;
    unsigned long const fWhite;

    Visual* fVisual32;
    Colormap fColormap32;

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
    static Visual* visual32(Display* dpy, int scn);
    static Colormap cmap32(Display* dpy, int scn, Window rtw);
    static bool haveColormaps(Display* dpy);
    static int errorHandler(Display* display, XErrorEvent* xev);
};

extern YXApplication *xapp;

#endif

// vim: set sw=4 ts=4 et:
