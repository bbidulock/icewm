#ifndef __THEMES_H
#define __THEMES_H

#if 0

// !!! will do something else

#include "objmenu.h"
#include <sys/time.h>

class YMenu;

class ThemesMenu: public ObjectMenu {
public:
    ThemesMenu(YWindow *parent = 0);
    virtual ~ThemesMenu();

private:
    char *fPath;
    time_t fModTime;

    void findThemes(const char *path, YMenu *container);
    void addTheme(YMenu *container, const char *name, const char *name_theme);
    YMenuItem* findTheme(YMenu *container, const char *themeName);
    void findThemeAlternatives(const char *path, int plen, YMenuItem *item);
};

#endif

#endif
