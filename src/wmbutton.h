#ifndef __WMBUTTON_H
#define __WMBUTTON_H

#include "yactionbutton.h"
#include "ymenu.h"

class YFrameWindow;

class YFrameButton: public YButton {
public:
    YFrameButton(YWindow *parent, YFrameWindow *frame, YAction *action, YAction *action2 = 0);
    virtual ~YFrameButton();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void paintFocus(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    void setActions(YAction *action, YAction *action2 = 0);
    virtual void updatePopup();

    YPixmap *getImage(int pn) const;
    YFrameWindow *getFrame() const { return fFrame; };
private:
    YFrameWindow *fFrame;
    YAction *fAction;
    YAction *fAction2;
};
//changed robc
extern YPixmap *closePixmap[3];
extern YPixmap *minimizePixmap[3];
extern YPixmap *maximizePixmap[3];
extern YPixmap *restorePixmap[3];
extern YPixmap *hidePixmap[3];
extern YPixmap *rollupPixmap[3];
extern YPixmap *rolldownPixmap[3];
extern YPixmap *depthPixmap[3];

#endif
