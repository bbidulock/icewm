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

class YAtomName {
public:
    Atom* atom;
    const char* name;
};

class YTextProperty : public XTextProperty {
public:
    YTextProperty(const char* str);
    ~YTextProperty();
};

enum YFormat { F8 = 8, F16 = 16, F32 = 32 };

class YProperty {
public:
    YProperty(YWindow* window, Atom prop, YFormat format = F32,
              long limit = 1L, Atom type = AnyPropertyType, bool remove = False):
        fWind(window->handle()), fData(nullptr), fProp(prop), fKind(type),
        fType(None), fLimit(limit), fSize(None), fMore(None), fBits(format),
        fDelete(remove)
    { update(); }

    YProperty(Window handle, Atom prop, YFormat format = F32,
              long limit = 1L, Atom type = AnyPropertyType, bool remove = False):
        fWind(handle), fData(nullptr), fProp(prop), fKind(type), fType(None),
        fLimit(limit), fSize(None), fMore(None), fBits(format), fDelete(remove)
    { update(); }

    ~YProperty() { discard(); }

    void discard();
    const YProperty& update();
    Atom property() const { return fProp; }
    Atom type() const { return fType; }
    YFormat bits() const { return fBits; }
    unsigned long size() const { return fSize; }
    unsigned long more() const { return fMore; }
    operator bool() const { return fData; }
    bool typed(Atom type) const { return fData && type == fType; }

    template<class T> T* data() const { return reinterpret_cast<T*>(fData); }
    template<class T> T* retrieve() { T* t(data<T>()); fData = nullptr; return t; }
    long operator[](int i) const { return data<long>()[i]; }
    long operator*() const { return *data<long>(); }
    template<class T> T* operator->() const { return data<T>(); }

private:
    Window fWind;
    unsigned char* fData;
    Atom fProp;
    Atom fKind;
    Atom fType;
    long fLimit;
    unsigned long fSize;
    unsigned long fMore;
    YFormat fBits;
    bool fDelete;
};

class YXPoll: public YPoll<class YXApplication> {
public:
    explicit YXPoll(YXApplication* owner) : YPoll(owner) { }
    virtual void notifyRead();
    virtual bool forRead() { return true; }
};

class YXApplication: public YApplication {
public:
    YXApplication(int *argc, char ***argv, const char *displayName = nullptr);
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
    bool alpha() const { return fAlpha; }
    Visual* visualForDepth(unsigned depth) const;
    Colormap colormapForDepth(unsigned depth) const;
    Colormap colormapForVisual(Visual* visual) const;
    XRenderPictFormat* formatForDepth(unsigned depth) const;
    XRenderPictFormat* format() const { return formatForDepth(fDepth); }

    void saveEventTime(const XEvent &xev);
    Time getEventTime(const char *debug) const;

    bool grabEvents(YWindow *win, Cursor ptr, unsigned long eventMask,
                    bool grabMouse = true, bool grabKeyboard = true,
                    bool grabTree = false);
    bool releaseEvents();
    void handleGrabEvent(YWindow *win, XEvent &xev);
    virtual bool handleIdle();
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

    void setClipboardText(mstring data);
    void dropClipboard();

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

    bool hasControlAlt(unsigned state) const;
    bool hasWinMask(unsigned state) const;

    static const char* getHelpText();

protected:
    virtual int handleError(XErrorEvent* xev);

private:
    XRenderPictFormat* findFormat(int depth) const;
    Visual* findVisual(int depth) const;
    Colormap getColormap(int depth) const;

    Display* const fDisplay;
    int const fScreen;
    Window const fRoot;
    XRenderPictFormat* fFormat32;
    XRenderPictFormat* fFormat24;
    Visual* const fVisual32;
    Visual* const fVisual24;
    Colormap const fColormap32;
    Colormap const fColormap24;
    bool const fAlpha;

    unsigned const fDepth;
    Visual* const fVisual;
    Colormap const fColormap;
    bool const fHasColormaps;
    unsigned long const fBlack;
    unsigned long const fWhite;

    Time lastEventTime;
    YPopupWindow *fPopup;
    friend class YXPoll;
    YXPoll xfd;

    lazy<class YClipboard> fClip;
    YWindow *fXGrabWindow;
    YWindow *fGrabWindow;

    bool fGrabTree;
    bool fGrabMouse;
    bool fReplayEvent;

    virtual bool handleXEvents();
    virtual void flushXEvents();

    void initModifiers();
    static void initAtoms();
    static void initPointers();

    static const char* parseArgs(int argc, char **argv, const char *displayName);
    static Display* openDisplay(const char* displayName);
    static void initExtensions(Display* dpy);
    static bool haveColormaps(Display* dpy);
    static int errorHandler(Display* display, XErrorEvent* xev);
    static int cmapError(Display* display, XErrorEvent* xev);
    static int sortAtoms(const void* p1, const void* p2);
    static YAtomName atom_info[];
public:
    static const char* atomName(Atom atom);
};

extern YXApplication *xapp;

#endif

// vim: set sw=4 ts=4 et:
