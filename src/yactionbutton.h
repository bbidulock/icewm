#ifndef __YACTIONBUTTON_H
#define __YACTIONBUTTON_H

#include "ybutton.h"
#include "yaction.h"

class YActionButton: public YButton {
public:
    YActionButton(YWindow *parent):
        YButton(parent, YAction())
    {
        addStyle(wsNoExpose);
        setParentRelative();
    }

    operator YAction() const { return getAction(); }

    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void configure(const YRect2& r);
    virtual void repaint();
};

#endif

// vim: set sw=4 ts=4 et:
