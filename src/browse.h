#ifndef __BROWSE_H
#define __BROWSE_H

class BrowseMenu: public ObjectMenu {
public:
    BrowseMenu(
        IApp *app,
        YSMListener *smActionListener,
        YActionListener *wmActionListener,
        upath path,
        YWindow *parent = 0);
    virtual ~BrowseMenu();
    virtual void updatePopup();
private:
    upath fPath;
    time_t fModTime;
    YSMListener *smActionListener;
    IApp *app;
};

#endif

// vim: set sw=4 ts=4 et:
