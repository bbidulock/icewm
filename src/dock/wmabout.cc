/*
 * IceWM
 *
 * Copyright (C) 1997,1998,1999 Marko Macek
 *
 * Dialogs
 */
#include "config.h"

#include "ylib.h"
#include "wmabout.h"

#include "yapp.h"

#include "prefs.h"
//#include "wmapp.h"
//#include "wmframe.h"
#include "sysdep.h"
#include "WinMgr.h"
#include "MwmUtil.h"

AboutDlg::AboutDlg(): YDialog() {
    char title[128], copyright[128];

    sprintf(title, "icewm " VERSION);
    fProgTitle = new YLabel(title, this);
    sprintf(copyright, "Copyright 1997-1999 Marko Macek");
    fCopyright = new YLabel(copyright, this);
    fThemeNameS = new YLabel("Theme:", this);
    fThemeDescriptionS = new YLabel("Theme Description:", this);
    fThemeAuthorS = new YLabel("Theme Author:", this);
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

#if 0
    setWindowTitle("icewm - About");
    //setIconTitle("icewm - About");
#ifdef GNOME1_HINTS
    setWinLayerHint(WinLayerAboveDock);
    setWinStateHint(WinStateAllWorkspaces, WinStateAllWorkspaces);
    setWinHintsHint(WinHintsSkipWindowMenu);
#endif
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
#endif
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
#if 0
    if (getFrame() == 0)
        desktop->manageWindow(this, false);
    if (getFrame() != 0) {
        getFrame()->setPosition(desktop->width() / 2 - getFrame()->width() / 2,
                                desktop->height() / 2 - getFrame()->height() / 2);
        getFrame()->activate(true);
    }
#else
    show();
#endif
}

void AboutDlg::handleClose() {
#if 0
    if (!getFrame()->isHidden())
        getFrame()->wmHide();
#endif
    app->exit(0);

}

// ---

#define NO_KEYBIND
//#include "bindkey.h"
#include "prefs.h"
#define CFGDEF
//#include "bindkey.h"
#include "default.h"


int main(int argc, char **argv) {
    YApplication app("wmabout", &argc, &argv);

    AboutDlg *dlg = new AboutDlg();

    dlg->show();

    return app.mainLoop();
}
