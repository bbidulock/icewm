/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ykey.h"
#include "wmframe.h"

#include "wmclient.h"
#include "wmstatus.h"
#include "yapp.h"
#include "wmapp.h"

void YFrameWindow::snapTo(int &wx, int &wy,
                          int rx1, int ry1, int rx2, int ry2,
                          int &flags)
{
    int d = snapDistance;

    if (flags & 4) { // snap to container window (root, workarea)
        int iw = width();
        if (flags & 8)
            iw -= 2 * borderX();

        if (flags & 1) { // x
            int wxw = wx + iw;

            if (wx >= rx1 - d && wx <= rx1 + d) {
                wx = rx1;
                flags &= ~1;
            } else if (wxw >= rx2 - d && wxw <= rx2 + d) {
                wx = rx2 - iw;
                flags &= ~1;
            }
        }
    } else { // snap to other window
        int iw = width();
        int ih = height();
        bool touchY = false;

        //if (wy >= ry1 && wy <= ry2 ||
        //    wy + ih >= rx1 && wy + ih <= ry2)
        if (wy <= ry2 && wy + ih >= ry1)
            touchY = true;

        if ((flags & 1) && touchY) { // x
            int wxw = wx + iw;

            if (wx >= rx2 - d && wx <= rx2 + d) {
                wx = rx2;
                flags &= ~1;
            } else if (wxw >= rx1 - d && wxw <= rx1 + d) {
                wx = rx1 - iw;
                flags &= ~1;
            }
        }
    }
    if (flags & 4) {
        int ih = height();

        if (flags & 16)
            ih -= 2 * borderY();

        if (flags & 2) { // y
            int wyh = wy + ih;

            if (wy >= ry1 - d && wy <= ry1 + d) {
                wy = ry1;
                flags &= ~2;
            } else if (wyh >= ry2 - d && wyh <= ry2 + d) {
                wy = ry2 - ih;
                flags &= ~2;
            }
        }
    } else {
        int iw = width();
        int ih = height();
        bool touchX = false;

        //if (wx >= rx1 && wx <= rx2 ||
        //    wx + iw >= rx1 && wx + iw <= rx2)
        if (wx <= rx2 && wx + iw >= rx1)
            touchX = true;

        if ((flags & 2) && touchX) { // y
            int wyh = wy + ih;

            if (wy >= ry2 - d && wy <= ry2 + d) {
                wy = ry2;
                flags &= ~2;
            } else if (wyh >= ry1 - d && wyh <= ry1 + d) {
                wy = ry1 - ih;
                flags &= ~2;
            }
        }
    }
}

void YFrameWindow::snapTo(int &wx, int &wy) {
    YFrameWindow *f = manager->topLayer();
    int flags = 1 | 2;
    int xp = wx, yp = wy;
    int rx1, ry1, rx2, ry2;

    /// !!! clean this up, it should snap to the closest thing it finds
    
    // try snapping to the border first
    flags |= 4;
    if (xp < manager->minX(this) || 
        xp + int(width()) > manager->maxX(this)) {
        xp += borderX();
        flags |= 8;
    }
    if (yp < manager->minY(this) || 
        yp + int(height()) > manager->maxY(this)) {
        yp += borderY();
        flags |= 16;
    }

    snapTo(xp, yp, manager->minX(this), manager->minY(this),
		   manager->maxX(this), manager->maxY(this), flags);

    if (flags & 8) {
        xp -= borderX();
        flags &= ~8;
    }
    if (flags & 16) {
        yp -= borderY();
        flags &= ~16;
    }
    if (xp < 0 || xp + width() > desktop->width()) {
        xp += borderX();
        flags |= 8;
    }
    if (yp < 0 || yp + height() > desktop->height()) {
        yp += borderY();
        flags |= 16;
    }
    snapTo(xp, yp, 0, 0, manager->width(), manager->height(), flags);
    if (flags & 8) {
        xp -= borderX();
        flags &= ~8;
    }
    if (flags & 16) {
        yp -= borderY();
        flags &= ~16;
    }
    flags &= ~4;

    if (flags & (1 | 2)) {
        // we only snap to windows below, hope that's ok
        while (f) {
            if (f != this && f->visible()) {
                rx1 = f->x();
                ry1 = f->y();
                rx2 = f->x() + f->width();
                ry2 = f->y() + f->height();
                snapTo(xp, yp, rx1, ry1, rx2, ry2, flags);
                if (!(flags & (1 | 2)))
                    break;
            }
            f = f->nextLayer();
        }
    }
    wx = xp;
    wy = yp;
}

void YFrameWindow::drawOutline(int x, int y, int w, int h) {
    int bw = (wsBorderX + wsBorderY) / 2;
    static GC outlineGC = None;

    if (outlineGC == None) {
        XGCValues gcv;

        gcv.foreground = YColor(clrActiveBorder).pixel();
        gcv.function = GXxor;
        gcv.graphics_exposures = False;
        gcv.line_width = (wsBorderX + wsBorderY) / 2;
        gcv.subwindow_mode = IncludeInferiors;

        outlineGC = XCreateGC(app->display(), desktop->handle(),
                              GCForeground |
                              GCFunction |
                              GCGraphicsExposures |
                              GCLineWidth |
                              GCSubwindowMode,
                              &gcv);
    }

    x += bw / 2;
    y += bw / 2;
    w -= bw;
    h -= bw;
    XDrawRectangle(app->display(), manager->handle(), outlineGC,
                   x, y, w, h);
}

int YFrameWindow::handleMoveKeys(const XKeyEvent &key, int &newX, int &newY) {
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    int m = KEY_MODMASK(key.state);
    int factor = 1;

    if (m == ShiftMask)
        factor = 4;

    if (k == XK_Left || k == XK_KP_Left)
        newX -= factor;
    else if (k == XK_Right || k == XK_KP_Right)
        newX += factor;
    else if (k == XK_Up || k == XK_KP_Up)
        newY -= factor;
    else if (k == XK_Down || k == XK_KP_Down)
        newY += factor;
    else if (k == XK_Home || k == XK_KP_Home)
        newX = manager->minX(this) - borderX();
    else if (k == XK_End || k == XK_KP_End)
        newX = manager->maxX(this) - width() + borderX();
    else if (k == XK_Prior || k == XK_KP_Prior)
        newY = manager->minY(this) - borderY();
    else if (k == XK_Next || k == XK_KP_Next)
        newY = manager->maxY(this) - height() + borderY();
    else if (k == XK_Return || k == XK_KP_Enter)
        return -1;
    else if (k ==  XK_Escape) {
        newX = origX;
        newY = origY;
        return -2;
    } else
        return 0;
    return 1;
}

int YFrameWindow::handleResizeKeys(const XKeyEvent &key,
                                   int &newX, int &newY, int &newWidth, int &newHeight,
                                   int incX, int incY)
{
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    int m = KEY_MODMASK(key.state);
    int factor = 1;

    if (m == ShiftMask)
        factor = 4;

    if (k == XK_Left || k == XK_KP_Left) {
        if (grabX == 0) {
            grabX = -1;
        } else if (grabX == 1) {
            newWidth -= incX * factor;
        } else if (grabX == -1) {
            newWidth += incX * factor;
            newX -= incX * factor;
        }
    } else if (k == XK_Right || k == XK_KP_Right) {
        if (grabX == 0) {
            grabX = 1;
        } else if (grabX == 1) {
            newWidth += incX * factor;
        } else if (grabX == -1) {
            newWidth -= incX * factor;
            newX += incX * factor;
        }
    } else if (k == XK_Up || k == XK_KP_Up) {
        if (grabY == 0) {
            grabY = -1;
        } else if (grabY == 1) {
            newHeight -= incY * factor;
        } else if (grabY == -1) {
            newHeight += incY * factor;
            newY -= incY * factor;
        }
    } else if (k == XK_Down || k == XK_KP_Down) {
        if (grabY == 0) {
            grabY = 1;
        } else if (grabY == 1) {
            newHeight += incY * factor;
        } else if (grabY == -1) {
            newHeight -= incY * factor;
            newY += incY * factor;
        }
    } else if (k == XK_Return || k == XK_KP_Enter)
        return -1;
    else if (k ==  XK_Escape) {
        newX = origX;
        newY = origY;
        newWidth = origW;
        newHeight = origH;
        return -2;
    } else
        return 0;
    return 1;
}

void YFrameWindow::handleMoveMouse(const XMotionEvent &motion, int &newX, int &newY) {
    int mouseX = motion.x_root;
    int mouseY = motion.y_root;

    constrainMouseToWorkspace(mouseX, mouseY);

    newX = mouseX - buttonDownX;
    newY = mouseY - buttonDownY;

    constrainPositionByModifier(newX, newY, motion);

    newX += borderX();
    newY += borderY();
    int n = -2;

    if (!(motion.state & app->AltMask)) {
        if (EdgeResistance == 10000) {
            if (newX + int(width() + n * borderX()) > manager->maxX(this))
                newX = manager->maxX(this) - width() - n * borderX();
            if (newY + int(height() + n * borderY()) > manager->maxY(this))
                newY = manager->maxY(this) - height() - n * borderY();
            if (newX < manager->minX(this))
                newX = manager->minX(this);
            if (newY < manager->minY(this))
                newY = manager->minY(this);
        } else if (/*EdgeResistance >= 0 && %%% */ EdgeResistance < 10000) {
            if (newX + int(width() + n * borderX()) > manager->maxX(this))
                if (newX + int(width() + n * borderX()) < int(manager->maxX(this) + EdgeResistance))
                    newX = manager->maxX(this) - width() - n * borderX();
                else if (motion.state & ShiftMask)
                    newX -= EdgeResistance;
            if (newY + int(height() + n * borderY()) > manager->maxY(this))
                if (newY + int(height() + n * borderY()) < int(manager->maxY(this) + EdgeResistance))
                    newY = manager->maxY(this) - height() - n * borderY();
                else if (motion.state & ShiftMask)
                    newY -= EdgeResistance;
            if (newX < manager->minX(this))
                if (newX > int(- EdgeResistance + manager->minX(this)))
                    newX = manager->minX(this);
                else if (motion.state & ShiftMask)
                    newX += EdgeResistance;
            if (newY < manager->minY(this))
                if (newY > int(- EdgeResistance + manager->minY(this)))
                    newY = manager->minY(this);
                else if (motion.state & ShiftMask)
                    newY += EdgeResistance;
        }
    }
    newX -= borderX();
    newY -= borderY();
}

void YFrameWindow::handleResizeMouse(const XMotionEvent &motion,
                                     int &newX, int &newY, int &newWidth, int &newHeight)
{
    int mouseX = motion.x_root;
    int mouseY = motion.y_root;

    constrainMouseToWorkspace(mouseX, mouseY);

    if (grabX == -1) {
        newX = mouseX - buttonDownX;
        newWidth = width() + x() - newX;
    } else if (grabX == 1) {
        newWidth = mouseX + buttonDownX - x();
    }

    if (grabY == -1) {
        newY = mouseY - buttonDownY;
        newHeight = height() + y() - newY;
    } else if (grabY == 1) {
        newHeight = mouseY + buttonDownY - y();
    }

    newWidth -= 2 * borderX();
    newHeight -= 2 * borderY() + titleY();
    client()->constrainSize(newWidth, newHeight,
                            YFrameClient::csRound |
                            (grabX ? YFrameClient::csKeepX : 0) |
                            (grabY ? YFrameClient::csKeepY : 0));
    newWidth += 2 * borderX();
    newHeight += 2 * borderY() + titleY();

    if (grabX == -1)
        newX = x() + width() - newWidth;
    if (grabY == -1)
        newY = y() + height() - newHeight;

}

void YFrameWindow::outlineMove() {
    int xx = x(), yy = y();
    unsigned int modifiers = 0;

    XGrabServer(app->display());
    XSync(app->display(), False);
    drawOutline(xx, yy, width(), height());
    while (1) {
        XEvent xev;

        XWindowEvent(app->display(),
                     handle(),
                     KeyPressMask | ExposureMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &xev);
        switch (xev.type) {
        case KeyPress:
            modifiers = xev.xkey.state;
            {
                int ox = xx;
                int oy = yy;
                int r;

                switch (r = handleMoveKeys(xev.xkey, xx, yy)) {
                case 0:
                    break;
                case 1:
                case -2:
                    if (xx != ox || yy != oy) {
                        drawOutline(ox, oy, width(), height());
#ifndef LITE
                        statusMoveSize->setStatus(this, xx, yy, width(), height());
#endif
                        drawOutline(xx, yy, width(), height());
                    }
                    if (r == -2)
                        goto end;
                    break;
                case -1:
                    goto end;
                }
            }
            break;
        case ButtonPress:
        case ButtonRelease:
            modifiers = xev.xbutton.state;
            goto end;
        case MotionNotify:
            {
                int ox = xx;
                int oy = yy;

                handleMoveMouse(xev.xmotion, xx, yy);
                if (xx != ox || yy != oy) {
#ifndef LITE
                    statusMoveSize->setStatus(this, xx, yy, width(), height());
#endif
                    drawOutline(ox, oy, width(), height());
                    drawOutline(xx, yy, width(), height());
                }
            }
            break;
        }
    }
end:
    drawOutline(xx, yy, width(), height());
    XSync(app->display(), False);
    moveWindow(xx, yy);
    XUngrabServer(app->display());
}

void YFrameWindow::outlineResize() {
    int xx = x(), yy = y(), ww = width(), hh = height();
    int incX = 1;
    int incY = 1;

    if (client()->sizeHints()) {
        incX = client()->sizeHints()->width_inc;
        incY = client()->sizeHints()->height_inc;
    }

    XGrabServer(app->display());
    XSync(app->display(), False);
    drawOutline(xx, yy, ww, hh);
    while (1) {
        XEvent xev;

        XWindowEvent(app->display(),
                     handle(),
                     KeyPressMask | ExposureMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &xev);
        switch (xev.type) {
        case KeyPress:
            {
                int ox = xx;
                int oy = yy;
                int ow = ww;
                int oh = hh;
                int r;

                switch (r = handleResizeKeys(xev.xkey, xx, yy, ww, hh, incX, incY)) {
                case 0:
                    break;
                case -2:
                case 1:
                    if (ox != xx || oy != yy || ow != ww || oh != hh) {
                        drawOutline(ox, oy, ow, oh);
#ifndef LITE
                        statusMoveSize->setStatus(this, xx, yy, ww, hh);
#endif
                        drawOutline(xx, yy, ww, hh);
                    }
                    if (r == -2)
                        goto end;
                    break;
                case -1:
                    goto end;
                }
            }
            break;
        case ButtonPress:
        case ButtonRelease:
            goto end;
        case MotionNotify:
            {
                int ox = xx;
                int oy = yy;
                int ow = ww;
                int oh = hh;

                handleResizeMouse(xev.xmotion, xx, yy, ww,hh);
                if (ox != xx || oy != yy || ow != ww || oh != hh) {
                    drawOutline(ox, oy, ow, oh);
#ifndef LITE
                    statusMoveSize->setStatus(this, xx, yy, ww, hh);
#endif
                    drawOutline(xx, yy, ww, hh);
                }
            }
            break;
        }
    }
end:
    drawOutline(xx, yy, ww, hh);
    XSync(app->display(), False);
    setGeometry(xx, yy, ww, hh);
    XUngrabServer(app->display());
}

void YFrameWindow::manualPlace() {
    int xx = x(), yy = y();

    grabX = borderX();
    grabY = borderY();
    origX = x();
    origY = y();
    origW = width();
    origH = height();
    buttonDownX = 0;
    buttonDownY = 0;

    if (!app->grabEvents(desktop,
    			 YApplication::movePointer.handle(),
                         ButtonPressMask |
                         ButtonReleaseMask |
                         PointerMotionMask))
        return ;
    XGrabServer(app->display());
#ifndef LITE
    statusMoveSize->begin(this);
#endif
    drawOutline(xx, yy, width(), height());
    while (1) {
        XEvent xev;

        XMaskEvent(app->display(),
                   KeyPressMask |
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                   &xev);
        switch (xev.type) {
        case KeyPress:
            {
                int ox = xx;
                int oy = yy;
                int r;

                switch (r = handleMoveKeys(xev.xkey, xx, yy)) {
                case 0:
                    break;
                case 1:
                case -2:
                    if (xx != ox || yy != oy) {
                        drawOutline(ox, oy, width(), height());
#ifndef LITE
                        statusMoveSize->setStatus(this, xx, yy, width(), height());
#endif
                        drawOutline(xx, yy, width(), height());
                    }
                    if (r == -2)
                        goto end;
                    break;
                case -1:
                    goto end;
                }
            }
            break;
        case ButtonPress:
        case ButtonRelease:
            goto end;
        case MotionNotify:
            {
                int ox = xx;
                int oy = yy;

                handleMoveMouse(xev.xmotion, xx, yy);
                if (xx != ox || yy != oy) {
#ifndef LITE
                    statusMoveSize->setStatus(this, xx, yy, width(), height());
#endif
                    drawOutline(ox, oy, width(), height());
                    drawOutline(xx, yy, width(), height());
                }
            }
            break;
        }
    }
end:
    drawOutline(xx, yy, width(), height());
#ifndef LITE
    statusMoveSize->end();
#endif
    moveWindow(xx, yy);
    app->releaseEvents();
    XUngrabServer(app->display());
}

bool YFrameWindow::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        if (movingWindow) {
            int newX = x();
            int newY = y();

            switch (handleMoveKeys(key, newX, newY)) {
            case -2:
                moveWindow(newX, newY);
                /* nobreak */
            case -1:
                endMoveSize();
                break;
            case 0:
                return true;
            case 1:
                moveWindow(newX, newY);
                break;
            }
        } else if (sizingWindow) {
            int newX = x();
            int newY = y();
            int newWidth = width();
            int newHeight = height();
            int incX = 1;
            int incY = 1;

            if (client()->sizeHints()) {
                incX = client()->sizeHints()->width_inc;
                incY = client()->sizeHints()->height_inc;
            }

            switch (handleResizeKeys(key, newX, newY, newWidth, newHeight, incX, incY)) {
            case 0:
                break;
            case 1:
                newWidth -= 2 * borderX();
                newHeight -= 2 * borderY() + titleY();
                client()->constrainSize(newWidth, newHeight,
                                        YFrameClient::csRound |
                                        (grabX ? YFrameClient::csKeepX : 0) |
                                        (grabY ? YFrameClient::csKeepY : 0));
                newWidth += 2 * borderX();
                newHeight += 2 * borderY() + titleY();

                if (grabX == -1)
                    newX = x() + width() - newWidth;
                if (grabY == -1)
                    newY = y() + height() - newHeight;

                setGeometry(newX,
                            newY,
                            newWidth,
                            newHeight);

#ifndef LITE
                statusMoveSize->setStatus(this);
#endif
                break;
            case -2:
                setGeometry(newX, newY, newWidth, newHeight);
                /* nobreak */
            case -1:
                endMoveSize();
                break;
            }
        } else if (app->AltMask != 0) {
            KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
            unsigned int m = KEY_MODMASK(key.state);
            unsigned int vm = VMod(m);

            if (!isRollup() && !isIconic() &&
                key.window != handle())
                return true;

            if (IS_WMKEY(k, vm, gKeyWinClose)) {
                if (canClose()) wmClose();
            } else if (IS_WMKEY(k, vm, gKeyWinPrev)) {
                wmPrevWindow();
            } else if (IS_WMKEY(k, vm, gKeyWinMaximizeVert)) {
                wmMaximizeVert();
            } else if (IS_WMKEY(k, vm, gKeyWinRaise)) {
                if (canRaise()) wmRaise();
            } else if (IS_WMKEY(k, vm, gKeyWinOccupyAll)) {
                wmOccupyAllOrCurrent();
            } else if (IS_WMKEY(k, vm, gKeyWinLower)) {
                if (canLower()) wmLower();
            } else if (IS_WMKEY(k, vm, gKeyWinRestore)) {
                wmRestore();
            } else if (IS_WMKEY(k, vm, gKeyWinNext)) {
                wmNextWindow();
            } else if (IS_WMKEY(k, vm, gKeyWinMove)) {
                if (canMove()) wmMove();
            } else if (IS_WMKEY(k, vm, gKeyWinSize)) {
                if (canSize()) wmSize();
            } else if (IS_WMKEY(k, vm, gKeyWinMinimize)) {
                if (canMinimize()) wmMinimize();
            } else if (IS_WMKEY(k, vm, gKeyWinMaximize)) {
                if (canMaximize()) wmMaximize();
            } else if (IS_WMKEY(k, vm, gKeyWinHide)) {
                if (canHide()) wmHide();
            } else if (IS_WMKEY(k, vm, gKeyWinRollup)) {
                if (canRollup()) wmRollup();
            } else if (IS_WMKEY(k, vm, gKeyWinMenu)) {
                popupSystemMenu();
            }
            if (isIconic() || isRollup()) {
                if (k == XK_Return || k == XK_KP_Enter) {
                    wmRestore();
                } else if ((k == XK_Menu) || (k == XK_F10 && m == ShiftMask)) {
                    popupSystemMenu();
                }
            }
        }
    }
    return true;
}

void YFrameWindow::constrainPositionByModifier(int &x, int &y, const XMotionEvent &motion) {
    unsigned int mask = motion.state & (ShiftMask | ControlMask);

    x += borderX();
    y += borderY();
    if (mask == ShiftMask) {
        x = x / 4 * 4;
        y = y / 4 * 4;
    }
    x -= borderX();
    y -= borderY();

    if (snapMove && !(mask & (ControlMask | ShiftMask))) {
        snapTo(x, y);
    }
}

void YFrameWindow::constrainMouseToWorkspace(int &x, int &y) {
    x = clamp(x, manager->minX(this), manager->maxX(this) - 1);
    y = clamp(y, manager->minY(this), manager->maxY(this) - 1);
}

bool YFrameWindow::canSize(bool horiz, bool vert) {
    if (isRollup())
        return false;
#ifndef NO_MWM_HINTS
    if (!(client()->mwmFunctions() & MWM_FUNC_RESIZE))
        return false;
#endif
    if (!sizeMaximized) {
        if ((!vert || isMaximizedVert()) &&
            (!horiz || isMaximizedHoriz()))
            return false;
    }
    return true;
}

bool YFrameWindow::canMove() {
#ifndef NO_MWM_HINTS
    if (!(client()->mwmFunctions() & MWM_FUNC_MOVE))
        return false;
#endif
    return true;
}

void YFrameWindow::startMoveSize(int doMove, int byMouse,
                                 int sideX, int sideY,
                                 int mouseXroot, int mouseYroot)
{
    Cursor grabPointer = None;

    sizeByMouse = byMouse;
    grabX = sideX;
    grabY = sideY;
    origX = x();
    origY = y();
    origW = width();
    origH = height();

    if (doMove && grabX == 0 && grabY == 0) {
        buttonDownX = mouseXroot;
        buttonDownY = mouseYroot;

#ifdef CONFIG_GUIEVENTS
	wmapp->signalGuiEvent(geWindowMoved);
#endif
        grabPointer = YApplication::movePointer.handle();
    } else if (!doMove) {
#ifdef CONFIG_GUIEVENTS
	wmapp->signalGuiEvent(geWindowSized);
#endif

        if (grabY == -1) {
            if (grabX == -1)
                grabPointer = YWMApp::sizeTopLeftPointer.handle();
            else if (grabX == 1)
                grabPointer = YWMApp::sizeTopRightPointer.handle();
            else
                grabPointer = YWMApp::sizeTopPointer.handle();
        } else if (grabY == 1) {
            if (grabX == -1)
                grabPointer = YWMApp::sizeBottomLeftPointer.handle();
            else if (grabX == 1)
                grabPointer = YWMApp::sizeBottomRightPointer.handle();
            else
                grabPointer = YWMApp::sizeBottomPointer.handle();
        } else {
            if (grabX == -1)
                grabPointer = YWMApp::sizeLeftPointer.handle();
            else if (grabX == 1)
                grabPointer = YWMApp::sizeRightPointer.handle();
	    else
        	grabPointer = YApplication::leftPointer.handle();
	    
        }

        if (grabX == 1)
            buttonDownX = x() + width() - mouseXroot;
        else if (grabX == -1)
            buttonDownX = mouseXroot - x();

        if (grabY == 1)
            buttonDownY = y() + height() - mouseYroot;
        else if (grabY == -1)
            buttonDownY = mouseYroot - y();
    }

    XSync(app->display(), False);
    if (!app->grabEvents(this, grabPointer,
                         ButtonPressMask |
                         ButtonReleaseMask |
                         PointerMotionMask))
    {
        return ;
    }

    if (doMove)
        movingWindow = 1;
    else
        sizingWindow = 1;

#ifndef LITE
    statusMoveSize->begin(this);
#endif
    if (doMove && !opaqueMove) {
        outlineMove();
        endMoveSize();
    } else if (!doMove && !opaqueResize) {
        outlineResize();
        endMoveSize();
    }
}

void YFrameWindow::endMoveSize() {
    app->releaseEvents();
#ifndef LITE
    statusMoveSize->end();
#endif
    movingWindow = 0;
    sizingWindow = 0;

    if (client()) // !!! this can happen at destruction
        updateNormalSize();
}

void YFrameWindow::handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion) {
    if ((down.button == 3) && canMove()) {
        startMoveSize(1, 1,
                      0, 0,
                      down.x, down.y);
        handleDrag(down, motion);
    } else if ((down.button == 1) && canSize()) {
        grabX = 0;
        grabY = 0;

        if (down.x < int(borderX())) grabX = -1;
        else if (width() - down.x <= borderX()) grabX = 1;

        if (down.y < int(borderY())) grabY = -1;
        else if (height() - down.y <= borderY()) grabY = 1;

        if (grabY != 0 && grabX == 0) {
            if (down.x < int(wsCornerX)) grabX = -1;
            else if (width() - down.x <= wsCornerX) grabX = 1;
        }

        if (grabX != 0 && grabY == 0) {
            if (down.y < int(wsCornerY)) grabY = -1;
            else if (height() - down.y <= wsCornerY) grabY = 1;
        }

        if (grabX != 0 || grabY != 0) {
            startMoveSize(0, 1,
                          grabX, grabY,
                          down.x_root, down.y_root);

        }
    }
}

void YFrameWindow::moveWindow(int newX, int newY) {
    if (!doNotCover()) {
	newX = clamp(newX,
    		     (int)(manager->minX(this) + borderX() - width()),
    		     (int)(manager->maxX(this) - borderX()));
	newY = clamp(newY,
		     (int)(manager->minY(this) + borderY() - height()),
		     (int)(manager->maxY(this) - borderY()));
    }

    setPosition(newX, newY);

#ifndef LITE
    statusMoveSize->setStatus(this);
#endif
}

void YFrameWindow::handleDrag(const XButtonEvent &/*down*/, const XMotionEvent &/*motion*/) {
}

void YFrameWindow::handleEndDrag(const XButtonEvent &/*down*/, const XButtonEvent &/*up*/) {
}

void YFrameWindow::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (button.button == 1 && !(button.state & ControlMask)) {
            activate();
            if (raiseOnClickFrame)
                wmRaise();
        }
    } else if (button.type == ButtonRelease) {
        if (movingWindow || sizingWindow) {
            endMoveSize();
            return ;
        }
    }
    YWindow::handleButton(button);
}

void YFrameWindow::handleMotion(const XMotionEvent &motion) {
    if (sizingWindow) {
        int newX = x(), newY = y();
        int newWidth = width(), newHeight = height();

        handleResizeMouse(motion, newX, newY, newWidth, newHeight);

        setGeometry(newX,
                    newY,
                    newWidth,
                    newHeight);
#ifndef LITE
        statusMoveSize->setStatus(this);
#endif
        return ;
    } else if (movingWindow) {
        int newX = x();
        int newY = y();

        handleMoveMouse(motion, newX, newY);
        moveWindow(newX, newY);
        return ;
    }
    YWindow::handleMotion(motion);
}

