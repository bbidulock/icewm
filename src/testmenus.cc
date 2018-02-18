#include "config.h"
#include "ywindow.h"
#include "ymenu.h"
#include "yaction.h"
#include "ylocale.h"
#include "yxapp.h"
#include "yicon.h"
#include "ymenuitem.h"
#include "wmprog.h"
#include "wmapp.h"
#include "yprefs.h"
#include "default.h"

const char *ApplicationName = "testmenus";
YMenu *logoutMenu(NULL);
YWMApp *wmapp(NULL);
YMenu *windowListMenu(NULL);
YWindowManager *manager;

void YWMApp::restartClient(const char *path, char *const *args) {
}
void YWMApp::runOnce(const char *resource, const char *path, char *const *args) {
}
void YWMApp::signalGuiEvent(GUIEvent) {
}

class MenuWindow: public YWindow {
public:
    YMenu *menu;
    YMenu *submenu0;
    YMenu *submenu1;
    YMenu *submenu2;
    YIcon *file;

    MenuWindow(IApp *app, YSMListener *smListener, YActionListener *wmActionListener) {
        menu = new YMenu();

        menu = new StartMenu(app, smListener, wmActionListener, "menu");

#if 0
        file = YIcon::getIcon("file");

        YAction actionNone = actionNull;

        submenu0 = new YMenu();
        submenu0->addItem("XML Tree", 0, null, actionNone);
        submenu0->addItem("Text", 0, null, actionNone);
        submenu0->addItem("Hex", 0, null, actionNone);

        submenu1 = new YMenu();
        submenu1->addItem("Name", 0, null, actionNone);
        submenu1->addItem("Size", 0, null, actionNone);
        submenu1->addItem("Modified", 0, null, actionNone);
        submenu1->addItem("Modified", 0, null, actionNone);

        submenu2 = new YMenu();
        submenu2->addItem("Contents", 0, null, actionNone);
        submenu2->addItem("Search", 0, null, actionNone);
        submenu2->addSeparator();
        submenu2->addItem("About", 0, null, actionNone);

        menu = new YMenu();
        menu->addItem("Open", 0, 0, submenu0);
        menu->addItem("Properties", 0, null, actionNone);
        menu->addSeparator();
        menu->addItem("Help", 0, 0, submenu2);
        menu->addSeparator();
        menu->addItem("Cut", 0, null, actionNone)->setIcon(file);
        menu->addItem("Copy", 0, null, actionNone)->setIcon(file);
        menu->addItem("Paste", 0, null, actionNone)->setIcon(file);
        menu->addItem("Delete", 0, null, actionNone)->setIcon(file);
        menu->addSeparator();
        menu->addItem("Sort", 0, actionNone, submenu1);
        menu->addSeparator();
        menu->addItem("Close", 0, null, actionNone);
#endif
    }

    void handleButton(const XButtonEvent &button) {
        if ((button.type == ButtonRelease) &&
            (button.button == 3))
        {
            menu->popup(this, 0, 0, button.x_root, button.y_root,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
        }
    }
};

int main(int argc, char **argv) {
    iconPath = "/usr/share/icons/crystalsvg/48x48/apps/:/usr/share/icons/Bluecurve/16x16/apps/:/usr/share/pixmap";
    YLocale locale;
    YXApplication xapp(&argc, &argv);

    ////XSynchronize(xapp.display(), True);

    YWindow *w = new MenuWindow(&xapp, 0, 0);
    w->setSize(200, 200);
    w->show();

    return xapp.mainLoop();
}

// vim: set sw=4 ts=4 et:
