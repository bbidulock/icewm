#ifndef __SMWIN_H
#define __SMWIN_H

#ifdef CONFIG_SESSION

class YFrameWindow;

class SMWindowKey {
public:
    SMWindowKey(YFrameWindow *f);
    SMWindowKey(char *id, char *role);
    SMWindowKey(char *id, char *klass, char *instance);
    ~SMWindowKey();

    friend class SMWindows;
private:
    char *clientId;
    char *windowRole;
    // not used if role != 0 ?
    char *windowClass;
    char *windowInstance;
};

class SMWindowInfo {
public:
    SMWindowInfo(YFrameWindow *f);
    SMWindowInfo(char *id, char *role,
                 int x, int y, int w, int h,
                 unsigned long state, int layer, int workspace);
    SMWindowInfo(char *id, char *klass, char *instance,
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

char *getsesfile();
    
#endif

#endif
