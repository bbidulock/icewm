/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#ifndef LITE
#include "ylib.h"
#include "wmabout.h"

#include "yprefs.h"
#include "prefs.h"
#include "wmapp.h"
#include "wmframe.h"
#include "sysdep.h"
#include "WinMgr.h"

#include "intl.h"

AboutDlg *aboutDlg = 0;

AboutDlg::AboutDlg(): YDialog() {
    char const *version("IceWM "VERSION" ("HOSTOS"/"HOSTCPU")");
    char *copyright(strJoin("Copyright ", _("(C)"), " 1997-2004 Marko Macek, ",
                            _("(C)"), " 2001 Mathias Hasselmann",
                            NULL));

    fProgTitle = new YLabel(version, this);
    fCopyright = new YLabel(copyright, this);
    delete[] copyright;

    fThemeNameS = new YLabel(_("Theme:"), this);
    fThemeDescriptionS = new YLabel(_("Theme Description:"), this);
    fThemeAuthorS = new YLabel(_("Theme Author:"), this);
    fThemeName = new YLabel(themeName, this);
    fThemeDescription = new YLabel(themeDescription, this);
    fThemeAuthor = new YLabel(themeAuthor, this);
    autoSize();
    fProgTitle->show();
    fCopyright->show();
    fThemeNameS->show();
    fThemeName->show();
    fThemeDescriptionS->show();
    fThemeDescription->show();
    fThemeAuthorS->show();
    fThemeAuthor->show();

    setWindowTitle(_("icewm - About"));
    //setIconTitle("icewm - About");
    setWinLayerHint(WinLayerAboveDock);
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    setWinHintsHint(WinHintsSkipWindowMenu);
    {
        MwmHints mwm;

        memset(&mwm, 0, sizeof(mwm));
        mwm.flags =
            MWM_HINTS_FUNCTIONS |
            MWM_HINTS_DECORATIONS;
        mwm.functions =
            MWM_FUNC_MOVE | MWM_FUNC_CLOSE;
        mwm.decorations =
            MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU;

        setMwmHints(mwm);
    }
}

#define RX(w) (int((w)->x() + (w)->width()))
#define XMAX(x,nx) ((nx) > (x) ? (nx) : (x))

void AboutDlg::autoSize() {
    int dx = 20, dx1 = 20;
    int dy = 20;
    int W = 0, H;
    int cy;

    fProgTitle->setPosition(dx, dy); dy += fProgTitle->height();
    W = XMAX(W, RX(fProgTitle));
    dy += 4;
    fCopyright->setPosition(dx, dy); dy += fCopyright->height();
    W = XMAX(W, RX(fCopyright));
    dy += 20;

    fThemeNameS->setPosition(dx, dy);
    fThemeDescriptionS->setPosition(dx, dy);
    fThemeAuthorS->setPosition(dx, dy);
    
    dx = XMAX(dx, RX(fThemeNameS));
    dx = XMAX(dx, RX(fThemeDescriptionS));
    dx = XMAX(dx, RX(fThemeAuthorS));
    dx += 20;

    fThemeNameS->setPosition(dx1, dy);
    cy = fThemeNameS->height();
    W = XMAX(W, RX(fThemeName));
    fThemeName->setPosition(dx, dy);
    cy = XMAX(cy, int(fThemeName->height()));
    W = XMAX(W, RX(fThemeName));
    dy += cy;
    dy += 4;
    
    fThemeDescriptionS->setPosition(dx1, dy);
    cy = fThemeDescriptionS->height();
    W = XMAX(W, RX(fThemeDescriptionS));
    fThemeDescription->setPosition(dx, dy);
    cy = XMAX(cy, int(fThemeDescription->height()));
    W = XMAX(W, RX(fThemeDescription));

    dy += cy;
    dy += 4;
    
    fThemeAuthorS->setPosition(dx1, dy);
    cy = fThemeAuthorS->height();
    W = XMAX(W, RX(fThemeAuthorS));
    fThemeAuthor->setPosition(dx, dy);
    cy = XMAX(cy, int(fThemeAuthor->height()));
    W = XMAX(W, RX(fThemeAuthor));
    dy += cy;

    H = dy + 20;
    
    W += 20;
    setSize(W, H);
}

void AboutDlg::showFocused() {
    int dx, dy, dw, dh;
    manager->getScreenGeometry(&dx, &dy, &dw, &dh);

    if (getFrame() == 0)
        manager->manageClient(handle(), false);
    if (getFrame() != 0) {
        getFrame()->setNormalPositionOuter(
            dx + dw / 2 - getFrame()->width() / 2,
            dy + dh / 2 - getFrame()->height() / 2);
        getFrame()->activate(true);
    }
}

void AboutDlg::handleClose() {
    if (!getFrame()->isHidden())
        getFrame()->wmHide();
}
#endif
