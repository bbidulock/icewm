/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
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

DTheme::DTheme(IApp *app, YSMListener *smActionListener, const mstring &label, const mstring &theme):
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

    WMConfig::setDefaultTheme(fTheme);

    const char *bg[] = { ICEWMBGEXE, "-r", nullptr };
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

    mstring themes("/themes/");
    upath libThemes = YApplication::getLibDir() + themes;
    upath cnfThemes = YApplication::getConfigDir() + themes;
    upath prvThemes = YApplication::getPrivConfDir() + themes;

    if (nestedThemeMenuMinNumber)
        themeCount =
            countThemes(libThemes) +
            countThemes(cnfThemes) +
            countThemes(prvThemes);

    findThemes(prvThemes, this);
    findThemes(cnfThemes, this);
    findThemes(libThemes, this);

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
        const mstring& label,
        const mstring& relThemeName) {
    DTheme *dtheme = new DTheme(app, smActionListener, label, relThemeName);

    if (dtheme) {
        YMenuItem *item(new DObjectMenuItem(dtheme));

        if (item) {
            item->setChecked(themeName && relThemeName == themeName);
            return item;
        }
    }
    return nullptr;
}

void ThemesMenu::findThemes(const upath& path, YMenu *container) {
    mstring defTheme("/default.theme");

    bool bNesting = inrange(nestedThemeMenuMinNumber, 1, themeCount - 1);
    char subName[5] = { 'X', '.','.','.', 0};

    for (udir dir(path); dir.next(); ) {
        YMenuItem *im(nullptr);
        YMenu* targetMenu = container;
        upath subdir = path + dir.entry();
        upath defThemePath = subdir + defTheme;

        if (defThemePath.isReadable()) {
            mstring relThemeName = dir.entry() + defTheme;
            im = newThemeItem(app, smActionListener, dir.entry(), relThemeName);
        }

        // maybe shift some stuff around to create a simple structure
        if (im && bNesting) {
                char fLetter = ASCII::toUpper(dir.entry()[0]);
                subName[0] = fLetter;
            int relatedItemPos = -2;

            YMenuItem *subMenuItemTest = container->findName(subName);
            if (subMenuItemTest && subMenuItemTest->getSubmenu())
            {
                targetMenu = subMenuItemTest->getSubmenu();
                if (im->isChecked())
                        subMenuItemTest->setChecked(true);
            }
            else if (subMenuItemTest)
            {
                // looks like a submenu but is an item
                // of that kind... weird, ignore
            }
            else if (0 > (relatedItemPos = container->findFirstLetRef(fLetter, 0, 1)))
            {
                MSG(("adding: %s to main menu", subdir.string()));
            }
            else
            {
                // ok, have the position of the related entry
                // which needs to be moved to submenu
                YMenuItem *relatedItem = container->getItem(relatedItemPos);
                MSG(("Moving %s to submenu to prepare for %s",
                                relatedItem->getName().c_str(),
                                subdir.string()));
                YMenu *smenu = new YMenu();
                smenu->addSorted(relatedItem, false, true);
                YMenuItem *newItem = new YMenuItem(subName, 0, null, actionNull, smenu);
                newItem->setChecked(relatedItem->isChecked() || im->isChecked());
                container->setItem(relatedItemPos, newItem);
                targetMenu = smenu;
            }
        }

        if (im) {
            if (targetMenu->addSorted(im, false, true) == nullptr) {
                delete im;
                im = nullptr;
            }
        }
        if (im) {
            findThemeAlternatives(app, smActionListener, subdir, dir.entry(), im);
            if (im->isChecked()) {
                YMenuItem *sub = container->findName(subName);
                if (sub && sub->getSubmenu()) {
                    sub->setChecked(true);
                }
            }
        }
    }
}


void ThemesMenu::findThemeAlternatives(
    IApp *app,
    YSMListener *smActionListener,
    const upath& path,
    const mstring& relName,
    YMenuItem *item)
{
    mstring defTheme("default.theme");
    mstring extension(".theme");
    for (udir dir(path); dir.nextExt(extension); ) {
        const mstring entry(dir.entry());
        if (entry != defTheme) {
            upath altThemePath(path + entry);

            if (altThemePath.isReadable()) {
                YMenu *sub(item->getSubmenu());

                if (sub == nullptr)
                    item->setSubmenu(sub = new YMenu());

                if (sub) {
                    int prefixLength = entry.length() - extension.length();
                    mstring tname(entry.substring(0, prefixLength));
                    mstring relThemeName = upath(relName) + entry;
                    YMenuItem *im = newThemeItem(app, smActionListener,
                                tname, relThemeName);
                    sub->add(im);
                    if (im->isChecked())
                        item->setChecked(true);
                }
            }
        }
    }
}

// vim: set sw=4 ts=4 et:
