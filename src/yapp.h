#ifndef __YAPP_H
#define __YAPP_H

#include <signal.h>

#include "ywindow.h"
#include "ycursor.h"
#include "ypaths.h"

class YTimer;
class YPoll;
class YClipboard;

class YApplication {
public:
    YApplication(int *argc, char ***argv, const char *displayName = 0);
    virtual ~YApplication();

    int mainLoop();
    void exitLoop(int exitCode);
    void exit(int exitCode);
    
    Display * display() const { return fDisplay; }
    int screen() { return DefaultScreen (display()); }
    Visual * visual() { return DefaultVisual(display(), screen()); }
    Colormap colormap() { return DefaultColormap(display(), screen()); }
    unsigned depth() { return DefaultDepth(display(), screen()); }
    char const * executable() { return fExecutable; }

    bool hasColormap();
    bool hasGNOME();

    void saveEventTime(const XEvent &xev);
    Time getEventTime() const { return lastEventTime; }

    int grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask, int grabMouse = 1, int grabKeyboard = 1, int grabTree = 0);
    int releaseEvents();
    void handleGrabEvent(YWindow *win, XEvent &xev);

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

    virtual void handleSignal(int sig);
    virtual void handleIdle();

    void catchSignal(int sig);
    void resetSignals();
    //void unblockSignal(int sig);

    void initModifiers();

    void alert();

    void runProgram(const char *path, const char *const *args);
    void runCommand(const char *prog);
    
    static const char *getPrivConfDir();

    static char * findConfigFile(const char *name);
    static char * findConfigFile(const char *name, int mode);

    void setClipboardText(char *data, int len);

    virtual int readFdCheckSM() { return -1; }
    virtual void readFdActionSM() {}

    static YCursor leftPointer;
    static YCursor rightPointer;
    static YCursor movePointer;

#ifndef LITE
    static YResourcePaths iconPaths;
#endif

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
    
    static char const *& Name;

private:
    Display *fDisplay;
    Time lastEventTime;
    YPopupWindow *fPopup;

    int fGrabTree;
    YWindow *fXGrabWindow;
    int fGrabMouse;
    YWindow *fGrabWindow;

    YTimer *fFirstTimer, *fLastTimer;
    YPoll *fFirstPoll, *fLastPoll;
    YClipboard *fClip;

    bool fReplayEvent;

    int fLoopLevel;
    int fExitLoop;
    int fExitCode;
    int fExitApp;
    
    char const * fExecutable;

    friend class YTimer;
    friend class YSocket;
    friend class YPipeReader;
    
    void registerTimer(YTimer *t);
    void unregisterTimer(YTimer *t);
    void getTimeout(struct timeval *timeout);
    void handleTimeouts();
    void registerPoll(YPoll *t);
    void unregisterPoll(YPoll *t);
};

extern YApplication *app;

#endif
