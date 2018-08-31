#ifndef __SWITCH_H
#define __SWITCH_H

#include "ymenu.h"

class YFrameWindow;
class YWindowManager;
class YIcon;

// XXX: this only exists because there are no lambda functions
class IClosablePopup
{
public:
    // report true if window was up; close in any case
    virtual bool close()=0;
    virtual ~IClosablePopup() {}
};

// data model, default implementation is list of windows
//class ISwitchItems;
class ISwitchItems
{
public:
    virtual void updateList() =0;
    virtual int getCount() =0;
    virtual ~ISwitchItems() {}

    // move the focused target up or down and return the new focused element
    virtual int moveTarget(bool zdown)=0;
    // set the target explicitly rather than switching around
    virtual int setTarget(int zPosition)=0;
    /// Show changed focus preview to user
    virtual void displayFocusChange(int idxFocused)=0;

    // set target cursor and implementation specific stuff in the beginning
    virtual void begin(bool zdown)=0;
    // just reset the cursor, no further activities; the apparent state might be inconsistent
    virtual void reset()=0;
    virtual void cancel()=0;
    virtual void accept(IClosablePopup *parent)=0;

    // XXX: convert to iterator
    virtual int getActiveItem()=0;
    virtual ustring getTitle(int idx) =0;
    virtual ref<YIcon> getIcon(int idx) =0;

    // Manager notification about windows disappearing under the fingers
    virtual void destroyedItem(void* framePtr) =0;

    virtual bool isKey(KeySym k, unsigned int mod) =0;
};

class SwitchWindow: public YPopupWindow, IClosablePopup {
public:
    SwitchWindow(YWindow *parent = 0,
                 ISwitchItems *items = 0, bool verticalStyle=true);
    virtual ~SwitchWindow();

    virtual void paint(Graphics &g, const YRect &r);

    void begin(bool zdown, int mods);

    virtual void activatePopup(int flags);
    virtual void deactivatePopup();

    virtual bool handleKey(const XKeyEvent &key) OVERRIDE;
    virtual void handleButton(const XButtonEvent &button) OVERRIDE;
    void handleMotion(const XMotionEvent &motion) OVERRIDE;
    void destroyedFrame(YFrameWindow *frame);

private:
    ISwitchItems* zItems;
    bool m_verticalStyle;
    // backup of user's config, needs to be enforced temporarily
    bool m_oldMenuMouseTracking;
    int m_hintedItem;
    // hints for fast identification of the entry under the cursor
    int m_hintAreaStart, m_hintAreaStep;

    ref<YImage> fGradient;

    YColorName switchFg;
    YColorName switchBg;
    YColorName switchHl;
    YColorName switchMbg;
    YColorName switchMfg;
    ref<YFont> switchFont;

    int modsDown;

    bool isUp;

    bool modDown(int m);
    bool isModKey(KeyCode c);
    void resize(int xiscreen);

    void cancel();
    bool close();
    void accept();
    void displayFocus(int itemIdx);
    //YFrameWindow *nextWindow(YFrameWindow *from, bool zdown, bool next);
    YFrameWindow *nextWindow(bool zdown);

    void paintHorizontal(Graphics &g);
    void paintVertical(Graphics &g);

private: // not-used
    SwitchWindow(const SwitchWindow &);
    SwitchWindow &operator=(const SwitchWindow &);
};

#endif

// vim: set sw=4 ts=4 et:
