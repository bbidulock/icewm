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

    void activate();
    void deactivate();

    virtual void paint(Graphics &g, const YRect &er);
    virtual void paintFocus(Graphics &g, const YRect &er);
    virtual bool eventButton(const YButtonEvent &button);
    virtual bool eventClick(const YClickEvent &up);
    virtual bool eventBeginDrag(const YButtonEvent &down, const YMotionEvent &motion);

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

    static YColorPrefProperty gColorBgA;
    static YColorPrefProperty gColorBgI;

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
