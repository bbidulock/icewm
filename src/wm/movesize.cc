/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "yxkeydef.h"
#include "yxutil.h"
#include "ykeyevent.h"
#include "ybuttonevent.h"
#include "ymotionevent.h"
#include "ypointer.h"
#include "wmframe.h"
#include "ypaint.h"

#include "wmclient.h"
#include "wmstatus.h"
#include "yapp.h"
#include "bindkey.h"

void YFrameWindow::snapTo(int &wx, int &wy,
                          int rx1, int ry1, int rx2, int ry2,
                          int &flags)
{
    int d = gSnapDistance.getNum();

    if (flags & 4) { // snap to container window (root, workarea)
        int iw = width();
        if (flags & 8)
            iw -= borderLeft() + borderRight();

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
            ih -= borderTop() + borderBottom();

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
    YFrameWindow *f = fRoot->topLayer();
    int flags = 1 | 2;
    int xp = wx, yp = wy;
    int rx1, ry1, rx2, ry2;

    /// !!! clean this up, it should snap to the closest thing it finds
    
    // try snapping to the border first
    flags |= 4;
    if (xp < fRoot->minX(getLayer()) || xp + int(width()) > fRoot->maxX(getLayer())) {
        xp += borderLeft();
        flags |= 8;
    }
    if (yp < fRoot->minY(getLayer()) || yp + int(height()) > fRoot->maxY(getLayer())) {
        yp += borderTop();
        flags |= 16;
    }
    snapTo(xp, yp,
           fRoot->minX(getLayer()),
           fRoot->minY(getLayer()),
           fRoot->maxX(getLayer()),
           fRoot->maxY(getLayer()),
           flags);
    if (flags & 8) {
        xp -= borderLeft();
        flags &= ~8;
    }
    if (flags & 16) {
        yp -= borderTop();
        flags &= ~16;
    }
    if (xp < 0 || xp + width() > desktop->width()) {
        xp += borderLeft();
        flags |= 8;
    }
    if (yp < 0 || yp + height() > desktop->height()) {
        yp += borderTop();
        flags |= 16;
    }
    snapTo(xp, yp, 0, 0, fRoot->width(), fRoot->height(), flags);
    if (flags & 8) {
        xp -= borderLeft();
        flags &= ~8;
    }
    if (flags & 16) {
        yp -= borderTop();
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

int YFrameWindow::handleMoveKeys(const YKeyEvent &key, int &newX, int &newY) {
    KeySym k = key.getKey();
    int factor = 1;

    if (key.isShift())
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
        newX = fRoot->minX(getLayer()) - borderLeft();
    else if (k == XK_End || k == XK_KP_End)
        newX = fRoot->maxX(getLayer()) - width() + borderRight();
    else if (k == XK_Prior || k == XK_KP_Prior)
        newY = fRoot->minY(getLayer()) - borderTop();
    else if (k == XK_Next || k == XK_KP_Next)
        newY = fRoot->maxY(getLayer()) - height() + borderBottom();
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

int YFrameWindow::handleResizeKeys(const YKeyEvent &key,
                                   int &newX, int &newY, int &newWidth, int &newHeight,
                                   int incX, int incY)
{
    KeySym k = key.getKey(); //XKeycodeToKeysym(app->display(), key.keycode, 0);
    int factor = 1;

    if (key.isShift())
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

void YFrameWindow::handleMoveMouse(const YMotionEvent &motion, int &newX, int &newY) {
    int mouseX = motion.x_root();
    int mouseY = motion.y_root();

    constrainMouseToWorkspace(mouseX, mouseY);

    newX = mouseX - buttonDownX;
    newY = mouseY - buttonDownY;

    constrainPositionByModifier(newX, newY, motion);

    newX += borderLeft();
    newY += borderTop();
    int nx = - (borderLeft() + borderRight());
    int ny = - (borderTop() + borderBottom());

    if (!motion.isAlt()) {
        int er = gEdgeResistance.getNum();

        if (er == 10000) {
            if (newX + int(width() + nx) > fRoot->maxX(getLayer()))
                newX = fRoot->maxX(getLayer()) - width() - nx;
            if (newY + int(height() + ny) > fRoot->maxY(getLayer()))
                newY = fRoot->maxY(getLayer()) - height() - ny;
            if (newX < fRoot->minX(getLayer()))
                newX = fRoot->minX(getLayer());
            if (newY < fRoot->minY(getLayer()))
                newY = fRoot->minY(getLayer());
        } else if (/*EdgeResistance >= 0 && %%% */ er < 10000) {
            if (newX + int(width() + nx) > fRoot->maxX(getLayer())) {
                if (newX + int(width() + nx) < int(fRoot->maxX(getLayer()) + er))
                    newX = fRoot->maxX(getLayer()) - width() - nx;
                else if (motion.isShift())
                    newX -= er;
            }
            if (newY + int(height() + ny) > fRoot->maxY(getLayer())) {
                if (newY + int(height() + ny) < int(fRoot->maxY(getLayer()) + er))
                    newY = fRoot->maxY(getLayer()) - height() - ny;
                else if (motion.isShift())
                    newY -= er;
            }
            if (newX < fRoot->minX(getLayer())) {
                if (newX > int(- er + fRoot->minX(getLayer())))
                    newX = fRoot->minX(getLayer());
                else if (motion.isShift())
                    newX += er;
            }
            if (newY < fRoot->minY(getLayer())) {
                if (newY > int(- er + fRoot->minY(getLayer())))
                    newY = fRoot->minY(getLayer());
                else if (motion.isShift())
                    newY += er;
            }
        }
    }
    newX -= borderLeft();
    newY -= borderTop();
}

void YFrameWindow::handleResizeMouse(const YMotionEvent &motion,
                                     int &newX, int &newY, int &newWidth, int &newHeight)
{
    int mouseX = motion.x_root();
    int mouseY = motion.y_root();

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

    newWidth -= borderLeft() + borderRight();
    newHeight -= borderTop() + borderBottom() + titleY();
    client()->constrainSize(newWidth, newHeight,
                            YFrameClient::csRound |
                            (grabX ? YFrameClient::csKeepX : 0) |
                            (grabY ? YFrameClient::csKeepY : 0));
    newWidth += borderLeft() + borderRight();
    newHeight += borderTop() + borderBottom() + titleY();

    if (grabX == -1)
        newX = x() + width() - newWidth;
    if (grabY == -1)
        newY = y() + height() - newHeight;

}

void YFrameWindow::outlineMove() {
    int xx = x(), yy = y();

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
            {
                int ox = xx;
                int oy = yy;
                int r;
                YKeyEvent key(YEvent::etKeyPress,
                              XKeycodeToKeysym(app->display(), xev.xkey.keycode, 0),
                              -1,
                              app->VMod(xev.xkey.state),
                              xev.xkey.time,
                              xev.xkey.window);

                switch (r = handleMoveKeys(key, xx, yy)) {
                case 0:
                    break;
                case 1:
                case -2:
                    if (xx != ox || yy != oy) {
                        drawOutline(ox, oy, width(), height());
#ifdef CONFIG_MOVESIZE_STATUS
                        if (fRoot->getMoveSizeStatus())
                            fRoot->getMoveSizeStatus()->setStatus(this, xx, yy, width(), height());
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

                YMotionEvent motion(YEvent::etButtonRelease,
                                    xev.xmotion.x,
                                    xev.xmotion.y,
                                    xev.xmotion.x_root,
                                    xev.xmotion.y_root,
                                    app->VMod(xev.xmotion.state),
                                    xev.xmotion.time,
                                    xev.xmotion.window);

                handleMoveMouse(motion, xx, yy);
                if (xx != ox || yy != oy) {
#ifdef CONFIG_MOVESIZE_STATUS
                    if (fRoot->getMoveSizeStatus())
                        fRoot->getMoveSizeStatus()->setStatus(this, xx, yy, width(), height());
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

                YKeyEvent key(YEvent::etKeyPress,
                              XKeycodeToKeysym(app->display(), xev.xkey.keycode, 0),
                              -1,
                              xev.xkey.state,
                              xev.xkey.time,
                              xev.xkey.window);

                switch (r = handleResizeKeys(key, xx, yy, ww, hh, incX, incY)) {
                case 0:
                    break;
                case -2:
                case 1:
                    if (ox != xx || oy != yy || ow != ww || oh != hh) {
                        drawOutline(ox, oy, ow, oh);
#ifdef CONFIG_MOVESIZE_STATUS
                        if (fRoot->getMoveSizeStatus())
                            fRoot->getMoveSizeStatus()->setStatus(this, xx, yy, ww, hh);
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

                YMotionEvent motion(YEvent::etButtonRelease,
                                    xev.xmotion.x,
                                    xev.xmotion.y,
                                    xev.xmotion.x_root,
                                    xev.xmotion.y_root,
                                    app->VMod(xev.xmotion.state),
                                    xev.xmotion.time,
                                    xev.xmotion.window);
                handleResizeMouse(motion, xx, yy, ww, hh);
                if (ox != xx || oy != yy || ow != ww || oh != hh) {
                    drawOutline(ox, oy, ow, oh);
#ifdef CONFIG_MOVESIZE_STATUS
                    if (fRoot->getMoveSizeStatus())
                        fRoot->getMoveSizeStatus()->setStatus(this, xx, yy, ww, hh);
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

    grabX = borderLeft();
    grabY = borderTop();
    origX = x();
    origY = y();
    origW = width();
    origH = height();
    buttonDownX = 0;
    buttonDownY = 0;

    if (!app->grabEvents(desktop, YPointer::move(),
                         ButtonPressMask |
                         ButtonReleaseMask |
                         PointerMotionMask))
        return;
    XGrabServer(app->display());
#ifdef CONFIG_MOVESIZE_STATUS
    if (fRoot->getMoveSizeStatus())
        fRoot->getMoveSizeStatus()->begin(this);
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

                YKeyEvent key(YEvent::etKeyPress,
                              XKeycodeToKeysym(app->display(), xev.xkey.keycode, 0),
                              -1,
                              app->VMod(xev.xkey.state),
                              xev.xkey.time,
                              xev.xkey.window);

                switch (r = handleMoveKeys(key, xx, yy)) {
                case 0:
                    break;
                case 1:
                case -2:
                    if (xx != ox || yy != oy) {
                        drawOutline(ox, oy, width(), height());
#ifdef CONFIG_MOVESIZE_STATUS
                        if (fRoot->getMoveSizeStatus())
                            fRoot->getMoveSizeStatus()->setStatus(this, xx, yy, width(), height());
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

                YMotionEvent motion(YEvent::etButtonRelease,
                                    xev.xmotion.x,
                                    xev.xmotion.y,
                                    xev.xmotion.x_root,
                                    xev.xmotion.y_root,
                                    app->VMod(xev.xmotion.state),
                                    xev.xmotion.time,
                                    xev.xmotion.window);
                handleMoveMouse(motion, xx, yy);
                if (xx != ox || yy != oy) {
#ifdef CONFIG_MOVESIZE_STATUS
                    if (fRoot->getMoveSizeStatus())
                        fRoot->getMoveSizeStatus()->setStatus(this, xx, yy, width(), height());
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
#ifdef CONFIG_MOVESIZE_STATUS
    if (fRoot->getMoveSizeStatus())
        fRoot->getMoveSizeStatus()->end();
#endif
    moveWindow(xx, yy);
    app->releaseEvents();
    XUngrabServer(app->display());
}

bool YFrameWindow::eventKey(const YKeyEvent &key) {

//bool YFrameWindow::handleKeySym(const XKeyEvent &key, KeySym k, int vm) {
    if (key.type() == YEvent::etKeyPress) {
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
                newWidth -= borderLeft() + borderRight();
                newHeight -= borderTop() + borderBottom() + titleY();
                client()->constrainSize(newWidth, newHeight,
                                        YFrameClient::csRound |
                                        (grabX ? YFrameClient::csKeepX : 0) |
                                        (grabY ? YFrameClient::csKeepY : 0));
                newWidth += borderLeft() + borderRight();
                newHeight += borderTop() + borderBottom() + titleY();

                if (grabX == -1)
                    newX = x() + width() - newWidth;
                if (grabY == -1)
                    newY = y() + height() - newHeight;

                setGeometry(newX,
                            newY,
                            newWidth,
                            newHeight);

#ifdef CONFIG_MOVESIZE_STATUS
                if (fRoot->getMoveSizeStatus())
                    fRoot->getMoveSizeStatus()->setStatus(this);
#endif
                break;
            case -2:
                setGeometry(newX, newY, newWidth, newHeight);
                /* nobreak */
            case -1:
                endMoveSize();
                break;
            }
        } else {
            if (!isRollup() && !isIconic() && key.getWindow() != handle())
                return true;

            if (IS_WMKEY(key, gKeyWinClose)) {
                if (canClose()) wmClose();
            } else if (IS_WMKEY(key, gKeyWinPrev)) {
                wmPrevWindow();
            } else if (IS_WMKEY(key, gKeyWinMaximizeVert)) {
                wmMaximizeVert();
            } else if (IS_WMKEY(key, gKeyWinRaise)) {
                if (canRaise()) wmRaise();
            } else if (IS_WMKEY(key, gKeyWinOccupyAll)) {
                wmOccupyAllOrCurrent();
            } else if (IS_WMKEY(key, gKeyWinLower)) {
                if (canLower()) wmLower();
            } else if (IS_WMKEY(key, gKeyWinRestore)) {
                wmRestore();
            } else if (IS_WMKEY(key, gKeyWinNext)) {
                wmNextWindow();
            } else if (IS_WMKEY(key, gKeyWinMove)) {
                if (canMove()) wmMove();
            } else if (IS_WMKEY(key, gKeyWinSize)) {
                if (canSize()) wmSize();
            } else if (IS_WMKEY(key, gKeyWinMinimize)) {
                if (canMinimize()) wmMinimize();
            } else if (IS_WMKEY(key, gKeyWinMaximize)) {
                if (canMaximize()) wmMaximize();
            } else if (IS_WMKEY(key, gKeyWinHide)) {
                if (canHide()) wmHide();
            } else if (IS_WMKEY(key, gKeyWinRollup)) {
                if (canRollup()) wmRollup();
            } else if (IS_WMKEY(key, gKeyWinMenu)) {
                popupSystemMenu();
            }
            if (isIconic() || isRollup()) {
                if (key.getKey() == XK_Return || key.getKey() == XK_KP_Enter) {
                    wmRestore();
                } else if ((key.getKey() == XK_Menu) || (key.getKey() == XK_F10 && key.isShift())) {
                    popupSystemMenu();
                }
            }
        }
    }
    return true;
}

void YFrameWindow::constrainPositionByModifier(int &x, int &y, const YMotionEvent &motion) {
//    unsigned int mask = motion.state & (ShiftMask | ControlMask);

    x += borderLeft();
    y += borderTop();
    if (motion.isShift() && !motion.isCtrl()) {
        x = x / 4 * 4;
        y = y / 4 * 4;
    }
    x -= borderLeft();
    y -= borderTop();

    if (gSnapMove.getBool() && !motion.isShift() && !motion.isCtrl()) {
        snapTo(x, y);
    }
}

void YFrameWindow::constrainMouseToWorkspace(int &x, int &y) {
    if (x < fRoot->minX(getLayer()))
        x = fRoot->minX(getLayer());
    else if (x >= fRoot->maxX(getLayer()))
        x = fRoot->maxX(getLayer()) - 1;
    if (y < fRoot->minY(getLayer()))
        y = fRoot->minY(getLayer());
    else if (y >= fRoot->maxY(getLayer()))
        y = fRoot->maxY(getLayer()) - 1;
}

bool YFrameWindow::canSize(bool horiz, bool vert) {
    if (isRollup())
        return false;
#ifndef NO_MWM_HINTS
    if (!(client()->mwmFunctions() & MWM_FUNC_RESIZE))
        return false;
#endif
    if (!gSizeMaximized.getBool()) {
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
    YPointer *grabPointer = 0;

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

        grabPointer = YPointer::move();
    } else if (!doMove) {
        YPointer *ptr = YPointer::left();

        if (grabY == -1) {
            if (grabX == -1)
                ptr = YPointer::resize_top_left();
            else if (grabX == 1)
                ptr = YPointer::resize_top_right();
            else
                ptr = YPointer::resize_top();
        } else if (grabY == 1) {
            if (grabX == -1)
                ptr = YPointer::resize_bottom_left();
            else if (grabX == 1)
                ptr = YPointer::resize_bottom_right();
            else
                ptr = YPointer::resize_bottom();
        } else {
            if (grabX == -1)
                ptr = YPointer::resize_left();
            else if (grabX == 1)
                ptr = YPointer::resize_right();
        }

        if (grabX == 1)
            buttonDownX = x() + width() - mouseXroot;
        else if (grabX == -1)
            buttonDownX = mouseXroot - x();

        if (grabY == 1)
            buttonDownY = y() + height() - mouseYroot;
        else if (grabY == -1)
            buttonDownY = mouseYroot - y();

        grabPointer = ptr;
    }

    XSync(app->display(), False);
    if (!app->grabEvents(this, grabPointer,
                         ButtonPressMask |
                         ButtonReleaseMask |
                         PointerMotionMask))
    {
        return;
    }

    if (doMove)
        movingWindow = true;
    else
        sizingWindow = true;

#ifdef CONFIG_MOVESIZE_STATUS
    if (fRoot->getMoveSizeStatus())
        fRoot->getMoveSizeStatus()->begin(this);
#endif
    if (doMove && !gOpaqueMove.getBool()) {
        outlineMove();
        endMoveSize();
    } else if (!doMove && !gOpaqueResize.getBool()) {
        outlineResize();
        endMoveSize();
    }
}

void YFrameWindow::endMoveSize() {
    app->releaseEvents();
#ifdef CONFIG_MOVESIZE_STATUS
    if (fRoot->getMoveSizeStatus())
        fRoot->getMoveSizeStatus()->end();
#endif
    movingWindow = false;
    sizingWindow = false;
}

bool YFrameWindow::eventBeginDrag(const YButtonEvent &down, const YMotionEvent &motion) {
    if ((down.getButton() == 3) && canMove()) {
        startMoveSize(1, 1,
                      0, 0,
                      down.x(), down.y());
        eventDrag(down, motion);
    } else if ((down.getButton() == 1) && canSize()) {
        grabX = 0;
        grabY = 0;

        int cx = gCornerX.getNum();
        int cy = gCornerY.getNum();

        if (down.x() < int(borderLeft())) grabX = -1;
        else if (width() - down.x() <= borderRight()) grabX = 1;

        if (down.y() < int(borderTop())) grabY = -1;
        else if (height() - down.y() <= borderBottom()) grabY = 1;

        if (grabY != 0 && grabX == 0) {
            if (down.x() < int(cx)) grabX = -1;
            else if (int(width() - down.x()) <= cx) grabX = 1;
        }

        if (grabX != 0 && grabY == 0) {
            if (down.y() < int(cy)) grabY = -1;
            else if (int(height() - down.y()) <= cy) grabY = 1;
        }

        if (grabX != 0 || grabY != 0) {
            startMoveSize(0, 1,
                          grabX, grabY,
                          down.x_root(), down.y_root());

        }
    }
    return true;
}

void YFrameWindow::moveWindow(int newX, int newY) {
    if (newX >= int(fRoot->maxX(getLayer()) - borderLeft()))
        newX = fRoot->maxX(getLayer()) - borderLeft();

    if (newY >= int(fRoot->maxY(getLayer()) - borderTop() - titleY()))
        newY = fRoot->maxY(getLayer()) - borderTop() - titleY();

    if (newX < int(fRoot->minX(getLayer()) - (width() - borderRight())))
        newX = fRoot->minX(getLayer()) - (width() - borderRight());

    if (newY < int(fRoot->minY(getLayer()) - (height() - borderBottom())))
        newY = fRoot->minY(getLayer()) - (height() - borderBottom());

    setPosition(newX, newY);

#ifdef CONFIG_MOVESIZE_STATUS
    if (fRoot->getMoveSizeStatus())
        fRoot->getMoveSizeStatus()->setStatus(this);
#endif
}

bool YFrameWindow::eventDrag(const YButtonEvent &/*down*/, const YMotionEvent &/*motion*/) {
    return false;
}

bool YFrameWindow::eventEndDrag(const YButtonEvent &/*down*/, const YButtonEvent &/*up*/) {
    return false;
}

bool YFrameWindow::eventButton(const YButtonEvent &button) {
    if (button.type() == YEvent::etButtonPress) {
        if (button.getButton() == 1 && !(button.isCtrl())) {
            activate();
            if (gRaiseOnClickFrame.getBool())
                wmRaise();
        }
    } else if (button.type() == YEvent::etButtonRelease) {
        if (movingWindow || sizingWindow) {
            endMoveSize();
            return true;
        }
    }
    return YWindow::eventButton(button);
}

bool YFrameWindow::eventMotion(const YMotionEvent &motion) {
    if (sizingWindow) {
        int newX = x(), newY = y();
        int newWidth = width(), newHeight = height();

        handleResizeMouse(motion, newX, newY, newWidth, newHeight);

        setGeometry(newX,
                    newY,
                    newWidth,
                    newHeight);
#ifdef CONFIG_MOVESIZE_STATUS
        if (fRoot->getMoveSizeStatus())
            fRoot->getMoveSizeStatus()->setStatus(this);
#endif
        return true;
    } else if (movingWindow) {
        int newX = x();
        int newY = y();

        handleMoveMouse(motion, newX, newY);
        moveWindow(newX, newY);
        return true;
    }
    return YWindow::eventMotion(motion);
}
