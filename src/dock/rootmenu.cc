#include "config.h"
#include "yxlib.h"
#include "rootmenu.h"
#include "wmprog.h"
#include "yapp.h"
#include "ybuttonevent.h"

#include <stdlib.h>
#include <unistd.h>

DesktopHandler::DesktopHandler(): YWindow(0, RootWindow(app->display(), DefaultScreen(app->display())))
{
    setStyle(wsManager);
}

bool DesktopHandler::eventClick(const YClickEvent &up) {
    if (up.rightButton()) {
        YMenu *rootMenu = getRootMenu();
        if (rootMenu)
            rootMenu->popup(0, 0, up.x(), up.y(), -1, -1,
                            YPopupWindow::pfCanFlipVertical |
                            YPopupWindow::pfCanFlipHorizontal);
        return true;
    }
    return YWindow::eventClick(up);
}

ObjectMenu *DesktopHandler::getRootMenu() {
    if (fRootMenu)
        return fRootMenu;

    fRootMenu = new StartMenu("foo");
    if (fRootMenu == 0)
        return 0;

    return fRootMenu;
}
