#ifndef __WMBUTTON_H
#define __WMBUTTON_H

#include "yactionbutton.h"

class YFrameWindow;
class YFrameTitleBar;

class YFrameButton: public YButton {
public:
    YFrameButton(YFrameTitleBar *parent, char kind);
    virtual ~YFrameButton();

    virtual void paint(Graphics &g, const YRect &r);
    virtual void paintFocus(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleVisibility(const XVisibilityEvent& visibility);
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void configure(const YRect2 &r);
    virtual void repaint();

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    void setKind(char kind);
    virtual void updatePopup();

    ref<YPixmap> getPixmap(int pn) const;
    YFrameWindow *getFrame() const;
    bool onRight() const;
    char getKind() const { return fKind; }

private:
    static YColor background(bool active);

    bool focused() const;

    char fKind;
    bool fVisible;
};

#endif

// vim: set sw=4 ts=4 et:
