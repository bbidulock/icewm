#ifndef __SMWIN_H
#define __SMWIN_H

#ifdef CONFIG_SM

class YFrameWindow;
class CStr;

class SMWindowKey {
public:
    //SMWindowKey(YFrameWindow *f);
    SMWindowKey(char *id, char *role);
    SMWindowKey(char *id, char *klass, char *instance);
    ~SMWindowKey();

    friend class SMWindows;
private:
    CStr *clientId;
    CStr *windowRole;
    // not used if role != 0 ?
    CStr *windowClass;
    CStr *windowInstance;
private: // not-used
    SMWindowKey(const SMWindowKey &);
    SMWindowKey &operator=(const SMWindowKey &);
};

class SMWindowInfo {
public:
    //SMWindowInfo(YFrameWindow *f);
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
private: // not-used
    SMWindowInfo(const SMWindowInfo &);
    SMWindowInfo &operator=(const SMWindowInfo &);
};

class SMWindows {
public:
    SMWindows();
    ~SMWindows();

    void clearAllInfo();
    void addWindowInfo(SMWindowInfo *info);
    //void setWindowInfo(YFrameWindow *f);
    //bool getWindowInfo(YFrameWindow *f, SMWindowInfo *info);
    //bool removeWindowInfo(YFrameWindow *f);
    bool findWindowInfo(YFrameWindow *f);
private:
    int windowCount;
    SMWindowInfo **windows;
private: // not-used
    SMWindows(const SMWindows &);
    SMWindows &operator=(const SMWindows &);
};


void loadWindowInfo(YWindowManager *fManager);
bool findWindowInfo(YFrameWindow *f);

char *getsesfile();
    
#endif

#endif
