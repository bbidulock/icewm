#ifndef __SWITCH_H
#define __SWITCH_H

#include "ymenu.h"

class YFrameWindow;
class YWindowManager;

class SwitchWindow: public YPopupWindow {
public:
    SwitchWindow(YWindow *parent = 0);
    virtual ~SwitchWindow();

    virtual void paint(Graphics &g, const YRect &r);

    void begin(bool zdown, int mods);

    virtual void activatePopup();
    virtual void deactivatePopup();
    
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);

    void destroyedFrame(YFrameWindow *frame);

private:
    YWindowManager *fRoot;
    YFrameWindow *fActiveWindow;
    YFrameWindow *fLastWindow;

#ifdef CONFIG_GRADIENTS
    class YPixbuf * fGradient;
#endif

    static YColor *switchFg;
    static YColor *switchBg;
    static YColor *switchHl;
    static YFont *switchFont;

    int modsDown;

    bool isUp;

    bool modDown(int m);
    bool isModKey(KeyCode c);
    void resize();

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
    //YFrameWindow *nextWindow(YFrameWindow *from, bool zdown, bool next);
    YFrameWindow *nextWindow(bool zdown);

private: // not-used
    SwitchWindow(const SwitchWindow &);
    SwitchWindow &operator=(const SwitchWindow &);
};

extern SwitchWindow * switchWindow;
extern YPixmap * switchbackPixmap;

#ifdef CONFIG_GRADIENTS
extern class YPixbuf * switchbackPixbuf;
#endif

#endif
