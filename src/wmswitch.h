#ifndef __SWITCH_H
#define __SWITCH_H

#include "ymenu.h"

class YFrameWindow;
class YWindowManager;

// data model, default implementation is list of windows
class ISwitchItems;

// XXX: this only exists because there are no lambda functions
class IClosablePopup
{
public:
    // report true if window was up; close in any case
    virtual bool close()=0;
    virtual ~IClosablePopup(){};
};

class SwitchWindow: public YPopupWindow, IClosablePopup {
    ISwitchItems* zItems;
    bool m_verticalStyle;
public:
    SwitchWindow(YWindow *parent = 0,
                 ISwitchItems *items = 0, bool verticalStyle=true);
    virtual ~SwitchWindow();

    virtual void paint(Graphics &g, const YRect &r);

    void begin(bool zdown, int mods);

    virtual void activatePopup(int flags);
    virtual void deactivatePopup();

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);

    void destroyedFrame(YFrameWindow *frame);

private:

#ifdef CONFIG_GRADIENTS
    ref<YImage> fGradient;
#endif

    static YColor *switchFg;
    static YColor *switchBg;
    static YColor *switchHl;
    static YColor *switchMbg;
    static YColor *switchMfg;
    static ref<YFont> switchFont;

    int modsDown;

    bool isUp;

    bool modDown(int m);
    bool isModKey(KeyCode c);
    void resize(int xiscreen);

    void cancel();
    bool close();
    void accept();
    void displayFocus(void *frame);
    //YFrameWindow *nextWindow(YFrameWindow *from, bool zdown, bool next);
    YFrameWindow *nextWindow(bool zdown);

private: // not-used
    SwitchWindow(const SwitchWindow &);
    SwitchWindow &operator=(const SwitchWindow &);
};

#endif

// vim: set sw=4 ts=4 et:
