/*
 * IceWM
 *
 * Copyright (C) 1997,2001 Marko Macek
 */
#pragma implementation
#include "config.h"

#include "yfull.h"
#include "ytopwindow.h"
#include "yapp.h"
#include "MwmUtil.h"

YTopWindow::YTopWindow(): YWindow(0) {
}

YTopWindow::~YTopWindow() {
}

void YTopWindow::configure(int x, int y, unsigned int width, unsigned int height) {
    YWindow::configure(x, y, width, height);

    YWindow *w = firstChild();

    if (w)
        w->setGeometry(0, 0, width, height);
}

#warning "fix the window icon setting"
void YTopWindow::setIcon(YIcon *icon) {
#ifdef GNOME1_HINTS
    // !!! fix this
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
                    (unsigned char *)&mwm, sizeof(mwm)/sizeof(long)); ///!!! ?????????
}

void YTopWindow::setTitle(const char *title) {
    XStoreName(app->display(), handle(), title);
}

void YTopWindow::setIconTitle(const char *iconTitle) {
    XSetIconName(app->display(), handle(), iconTitle);
}
