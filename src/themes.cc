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

#include "intl.h"

extern char *configArg;

void setDefaultTheme(const char *theme) {
    const char *confDir = strJoin(getenv("HOME"), "/.icewm", NULL);
    mkdir(confDir, 0777);
    delete[] confDir;
    const char *themeConfNew = strJoin(getenv("HOME"), "/.icewm/theme.new.tmp", NULL);
    const char *themeConf = strJoin(getenv("HOME"), "/.icewm/theme", NULL);
    int fd = open(themeConfNew, O_RDWR | O_TEXT | O_CREAT | O_TRUNC | O_EXCL, 0666);
    if(fd == -1)
    {
       fprintf(stderr, "Unable to write %s!", themeConfNew);
       return;
    }
/// TODO #warning "do proper escaping"
    const char *buf = strJoin("Theme=\"", theme, "\"\n", NULL);
    int len = strlen(buf);
    int nlen;
    nlen = write(fd, buf, len);
    delete [] buf;
    
    FILE *fdold = fopen(themeConf, "r");
    if (fdold) {
       char *tmpbuf = new char[300];
       if (tmpbuf) {
          *tmpbuf = '#';
          for (int i = 0; i < 10; i++)
             if (fgets(tmpbuf + 1, 298, fdold))
                write(fd, tmpbuf, strlen(tmpbuf));
             else 
                break;
          delete[] tmpbuf;
       }
       fclose(fdold);
    }

    close(fd);
    if (nlen == len) {
        rename(themeConfNew, themeConf);
    } else {
        remove(themeConfNew);
    }
    delete[] themeConfNew;
    delete[] themeConf;
}

DTheme::DTheme(const char *label, const char *theme): DObject(label, 0) {
    fTheme = newstr(theme);
}

DTheme::~DTheme() {
    delete[] fTheme;
}

void DTheme::open() {
    if (!fTheme)
        return;

    setDefaultTheme(fTheme);

    const char *bg[] = { ICEWMBGEXE, "-r", 0 };
    int pid = app->runProgram(bg[0], bg);
    app->waitProgram(pid);

    YStringArray args(4);

    wmapp->restartClient(0, 0);
}

ThemesMenu::ThemesMenu(YWindow *parent): ObjectMenu(parent) {
}

void ThemesMenu::updatePopup() {
    refresh();
}

void ThemesMenu::refresh() {
    removeAll();

    char *path;

    if (nestedThemeMenuMinNumber)
        themeCount =
            countThemes(strJoin(libDir, "/themes/", NULL)) +
            countThemes(strJoin(configDir, "/themes/", NULL)) +
            countThemes(strJoin(YApplication::getPrivConfDir(),
                                "/themes/", NULL));

    path = strJoin(libDir, "/themes/", NULL);
    findThemes(path, this);
    delete[] path;

    path = strJoin(configDir, "/themes/", NULL);
    findThemes(path, this);
    delete[] path;

    path = strJoin(YApplication::getPrivConfDir(), "/themes/", NULL);
    findThemes(path, this);
    delete[] path;

    addSeparator();
    add(newThemeItem(_("Default"), CONFIG_DEFAULT_THEME, CONFIG_DEFAULT_THEME));
}

int ThemesMenu::countThemes(const char *path) {
    DIR *dir(opendir(path));
    int ret=0;

    if (dir != NULL) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
           // assume that OS caches this. Otherwise, construct a new
           // object to just store the string and an YArrayList of them,
           // read the entries once and work with List contents later
           ret++;
        }
        closedir(dir);
    }
    // this just assumes that there is no other trash
    return ret-1;
}

ThemesMenu::~ThemesMenu() {
}

YMenuItem * ThemesMenu::newThemeItem(char const *label, char const */*theme*/, char const *relThemeName) {
    DTheme *dtheme = new DTheme(label, relThemeName);

    if (dtheme) {
        YMenuItem *item(new DObjectMenuItem(dtheme));

        if (item) {
            item->setChecked(themeName && 0 == strcmp(themeName, relThemeName));
            return item;
        }
    }
    return NULL;
}

void ThemesMenu::findThemes(const char *path, YMenu *container) {
    char const *const tname("/default.theme");
    bool isFirst(true);

    int dplen(strlen(path));
    char *npath = NULL, *dpath = NULL;

    if (dplen == 0 || path[dplen - 1] != '/') {
        npath = strJoin(path, "/", NULL);
        dplen++;
    } else {
        dpath = newstr(path);
    }

    DIR *dir(opendir(path));

    if (dir != NULL) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            YMenuItem *im(NULL);
            npath = strJoin(dpath, de->d_name, tname, NULL);

            if (npath && access(npath, R_OK) == 0) {
                if (isFirst) {
                    isFirst = false;
//              if (itemCount())
//                        addSeparator();
                    //addLabel(path);
                    //addSeparator();
                }
                char *relThemeName = strJoin(de->d_name, tname, NULL);
                im = newThemeItem(de->d_name, npath, relThemeName);
                if (im) {
                    if (nestedThemeMenuMinNumber && themeCount>nestedThemeMenuMinNumber) {
                        int targetItem = container->findFirstLetRef(de->d_name[0], 0, 1);
                        char *smname = strdup("....");
                        *smname = ASCII::toUpper(de->d_name[0]);
                        if (targetItem >= 0) {
                            YMenuItem *oldSibling = container->getItem(targetItem);
                            // we have something with this letter
                            if (0 == strcmp(smname, oldSibling->getName())) {
                                // is our submenu
                                (oldSibling->getSubmenu())->addSorted(im, true);
                            } else {
                                // menu a new item, a menu under it, move
                                // the theme item to the submenu and assign
                                // oldSibling reference to it
                                YMenu *smenu = new YMenu();
                                YMenuItem *smItem = new YMenuItem(smname, 0, NULL, NULL, smenu);
                                if(smItem && smenu) {
                                   smenu->addSorted(oldSibling, false);
                                   smenu->addSorted(im, false);
                                   container->setItem(targetItem, smItem);
                                }
                            }
                        } else //no sibling, add a plain icon
                            container->addSorted(im, false);
                    } else //the default method without Extra SubMenues
                        container->addSorted(im, false);
                }
                delete [] relThemeName;
            }

            delete [] npath;

            char *subdir(strJoin(dpath, de->d_name, NULL));
            if (im && subdir) findThemeAlternatives(subdir, de->d_name, im);
            delete [] subdir;
        }

        closedir(dir);
    }

    delete [] dpath;
}

void ThemesMenu::findThemeAlternatives(const char *path, const char *relName,
                                       YMenuItem *item)
{
    DIR *dir(opendir(path));

    if (dir != NULL) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            char const *ext(strstr(de->d_name, ".theme"));

            if (ext != NULL && ext[sizeof("theme")] == '\0' &&
                strcmp(de->d_name, "default.theme"))
            {
                char *npath(strJoin(path, "/", de->d_name, NULL));

                if (npath && access(npath, R_OK) == 0) {
                    YMenu *sub(item->getSubmenu());

                    if (sub == NULL)
                        item->setSubmenu(sub = new YMenu());

                    if (sub) {
                        char *tname(newstr(de->d_name, ext - de->d_name));
                        char *relThemeName = strJoin(relName, "/",
                                                     de->d_name, NULL);
                        sub->add(newThemeItem(tname, npath, relThemeName));
                        delete[] tname;
                        delete[] relThemeName;
                    }
                }
                delete[] npath;
            }
        }
        closedir(dir);
    }
}
#endif
