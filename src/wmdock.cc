#include "config.h"
#include "wmdock.h"
#include "wmclient.h"
#include "wmmgr.h"
#include "yxapp.h"
#include "yxcontext.h"
#include <X11/Xatom.h>

DockApp::DockApp():
    YWindow(nullptr, None,
            DefaultDepth(xapp->display(), DefaultScreen(xapp->display())),
            DefaultVisual(xapp->display(), DefaultScreen(xapp->display())),
            DefaultColormap(xapp->display(), DefaultScreen(xapp->display()))),
    saveset(None),
    isRight(false),
    isLeft(false),
    isAbove(false),
    isBelow(false)
{
    setStyle(wsOverrideRedirect | wsNoExpose);
}

DockApp::~DockApp() {
    hide();
    for (int i = docks.getCount(); --i >= 0; ) {
        docking dock = docks[i];
        undock(i);
        delete dock.client;
    }
    if (saveset) {
        XDestroyWindow(xapp->display(), saveset);
    }
}

bool DockApp::setup() {
    extern const char* dockApps;
    mstring config(mstring(dockApps).trim());
    if (config.isEmpty()) {
        return false;
    }
    else {
        for (mstring s(config), r; s.splitall(' ', &s, &r); s = r) {
            if (s == "right") {
                isRight = true; isLeft = false;
            } else if (s == "left") {
                isLeft = true; isRight = false;
            } else if (s == "above") {
                isAbove = true; isBelow = false;
            } else if (s == "below") {
                isBelow = true; isAbove = false;
            }
        }
        if (isRight + isLeft == 0)
            isRight = true;
        if (isAbove + isBelow == 0)
            isAbove = true;
    }

    extern const char* clrNormalButton;
    YColor bg(clrNormalButton);
    setBackground(bg.pixel());
    setTitle("IceDock");
    unsigned char wmClassName[] = "icedock\0IceWM";
    XChangeProperty(xapp->display(), handle(), XA_WM_CLASS, XA_STRING, 8,
                    PropModeReplace, wmClassName, sizeof(wmClassName));
    return true;
}

Window DockApp::savewin() {
    if (saveset == None) {
        saveset = XCreateSimpleWindow(xapp->display(), xapp->root(),
                                      -1, -1, 1, 1, 0, None, None);
        unsigned char wmClassName[] = "icesave\0IceWM";
        XChangeProperty(xapp->display(), saveset, XA_WM_CLASS, XA_STRING, 8,
                        PropModeReplace, wmClassName, sizeof(wmClassName));
        XStoreName(xapp->display(), saveset, "IceSave");
    }
    return saveset;
}

bool DockApp::dock(YFrameClient* client) {
    Window icon = None;
    if (client->adopted()) {
        if (client->isDockAppIcon()) {
            icon = client->iconWindowHint();
            YWindow* ptr = nullptr;
            if (windowContext.find(icon, &ptr) && ptr && ptr != client) {
                YFrameClient* other = dynamic_cast<YFrameClient*>(ptr);
                if (other && other->adopted()) {
                    manager->unmanageClient(other);
                }
            }
        }
        else if (client->isDockAppWindow()) {
            icon = client->handle();
        }
    }
    if (icon) {
        Window root;
        int x, y;
        unsigned w, h, border, depth;
        if (XGetGeometry(xapp->display(), icon, &root, &x, &y,
                         &w, &h, &border, &depth) == False) {
            icon = None;
        }
        else if (w > 64 || h > 64) {
            icon = None;
        }
        else if (created() == false && setup() == false) {
            icon = None;
        }
        else {
            XAddToSaveSet(xapp->display(), icon);
            XReparentWindow(xapp->display(), icon, handle(),
                            0, 64 * docks.getCount());

            bool found = false;
            Window parent;
            unsigned count = 0;
            xsmart<Window> child;
            if (XQueryTree(xapp->display(), handle(), &root, &parent,
                           &child, &count) == True) {
                for (unsigned i = 0; i < count; ++i) {
                    if (icon == child[i]) {
                        found = true;
                        break;
                    }
                }
            }
            if (found) {
                XMapWindow(xapp->display(), icon);
                if (icon != client->handle()) {
                    XAddToSaveSet(xapp->display(), client->handle());
                    XReparentWindow(xapp->display(), client->handle(),
                                    savewin(), 0, 0);
                }
                docks += docking(icon, client);
                client->setDocked(true);
                adapt();
            }
            else {
                XRemoveFromSaveSet(xapp->display(), icon);
                icon = None;
            }
        }
    }
    return bool(icon);
}

void DockApp::adapt() {
    if (docks.nonempty()) {
        int mx, my, Mx, My;
        manager->getWorkArea(&mx, &my, &Mx, &My);
        int rows = min(docks.getCount(), (My - my) / 64);
        int cols = (docks.getCount() + (rows - 1)) / rows;
        rows = (docks.getCount() + (cols - 1)) / cols;
        int xpos = isRight ? Mx - cols * 64 : 0;
        int ypos = my + (My - my - rows * 64) / 2;
        setGeometry(YRect(xpos, ypos, cols * 64, rows * 64));
        for (int i = 0; i < docks.getCount(); i++) {
            int x = 64 * (i / rows);
            int y = 64 * (i % rows);
            if (docks[i].window == docks[i].client->handle()) {
                x += (64 - min(64, int(docks[i].client->width()))) / 2;
                y += (64 - min(64, int(docks[i].client->height()))) / 2;
            }
            XMoveWindow(xapp->display(), docks[i].window, x, y);
            XMapWindow(xapp->display(), docks[i].window);
        }
#ifdef CONFIG_SHAPE_MM
        if (false && shapes.supported) {
            XRectangle full;
            full.x = 0;
            full.y = 0;
            full.width = width();
            full.height = height();
            XShapeCombineRectangles(xapp->display(), handle(),
                                    ShapeBounding, 0, 0, &full, 1,
                                    ShapeSet, Unsorted);
            for (int i = 0; i < docks.getCount(); i++) {
                XShapeCombineShape(xapp->display(), handle(), ShapeBounding,
                                   64 * (i / rows), 64 * (i % rows),
                                   docks[i].window, ShapeBounding,
                                   i == 0 ? ShapeSet : ShapeUnion);
            }
        }
#endif
        show();
    } else {
        hide();
    }
}

void DockApp::undock(int index) {
    docking dock(docks[index]);
    if (dock.client->destroyed() == false) {
        if (dock.client->handle() == dock.window) {
            XReparentWindow(xapp->display(), dock.window,
                            xapp->root(), 0, 0);
            XRemoveFromSaveSet(xapp->display(), dock.window);
        } else {
            XUnmapWindow(xapp->display(), dock.window);
            XReparentWindow(xapp->display(), dock.window,
                            xapp->root(), 0, 0);
            XRemoveFromSaveSet(xapp->display(), dock.window);
            XReparentWindow(xapp->display(), dock.client->handle(),
                            xapp->root(), 0, 0);
            XRemoveFromSaveSet(xapp->display(), dock.client->handle());
            XMapWindow(xapp->display(), dock.client->handle());
        }
    }
    docks.remove(index);
}

bool DockApp::undock(YFrameClient* client) {
    for (int i = docks.getCount(); --i >= 0; ) {
        if (docks[i].client == client) {
            undock(i);
            adapt();
            return true;
        }
    }
    return false;
}

