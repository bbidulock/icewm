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

    virtual void paint(Graphics &g, const YRect &er);

    void begin(bool zdown, int mods);

    virtual void activatePopup();
    virtual void deactivatePopup();
    
    virtual bool eventKey(const YKeyEvent &key);
    virtual bool eventButton(const YButtonEvent &button);

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
