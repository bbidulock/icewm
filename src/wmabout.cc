/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Dialogs
 */
#include "config.h"
#include "wmabout.h"
#include "yprefs.h"
#include "prefs.h"
#include "wmframe.h"
#ifdef CONFIG_I18N
#include <langinfo.h>
#endif

#include "intl.h"

AboutDlg::AboutDlg(): YDialog() {
    char const *version("IceWM " VERSION " (" HOSTOS "/" HOSTCPU ")");
    mstring copyright =
        mstring("Copyright ")
        .append(_("(C)"))
        .append(" 1997-2012 Marko Macek, ")
        .append(_("(C)"))
        .append(" 2001 Mathias Hasselmann");

    fProgTitle = new YLabel(version, this);
    fCopyright = new YLabel(copyright, this);

    fThemeNameS = new YLabel(_("Theme:"), this);
    fThemeDescriptionS = new YLabel(_("Theme Description:"), this);
    fThemeAuthorS = new YLabel(_("Theme Author:"), this);
    fThemeName = new YLabel(themeName, this);
    fThemeDescription = new YLabel(themeDescription, this);
    fThemeAuthor = new YLabel(themeAuthor, this);
    fCodeSetS = new YLabel(_("CodeSet:"), this);
    fLanguageS = new YLabel(_("Language:"), this);
    const char *codeset = "";
    const char *language = "";
#ifdef CONFIG_I18N
    codeset = nl_langinfo(CODESET);
    language = getenv("LANG");
#endif

    fCodeSet = new YLabel(codeset, this);
    fLanguage = new YLabel(language, this);

    autoSize();
    fProgTitle->show();
    fCopyright->show();
    fThemeNameS->show();
    fThemeName->show();
    fThemeDescriptionS->show();
    fThemeDescription->show();
    fThemeAuthorS->show();
    fThemeAuthor->show();
    fCodeSetS->show();
    fCodeSet->show();
    fLanguageS->show();
    fLanguage->show();

    setTitle("About");
    setWindowTitle(_("icewm - About"));
    //setIconTitle("icewm - About");
    setClassHint("about", "IceWM");

    setWinLayerHint(WinLayerAboveDock);
    setWinWorkspaceHint(AllWorkspaces);
    setWinHintsHint(WinHintsSkipWindowMenu);
    setMwmHints(MwmHints(
       MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
       MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
       MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU,
       0,
       0));
}

AboutDlg::~AboutDlg() {
    delete fProgTitle;
    delete fCopyright;
    delete fThemeName;
    delete fThemeDescription;
    delete fThemeAuthor;
    delete fThemeNameS;
    delete fThemeDescriptionS;
    delete fThemeAuthorS;
    delete fCodeSetS;
    delete fCodeSet;
    delete fLanguageS;
    delete fLanguage;
}

static unsigned right(YLabel *w) {
    return w->x() + w->width();
}

void AboutDlg::autoSize() {
    unsigned const margin = 20;
    unsigned dx = margin, dx1 = margin;
    unsigned dy = margin;
    unsigned W = 0;
    unsigned cy;

    fProgTitle->setPosition(dx, dy);
    dy += fProgTitle->height();
    W = max(W, right(fProgTitle));
    dy += 4;

    fCopyright->setPosition(dx, dy);
    dy += fCopyright->height();
    W = max(W, right(fCopyright));
    dy += 20;

    fThemeNameS->setPosition(dx, dy);
    fThemeDescriptionS->setPosition(dx, dy);
    fThemeAuthorS->setPosition(dx, dy);
    fCodeSetS->setPosition(dx, dy);
    fLanguageS->setPosition(dx, dy);


    dx = max(dx, right(fThemeNameS));
    dx = max(dx, right(fThemeDescriptionS));
    dx = max(dx, right(fThemeAuthorS));
    dx = max(dx, right(fCodeSetS));
    dx = max(dx, right(fLanguageS));
    dx += 20;

    fThemeNameS->setPosition(dx1, dy);
    cy = fThemeNameS->height();
    W = max(W, right(fThemeName));
    fThemeName->setPosition(dx, dy);
    cy = max(cy, fThemeName->height());
    W = max(W, right(fThemeName));
    dy += cy;
    dy += 4;

    fThemeDescriptionS->setPosition(dx1, dy);
    cy = fThemeDescriptionS->height();
    W = max(W, right(fThemeDescriptionS));
    fThemeDescription->setPosition(dx, dy);
    cy = max(cy, fThemeDescription->height());
    W = max(W, right(fThemeDescription));
    dy += cy;

    dy += 4;

    fThemeAuthorS->setPosition(dx1, dy);
    cy = fThemeAuthorS->height();
    W = max(W, right(fThemeAuthorS));
    fThemeAuthor->setPosition(dx, dy);
    cy = max(cy, fThemeAuthor->height());
    W = max(W, right(fThemeAuthor));
    dy += cy;

    dy += 20;

    fCodeSetS->setPosition(dx1, dy);
    cy = fCodeSetS->height();
    W = max(W, right(fCodeSetS));
    fCodeSet->setPosition(dx, dy);
    cy = max(cy, fCodeSet->height());
    W = max(W, right(fCodeSet));
    dy += cy;

    dy += 4;

    fLanguageS->setPosition(dx1, dy);
    cy = fLanguageS->height();
    W = max(W, right(fLanguageS));
    fLanguage->setPosition(dx, dy);
    cy = max(cy, fLanguage->height());
    W = max(W, right(fLanguage));
    dy += cy;

    setSize(W + margin, dy + margin);
}

void AboutDlg::showFocused() {
    int dx, dy;
    unsigned dw, dh;
    desktop->getScreenGeometry(&dx, &dy, &dw, &dh);

    if (getFrame() == nullptr)
        manager->manageClient(handle(), false);
    if (getFrame() != nullptr) {
        getFrame()->setNormalPositionOuter(
            dx + int(dw / 2 - getFrame()->width() / 2),
            dy + int(dh / 2 - getFrame()->height() / 2));
        getFrame()->activateWindow(true);
    }
}

void AboutDlg::handleClose() {
    if (!getFrame()->isHidden())
        getFrame()->wmHide();
}

// vim: set sw=4 ts=4 et:
