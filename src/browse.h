#ifndef __BROWSE_H
#define __BROWSE_H

#ifndef NO_CONFIGURE_MENUS

#include <sys/time.h>

class BrowseMenu: public ObjectMenu {
public:
    BrowseMenu(upath path, YWindow *parent = 0);
    virtual ~BrowseMenu();
    virtual void updatePopup();
private:
    upath fPath;
    time_t fModTime;
};

#endif

#endif
