#ifndef __THEMES_H
#define __THEMES_H

#ifndef NO_CONFIGURE_MENUS

#include "objmenu.h"
#include <sys/time.h>

class YMenu;

class ThemesMenu: public ObjectMenu {
public:
    ThemesMenu(YWindow *parent = 0);
    virtual ~ThemesMenu();

private:
    void findThemes(char const *path, YMenu *container);

    static YMenuItem *newThemeItem(char const *label, char const *theme);
    static void findThemeAlternatives(char const *path, YMenuItem *item);
};

#endif

#endif
