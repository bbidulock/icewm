#ifndef __YAPP_H
#define __YAPP_H

#include "yxbase.h"

#pragma interface

class YWindow;
class YPopupWindow;
class YTimer;
class YSocket;
class YClipboard;
class YResourcePath;
class YCachedPref;
class YPixmap;
class YPrefDomain;
class CStr;
class YPointer;

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

    int grabEvents(YWindow *win, YPointer *ptr, unsigned int eventMask, int grabMouse = 1, int grabKeyboard = 1, int grabTree = 0);
    int releaseEvents();
    void setGrabPointer(YPointer *pointer);
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

    void runProgram(const char *str, const char *const *args);
    void runCommand(const char *prog);

    ///static char *findConfigFile(const char *name);

#ifdef CONFIG_SM
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

    bool parseGeometry(const char *geometry,
                       int &ogx,
                       int &ogy,
                       unsigned int &ogw,
                       unsigned int &ogh,
                       bool &gpos,
                       bool &gsize);

    void beep();

#ifdef OBSOLETE
    YResourcePath *getResourcePath(const char *base);
    YPixmap *loadResourcePixmap(YResourcePath *rp, const char *name);
#endif
    int VMod(int modifiers);
    bool XMod(int vmod, int &modifiers);

//#warning "remove public modifier methods"
    unsigned int getWinL();
    unsigned int getWinR();

private:
    friend class YWindow;
    unsigned int getKeyMask();
    unsigned int getAltMask();
    unsigned int getWinMask();

private:
    Display *fDisplay;
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

    void freePrefs();

    static bool fInitModifiers;
    void initModifiers();

    unsigned int AltMask;
    unsigned int WinMask;
    unsigned int WinL;
    unsigned int WinR;

    unsigned int MetaMask;
    unsigned int NumLockMask;
    unsigned int ScrollLockMask;
    unsigned int SuperMask;
    unsigned int HyperMask;

    unsigned int KeyMask;
    unsigned int ButtonMask;
    unsigned int ButtonKeyMask;

    unsigned int getButtonMask();
    unsigned int getButtonKeyMask();

    unsigned int getNumLockMask();
    unsigned int getCapsLockMask();
    unsigned int getScrollLockMask();

    unsigned int getSuperMask();
    unsigned int getHyperMask();
    unsigned int getMetaMask();

private: // not-used
    YApplication(YApplication &);
    YApplication &operator=(const YApplication &);
};

extern YApplication *app;

#endif
