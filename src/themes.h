#ifndef THEMES_H
#define THEMES_H

#include "objmenu.h"
#include "obj.h"

class YMenu;
class YSMListener;
class YActionListener;

class DTheme: public DObject {
public:
    DTheme(IApp* app, YSMListener* listener, mstring label, mstring theme);
    virtual ~DTheme();

    virtual void open();
private:
    YSMListener *smActionListener;
    mstring fTheme;
};

class ThemesMenu: public ObjectMenu {
public:
    ThemesMenu(IApp *app, YSMListener *smListener, YActionListener *wmListener);
    virtual ~ThemesMenu();

    virtual void updatePopup();
    virtual void refresh();

private:
    void findThemes(upath path, ObjectMenu* container);

    YMenuItem *newThemeItem(
        IApp *app,
        YSMListener *smActionListener,
        mstring label,
        mstring relThemeName,
        ObjectMenu* container);

    void findThemeAlternatives(
        IApp *app,
        YSMListener *smActionListener,
        upath path,
        mstring relName,
        YMenuItem *item,
        ObjectMenu* container);

    // this solution isn't nice. Saving it globaly somewhere would be
    // much better, we would have a themeCound from the last refresh
    // cycle and update it after menu construction, counting themes that
    // are actually added to menues
    int countThemes(upath path);
    int themeCount;

    YSMListener *smActionListener;
    IApp *app;
};

#endif

// vim: set sw=4 ts=4 et:
