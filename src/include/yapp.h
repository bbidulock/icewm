#ifndef __YAPP_H
#define __YAPP_H

#include <signal.h>

#include "ywindow.h"

class YTimer;
class YSocket;
class YClipboard;
class YResourcePath;
class YCachedPref;
class YPrefDomain;

class YApplication {
public:
    YApplication(const char *appname, int *argc, char ***argv, const char *displayName = 0);
    virtual ~YApplication();

    int mainLoop();
    void exitLoop(int exitCode);
    void exit(int exitCode);
    
    //YRootWindow *root() { return fRoot; }
    Display *display() const { return fDisplay; }

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

    void runProgram(const char *str, const char *const *args);
    void runCommand(const char *prog);

    static char *findConfigFile(const char *name);

#ifdef SM
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

    bool hasColormap();

    YCachedPref *getPref(const char *domain, const char *pref);

    YPixmap *loadPixmap(const char *fileName);
    YPixmap *loadPixmap(const char *fileName, int w, int h);
    YIcon *getIcon(const char *name);

    YResourcePath *getResourcePath(const char *base);
    YPixmap *loadResourcePixmap(YResourcePath *rp, const char *name);

    // !!! this needs to go away
    unsigned int AltMask;
    unsigned int MetaMask;
    unsigned int NumLockMask;
    unsigned int ScrollLockMask;
    unsigned int SuperMask;
    unsigned int HyperMask;

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

    YTimer *fFirstTimer, *fLastTimer;
    YSocket *fFirstSocket, *fLastSocket;
    YClipboard *fClip;

    CStr *fAppName;
    YPrefDomain *fPrefDomains;

    bool fReplayEvent;

    int fLoopLevel;
    int fExitLoop;
    int fExitCode;
    int fExitApp;

    friend class YTimer;
    friend class YSocket;
    
    void registerTimer(YTimer *t);
    void unregisterTimer(YTimer *t);
    void getTimeout(struct timeval *timeout);
    void handleTimeouts();
    void registerSocket(YSocket *t);
    void unregisterSocket(YSocket *t);

    friend class YIcon; // clean this up...!!!

    YResourcePath *fIconPaths; //!!! make app local?

    void initIcons();
    //bool findIcon(char *base, char **fullPath, int size);
    //bool findIcon(char **fullPath, int size);
    bool findIcon(char *base, char **fullPath);
    void freeIcons();


    void freePrefs();
};

extern YApplication *app;

bool parseKey(const char *arg, KeySym *key, int *mod); // !!!

#endif
