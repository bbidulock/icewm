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

#include <stdio.h>

void YFrameWindow::snapTo(int &wx, int &wy,
                          int rx1, int ry1, int rx2, int ry2,
                          int &flags) {
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

/******************************************************************************/

#ifdef CONFIG_MOVESIZE_FX
void YFrameWindow::drawMoveSizeFX(int x, int y, int w, int h, bool /*interior*/) {
    struct FXFrame { 
	FXFrame(int const l, int const c, int const r,
		int const t, int const m, int const b):
	    l(l), c(c), r(r), t(t), m(m), b(b) {}

	int const l, c, r, t, m, b;
    };

    struct FXRect { 
	FXRect(int const l, int const r, int const t, int const b): 
	    l(l), r(r), t(t), b(b) {}

	int const l, r, t, b;
    };

    static YFont * font(NULL);
    static Graphics * gc(NULL);

    if (font == NULL)
	font = YFont::getFont(moveSizeFontName, false);

    if (gc == NULL) {
        XGCValues gcv;

        gcv.function = GXxor;
        gcv.line_width = 0;
        gcv.subwindow_mode = IncludeInferiors;
        gcv.graphics_exposures = False;

        gc = new Graphics(*desktop, GCFunction | GCGraphicsExposures | 
				    GCLineWidth | GCSubwindowMode, &gcv);
        gc->setFont(font);
        gc->setColor(new YColor(clrActiveBorder));
    }

    const int dBase(font->height() + 4);
    const int titlebar(moveSizeInterior & 2 ? titleY() : 0);

    FXFrame const frame(x, x + w/2, x + w - 1, y, y + h/2, y + h - 1);
    FXRect const client(x + borderX(), x + w - borderX() - 1, 
    			y + borderY() + titlebar, y + h - borderY() - 1);
    FXRect const dimBase(frame.l - dBase, frame.r + dBase,
    			 frame.t - dBase, frame.b + dBase);
    FXRect const dimLine(moveSizeGaugeLines & 1 ? dimBase.l - 4 : frame.l,
    			 moveSizeGaugeLines & 2 ? dimBase.r + 4 : frame.r,
    			 moveSizeGaugeLines & 4 ? dimBase.t - 4 : frame.t,
			 moveSizeGaugeLines & 8 ? dimBase.b + 4 : frame.b);
    FXRect const outerLabel(dimBase.l + font->descent() + 2,
			    dimBase.r - font->ascent() - 2,
			    dimBase.t + font->ascent() + 2,
			    dimBase.b - font->descent() - 2);
    FXRect const desktop(0, desktop->width() - 1, 0, desktop->height() - 1);

    int const cw(client.r - client.l + 1);
    int const ch(client.b - client.t + 1);

/*** FX: Frame Border *********************************************************/

    if (!((movingWindow && opaqueMove) ||
	  (sizingWindow && opaqueResize)))
	if (moveSizeInterior & 1) {
	    XRectangle border[] = {
		{ frame.l, frame.t, w, h }, 
		{ client.l, client.t, cw, ch }
	    };

	    gc->drawRects(border, frameDecors() & fdBorder ? 2 : 1);
	} else {
            XRectangle border[] = {
		{ frame.l, frame.t, frame.r - frame.l + 1, client.t - frame.t + 1 },
		{ frame.l, client.t + 1, client.l - frame.l + 1, client.b - client.t - 1 },
		{ client.r, client.t + 1, frame.r - client.r + 1, client.b - client.t - 1 },
		{ frame.l, client.b, frame.r - frame.l + 1, frame.b - client.b + 1 }
	     };

	     gc->fillRects(border, 4);
	}
    
/*** FX: Dimension Lines ******************************************************/
    
    if (moveSizeDimLines & 00001)
	gc->drawLine(dimLine.r, frame.t, desktop.r, frame.t);
    if (moveSizeDimLines & 00002)
	gc->drawLine(dimLine.r, frame.m, desktop.r, frame.m);
    if (moveSizeDimLines & 00004)
	gc->drawLine(dimLine.r, frame.b, desktop.r, frame.b);

    if (moveSizeDimLines & 00010)
	gc->drawLine(desktop.l, frame.t, dimLine.l, frame.t);
    if (moveSizeDimLines & 00020)
	gc->drawLine(desktop.l, frame.m, dimLine.l, frame.m);
    if (moveSizeDimLines & 00040)
	gc->drawLine(desktop.l, frame.b, dimLine.l, frame.b);

    if (moveSizeDimLines & 00100)
	gc->drawLine(frame.l, desktop.t, frame.l, dimLine.t);
    if (moveSizeDimLines & 00200)
	gc->drawLine(frame.c, desktop.t, frame.c, dimLine.t);
    if (moveSizeDimLines & 00400)
	gc->drawLine(frame.r, desktop.t, frame.r, dimLine.t);
	
    if (moveSizeDimLines & 01000)
	gc->drawLine(frame.l, dimLine.b, frame.l, desktop.b);
    if (moveSizeDimLines & 02000)
	gc->drawLine(frame.c, dimLine.b, frame.c, desktop.b);
    if (moveSizeDimLines & 04000)
	gc->drawLine(frame.r, dimLine.b, frame.r, desktop.b);

/*** FX: Dimension Base Lines *************************************************/
    
    if (moveSizeGaugeLines & 1)
	gc->drawLine(frame.l, dimBase.t, frame.r, dimBase.t);
    if (moveSizeGaugeLines & 2)
	gc->drawLine(dimBase.l, frame.t, dimBase.l, frame.b);
    if (moveSizeGaugeLines & 4)
	gc->drawLine(dimBase.r, frame.t, dimBase.r, frame.b);
    if (moveSizeGaugeLines & 8)
	gc->drawLine(frame.l, dimBase.b, frame.r, dimBase.b);
	
/*** FX: Dimension Labels *****************************************************/
    
    if (moveSizeDimLabels & 01001) {
	char const * label(itoa(x));
	int const pos(frame.l);
	
	if (moveSizeDimLabels & 00001)
	    gc->drawString(pos, outerLabel.t, label);
	if (moveSizeDimLabels & 01000)
	    gc->drawString(pos, outerLabel.b, label);
    }

    if (moveSizeDimLabels & 02002) {
	char const * label(itoa(w));
	int const pos(frame.c - font->textWidth(label)/2);
	
	if (moveSizeDimLabels & 00002)
	    gc->drawString(pos, outerLabel.t, label);
	if (moveSizeDimLabels & 02000)
	    gc->drawString(pos, outerLabel.b, label);
    }

    if (moveSizeDimLabels & 04004) {
	char const * label(itoa(x + w));
	int const pos(frame.r - font->textWidth(label));
	
	if (moveSizeDimLabels & 00004)
	    gc->drawString(pos, outerLabel.t, label);
	if (moveSizeDimLabels & 04000)
	    gc->drawString(pos, outerLabel.b, label);
    }

    if (moveSizeDimLabels & 00110) {
	char const * label(itoa(y));
	int const pos(frame.t);
	
	if (moveSizeDimLabels & 00010)
	    gc->drawString270(outerLabel.l, pos, label);
	if (moveSizeDimLabels & 00100)
	    gc->drawString90(outerLabel.r, pos, label);
    }

    if (moveSizeDimLabels & 00220) {
	char const * label(itoa(h));
	int const pos(frame.m - font->textWidth(label)/2);
	
	if (moveSizeDimLabels & 00020)
	    gc->drawString270(outerLabel.l, pos, label);
	if (moveSizeDimLabels & 00200)
	    gc->drawString90(outerLabel.r, pos, label);
    }

    if (moveSizeDimLabels & 00440) {
	char const * label(itoa(x + h));
	int const pos(frame.b - font->textWidth(label));
	
	if (moveSizeDimLabels & 00040)
	    gc->drawString270(outerLabel.l, pos, label);
	if (moveSizeDimLabels & 00400)
	    gc->drawString90(outerLabel.r, pos, label);
    }
    
/*** FX: Geometry Labels ******************************************************/
    
    if (moveSizeGeomLabels & 0x7f) {
	static char label[50];
	sprintf(label, "%dx%d+%d+%d", w, h, x, y);

	int const tw(font->textWidth(label));
	int const th(font->height());

	FXFrame const innerLabel
	    (client.l + 2, frame.c - tw/2, client.r - tw - 2,
	     client.t + font->ascent() + 2,
	     frame.m + th/2 - font->descent(),
	     client.b - font->descent() - 2);

	if (moveSizeGeomLabels & 0x01)
	    gc->drawString(innerLabel.l, innerLabel.t, label);
	if (moveSizeGeomLabels & 0x02)
	    gc->drawString(innerLabel.c, innerLabel.t, label);
	if (moveSizeGeomLabels & 0x04)
	    gc->drawString(innerLabel.r, innerLabel.t, label);
	if (moveSizeGeomLabels & 0x08)
	    gc->drawString(innerLabel.c, innerLabel.m, label);
	if (moveSizeGeomLabels & 0x10)
	    gc->drawString(innerLabel.l, innerLabel.b, label);
	if (moveSizeGeomLabels & 0x20)
	    gc->drawString(innerLabel.c, innerLabel.b, label);
	if (moveSizeGeomLabels & 0x40)
	    gc->drawString(innerLabel.r, innerLabel.b, label);
    }

/*** FX: Grids ****************************************************************/

    if (moveSizeInterior & 4) {
        XSegment grid[] = { 
	    { client.l + cw * 1/3, client.t + 1,
	      client.l + cw * 1/3, client.b },
	    { client.l + cw * 2/3, client.t + 1,
	      client.l + cw * 2/3, client.b },
	    { client.l + 1, client.t + ch * 1/3,
	      client.r, client.t + ch * 1/3 },
	    { client.l + 1, client.t + ch * 2/3,
	      client.r, client.t + ch * 2/3 }
	};
			  
	gc->drawSegments(grid, 4);
    }

    if (moveSizeInterior & 8) {
        XSegment grid[] = { 
	    { client.l + cw / 2, client.t + 1,
	      client.l + cw / 2, client.b },
	    { client.l + 1, client.t + ch / 2,
	      client.r, client.t + ch / 2 }
	};

	gc->drawSegments(grid, 2);
    }

    if (moveSizeInterior & 8) {
        XSegment grid[] = { 
	    { client.l + 1, client.t + 1, client.r, client.b },
	    { client.r, client.t + 1, client.l + 1, client.b }
	};

	gc->drawSegments(grid, 2);
    }
}
#else
void YFrameWindow::drawMoveSizeFX(int x, int y, int w, int h, bool) {
    if ((movingWindow && opaqueMove) ||
	(sizingWindow && opaqueResize))
	return;

    int const bw((wsBorderX + wsBorderY) / 2);
    int const bo((wsBorderX + wsBorderY) / 4);
    static Graphics * gc(NULL);

    if (gc == NULL) {
        XGCValues gcv;

        gcv.foreground = YColor(clrActiveBorder).pixel();
        gcv.function = GXxor;
        gcv.graphics_exposures = False;
        gcv.line_width = bw;
        gcv.subwindow_mode = IncludeInferiors;

        gc = new Graphics(*desktop, GCForeground | GCFunction |
				    GCGraphicsExposures | GCLineWidth |
				    GCSubwindowMode, &gcv);
    }

    gc->drawRect(x + bo, y + bo, w - bw, h - bw);
}
#endif

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

/******************************************************************************/

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
    int xx(x()), yy(y());
    unsigned modifiers(0);

    XGrabServer(app->display());
    XSync(app->display(), False);

    for(;;) {
        XEvent xev;

        XWindowEvent(app->display(), handle(),
                     KeyPressMask | ExposureMask | 
		     ButtonPressMask | ButtonReleaseMask | 
		     PointerMotionMask, &xev);

        switch (xev.type) {
	    case KeyPress: {
		modifiers = xev.xkey.state;

                int const ox(xx), oy(yy);
		int r;

                switch (r = handleMoveKeys(xev.xkey, xx, yy)) {
		    case 1:
		    case -2:
			if (xx != ox || yy != oy) {
			    drawMoveSizeFX(ox, oy, width(), height());
#ifndef LITE
			    statusMoveSize->setStatus(this, xx, yy, width(), height());
#endif
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
		modifiers = xev.xbutton.state;
		goto end;

	    case MotionNotify: {
                int const ox(xx), oy(yy);

                handleMoveMouse(xev.xmotion, xx, yy);

                if (xx != ox || yy != oy) {
                    drawMoveSizeFX(ox, oy, width(), height());
#ifndef LITE
                    statusMoveSize->setStatus(this, xx, yy, width(), height());
#endif
                    drawMoveSizeFX(xx, yy, width(), height());
                }

		break;
            }
        }
    }

end:
    drawMoveSizeFX(xx, yy, width(), height());

    XSync(app->display(), False);
    moveWindow(xx, yy);
    XUngrabServer(app->display());
}

void YFrameWindow::outlineResize() {
    int xx(x()), yy(y()), ww(width()), hh(height());
    int incX(1), incY(1);

    if (client()->sizeHints()) {
        incX = client()->sizeHints()->width_inc;
        incY = client()->sizeHints()->height_inc;
    }

    XGrabServer(app->display());
    XSync(app->display(), False);

    for(;;) {
        XEvent xev;

        XWindowEvent(app->display(), handle(),
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
#ifndef LITE
			    statusMoveSize->setStatus(this, xx, yy, ww, hh);
#endif
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
#ifndef LITE
                    statusMoveSize->setStatus(this, xx, yy, ww, hh);
#endif
                    drawMoveSizeFX(xx, yy, ww, hh);
                }

		break;
            }
        }
    }

end:
    drawMoveSizeFX(xx, yy, ww, hh);

    XSync(app->display(), False);
    setGeometry(xx, yy, ww, hh);
    XUngrabServer(app->display());
}

void YFrameWindow::manualPlace() {
    int xx(x()), yy(y());

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
        return;

    XGrabServer(app->display());
#ifndef LITE
    statusMoveSize->begin(this);
#endif

    drawMoveSizeFX(xx, yy, width(), height());

    for(;;) {
        XEvent xev;

        XMaskEvent(app->display(),
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
#ifndef LITE
                            statusMoveSize->setStatus(this, xx, yy, width(), height());
#endif
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
#ifndef LITE
                    statusMoveSize->setStatus(this, xx, yy, width(), height());
#endif
                    drawMoveSizeFX(xx, yy, width(), height());
                }

		break;
            }
        }
    }

end:
    drawMoveSizeFX(xx, yy, width(), height());

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

		drawMoveSizeFX(x(), y(), width(), height());
                setGeometry(newX, newY, newWidth, newHeight);
		drawMoveSizeFX(x(), y(), width(), height());

#ifndef LITE
                statusMoveSize->setStatus(this);
#endif
                break;
            case -2:
		drawMoveSizeFX(x(), y(), width(), height());
                setGeometry(newX, newY, newWidth, newHeight);
		drawMoveSizeFX(x(), y(), width(), height());
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
    if (!(frameFunctions() & ffResize))
        return false;
#if 0
#ifndef NO_MWM_HINTS
    if (!(client()->mwmFunctions() & MWM_FUNC_RESIZE))
        return false;
#endif
#endif
    if (!sizeMaximized) {
        if ((!vert || isMaximizedVert()) &&
            (!horiz || isMaximizedHoriz()))
            return false;
    }
    return true;
}

bool YFrameWindow::canMove() {
    if (!(frameFunctions() & ffMove))
        return false;
#if 0
#ifndef NO_MWM_HINTS
    if (!(client()->mwmFunctions() & MWM_FUNC_MOVE))
        return false;
#endif
#endif
    return true;
}

void YFrameWindow::startMoveSize(int doMove, int byMouse,
                                 int sideX, int sideY,
                                 int mouseXroot, int mouseYroot) {
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
    app->releaseEvents();
#ifndef LITE
    statusMoveSize->end();
#endif

    if ((movingWindow && opaqueMove) ||
	(sizingWindow && opaqueResize))
	drawMoveSizeFX(x(), y(), width(), height());

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

    if (opaqueMove)
	drawMoveSizeFX(x(), y(), width(), height());

    setPosition(newX, newY);

    if (opaqueMove)
	drawMoveSizeFX(x(), y(), width(), height());

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

	drawMoveSizeFX(x(), y(), width(), height());
        setGeometry(newX, newY, newWidth, newHeight);
	drawMoveSizeFX(x(), y(), width(), height());

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

