#ifndef __WMBUTTON_H
#define __WMBUTTON_H

#include "yactionbutton.h"

class YFrameWindow;

class YFrameButton: public YButton {
public:
    YFrameButton(YWindow *parent, YFrameWindow *frame, tActionId action, tActionId action2 = 0);
    virtual ~YFrameButton();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void paintFocus(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);

    virtual void actionPerformed(tActionId action, unsigned int modifiers);
    void setActions(tActionId action, tActionId action2 = 0);
    virtual void updatePopup();

    ref<YPixmap> getPixmap(int pn) const;
    YFrameWindow *getFrame() const { return fFrame; };
private:
    YFrameWindow *fFrame;
    tActionId fAction;
    tActionId fAction2;
};

#endif

// vim: set sw=4 tw=4 et:
