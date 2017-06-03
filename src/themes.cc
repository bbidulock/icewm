/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"

#ifndef NO_CONFIGURE_MENUS
#include "themes.h"

#include "yapp.h"
#include "ymenu.h"
#include "wmmgr.h"
#include "wmprog.h"
#include "ymenuitem.h"
#include "sysdep.h"
#include "base.h"
#include "prefs.h"
#include "yprefs.h"
#include "udir.h"
#include "wmapp.h"
#include "ascii.h"
#include "wmconfig.h"
#include "wmaction.h"
#include "appnames.h"

#include "intl.h"

static void setDefaultTheme(const char *theme) {
    char buf[600];
    snprintf(buf, sizeof buf, "Theme=\"%s\"\n", theme);
    setDefault("theme", buf);
}

DTheme::DTheme(IApp *app, YSMListener *smActionListener, const ustring &label, const ustring &theme):
    DObject(app, label, null), fTheme(theme)
{
    this->app = app;
    this->smActionListener = smActionListener;
}

DTheme::~DTheme() {
}

void DTheme::open() {
    if (fTheme == null)
        return;

    cstring cTheme(fTheme);
    setDefaultTheme(cTheme.c_str());

    const char *bg[] = { ICEWMBGEXE, "-r", 0 };
    int pid = app->runProgram(bg[0], bg);
    app->waitProgram(pid);

    smActionListener->handleSMAction(ICEWM_ACTION_RESTARTWM);
}

ThemesMenu::ThemesMenu(IApp *app, YSMListener *smActionListener, YActionListener *wmActionListener, YWindow *parent): ObjectMenu(wmActionListener, parent) {
    this->app = app;
    this->smActionListener = smActionListener;
    this->wmActionListener = wmActionListener;
}

void ThemesMenu::updatePopup() {
    refresh();
}

void ThemesMenu::refresh() {
    removeAll();

    pstring themes("/themes/");
    upath libThemes = YApplication::getLibDir() + themes;
    upath cnfThemes = YApplication::getConfigDir() + themes;
    upath prvThemes = YApplication::getPrivConfDir() + themes;
    upath xdgThemes = YApplication::getXdgConfDir() + themes;

    if (nestedThemeMenuMinNumber)
        themeCount =
            countThemes(libThemes) +
            countThemes(cnfThemes) +
            countThemes(prvThemes) +
            countThemes(xdgThemes);

    findThemes(libThemes, this);
    findThemes(cnfThemes, this);
    findThemes(prvThemes, this);
    findThemes(xdgThemes, this);

    addSeparator();
    add(newThemeItem(app, smActionListener, _("Default"), CONFIG_DEFAULT_THEME));
}

int ThemesMenu::countThemes(const upath& path) {
    int ret = 0;
    for (udir dir(path); dir.next(); ) {
        ret += (path + dir.entry() + "default.theme").isReadable();
    }
    return ret;
}

ThemesMenu::~ThemesMenu() {
}

YMenuItem * ThemesMenu::newThemeItem(
        IApp *app,
        YSMListener *smActionListener,
        const ustring& label,
        const ustring& relThemeName) {
    DTheme *dtheme = new DTheme(app, smActionListener, label, relThemeName);

    if (dtheme) {
        YMenuItem *item(new DObjectMenuItem(dtheme));

        if (item) {
            item->setChecked(themeName && relThemeName == themeName);
            return item;
        }
    }
    return NULL;
}

void ThemesMenu::findThemes(const upath& path, YMenu *container) {
    ustring defTheme("/default.theme");

    bool bNesting = nestedThemeMenuMinNumber && themeCount>nestedThemeMenuMinNumber;

    for (udir dir(path); dir.next(); ) {
        YMenuItem *im(NULL);
        upath subdir = path + dir.entry();
        upath defThemePath = subdir + defTheme;

        if (defThemePath.isReadable()) {
            ustring relThemeName = dir.entry() + defTheme;
            im = newThemeItem(app, smActionListener, dir.entry(), relThemeName);
        }
        if (im && bNesting) {
            char fLetter = ASCII::toUpper(dir.entry()[0]);
            int targetItem = container->findFirstLetRef(fLetter, 0, 1);
            if (targetItem < 0) // no submenu for us yet, create one
            {
                char *smname = strdup("....");
                smname[0] = fLetter;

                YMenu *smenu = new YMenu();
                YMenuItem *smItem = new YMenuItem(smname, 0, null, NULL, smenu);
                if(smItem && smenu)
                    container->addSorted(smItem, false);
                targetItem = container->findFirstLetRef(fLetter, 0, 1);
                if (targetItem < 0)
                {
                    warn("Failed to add submenu");
                    return;
                }
            }
            container->getItem(targetItem)->getSubmenu()->addSorted(im, false);
        }
        else if (im) { //the default method without Extra SubMenues
            container->addSorted(im, false);
        }
        if (im) {
            findThemeAlternatives(app, smActionListener, subdir, dir.entry(), im);
        }
    }
}


void ThemesMenu::findThemeAlternatives(
    IApp *app,
    YSMListener *smActionListener,
    const upath& path,
    const ustring& relName,
    YMenuItem *item)
{
    ustring defTheme("default.theme");
    ustring extension(".theme");
    for (udir dir(path); dir.nextExt(extension); ) {
        const ustring entry(dir.entry());
        if (entry != defTheme) {
            upath altThemePath(path + entry);

            if (altThemePath.isReadable()) {
                YMenu *sub(item->getSubmenu());

                if (sub == NULL)
                    item->setSubmenu(sub = new YMenu());

                if (sub) {
                    int prefixLength = entry.length() - extension.length();
                    ustring tname(entry.substring(0, prefixLength));
                    ustring relThemeName = upath(relName) + entry;
                    sub->add(newThemeItem(app, smActionListener,
                                tname, relThemeName));
                }
            }
        }
    }
}
#endif
