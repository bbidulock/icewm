#ifndef __GNOMEAPPS_H
#define __GNOMEAPPS_H

#ifdef GNOME

#include "objmenu.h"
#include <gnome.h>
#include "obj.h"

class DGnomeDesktopEntry: public DObject {
public:
    DGnomeDesktopEntry(const char *name, YIcon *icon, GnomeDesktopEntry *dentry);
    ~DGnomeDesktopEntry();

    virtual void open();
private:
    GnomeDesktopEntry *fEntry;
};

class GnomeMenu: public ObjectMenu {
public:
    GnomeMenu(YWindow *parent, const char *path);
    virtual ~GnomeMenu();
    virtual void updatePopup();
private:
    char *fPath;
    time_t fModTime;
};
#endif

#endif
