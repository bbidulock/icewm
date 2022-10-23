#ifndef __SMWIN_H
#define __SMWIN_H

#ifdef CONFIG_SESSION

class YFrameWindow;

class SMWindowKey {
public:
    SMWindowKey(YFrameWindow *f);
    SMWindowKey(mstring id, mstring role);
    SMWindowKey(mstring id, mstring klass, mstring instance);
    ~SMWindowKey();

    friend class SMWindows;
private:
    mstring clientId;
    mstring windowRole;
    mstring windowClass;
    mstring windowInstance;
};

class SMWindowInfo {
public:
    SMWindowInfo(YFrameWindow *f);
    SMWindowInfo(mstring id, mstring role,
                 int x, int y, int w, int h,
                 unsigned long state, int layer, int workspace);
    SMWindowInfo(mstring id, mstring klass, mstring instance,
                 int x, int y, int w, int h,
                 unsigned long state, int layer, int workspace);
    ~SMWindowInfo();

    friend class SMWindows;
private:
    SMWindowKey key;
    int x, y;
    unsigned int width, height;
    unsigned long state;
    unsigned int layer;
    unsigned int workspace;
};

class SMWindows {
public:
    void addWindowInfo(SMWindowInfo *info);
    void setWindowInfo(YFrameWindow *f);
    bool getWindowInfo(YFrameWindow *f, SMWindowInfo *info);
    bool removeWindowInfo(YFrameWindow *f);
    bool findWindowInfo(YFrameWindow *f);

private:
    YObjectArray<SMWindowInfo> fWindows;
};


void loadWindowInfo();
bool findWindowInfo(YFrameWindow *f);

upath getsesfile();

#endif

#endif

// vim: set sw=4 ts=4 et:
