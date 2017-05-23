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
#include <dirent.h>
#include "wmapp.h"
#include "ascii.h"
#include "wmconfig.h"
#include "wmaction.h"
#include "appnames.h"

#include "intl.h"

void setDefaultTheme(const char *theme) {
    const char *buf = cstrJoin("Theme=\"", theme, "\"\n", NULL);

    setDefault("theme", buf);

    delete [] buf;
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
    DIR *dir(opendir(path.string()));
    int ret=0;

    if (dir != NULL) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
           // assume that OS caches this. Otherwise, construct a new
           // object to just store the string and an YArrayList of them,
           // read the entries once and work with List contents later

           if(de->d_name[0] == '.')
              continue;
           ret++;
        }
        closedir(dir);
    }
    // this just assumes that there is no other trash
    return ret-1;
}

ThemesMenu::~ThemesMenu() {
}

YMenuItem * ThemesMenu::newThemeItem(IApp *app, YSMListener *smActionListener, char const *label, char const *relThemeName) {
    DTheme *dtheme = new DTheme(app, smActionListener, label, relThemeName);

    if (dtheme) {
        YMenuItem *item(new DObjectMenuItem(dtheme));

        if (item) {
            item->setChecked(themeName && 0 == strcmp(themeName, relThemeName));
            return item;
        }
    }
    return NULL;
}

void ThemesMenu::findThemes(const upath& path, YMenu *container) {
    char const *const tname("/default.theme");

    bool bNesting = nestedThemeMenuMinNumber && themeCount>nestedThemeMenuMinNumber;

    DIR *dir(opendir(path.string()));

    if (dir != NULL) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {

            if (de->d_name[0] == '.')
                continue;

            YMenuItem *im(NULL);
            upath subdir = path + de->d_name;
            if (false == subdir.dirExists())
                continue;
            upath npath = subdir + tname;

            if (npath.access(R_OK) == 0) {
                char *relThemeName = cstrJoin(de->d_name, tname, NULL);
                im = newThemeItem(app, smActionListener, de->d_name, relThemeName);
                if (im) {
                    if (bNesting) 
                    {
                        char fLetter = ASCII::toUpper(de->d_name[0]);

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
                    } else //the default method without Extra SubMenues
                        container->addSorted(im, false);
                }
                delete [] relThemeName;
            }

            if (im) {
                findThemeAlternatives(app, smActionListener, subdir, de->d_name, im);
            }
        }

        closedir(dir);
    }
}


void ThemesMenu::findThemeAlternatives(
    IApp *app,
    YSMListener *smActionListener,
    const upath& path,
    const char *relName,
    YMenuItem *item)
{
    DIR *dir(opendir(path.string()));

    if (dir != NULL) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            char const *ext(strstr(de->d_name, ".theme"));

            if (ext != NULL && 0 == strcmp(ext, ".theme") &&
                strcmp(de->d_name, "default.theme"))
            {
                upath npath(path + de->d_name);

                if (npath.access(R_OK) == 0) {
                    YMenu *sub(item->getSubmenu());

                    if (sub == NULL)
                        item->setSubmenu(sub = new YMenu());

                    if (sub) {
                        ustring tname(de->d_name, ext - de->d_name);
                        ustring relThemeName = upath(relName) + de->d_name;
                        sub->add(newThemeItem(app, smActionListener,
                                    cstring(tname), cstring(relThemeName)));
                    }
                }
            }
        }
        closedir(dir);
    }
}
#endif
