#include "config.h"
#include "wmdock.h"
#include "wmclient.h"
#include "wmframe.h"
#include "wmmgr.h"
#include "wmoption.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "yxapp.h"
#include "yxcontext.h"
#include <X11/Xatom.h>

const char DockApp::propertyName[] = "_ICEWM_DOCKAPPS";

DockApp::DockApp():
    YWindow(nullptr, None,
            DefaultDepth(xapp->display(), xapp->screen()),
            DefaultVisual(xapp->display(), xapp->screen()),
            DefaultColormap(xapp->display(), xapp->screen())),
    dragged(nullptr),
    saveset(None),
    intern(None),
    center(0),
    layered(WinLayerInvalid),
    direction(1),
    dragxpos(0),
    dragypos(0),
    restack(true),
    isRight(true)
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
    mstring config(mstring(dockApps).trim().lower());
    if (config.isEmpty()) {
        return false;
    }

    for (mstring s(config), r; s.splitall(' ', &s, &r); s = r) {
        if (s == "right") {
            isRight = true;
        } else if (s == "left") {
            isRight = false;
        } else if (s == "above") {
            layered = WinLayerAboveDock;
        } else if (s == "dock") {
            layered = WinLayerDock;
        } else if (s == "ontop") {
            layered = WinLayerOnTop;
        } else if (s == "normal") {
            layered = WinLayerNormal;
        } else if (s == "below") {
            layered = WinLayerBelow;
        } else if (s == "desktop") {
            layered = WinLayerDesktop;
        } else if (s == "center") {
            center = 0;
        } else if (s == "down") {
            center = 1;
        } else if (s == "high") {
            center = -1;
        }
    }
    if (layered == WinLayerInvalid)
        layered = WinLayerDesktop;

    extern const char* clrNormalButton;
    YColor bg(clrNormalButton);
    setBackground(bg.pixel());
    setTitle("IceDock");
    unsigned char wmClassName[] = "icedock\0IceWM";
    XChangeProperty(xapp->display(), handle(), XA_WM_CLASS, XA_STRING, 8,
                    PropModeReplace, wmClassName, sizeof(wmClassName));
    if (intern == None) {
        intern = xapp->atom(propertyName);
    }
    if (intern) {
        YProperty prop(desktop, intern, F32, 123L, XA_WINDOW, True);
        for (Atom window : prop) {
            recover += window;
        }
    }
    return true;
}

void DockApp::grabit() {
    XGrabButton(xapp->display(), AnyButton, ControlMask, handle(), False,
                ButtonPressMask | ButtonReleaseMask | Button1MotionMask,
                GrabModeSync, GrabModeAsync, None, None);
}

void DockApp::ungrab() {
    XUngrabButton(xapp->display(), AnyButton, ControlMask, handle());
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

bool DockApp::isChild(Window window) {
    Window rp;
    unsigned count = 0;
    xsmart<Window> child;
    if (XQueryTree(xapp->display(), handle(), &rp, &rp, &child, &count)) {
        for (unsigned i = 0; i < count; ++i) {
            if (window == child[i]) {
                return true;
            }
        }
    }
    return false;
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
    }
    if (icon) {
        XAddToSaveSet(xapp->display(), icon);
        XReparentWindow(xapp->display(), icon, handle(),
                        0, height() + 64);
        if (isChild(icon)) {
            XMapWindow(xapp->display(), icon);
            if (icon != client->handle()) {
                XAddToSaveSet(xapp->display(), client->handle());
                XReparentWindow(xapp->display(), client->handle(),
                                savewin(), 0, 0);
            }

            bool closing = false, forcing = false;
            int order = ordering(client, &closing, &forcing);
            if (closing) {
                client->setDocked(true);
                int k = docks.getCount();
                docks += docking(icon, client, order);
                revoke(k, forcing);
                return true;
            }

            int found = find(recover, client->handle());
            int k = docks.getCount();
            while (k > 0 &&
                   (order < docks[k-1].order ||
                    (order == docks[k-1].order &&
                     found >= 0 &&
                     !inrange(find(recover, docks[k-1].client->handle()),
                              0, found))))
            {
                k--;
            }
            docks.insert(k, docking(icon, client, order));

            client->setDocked(true);
            direction = +1;
            retime();
        }
        else {
            XRemoveFromSaveSet(xapp->display(), icon);
            icon = None;
        }
    }
    return bool(icon);
}

int DockApp::ordering(YFrameClient* client, bool* startClose, bool* forced) {
    char* name = client->classHint()->res_name;
    if (nonempty(name)) {
        char* base = const_cast<char*>(my_basename(name));
        if (nonempty(base) && base != name) {
            char* copy = strdup(base);
            if (copy) {
                XFree(name);
                name = copy;
                client->classHint()->res_name = copy;
            }
        }
    }

    xsmart<char> copy;
    if (isEmpty(name)) {
        client->fetchTitle(&copy);
        name = copy;
    }

    int order = 0;
    if (nonempty(name)) {
        WindowOption opt(name);
        if (hintOptions)
            hintOptions->mergeWindowOption(opt, name, true);
        if (defOptions)
            defOptions->mergeWindowOption(opt, name, false);
        order = opt.order;
        *startClose = hasbit(opt.option_mask, YFrameWindow::foClose)
                       && hasbit(opt.options, YFrameWindow::foClose);
        *forced = hasbit(opt.option_mask, YFrameWindow::foForcedClose)
                   && hasbit(opt.options, YFrameWindow::foForcedClose);
    } else {
        *startClose = false;
    }
    return order;
}

bool DockApp::handleTimer(YTimer* t) {
    bool restart = false;
    if (t == timer) {
        if (manager->isRunning()) {
            adapt();
        } else {
            restart = true;
        }
    }
    return restart;
}

void DockApp::adapt() {
    if (docks.nonempty()) {
        int mx, my, Mx, My;
        manager->getWorkArea(&mx, &my, &Mx, &My);
        int rows = min(docks.getCount(), (My - my) / 64);
        int cols = (docks.getCount() + (rows - 1)) / rows;
        rows = (docks.getCount() + (cols - 1)) / cols;
        int xpos = isRight ? Mx - cols * 64 : 0;
        int ypos = (center == -1) ? 0
                 : (center == +1) ? (My - rows * 64)
                 : my + (My - my - rows * 64) / 2;
        setGeometry(YRect(xpos, ypos, cols * 64, rows * 64));
        for (int k = 0; k < docks.getCount(); k++) {
            int i = (direction < 0) ? (docks.getCount() - 1 - k) : k;
            int x = 64 * (cols - 1 - i / rows);
            int y = 64 * (i % rows);
            if (docks[i].window == docks[i].client->handle()) {
                x += (64 - min(64, int(docks[i].client->width()))) / 2;
                y += (64 - min(64, int(docks[i].client->height()))) / 2;
            }
            XMoveWindow(xapp->display(), docks[i].window, x, y);
            XMapWindow(xapp->display(), docks[i].window);
        }
#ifdef CONFIG_SHAPE_RR
        if (false && shapes.supported) {
            const int count = docks.getCount();
            XRectangle rect[count];
            for (int i = 0; i < count; ++i) {
                rect[i].x = 64 * (cols - 1 - i / rows);
                rect[i].y = 64 * (i % rows);
                rect[i].width = 64;
                rect[i].height = 64;
            }
            XShapeCombineRectangles(xapp->display(), handle(),
                                    ShapeBounding, 0, 0, rect,
                                    count, ShapeSet, Unsorted);
        }
#endif
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
        if (visible() == false) {
            show();
            grabit();
            if (restack) {
                restack = false;
                manager->restackWindows();
            }
        }
        proper();
    }
    else if (visible()) {
        ungrab();
        hide();
        proper();
    }
    if (timer)
        timer = null;
}

void DockApp::proper() {
    if (intern == None) {
        intern = xapp->atom(propertyName);
    }
    if (intern) {
        const int count = docks.getCount();
        if (count) {
            Atom atoms[count];
            for (int i = 0; i < count; ++i) {
                atoms[i] = Atom(docks[i].client->handle());
            }
            desktop->setProperty(intern, XA_WINDOW, atoms, count);
        } else {
            desktop->deleteProperty(intern);
        }
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
        if (dragged == dock.client) {
            dragged = nullptr;
        }
    }
    docks.remove(index);
}

bool DockApp::undock(YFrameClient* client) {
    for (int i = docks.getCount(); --i >= 0; ) {
        if (docks[i].client == client) {
            undock(i);
            direction = +1;
            retime();
            return true;
        }
    }
    return false;
}

void DockApp::revoke(int k, bool kill) {
    if (inrange(k, 0, docks.getCount() - 1)) {
        docking dock(docks[k]);
        XUnmapWindow(xapp->display(), dock.window);
        if (kill || !dock.client->protocol(YFrameClient::wpDeleteWindow)) {
            XDestroyWindow(xapp->display(), dock.client->handle());
            if (dock.window != dock.client->handle())
                XDestroyWindow(xapp->display(), dock.window);
        } else {
            dock.client->sendPing();
            dock.client->sendDelete();
        }
        docks.remove(k);
        retime();
    }
}

void DockApp::actionPerformed(YAction action, unsigned modifiers) {
    revoke(action.ident() - 1, false);
}

void DockApp::handlePopDown(YPopupWindow* popup) {
    if (menu == popup)
        menu = null;
    if (docks.nonempty())
        grabit();
}

void DockApp::handleButton(const XButtonEvent& button) {
    if (hasbit(button.state, ControlMask)) {
        XAllowEvents(xapp->display(), AsyncPointer, CurrentTime);
    }
    YWindow::handleButton(button);
}

void DockApp::handleClick(const XButtonEvent& button, int count) {
    int click = int(button.button) * int(hasbit(button.state, ControlMask));
    if (click == Button2 && count == 1) {
        int k = 0;
        for (docking dock : docks) {
            if (dock.window == button.subwindow) {
                revoke(k, hasbit(button.state, ShiftMask));
                return;
            }
            ++k;
        }
    }
    if (click == Button3 && count == 1) {
        menu = null;
        int rows = height() / 64;
        // int cols = width() / 64;
        int k = 0, select = -1, separators = 0;
        for (docking dock : docks) {
            const char* name = dock.client->classHint()->res_name;
            xsmart<char> copy;
            if (isEmpty(name)) {
                dock.client->fetchTitle(&copy);
                name = copy;
            }
            mstring number;
            if (isEmpty(name)) {
                number = mstring(k);
                name = number.c_str();
            }
            menu->addItem(name, -1, null, YAction(EAction(k + 1)))
                ->setChecked(true);
            if (dock.window == button.subwindow) {
                select = k + separators;
            }
            if (++k % rows == 0 && k < docks.getCount()) {
                menu->addSeparator();
                ++separators;
            }
        }
        menu->setActionListener(this);
        menu->popup(nullptr, nullptr, this,
                    button.x_root, button.y_root,
                    YPopupWindow::pfCanFlipVertical |
                    YPopupWindow::pfCanFlipHorizontal);
        if (select >= 0) {
            menu->focusItem(select);
        }
    }
    if (click == Button4) {
        if (docks.getCount() > 1) {
            docking dock(docks[0]);
            docks.remove(0);
            docks += dock;
            direction = +1;
            retime();
        }
    }
    if (click == Button5) {
        if (docks.getCount() > 1) {
            docking dock(docks.last());
            docks.pop();
            docks.insert(0, dock);
            direction = -1;
            retime();
        }
    }
}

bool DockApp::handleBeginDrag(const XButtonEvent& down, const XMotionEvent& move) {
    dragged = nullptr;
    if (down.button == Button1 && hasbit(down.state, ControlMask)) {
        if (down.subwindow && move.subwindow &&
            down.subwindow == move.subwindow)
        {
            for (const docking& dock : docks) {
                if (dock.window == down.subwindow) {
                    XWindowAttributes attr;
                    if (XGetWindowAttributes(xapp->display(),
                                             dock.window, &attr))
                    {
                        dragged = dock.client;
                        dragxpos = attr.x;
                        dragypos = attr.y;
                        XRaiseWindow(xapp->display(), dock.window);
                        handleDrag(down, move);
                        break;
                    }
                }
            }
        }
    }
    return dragged;
}

void DockApp::handleDrag(const XButtonEvent& down, const XMotionEvent& move) {
    if (dragged) {
        for (const docking& dock : docks) {
            if (dock.client == dragged) {
                int x = dragxpos + (move.x_root - down.x_root);
                int y = dragypos + (move.y_root - down.y_root);
                XMoveWindow(xapp->display(), dock.window, x, y);
                break;
            }
        }
    }
}

void DockApp::handleEndDrag(const XButtonEvent& down, const XButtonEvent& up) {
    if (dragged) {
        XWindowAttributes attr = {};
        int x = dragxpos + (up.x_root - down.x_root);
        int y = dragypos + (up.y_root - down.y_root);
        int d = -1;
        int a = -1;
        for (int i = 0; i < docks.getCount(); ++i) {
            if (docks[i].client == dragged) {
                d = i;
            }
            else if (a == -1
                && XGetWindowAttributes(xapp->display(),
                                        docks[i].window, &attr)
                && attr.x / 64 == (x + 32) / 64
                && attr.y / 64 == (y + 32) / 64) {
                a = i;
            }
        }
        if (d >= 0 && a >= 0) {
            XMoveWindow(xapp->display(), docks[d].window,
                        (attr.x / 64) * 64 + dragxpos % 64,
                        (attr.y / 64) * 64 + dragypos % 64);
            docking copy(docks[d]);
            docks.remove(d);
            docks.insert(a, copy);
            proper();
            retime();
        }
        else if (d >= 0) {
            XMoveWindow(xapp->display(), docks[d].window,
                        dragxpos, dragypos);
        }
        dragged = nullptr;
    }
}

