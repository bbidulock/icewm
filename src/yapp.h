#ifndef __YAPP_H
#define __YAPP_H

#include <signal.h>

#include "ywindow.h"
#include "ycursor.h"
#include "ypaths.h"


#define GET_SHORT_ARGUMENT(Name) \
    (!strncmp(*arg, "-" Name, 2) ? ((*arg)[2] ? *arg + 2 : *++arg) : NULL)
#define GET_LONG_ARGUMENT(Name) \
    (!strpcmp(*arg, "-" Name, "=") ? \
        ('=' == (*arg)[sizeof(Name)] ? (*arg) + sizeof(Name) + 1 : *++arg) : \
     !strpcmp(*arg, "--" Name, "=") ? \
        ('=' == (*arg)[sizeof(Name) + 1] ? (*arg) + sizeof(Name) + 2 : *++arg) : \
     NULL)

#define IS_SHORT_SWITCH(Name)  (!strcmp(*arg, "-" Name))
#define IS_LONG_SWITCH(Name)   (!(strcmp(*arg, "-" Name) && \
                                  strcmp(*arg, "--" Name)))
#define IS_SWITCH(Short, Long) (IS_SHORT_SWITCH(Short) || \
                                IS_LONG_SWITCH(Long))


class YTimer;
class YSocket;
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

    void saveEventTime(XEvent &xev);
    Time getEventTime() const { return lastEventTime; }

    int grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask, int grabMouse = 1, int grabKeyboard = 1, int grabTree = 0);
    int releaseEvents();
    void handleGrabEvent(YWindow *win, XEvent &xev);

    void replayEvent();

    void captureGrabEvents(YWindow *win);
    void releaseGrabEvents(YWindow *win);

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

    void runProgram(const char *str, const char *const *args);
    void runCommand(const char *prog);
    
    static const char *getShell();
    static const char *getPrivConfDir();

    static char * findConfigFile(const char *name);
    static char * findConfigFile(const char *name, int mode);
    
#ifdef CONFIG_SESSION
    bool haveSessionManager();
    virtual void smSaveYourself(bool shutdown, bool fast);
    virtual void smSaveYourselfPhase2();
    virtual void smSaveComplete();
    virtual void smShutdownCancelled();
    virtual void smDie();
    void smSaveDone();
    void smRequestShutdown();
    void smCancelShutdown();
#endif

    void setClipboardText(char *data, int len);

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
    YSocket *fFirstSocket, *fLastSocket;
    YClipboard *fClip;

    bool fReplayEvent;

    int fLoopLevel;
    int fExitLoop;
    int fExitCode;
    int fExitApp;
    
    char const * fExecutable;

    friend class YTimer;
    friend class YSocket;
    
    void registerTimer(YTimer *t);
    void unregisterTimer(YTimer *t);
    void getTimeout(struct timeval *timeout);
    void handleTimeouts();
    void registerSocket(YSocket *t);
    void unregisterSocket(YSocket *t);
};

extern YApplication *app;

#endif
