#ifndef __BROWSE_H
#define __BROWSE_H

#ifndef NO_CONFIGURE_MENUS

#include <sys/time.h>

class BrowseMenu: public ObjectMenu {
public:
    BrowseMenu(const char *path, YWindow *parent = 0);
    virtual ~BrowseMenu();
    virtual void updatePopup();
private:
    char *fPath;
    time_t fModTime;
};

#endif

#endif
