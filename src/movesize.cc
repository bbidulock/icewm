/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "wmframe.h"
#include "wmstatus.h"
#include "wmapp.h"
#include "prefs.h"
#include "wmtaskbar.h"
#include "aworkspaces.h"
#include "intl.h"

extern YColorName activeBorderBg;

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

    int mx, my, Mx, My;
    manager->getWorkArea(this, &mx, &my, &Mx, &My, getScreen());

    /// !!! clean this up, it should snap to the closest thing it finds

    // try snapping to the border first
    flags |= 4;
    if (xp < mx || xp + int(width()) > Mx) {
        xp += borderX();
        flags |= 8;
    }
    if (yp < my || yp + int(height()) > My) {
        yp += borderY();
        flags |= 16;
    }

    snapTo(xp, yp, mx, my, Mx, My, flags);

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
        for (; f; f = f->nextLayer()) {
            if (affectsWorkArea() && f->inWorkArea())
                continue;

            if (f != this && f->visible()) {
                rx1 = f->x();
                ry1 = f->y();
                rx2 = f->x() + f->width();
                ry2 = f->y() + f->height();
                snapTo(xp, yp, rx1, ry1, rx2, ry2, flags);
                if (!(flags & (1 | 2)))
                    break;
            }
        }
    }
    wx = xp;
    wy = yp;
}

void YFrameWindow::drawMoveSizeFX(int x, int y, int w, int h, bool) {
    if ((movingWindow && opaqueMove) ||
        (sizingWindow && opaqueResize))
        return;

    int const bw((wsBorderX + wsBorderY) / 2);
    int const bo((wsBorderX + wsBorderY) / 4);

    XGCValues gcv;

    gcv.foreground = activeBorderBg.pixel();
    gcv.function = GXxor;
    gcv.graphics_exposures = False;
    gcv.line_width = bw;
    gcv.subwindow_mode = IncludeInferiors;

    Graphics g(*desktop, GCForeground | GCFunction | GCGraphicsExposures |
                         GCLineWidth | GCSubwindowMode, &gcv);
    g.drawRect(x + bo, y + bo, w - bw, h - bw);
}

int YFrameWindow::handleMoveKeys(const XKeyEvent &key, int &newX, int &newY) {
    KeySym k = keyCodeToKeySym(key.keycode);
    int m = KEY_MODMASK(key.state);
    int factor = 1;

    int mx, my, Mx, My;
    manager->getWorkArea(this, &mx, &my, &Mx, &My);

    if (m & ShiftMask)
        factor = 4;
    if (m & ControlMask)
        factor<<= 4;

    if (k == XK_Left || k == XK_KP_Left)
        newX -= factor;
    else if (k == XK_Right || k == XK_KP_Right)
        newX += factor;
    else if (k == XK_Up || k == XK_KP_Up)
        newY -= factor;
    else if (k == XK_Down || k == XK_KP_Down)
        newY += factor;
    else if (k == XK_Home || k == XK_KP_Home)
        newX = mx - borderX();
    else if (k == XK_End || k == XK_KP_End)
        newX = Mx - width() + borderX();
    else if (k == XK_Prior || k == XK_KP_Prior)
        newY = my - borderY();
    else if (k == XK_Next || k == XK_KP_Next)
        newY = My - height() + borderY();
    else if (k == XK_KP_Begin) {
        newX = (mx + Mx - (int)width()) / 2;
        newY = (my + My - (int)height()) / 2;
    } else if (k == XK_Return || k == XK_KP_Enter)
        return -1;
    else if (k ==  XK_Escape) {
        newX = origX;
        newY = origY;
        return -2;
    } else
        return 0;
    return 1;
}

/******************************************************************************/

int YFrameWindow::handleResizeKeys(const XKeyEvent &key,
                                   int &newX, int &newY, int &newWidth, int &newHeight,
                                   int incX, int incY)
{
    KeySym k = keyCodeToKeySym(key.keycode);
    int m = KEY_MODMASK(key.state);
    int factor = 1;

    if (m & ShiftMask)
        factor = 4;
    if (m & ControlMask)
        factor<<= 4;

    if (k == XK_Left || k == XK_KP_Left) {
        if (grabX == 0) {
            grabX = -1;
        }
        if (grabX == 1) {
            newWidth -= incX * factor;
        } else if (grabX == -1) {
            newWidth += incX * factor;
            newX -= incX * factor;
        }
    } else if (k == XK_Right || k == XK_KP_Right) {
        if (grabX == 0) {
            grabX = 1;
        }
        if (grabX == 1) {
            newWidth += incX * factor;
        } else if (grabX == -1) {
            newWidth -= incX * factor;
            newX += incX * factor;
        }
    } else if (k == XK_Up || k == XK_KP_Up) {
        if (grabY == 0) {
            grabY = -1;
        }
        if (grabY == 1) {
            newHeight -= incY * factor;
        } else if (grabY == -1) {
            newHeight += incY * factor;
            newY -= incY * factor;
        }
    } else if (k == XK_Down || k == XK_KP_Down) {
        if (grabY == 0) {
            grabY = 1;
        }
        if (grabY == 1) {
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

    //constrainMouseToWorkspace(mouseX, mouseY);

    newX = mouseX - buttonDownX;
    newY = mouseY - buttonDownY;

    constrainPositionByModifier(newX, newY, motion);

    newX += borderX();
    newY += borderY();
    int n = -2;

    int mx, my, Mx, My;
    manager->getWorkArea(this, &mx, &my, &Mx, &My);

    if (!(motion.state & ShiftMask)) {
        if (/*EdgeResistance >= 0 && %%% */ EdgeResistance < 10000) {
            if (newX + int(width()) + n * borderX() > Mx) {
                if (newX + int(width()) + n * borderX() < Mx + EdgeResistance)
                    newX = Mx - int(width()) - n * borderX();
                else if (motion.state & ShiftMask)
                    newX -= EdgeResistance;
            }
            if (newY + int(height()) + n * borderY() > My) {
                if (newY + int(height()) + n * borderY() < My + EdgeResistance)
                    newY = My - int(height()) - n * borderY();
                else if (motion.state & ShiftMask)
                    newY -= EdgeResistance;
            }
            if (newX < mx) {
                if (newX > mx - EdgeResistance)
                    newX = mx;
                else if (motion.state & ShiftMask)
                    newX += EdgeResistance;
            }
            if (newY < my) {
                if (newY > my - EdgeResistance)
                    newY = my;
                else if (motion.state & ShiftMask)
                    newY += EdgeResistance;
            }
        }
        if (EdgeResistance == 10000 || isMaximizedHoriz()) {
            if (newX + int(width()) + n * borderX() > Mx)
                newX = Mx - int(width()) - n * borderX();
            if (newX < mx)
                newX = mx;
        }
        if (EdgeResistance == 10000 || isMaximizedVert()) {
            if (newY + int(height()) + n * borderY() > My)
                newY = My - int(height()) - n * borderY();
            if (newY < my)
                newY = my;
        }
    }
    newX -= borderX();
    newY -= borderY();
}

void YFrameWindow::handleResizeMouse(const XMotionEvent &motion,
                                     int &newX, int &newY, int &newWidth, int &newHeight)
{
    if (buttonDownX == 0 && buttonDownY == 0)
        return;

    int mouseX = motion.x_root;
    int mouseY = motion.y_root;

//    constrainMouseToWorkspace(mouseX, mouseY);

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
                            ///getLayer(),
                            YFrameClient::csRound |
                            ((grabX != 0) ? YFrameClient::csKeepX : 0) |
                            ((grabY != 0) ? YFrameClient::csKeepY : 0));
    newWidth += 2 * borderX();
    newHeight += 2 * borderY() + titleY();

    if (grabX == -1)
        newX = x() + width() - newWidth;
    if (grabY == -1)
        newY = y() + height() - newHeight;
}

void YFrameWindow::outlineMove() {
    int xx(x()), yy(y());

    XGrabServer(xapp->display());

    for (;;) {
        XEvent xev;

        XWindowEvent(xapp->display(), handle(),
                     KeyPressMask | ExposureMask |
                     ButtonPressMask | ButtonReleaseMask |
                     PointerMotionMask, &xev);

        switch (xev.type) {
            case KeyPress: {

                int const ox(xx), oy(yy);
                int r;

                switch (r = handleMoveKeys(xev.xkey, xx, yy)) {
                    case 1:
                    case -2:
                        if (xx != ox || yy != oy) {
                            drawMoveSizeFX(ox, oy, width(), height());
                            statusMoveSize->setStatus(this, YRect(xx, yy, width(), height()));
                            drawMoveSizeFX(xx, yy, width(), height());
                        }

                        if (r == -2)
                            goto end;

                        break;

                    case 0:
                        break;

                    case -1:
                        goto end;
                }

                break;
            }

            case ButtonPress:
            case ButtonRelease:
                goto end;

            case MotionNotify: {
                int const ox(xx), oy(yy);

                handleMoveMouse(xev.xmotion, xx, yy);

                if (xx != ox || yy != oy) {
                    drawMoveSizeFX(ox, oy, width(), height());
                    statusMoveSize->setStatus(this, YRect(xx, yy, width(), height()));
                    drawMoveSizeFX(xx, yy, width(), height());
                }

                break;
            }
        }
    }

end:
    drawMoveSizeFX(xx, yy, width(), height());

    XSync(xapp->display(), False);
    moveWindow(xx, yy);
    XUngrabServer(xapp->display());
}

void YFrameWindow::outlineResize() {
    int xx(x()), yy(y()), ww(width()), hh(height());
    int incX(1), incY(1);

    if (client()->sizeHints()) {
        incX = client()->sizeHints()->width_inc;
        incY = client()->sizeHints()->height_inc;
    }

    XGrabServer(xapp->display());

    for (;;) {
        XEvent xev;

        XWindowEvent(xapp->display(), handle(),
                     KeyPressMask | ExposureMask |
                     ButtonPressMask | ButtonReleaseMask |
                     PointerMotionMask, &xev);

        switch (xev.type) {
            case KeyPress: {
                int const ox(xx), oy(yy), ow(ww), oh(hh);
                int r;

                switch (r = handleResizeKeys(xev.xkey, xx, yy, ww, hh,
                                             incX, incY)) {
                    case -2:
                    case 1:
                        if (ox != xx || oy != yy || ow != ww || oh != hh) {
                            drawMoveSizeFX(ox, oy, ow, oh);
                            statusMoveSize->setStatus(this, YRect(xx, yy, ww, hh));
                            drawMoveSizeFX(xx, yy, ww, hh);
                        }

                        if (r == -2)
                            goto end;

                        break;

                    case 0:
                        break;

                    case -1:
                        goto end;
                }

                break;
            }

            case ButtonPress:
            case ButtonRelease:
                goto end;

            case MotionNotify: {
                int const ox(xx), oy(yy), ow(ww), oh(hh);

                handleResizeMouse(xev.xmotion, xx, yy, ww,hh);

                if (ox != xx || oy != yy || ow != ww || oh != hh) {
                    drawMoveSizeFX(ox, oy, ow, oh);
                    statusMoveSize->setStatus(this, YRect(xx, yy, ww, hh));
                    drawMoveSizeFX(xx, yy, ww, hh);
                }

                break;
            }
        }
    }

end:
    drawMoveSizeFX(xx, yy, ww, hh);

    XSync(xapp->display(), False);
    setCurrentGeometryOuter(YRect(xx, yy, ww, hh));
    XUngrabServer(xapp->display());
}

void YFrameWindow::manualPlace() {
    int xx(x()), yy(y());

    grabX = 1;
    grabY = 1;

    origX = x();
    origY = y();
    origW = width();
    origH = height();
    buttonDownX = 0;
    buttonDownY = 0;

    if (!xapp->grabEvents(desktop,
                          YXApplication::movePointer,
                          ButtonPressMask |
                          ButtonReleaseMask |
                          PointerMotionMask))
        return;

    XGrabServer(xapp->display());
    statusMoveSize->begin(this);

    drawMoveSizeFX(xx, yy, width(), height());

    for (;;) {
        XEvent xev;

        XMaskEvent(xapp->display(),
                   KeyPressMask |
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                   &xev);

        switch (xev.type) {
            case KeyPress: {
                int const ox(xx), oy(yy);

                int r;

                switch (r = handleMoveKeys(xev.xkey, xx, yy)) {
                    case 1:
                    case -2:
                        if (xx != ox || yy != oy) {
                            drawMoveSizeFX(ox, oy, width(), height());
                            statusMoveSize->setStatus(this, YRect(xx, yy, width(), height()));
                            drawMoveSizeFX(xx, yy, width(), height());
                        }

                        if (r == -2)
                            goto end;

                        break;

                    case 0:
                        break;

                    case -1:
                        goto end;
                }

                break;
            }


            case ButtonPress:
            case ButtonRelease:
                goto end;

            case MotionNotify: {
                int const ox(xx), oy(yy);

                handleMoveMouse(xev.xmotion, xx, yy);
                if (xx != ox || yy != oy) {
                    drawMoveSizeFX(ox, oy, width(), height());
                    statusMoveSize->setStatus(this, YRect(xx, yy, width(), height()));
                    drawMoveSizeFX(xx, yy, width(), height());
                }

                break;
            }
        }
    }

end:
    drawMoveSizeFX(xx, yy, width(), height());

    statusMoveSize->end();
    moveWindow(xx, yy);
    xapp->releaseEvents();
    XUngrabServer(xapp->display());
}

bool YFrameWindow::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        if (movingWindow) {
            int newX = x();
            int newY = y();

            switch (handleMoveKeys(key, newX, newY)) {
            case -2:
                moveWindow(newX, newY);
                /* fall-through */
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
                newWidth -= 2 * borderXN();
                newHeight -= 2 * borderYN() + titleYN();
                client()->constrainSize(newWidth, newHeight,
                                        ///getLayer(),
                                        YFrameClient::csRound |
                                        (grabX ? YFrameClient::csKeepX : 0) |
                                        (grabY ? YFrameClient::csKeepY : 0));
                newWidth += 2 * borderXN();
                newHeight += 2 * borderYN() + titleYN();

                if (grabX == -1)
                    newX = x() + width() - newWidth;
                if (grabY == -1)
                    newY = y() + height() - newHeight;

                drawMoveSizeFX(x(), y(), width(), height());
                setCurrentGeometryOuter(YRect(newX, newY, newWidth, newHeight));
                drawMoveSizeFX(x(), y(), width(), height());

                statusMoveSize->setStatus(this);
                break;
            case -2:
                drawMoveSizeFX(x(), y(), width(), height());
                setCurrentGeometryOuter(YRect(newX, newY, newWidth, newHeight));
                drawMoveSizeFX(x(), y(), width(), height());
                /* fall-through */

            case -1:
                endMoveSize();
                break;
            }
        } else if (xapp->AltMask != 0) {
            KeySym k = keyCodeToKeySym(key.keycode);
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
            } else if (IS_WMKEY(k, vm, gKeyWinMaximizeHoriz)) {
                wmMaximizeHorz();
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
            } else if (IS_WMKEY(k, vm, gKeyWinFullscreen)) {
                if (canFullscreen()) wmToggleFullscreen();
            } else if (IS_WMKEY(k, vm, gKeyWinMenu)) {
                popupSystemMenu(this);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeN)) {
                if (canMove()) wmArrange(waTop, waCenter);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeNE)) {
                if (canMove()) wmArrange(waTop, waRight);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeE)) {
                if (canMove()) wmArrange(waCenter, waRight);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeSE)) {
                if (canMove()) wmArrange(waBottom, waRight);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeS)) {
                if (canMove()) wmArrange(waBottom, waCenter);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeSW)) {
                if (canMove()) wmArrange(waBottom, waLeft);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeW)) {
                if (canMove()) wmArrange(waCenter, waLeft);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeNW)) {
                if (canMove()) wmArrange(waTop, waLeft);
            } else if (IS_WMKEY(k, vm, gKeyWinArrangeC)) {
                if (canMove()) wmArrange(waCenter, waCenter);
            } else if (IS_WMKEY(k, vm, gKeyWinSnapMoveN)) {
                if (canMove()) wmSnapMove(waTop, waCenter);
            } else if (IS_WMKEY(k, vm, gKeyWinSnapMoveNE)) {
                if (canMove()) wmSnapMove(waTop, waRight);
            } else if (IS_WMKEY(k, vm, gKeyWinSnapMoveE)) {
                if (canMove()) wmSnapMove(waCenter, waRight);
            } else if (IS_WMKEY(k, vm, gKeyWinSnapMoveSE)) {
                if (canMove()) wmSnapMove(waBottom, waRight);
            } else if (IS_WMKEY(k, vm, gKeyWinSnapMoveS)) {
                if (canMove()) wmSnapMove(waBottom, waCenter);
            } else if (IS_WMKEY(k, vm, gKeyWinSnapMoveSW)) {
                if (canMove()) wmSnapMove(waBottom, waLeft);
            } else if (IS_WMKEY(k, vm, gKeyWinSnapMoveW)) {
                if (canMove()) wmSnapMove(waCenter, waLeft);
            } else if (IS_WMKEY(k, vm, gKeyWinSnapMoveNW)) {
                if (canMove()) wmSnapMove(waTop, waLeft);
            } else if (IS_WMKEY(k, vm, gKeyWinSmartPlace)) {
                if (canMove()) {
                    int newX = x();
                    int newY = y();
                    if (manager->getSmartPlace(true, this, newX, newY, width(), height(), getScreen())) {
                        setCurrentPositionOuter(newX, newY);
                    }
                }
            } else if (isIconic() || isRollup()) {
                if (k == XK_Return || k == XK_KP_Enter) {
                    wmRestore();
                } else if ((k == XK_Menu) || (k == XK_F10 && m == ShiftMask)) {
                    popupSystemMenu(this);
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
    x -= borderX();
    y -= borderY();

    if (snapMove && !(mask & (ControlMask | ShiftMask))) {
        snapTo(x, y);
    }
}

void YFrameWindow::constrainMouseToWorkspace(int &x, int &y) {
    int mx, my, Mx, My;
    manager->getWorkArea(this, &mx, &my, &Mx, &My);

    x = clamp(x, mx, Mx - 1);
    y = clamp(y, my, My - 1);
}

bool YFrameWindow::canSize(bool horiz, bool vert) {
    if (isRollup())
        return false;
    if (isFullscreen())
        return false;
    if (!(frameFunctions() & ffResize))
        return false;
    if (!sizeMaximized) {
        if ((!vert || isMaximizedVert()) &&
            (!horiz || isMaximizedHoriz()))
            return false;
    }
    return true;
}

bool YFrameWindow::canMove() const {
    return hasbit(frameFunctions(), ffMove);
}

void YFrameWindow::startMoveSize(int x, int y,
                                 int direction)
{
    int sx[] = { -1, 0, 1, 1, 1, 0, -1, -1, };
    int sy[] = { -1, -1, -1, 0, 1, 1, 1, 0, };

    if (inrange(direction, 0, int ACOUNT(sx) - 1)) {
        MSG(("move size %d %d direction %d", x, y, direction));
        startMoveSize(false, true, sx[direction], sy[direction], x, y);
    }
    else if (direction == _NET_WM_MOVERESIZE_MOVE) {
        MSG(("move size %d %d move by mouse", x, y));
        startMoveSize(true, true, 0, 0, x - this->x(), y - this->y());
    }
    else if (direction == _NET_WM_MOVERESIZE_SIZE_KEYBOARD) {
        MSG(("move size %d %d size by keyboard", x, y));
        startMoveSize(false, false, 0, 0, x - this->x(), y - this->y());
    }
    else if (direction == _NET_WM_MOVERESIZE_MOVE_KEYBOARD) {
        MSG(("move size %d %d move by keyboard", x, y));
        startMoveSize(true, false, 0, 0, x - this->x(), y - this->y());
    }
    else if (direction == _NET_WM_MOVERESIZE_CANCEL) {
    } else
        warn(_("Unknown direction in move/resize request: %d"), direction);
}

void YFrameWindow::startMoveSize(bool doMove, bool byMouse,
                                 int sideX, int sideY,
                                 int mouseXroot, int mouseYroot) {
    Cursor grabPointer = None;

    grabX = sideX;
    grabY = sideY;
    origX = x();
    origY = y();
    origW = width();
    origH = height();
    buttonDownX = 0;
    buttonDownY = 0;

    manager->setWorkAreaMoveWindows(true);
    if (doMove && grabX == 0 && grabY == 0) {
        buttonDownX = mouseXroot;
        buttonDownY = mouseYroot;

        wmapp->signalGuiEvent(geWindowMoved);
        grabPointer = YXApplication::movePointer;
    } else if (!doMove) {
        wmapp->signalGuiEvent(geWindowSized);

        if (grabY == -1) {
            if (grabX == -1)
                grabPointer = YWMApp::sizeTopLeftPointer;
            else if (grabX == 1)
                grabPointer = YWMApp::sizeTopRightPointer;
            else
                grabPointer = YWMApp::sizeTopPointer;
        } else if (grabY == 1) {
            if (grabX == -1)
                grabPointer = YWMApp::sizeBottomLeftPointer;
            else if (grabX == 1)
                grabPointer = YWMApp::sizeBottomRightPointer;
            else
                grabPointer = YWMApp::sizeBottomPointer;
        } else {
            if (grabX == -1)
                grabPointer = YWMApp::sizeLeftPointer;
            else if (grabX == 1)
                grabPointer = YWMApp::sizeRightPointer;
            else
                grabPointer = YXApplication::leftPointer;

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

    if (!xapp->grabEvents(this,
                          grabPointer,
                          ButtonPressMask |
                          ButtonReleaseMask |
                          PointerMotionMask))
    {
        movingWindow = false;
        sizingWindow = false;
        return ;
    }

    movingWindow = doMove;
    sizingWindow = !doMove;

    statusMoveSize->begin(this);

    drawMoveSizeFX(x(), y(), width(), height());

    if (doMove && !opaqueMove) {
        outlineMove();
        endMoveSize();
    } else if (!doMove && !opaqueResize) {
        outlineResize();
        endMoveSize();
    }
}

void YFrameWindow::endMoveSize() {
    xapp->releaseEvents();
    statusMoveSize->end();

    if ((movingWindow && opaqueMove) ||
        (sizingWindow && opaqueResize))
        drawMoveSizeFX(x(), y(), width(), height());

    movingWindow = false;
    sizingWindow = false;

    manager->setWorkAreaMoveWindows(false);

    if (taskBar && taskBar->workspacesPane()) {
        taskBar->workspacesPane()->repaint();
    }
}

void YFrameWindow::handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion) {
    if ((down.button == 3) && canMove()) {
        startMoveSize(true, true,
                      0, 0,
                      down.x, down.y);
        handleDrag(down, motion);
    } else if ((down.button == 1) && canSize()) {
        grabX = 0;
        grabY = 0;

        if (down.x < int(borderX())) grabX = -1;
        else if ((int) width() - down.x <= borderX()) grabX = 1;

        if (down.y < int(borderY())) grabY = -1;
        else if ((int) height() - down.y <= borderY()) grabY = 1;

        if (grabY != 0 && grabX == 0) {
            if (down.x < int(wsCornerX)) grabX = -1;
            else if ((int)width() - down.x <= (int) wsCornerX) grabX = 1;
        }

        if (grabX != 0 && grabY == 0) {
            if (down.y < int(wsCornerY)) grabY = -1;
            else if ((int)height() - down.y <= (int) wsCornerY) grabY = 1;
        }

        if (grabX != 0 || grabY != 0) {
            startMoveSize(false, true,
                          grabX, grabY,
                          down.x_root, down.y_root);

        }
    }
}

void YFrameWindow::moveWindow(int newX, int newY) {
/// TODO #warning "reevaluate if this is legacy"
#if 0
    if (!doNotCover()) {
        int mx, my, Mx, My;
        manager->getWorkArea(this, &mx, &my, &Mx, &My);

        newX = clamp(newX, (int)(mx + borderX() - width()),
                           (int)(Mx - borderX()));
        newY = clamp(newY, (int)(my + borderY() - height()),
                           (int)(My - borderY()));
    }
#endif

    if (opaqueMove)
        drawMoveSizeFX(x(), y(), width(), height());

    setCurrentPositionOuter(newX, newY);

    if (opaqueMove)
        drawMoveSizeFX(x(), y(), width(), height());

    statusMoveSize->setStatus(this);
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

        drawMoveSizeFX(x(), y(), width(), height());
        setCurrentGeometryOuter(YRect(newX, newY, newWidth, newHeight));
        drawMoveSizeFX(x(), y(), width(), height());

        statusMoveSize->setStatus(this);
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


// vim: set sw=4 ts=4 et:
