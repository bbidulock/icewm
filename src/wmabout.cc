/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Dialogs
 */
#include "config.h"
#include "wmabout.h"
#include "ylabel.h"
#include "yprefs.h"
#include "prefs.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "yxapp.h"
#include "ylayout.h"
#ifdef CONFIG_I18N
#include <langinfo.h>
#endif

#include "intl.h"

AboutDlg::AboutDlg():
    fSizeable(nullptr)
{
    const char version[] = "IceWM " VERSION " (" HOSTOS "/" HOSTCPU ")";
    mstring copyright = mstring("Copyright ")
                      + _("(C)") + " 1997-2012 Marko Macek, "
                      + _("(C)") + " 2001 Mathias Hasselmann";

    Ladder* ladder = new Ladder();
    *ladder += label(version);
    *ladder += label(copyright);
    *ladder += new Spacer(1, 20);

    Table* table = new Table();
    *ladder += table;

    *table += new Row(label(_("Theme:")), label(themeName));
    *table += new Row(label(_("Theme Description:")), label(themeDescription));
    *table += new Row(label(_("Theme Author:")), label(themeAuthor));
    *table += new Row(new Spacer(1, 20));

    const char *codeset = "";
    const char *language = "";
#ifdef CONFIG_I18N
    codeset = nl_langinfo(CODESET);
    language = getenv("LANG");
#endif
    *table += new Row(label(_("CodeSet:")), label(codeset));
    *table += new Row(label(_("Language:")), label(language));

    char text[123];
    snprintf(text, sizeof text, "%s %s %s",
             YImage::renderName(), doubleBuffer ? _("DoubleBuffer") : "",
             xapp->alpha() ? _("AlphaBlending") : "");
    *table += new Row(label(_("Renderer:")), label(text));

    fSizeable = new Padder(ladder, 20, 20);
    fSizeable->setPosition(0, 0);
    fSizeable->realize();
    setSize(fSizeable->width(), fSizeable->height());

    setTitle("About");
    setWindowTitle(_("icewm - About"));
    setClassHint("about", "IceWM");

    setLayerHint(WinLayerAboveDock);
    setWorkspaceHint(AllWorkspaces);
    setWinHintsHint(WinHintsSkipWindowMenu);
    setMwmHints(MwmHints(
       MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
       MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
       MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU));
}

AboutDlg::~AboutDlg() {
    delete fSizeable;
}

YLabel* AboutDlg::label(const char* text) {
    return new YLabel(text, this);
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
