#include "config.h"
#include "ylib.h"
#include "rootmenu.h"
#include "yapp.h"

DesktopHandler::DesktopHandler(): YWindow(0, RootWindow(app->display(), DefaultScreen(app->display())))
{
    setStyle(wsManager);
}

void DesktopHandler::handleClick(const XButtonEvent &/*up*/, int /*count*/) {
#if 0
    if (up.button == 3) {
        if (rootMenu)
            rootMenu->popup(0, 0, up.x, up.y, -1, -1,
                            YPopupWindow::pfCanFlipVertical |
                            YPopupWindow::pfCanFlipHorizontal |
                            YPopupWindow::pfPopupMenu);
    }
#endif
}
