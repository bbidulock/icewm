#ifndef __THEMES_H
#define __THEMES_H

#ifndef NO_CONFIGURE_MENUS

#include "objmenu.h"
#include "obj.h"
#include <sys/time.h>

class YMenu;

class DTheme: public DObject {
public:
    DTheme(const char *label, const char *theme);
    virtual ~DTheme();

    virtual void open();
private:
    const char *fTheme;
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
};

#endif

#endif
