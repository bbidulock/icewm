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

void YFrameWindow::drawMoveSizeFX(int x, int y, int w, int h) {
}

void YFrameWindow::drawOutline(int x, int y, int w, int h) {
    int const bw((wsBorderX + wsBorderY) / 2);
    int const bo((wsBorderX + wsBorderY) / 4);
    static Graphics * outline(NULL);

    if (outline == NULL) {
        XGCValues gcv;

        gcv.foreground = YColor(clrActiveBorder).pixel();
        gcv.function = GXxor;
        gcv.graphics_exposures = False;
        gcv.line_width = bw;
        gcv.subwindow_mode = IncludeInferiors;

        outline = new Graphics(desktop, GCForeground | GCFunction |
				        GCGraphicsExposures | GCLineWidth |
				        GCSubwindowMode, &gcv);
    }

    const int xa(x + bo), xe(x + bo + w - bw);
    const int ya(y + bo), ye(y + bo + h - bw);
    const int yi(y + bw + titleY());

    outline->drawRect(xa, ya, w - bw, h - bw);

#ifdef CONFIG_MOVESIZE_FX
    enum MoveSizeFX {
	fxOuterGapLines =	(1 << 0),
	fxCentralGapLines =	(1 << 1),
	fxGaugeLines =		(1 << 2),
	fxTitleBar =		(1 << 3),
	fxClientGrid =		(1 << 4),
	fxClientCrossA =	(1 << 5),
	fxClientCrossB =	(1 << 6),
	fxClientTLPos =		(1 << 7),
	fxClientTRPos =		(1 << 8),
	fxClientBLPos =		(1 << 9),
	fxClientBRPos =		(1 << 10),
	fxClientTSize =		(1 << 11),
	fxClientLSize =		(1 << 12),
	fxClientRSize =		(1 << 13),
	fxClientBSize =		(1 << 14)
    };

    moveSizeFX = (1 << 15) - 1;

    static YFont * hFont(NULL), * lFont(NULL), * rFont(NULL);
    
    if (!hFont) {
	hFont = YFont::getFont(fxFontName);
	
char size[6], *p;
p = YFont::getNameElement(fxFontName, 8, size, sizeof(size));
msg("ptSize: %s %d", size, hFont->height());
char fn[strlen(fxFontName) + strlen(size) + 9];
char * s(fn);
memcpy(s, fxFontName, p - fxFontName); s+= (p - fxFontName);
memcpy(s, "[ 0 ", 4); s+= 4;
strcpy(s, size); s+= strlen(size);
memcpy(s, " ~", 2); s+= 2;
strcpy(s, size); s+= strlen(size);
memcpy(s, " 0]", 3); s+= 3;
strcpy(s, p + strlen(size));

msg("%s", fn);

YFont::getNameElement(fxFontName, 7, size, sizeof(size));
msg("pxSize: %s", size);

	lFont = YFont::getFont("-adobe-helvetica-bold-r-normal--[0 12 ~12 0]-*-*-*-p-*-iso8859-1");
	rFont = YFont::getFont("-adobe-helvetica-bold-r-normal--[0 ~12 12 0]-*-*-*-p-*-iso8859-1");
    }	

    if (moveSizeFX & fxOuterGapLines) {
	outline->drawLine(0, ya, x - 2, ya);
	outline->drawLine(0, ye, x - 2, ye);
	outline->drawLine(xa, 0, xa, y - 2);
	outline->drawLine(xe, 0, xe, y - 2);
	outline->drawLine(xa, y + h + 3, xa, desktop->height() - 1);
	outline->drawLine(xe, y + h + 3, xe, desktop->height() - 1);
	outline->drawLine(x + w + 2, ya, desktop->width() - 1, ya);
	outline->drawLine(x + w + 2, ye, desktop->width() - 1, ye);
    }

    if (moveSizeFX & fxCentralGapLines) {
	outline->drawLine(0, y + h/2, x - hFont->height() - 2, y + h/2);
	outline->drawLine(x + w + hFont->height() + 2, y + h/2,
			  desktop->width() - 1, y + h/2);
	outline->drawLine(x + w/2, 0, x + w/2, y - hFont->height() - 2);
	outline->drawLine(x + w/2, y + h + hFont->height() + 2,
			  x + w/2, desktop->height() - 1);
    }

    if (titleY() && moveSizeFX & fxTitleBar)
	outline->drawLine(x + bw, y + bo + titleY(),
			  x + w - bw, y + bo + titleY());

    if (moveSizeFX & fxClientCrossB) {
	outline->drawLine(x + w/2, yi + 2, x + w/2, y + h - bw - 2);
	outline->drawLine(x + bw + 2, y + h/2, x + w - bw - 2, y + h/2);
    }

    XGCValues gcv;
    gcv.line_width = 1;
    XChangeGC(app->display(), outline->handle(), GCLineWidth, &gcv);

/* position/size bloat */
    char str[6];

    outline->setFont(hFont);
    
    sprintf(str, "%d", x);
    outline->drawChars(str, 0, strlen(str),
    		       x + bw + bw, y - hFont->descent());
    outline->drawChars(str, 0, strlen(str),
    		       x + bw + bw, y + h + hFont->ascent());

    sprintf(str, "%d", normalWidth);
    outline->drawChars(str, 0, strlen(str),
    		       x + (w - hFont->textWidth(str)) / 2, 
		       y - hFont->descent());
    outline->drawChars(str, 0, strlen(str),
    		       x + (w - hFont->textWidth(str)) / 2, 
		       y + h + hFont->ascent());

    sprintf(str, "%d", x + w);
    outline->drawChars(str, 0, strlen(str),
    		       x + w - bw - bw - hFont->textWidth(str), 
		       y - hFont->descent());
    outline->drawChars(str, 0, strlen(str),
    		       x + w - bw - bw - hFont->textWidth(str), 
		       y + h + hFont->ascent());

    int yy(0);
    sprintf(str, "%d", y);
    for (char * s(str); *s; yy+= hFont->textWidth(s, 1), ++s) {
	outline->setFont(lFont);
	outline->drawChars(s, 0, 1,
			   x - hFont->descent(), 
			   y + bw + bw + hFont->textWidth(str) - yy);
	outline->setFont(rFont);
	outline->drawChars(s, 0, 1,
			   x + w + hFont->descent(), y + bw + bw + yy);
    }

    yy = 0;
    sprintf(str, "%d", normalHeight);
    for (char * s(str); *s; yy+= hFont->textWidth(s, 1), ++s) {
	outline->setFont(lFont);
	outline->drawChars(s, 0, 1,
			   x - hFont->descent(), 
			   y + (h + hFont->textWidth(str)) / 2 - yy);
	outline->setFont(rFont);
	outline->drawChars(s, 0, 1,
			   x + w + hFont->descent(),
			   y + (h - hFont->textWidth(str)) / 2 + yy);
    }

    yy = 0;
    sprintf(str, "%d", y + h);
    for (char * s(str); *s; yy+= hFont->textWidth(s, 1), ++s) {
	outline->setFont(lFont);
	outline->drawChars(s, 0, 1,
			   x - hFont->descent(), 
			   y + h - bw - bw - yy);
	outline->setFont(rFont);
	outline->drawChars(s, 0, 1,
			   x + w + hFont->descent(),
			   y + h - bw - bw - hFont->textWidth(str) + yy);
    }

    if (moveSizeFX & fxGaugeLines) {
	outline->drawLine(x - hFont->height(), y,
			  x - hFont->height(), y + h);
	outline->drawLine(x, y - hFont->height(),
			  x + w, y - hFont->height());
	outline->drawLine(x + w + hFont->height(), y,
			  x + w + hFont->height(), y + h);
	outline->drawLine(x, y + h + hFont->height(),
			  x + w, y + h + hFont->height());
    }

    if (moveSizeFX & fxClientGrid) {
	outline->drawLine(x + (w - bw - bw) * 1/3, yi,
			  x + (w - bw - bw) * 1/3, y + h - bw);
	outline->drawLine(x + (w - bw - bw) * 2/3, yi,
			  x + (w - bw - bw) * 2/3, y + h - bw);
	outline->drawLine(x + bw, yi + (h - bw - bw - titleY()) * 1/3,
			  x + w - bw, yi + (h - bw - bw - titleY()) * 1/3);
	outline->drawLine(x + bw, yi + (h - bw - bw - titleY()) * 2/3,
			  x + w - bw, yi + (h - bw - bw - titleY()) * 2/3);
    }

    if (moveSizeFX & fxClientCrossA) {
	outline->drawLine(x + bw, yi, x + w - bw, y + h - bw);
	outline->drawLine(x + w - bw, yi, x + bw, y + h - bw);
    }

    gcv.line_width = bw;
    XChangeGC(app->display(), outline->handle(), GCLineWidth, &gcv);
    
#endif    
}

int YFrameWindow::handleMoveKeys(const XKeyEvent &key, int &newX, int &newY) {
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    int m = KEY_MODMASK(key.state);
    int factor = 1;

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
        newX = manager->minX(this) - borderX();
    else if (k == XK_End || k == XK_KP_End)
        newX = manager->maxX(this) - width() + borderX();
    else if (k == XK_Prior || k == XK_KP_Prior)
        newY = manager->minY(this) - borderY();
    else if (k == XK_Next || k == XK_KP_Next)
        newY = manager->maxY(this) - height() + borderY();
    else if (k == XK_KP_Begin) {
	newX = (manager->minX(getLayer()) + 
		manager->maxX(getLayer()) - (int)width()) / 2;
	newY = (manager->minY(getLayer()) + 
		manager->maxY(getLayer()) - (int)height()) / 2;
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

int YFrameWindow::handleResizeKeys(const XKeyEvent &key,
                                   int &newX, int &newY, int &newWidth, int &newHeight,
                                   int incX, int incY)
{
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    int m = KEY_MODMASK(key.state);
    int factor = 1;

    if (m & ShiftMask)
        factor = 4;
    if (m & ControlMask)
        factor<<= 4;

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
            else if ((int)width() - down.x <= wsCornerX) grabX = 1;
        }

        if (grabX != 0 && grabY == 0) {
            if (down.y < int(wsCornerY)) grabY = -1;
            else if ((int)height() - down.y <= wsCornerY) grabY = 1;
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
	newX = clamp(newX, (int)(manager->minX(this) + borderX() - width()),
			   (int)(manager->maxX(this) - borderX()));
	newY = clamp(newY, (int)(manager->minY(this) + borderY() - height()),
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

