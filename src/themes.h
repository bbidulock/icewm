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

    void updatePopup();
    void refresh();

private:
    void findThemes(char const *path, YMenu *container);

    static YMenuItem *newThemeItem(char const *label, char const *theme, const char *relThemeName);
    static void findThemeAlternatives(char const *path, const char *relName, YMenuItem *item);
};

#endif

#endif
