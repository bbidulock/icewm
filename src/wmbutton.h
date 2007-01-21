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

    ref<YPixmap> getPixmap(int pn) const;
    YFrameWindow *getFrame() const { return fFrame; };
private:
    YFrameWindow *fFrame;
    YAction *fAction;
    YAction *fAction2;
};
//changed robc
extern ref<YPixmap> closePixmap[3];
extern ref<YPixmap> minimizePixmap[3];
extern ref<YPixmap> maximizePixmap[3];
extern ref<YPixmap> restorePixmap[3];
extern ref<YPixmap> hidePixmap[3];
extern ref<YPixmap> rollupPixmap[3];
extern ref<YPixmap> rolldownPixmap[3];
extern ref<YPixmap> depthPixmap[3];

#endif
