#ifndef WMDIALOG_H
#define WMDIALOG_H

#include "ywindow.h"
#include "yaction.h"

class YActionButton;
class IApp;

class CtrlAltDelete: public YWindow, public YActionListener {
public:
    CtrlAltDelete(IApp *app, YWindow *parent);
    virtual ~CtrlAltDelete();

    virtual void paint(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void configure(const YRect2& rect);
    virtual void repaint();
    virtual void handleVisibility(const XVisibilityEvent&);

    void activate();
    void deactivate();

private:
    IApp *app;
    enum { Count = 12, };
    YActionButton* buttons[Count];
};

#endif

// vim: set sw=4 ts=4 et:
