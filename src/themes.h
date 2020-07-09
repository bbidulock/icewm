#ifndef __THEMES_H
#define __THEMES_H

#include "objmenu.h"
#include "obj.h"

class YMenu;
class YSMListener;
class YActionListener;

class DTheme: public DObject {
public:
    DTheme(IApp *app, YSMListener *smActionListener, const mstring &label, const mstring &theme);
    virtual ~DTheme();

    virtual void open();
private:
    YSMListener *smActionListener;
    mstring fTheme;
    IApp *app;
};

class ThemesMenu: public ObjectMenu {
public:
    ThemesMenu(IApp *app, YSMListener *smActionListener, YActionListener *wmActionListener, YWindow *parent = nullptr);
    virtual ~ThemesMenu();

    virtual void updatePopup();
    virtual void refresh();

private:
    void findThemes(const upath& path, YMenu *container);

    static YMenuItem *newThemeItem(
        IApp *app,
        YSMListener *smActionListener,
        const mstring& label,
        const mstring& relThemeName);

    static void findThemeAlternatives(
        IApp *app,
        YSMListener *smActionListener,
        const upath& path,
        const mstring& relName,
        YMenuItem *item);

    // this solution isn't nice. Saving it globaly somewhere would be
    // much better, we would have a themeCound from the last refresh
    // cycle and update it after menu construction, counting themes that
    // are actually added to menues
    int countThemes(const upath& path);
    int themeCount;

    YSMListener *smActionListener;
    YActionListener *wmActionListener;
    IApp *app;
};

#endif

// vim: set sw=4 ts=4 et:
