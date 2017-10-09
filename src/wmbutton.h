#ifndef __WMBUTTON_H
#define __WMBUTTON_H

#include "yactionbutton.h"

class YFrameWindow;

class YFrameButton: public YButton {
public:
    YFrameButton(YWindow *parent, YFrameWindow *frame, YAction action, YAction action2 = actionNull);
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
private:
    YFrameWindow *fFrame;
    YAction fAction;
    YAction fAction2;
};

#endif

// vim: set sw=4 ts=4 et:
