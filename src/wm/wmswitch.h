#ifndef __SWITCH_H
#define __SWITCH_H

#include "ypopup.h"
#include "yconfig.h"

class YFrameWindow;
class YWindowManager;

class SwitchWindow: public YPopupWindow {
public:
    SwitchWindow(YWindowManager *root, YWindow *parent = 0);
    virtual ~SwitchWindow();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

    void begin(bool zdown, int mods);

    virtual void activatePopup();
    virtual void deactivatePopup();
    
    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);
    virtual void handleButton(const XButtonEvent &button);

    void destroyedFrame(YFrameWindow *frame);
private:
    YWindowManager *fRoot;
    YFrameWindow *fActiveWindow;
    YFrameWindow *fLastWindow;

    static YColor *switchFg;
    static YColor *switchBg;
    static YFont *switchFont;
    //static YPixmap *switchbackPixmap;

    int modsDown;

    bool isUp;

    bool modDown(int m);
    bool isModKey(KeyCode c);

    int getZListCount();
    int getZList(YFrameWindow **list, int max);
    void updateZList();
    void freeZList();
    int zCount;
    int zTarget;
    YFrameWindow **zList;

    void cancel();
    void accept();
    void displayFocus(YFrameWindow *frame);
    YFrameWindow *nextWindow(bool zdown);

    static YBoolPrefProperty gSwitchToAllWorkspaces;
    static YBoolPrefProperty gSwitchToMinimized;
    static YBoolPrefProperty gSwitchToHidden;

    static YFontPrefProperty gSwitchFont;
    static YColorPrefProperty gSwitchBg;
    static YColorPrefProperty gSwitchFg;
    static YPixmapPrefProperty gPixmapBackground;
private: // not-used
    SwitchWindow(const SwitchWindow &);
    SwitchWindow &operator=(const SwitchWindow &);
};

#endif
