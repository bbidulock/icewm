#ifndef __SMWIN_H
#define __SMWIN_H

#ifdef CONFIG_SESSION

class YFrameWindow;

class SMWindowKey {
public:
    SMWindowKey(YFrameWindow *f);
    SMWindowKey(ustring id, ustring role);
    SMWindowKey(ustring id, ustring klass, ustring instance);
    ~SMWindowKey();

    friend class SMWindows;
private:
    ustring clientId;
    ustring windowRole;
    ustring windowClass;
    ustring windowInstance;
};

class SMWindowInfo {
public:
    SMWindowInfo(YFrameWindow *f);
    SMWindowInfo(ustring id, ustring role,
                 int x, int y, int w, int h,
                 unsigned long state, int layer, int workspace);
    SMWindowInfo(ustring id, ustring klass, ustring instance,
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
