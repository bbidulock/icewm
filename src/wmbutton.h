#ifndef __WMBUTTON_H
#define __WMBUTTON_H

#include "yactionbutton.h"

class YFrameWindow;

class YFrameButton: public YButton {
public:
    YFrameButton(YWindow *parent, bool right, YFrameWindow *frame, YAction action, YAction action2 = actionNull);
    virtual ~YFrameButton();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void paintFocus(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    void setActions(YAction action, YAction action2 = actionNull);
    virtual void updatePopup();

    ref<YPixmap> getPixmap(int pn) const;
    YFrameWindow *getFrame() const { return fFrame; };
    bool onRight() const { return fRight; }

private:
    static YColor background(bool active);

    bool focused() const { return getFrame()->focused(); }

    YFrameWindow *fFrame;
    bool fRight;
    YAction fAction;
    YAction fAction2;
};

#endif

// vim: set sw=4 ts=4 et:
