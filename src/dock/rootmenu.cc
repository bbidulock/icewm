#include "config.h"
#include "yfull.h"
#include "rootmenu.h"
#include "wmprog.h"
#include "yapp.h"

#include <stdlib.h>
#include <unistd.h>

DesktopHandler::DesktopHandler(): YWindow(0, RootWindow(app->display(), DefaultScreen(app->display())))
{
    setStyle(wsManager);
}

void DesktopHandler::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        YMenu *rootMenu = getRootMenu();
            if (rootMenu)
                rootMenu->popup(0, 0, up.x, up.y, -1, -1,
                                YPopupWindow::pfCanFlipVertical |
                                YPopupWindow::pfCanFlipHorizontal |
                                YPopupWindow::pfPopupMenu);
    }
}

ObjectMenu *DesktopHandler::getRootMenu() {
    if (fRootMenu)
        return fRootMenu;

    fRootMenu = new StartMenu("foo");
    if (fRootMenu == 0)
        return 0;

    return fRootMenu;
}
