#ifndef __THEMES_H
#define __THEMES_H

#ifndef NO_CONFIGURE_MENUS

#include "objmenu.h"
#include "obj.h"
#include <sys/time.h>

class YMenu;

class DTheme: public DObject {
public:
    DTheme(const ustring &label, const ustring &theme);
    virtual ~DTheme();

    virtual void open();
private:
    ustring fTheme;
};

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
    // this solution isn't nice. Saving it globaly somewhere would be
    // much better, we would have a themeCound from the last refresh
    // cycle and update it after menu construction, counting themes that
    // are actually added to menues
    int countThemes(char const *path);
    int themeCount;
};

#endif

#endif
