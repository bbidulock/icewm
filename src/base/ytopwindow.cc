/*
 * IceWM
 *
 * Copyright (C) 1997,2001 Marko Macek
 */
#pragma implementation
#include "config.h"

#include "yxfull.h"
#include "yproto.h"
#include "ytopwindow.h"
#include "yapp.h"
#include "MwmUtil.h"
#include "ypaint.h"

YTopWindow::YTopWindow(): YWindow(0) {
    fCanResize = true;
}

YTopWindow::~YTopWindow() {
}

void YTopWindow::configure(const YRect &cr) {
    YWindow::configure(cr);

    YWindow *w = firstChild();

    if (w)
        w->setGeometry(0, 0, width(), height());
}

#warning "fix the window icon setting"
void YTopWindow::setIcon(YIcon *icon) {
#ifdef GNOME1_HINTS
    Pixmap icons[4];
    icons[0] = icon->small()->pixmap();
    icons[1] = icon->small()->mask();
    icons[2] = icon->large()->pixmap();
    icons[3] = icon->large()->mask();

    XChangeProperty(app->display(), handle(),
                    _XA_WIN_ICONS, XA_PIXMAP,
                    32, PropModeReplace,
                    (unsigned char *)icons, 4);
#endif
}

void YTopWindow::setWMHints(const XWMHints &wmh) {
    XSetWMHints(app->display(), handle(), (XWMHints *)&wmh);
}

void YTopWindow::setClassHint(const XClassHint &clh) {
    XSetClassHint(app->display(), handle(), (XClassHint *)&clh);
}

void YTopWindow::setMwmHints(const MwmHints &mwm) {
    XChangeProperty(app->display(), handle(),
                    _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                    32, PropModeReplace,
                    (unsigned char *)&mwm, sizeof(mwm) / sizeof(long));
}

void YTopWindow::setTitle(const char *title) {
    XStoreName(app->display(), handle(), title);
}

void YTopWindow::setIconTitle(const char *iconTitle) {
    XSetIconName(app->display(), handle(), iconTitle);
}

void YTopWindow::setResizeable(bool canResize) {
    if (fCanResize != canResize) {
        fCanResize = canResize;

        if (fCanResize) {
            XDeleteProperty(app->display(), handle(), _XATOM_MWM_HINTS);
        } else {
            MwmHints mwm = { 0, 0, 0, 0, 0 };

            mwm.flags =
                MWM_HINTS_FUNCTIONS |
                MWM_HINTS_DECORATIONS;
            mwm.functions =
                MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE | MWM_FUNC_CLOSE;
            mwm.decorations =
                MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE;

            XChangeProperty(app->display(), handle(),
                            _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                            32, PropModeReplace,
                            (const unsigned char *)&mwm, sizeof(mwm) / sizeof(long));
        }
    }
}
