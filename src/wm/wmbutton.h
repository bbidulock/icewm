#ifndef __WMBUTTON_H
#define __WMBUTTON_H

#include "yactionbutton.h"
#include "ymenu.h"
#include "yconfig.h"

class YFrameWindow;

class YFrameButton: public YButton {
public:
    YFrameButton(YWindow *parent, YFrameWindow *frame, YAction *action, YAction *action2 = 0);
    virtual ~YFrameButton();

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void paintFocus(Graphics &g, int x, int y, unsigned int w, unsigned int h);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);

    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    void setActions(YAction *action, YAction *action2 = 0);
    virtual void updatePopup();

    YPixmap *getImage(int pn);
    YFrameWindow *getFrame() const { return fFrame; };
private:
    YFrameWindow *fFrame;
    YAction *fAction;
    YAction *fAction2;

    static YBoolPrefProperty gShowFrameIcon;
    static YBoolPrefProperty gRaiseOnClickButton;

    static YPixmapPrefProperty gPixmapDepthA;
    static YPixmapPrefProperty gPixmapCloseA;
    static YPixmapPrefProperty gPixmapMinimizeA;
    static YPixmapPrefProperty gPixmapMaximizeA;
    static YPixmapPrefProperty gPixmapRestoreA;
    static YPixmapPrefProperty gPixmapHideA;
    static YPixmapPrefProperty gPixmapRollupA;
    static YPixmapPrefProperty gPixmapRolldownA;
    static YPixmapPrefProperty gPixmapMenuA;

    static YPixmapPrefProperty gPixmapDepthI;
    static YPixmapPrefProperty gPixmapCloseI;
    static YPixmapPrefProperty gPixmapMinimizeI;
    static YPixmapPrefProperty gPixmapMaximizeI;
    static YPixmapPrefProperty gPixmapRestoreI;
    static YPixmapPrefProperty gPixmapHideI;
    static YPixmapPrefProperty gPixmapRollupI;
    static YPixmapPrefProperty gPixmapRolldownI;
    static YPixmapPrefProperty gPixmapMenuI;
private: // not-used
    YFrameButton(const YFrameButton &);
    YFrameButton &operator=(const YFrameButton &);
};

#endif
