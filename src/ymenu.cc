/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#include "ykey.h"
#include "ymenu.h"
#include "yaction.h"
#include "ymenuitem.h"

#include "yapp.h"
#include "prefs.h"

#include <string.h>

YColor *menuBg = 0;
YColor *menuItemFg = 0;
YColor *activeMenuItemBg = 0;
YColor *activeMenuItemFg = 0;
YColor *disabledMenuItemFg = 0;
YColor *disabledMenuItemSt = 0;

YFont *menuFont = 0;

int YMenu::fAutoScrollDeltaX = 0;
int YMenu::fAutoScrollDeltaY = 0;
int YMenu::fAutoScrollMouseX = -1;
int YMenu::fAutoScrollMouseY = -1;

void YMenu::setActionListener(YActionListener *actionListener) {
    fActionListener = actionListener;
}

void YMenu::finishPopup(YMenuItem *item, YAction *action, unsigned int modifiers) {
    YPopupWindow::finishPopup();

    if (item) item->actionPerformed(fActionListener, action, modifiers);
}

YTimer *YMenu::fMenuTimer = 0;
int YMenu::fTimerX = 0, YMenu::fTimerY = 0, YMenu::fTimerItem = 0,
    YMenu::fTimerSubmenu = 0;
bool YMenu::fTimerSlow = false;

YMenu::YMenu(YWindow *parent): YPopupWindow(parent) {
    if (menuFont == 0)
        menuFont = YFont::getFont(menuFontName);
    if (menuBg == 0)
        menuBg = new YColor(clrNormalMenu);
    if (menuItemFg == 0)
        menuItemFg = new YColor(clrNormalMenuItemText);
    if (activeMenuItemBg == 0)
        activeMenuItemBg = new YColor(clrActiveMenuItem);
    if (activeMenuItemFg == 0)
        activeMenuItemFg = new YColor(clrActiveMenuItemText);
    if (disabledMenuItemFg == 0)
        disabledMenuItemFg = new YColor(clrDisabledMenuItemText);
    if (disabledMenuItemSt == 0)
        disabledMenuItemSt = *clrDisabledMenuItemShadow
			   ? new YColor(clrDisabledMenuItemShadow)
			   : menuBg->brighter();

    fItems = 0;
    fItemCount = 0;
    paintedItem = selectedItem = -1;
    fPopup = 0;
    fActionListener = 0;
    fPopupActive = 0;
    fShared = false;
    activatedX = -1;
    activatedY = -1;
}

YMenu::~YMenu() {
    if (fMenuTimer && fMenuTimer->getTimerListener() == this) {
        fMenuTimer->setTimerListener(0);
        fMenuTimer->stopTimer();
    }
    if (fPopup) {
        fPopup->popdown();
        fPopup = 0;
    }

    for (int i = 0; i < fItemCount; i++)
        delete fItems[i];
    FREE(fItems); fItems = 0;
}

void YMenu::activatePopup() {
    if (popupFlags() & pfButtonDown)
        selectedItem = -1;
    else
        focusItem(findActiveItem(itemCount() - 1, 1), 0, 0);
}

void YMenu::deactivatePopup() {
    if (fPopup) {
        fPopup->popdown();
        fPopup = 0;
    }
}

void YMenu::donePopup(YPopupWindow *popup) {
    PRECONDITION(popup != 0);
    PRECONDITION(fPopup != 0);
    if (fPopup) {
        fPopup->popdown();
        fPopup = 0;
        if (selectedItem != -1)
            if (item(selectedItem)->submenu() == popup)
                paintItems();
    }
}

bool YMenu::isCondCascade(int selItem) {
    if (selItem != -1 &&
        item(selItem)->action() &&
        item(selItem)->submenu())
    {
        return true;
    }
    return false;
}

int YMenu::onCascadeButton(int selItem, int x, int /*y*/, bool /*checkPopup*/) {
    if (isCondCascade(selItem)) {
        if (fPopup && fPopup == item(selItem)->submenu())
            return 0;

        int fontHeight = menuFont->height() + 1;

        unsigned int h = fontHeight;

        if (item(selItem)->getPixmap() && item(selItem)->getPixmap()->height() > h)
            h = item(selItem)->getPixmap()->height();

        if (x <= int(width() - h - 4))
            return 1;
    }
    return 0;
}

void YMenu::focusItem(int itemNo, int submenu, int byMouse) {
    selectedItem = itemNo;

    if (selectedItem != -1) {
        if (x() < 0 || y() < 0 ||
            x() + width() > desktop->width() ||
            y() + height() > desktop->height())
        {
            int ix, iy, ih, t, b, p;
            int ny = y();

            findItemPos(selectedItem, ix, iy);
            getItemHeight(selectedItem, ih, t, b, p);

            if (y() + iy + ih > int(desktop->height()))
                ny = desktop->height() - ih - iy;
            else if (y() + iy < 0)
                ny = -iy;
            setPosition(x(), ny);
        }
    }

    YMenu *sub = 0;
    if (selectedItem != -1)
        sub = item(selectedItem)->submenu();

    if (sub != fPopup) {
        int repaint = 0;

        if (fPopup) {
            fPopup->popdown();
            fPopup = 0;
            repaint = 1;
        }

        if (submenu && sub && item(selectedItem)->isEnabled()) {
            int xp, yp;
            int l, t, r, b;

            getOffsets(l, t, r, b);
            findItemPos(selectedItem, xp, yp);
            sub->setActionListener(getActionListener());
            sub->popup(this, 0,
                       x() + width() - r, y() + yp - t,
                       width() - r - l, -1,
                       YPopupWindow::pfCanFlipHorizontal |
                       (popupFlags() & YPopupWindow::pfFlipHorizontal) |
                       (byMouse ? (unsigned int)YPopupWindow::pfButtonDown : 0U));
            fPopup = sub;
            repaint = 1;
        }
        if (repaint && selectedItem != -1 && paintedItem == selectedItem &&
            item(selectedItem)->action())
            paintItems();
    }
    if (paintedItem != selectedItem)
        paintItems();
}

int YMenu::findActiveItem(int cur, int direction) {
    PRECONDITION(direction == -1 || direction == 1);

    if (itemCount() == 0)
        return -1;

    if (cur == -1)
        if (direction == 1)
            cur = itemCount() - 1;
        else
            cur = 0;

    PRECONDITION(cur >= 0 && cur < itemCount());

    int c = cur;
    do {
        c += direction;
        if (c < 0) c = itemCount() - 1;
        if (c >= itemCount()) c = 0;
    } while (c != cur && (item(c)->action() == 0 && item(c)->submenu() == 0));
    return c;
}

#ifdef DEBUG
int YMenu::activateItem(int no, int byMouse, unsigned int modifiers) {
#else
int YMenu::activateItem(int /*no*/, int byMouse, unsigned int modifiers) {
#endif
    PRECONDITION(selectedItem == no && selectedItem != -1);
    if (item(selectedItem)->isEnabled()) {
        if (item(selectedItem)->action() == 0 &&
            item(selectedItem)->submenu() != 0)
            focusItem(selectedItem, 1, byMouse);
        else if (item(selectedItem)->action())
            finishPopup(item(selectedItem),
	    		item(selectedItem)->action(), modifiers);
    } else {
        //XBell(app->display(), 50);
        return -1;
    }
    return 0;
}

int YMenu::findHotItem(char k) {
    int count = 0;

    for (int i = 0; i < itemCount(); i++) {
        int hot = item(i)->hotChar();

        if (hot != -1 && TOUPPER(char(hot)) == k)
            count++;
    }
    if (count == 0)
        return 0;

    int cur = selectedItem;
    if (cur == -1)
        cur = itemCount() - 1;

    int c = cur;
    do {
        c++;
        if (c >= itemCount())
            c = 0;

        if (item(c)->action() != 0 || item(c)->submenu() != 0) {
            int hot = item(c)->hotChar();

            if (hot != -1 && TOUPPER(char(hot)) == k) {
                focusItem(c, 0, 0);
                break;
            }
        }
    } while (c != cur);
    return count;
}

bool YMenu::handleKey(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    int m = KEY_MODMASK(key.state);

    if (key.type == KeyPress) {
        if ((m & ~ShiftMask) == 0) {
            if (k == XK_Escape) {
                cancelPopup();
            } else if (k == XK_Left || k == XK_KP_Left) {
                if (prevPopup())
                    cancelPopup();
            } else if (itemCount() > 0) {
                if (k == XK_Up || k == XK_KP_Up)
                    focusItem(findActiveItem(selectedItem, -1), 0, 0);
                else if (k == XK_Down || k == XK_KP_Down)
                    focusItem(findActiveItem(selectedItem, 1), 0, 0);
                else if (k == XK_Home || k == XK_KP_Home)
                    focusItem(findActiveItem(itemCount() - 1, 1), 0, 0);
                else if (k == XK_End || k == XK_KP_End)
                    focusItem(findActiveItem(0, -1), 0, 0);
                else if (k == XK_Right || k == XK_KP_Right)
                    focusItem(selectedItem, 1, 0);
                else if (k == XK_Return || k == XK_KP_Enter) {
                    if (selectedItem != -1 &&
                        (item(selectedItem)->action() != 0 ||
                         item(selectedItem)->submenu() != 0)) {
                        activateItem(selectedItem, 0, key.state);
                        return true;
                    }
                } else if ((k < 256) && ((m & ~ShiftMask) == 0)) {
                    if (findHotItem(TOUPPER(char(k))) == 1) {
                        if (!(m & ShiftMask))
                            activateItem(selectedItem, 0, key.state);
                    }
                    return true;
                }
            }
        }
    }
    return YPopupWindow::handleKey(key);
}

void YMenu::handleButton(const XButtonEvent &button) {
    if (button.button == Button4)
	setPosition(x(), max(button.y_root - (int)height() + 1,
			     y() - (int)(button.state & ControlMask ? 
					 menuFont->height() * 5/2 :
					 menuFont->height())));
    else if (button.button == Button5)
	setPosition(x(), min(button.y_root,
			     y() + (int)(button.state & ControlMask ? 
					 menuFont->height() * 5/2 :
					 menuFont->height())));
    else if (button.button) {
        int const selItem(findItem(button.x_root - x(), button.y_root - y()));
        bool const nocascade(!onCascadeButton(selItem,
					      button.x_root - x(),
                                              button.y_root - y(), true) ||
                             (button.state & ControlMask));

        if (button.type == ButtonRelease &&
	    fPopupActive == fPopup && fPopup != NULL &&
	    nocascade) {
            fPopup->popdown();
            fPopupActive = fPopup = 0;
            focusItem(selItem, 0, 1);
            if (nocascade) paintItems();
            return;
        } else if (button.type == ButtonPress)
            fPopupActive = fPopup;

        focusItem(selItem, nocascade, 1);

        if (selectedItem != -1 &&
            button.type == ButtonRelease &&
            (item(selectedItem)->submenu() != NULL ||
             item(selectedItem)->action() != NULL)
            &&
            (item(selectedItem)->action() == NULL ||
             item(selectedItem)->submenu() == NULL ||
	     !nocascade)) { // ??? !!! ??? WTF
            activatedX = button.x_root;
            activatedY = button.y_root;
            activateItem(selectedItem, 1, button.state);
            return;
        }

        if (button.type == ButtonRelease &&
            (selectedItem == -1 || (item(selectedItem)->action() == 0 &&
	    item(selectedItem)->submenu() == 0)))
            focusItem(findActiveItem(itemCount() - 1, 1), 0, 0);
    }
    YPopupWindow::handleButton(button);
}

void YMenu::handleMotion(const XMotionEvent &motion) {
    bool isButton =
        (motion.state & (Button1Mask |
                         Button2Mask |
                         Button3Mask |
                         Button4Mask |
                         Button5Mask)) ? true : false;

    if (menuMouseTracking || isButton) {
        int selItem = findItem(motion.x_root - x(), motion.y_root - y());
        if (fMenuTimer && fMenuTimer->getTimerListener() == this) {
            //msg("sel=%d timer=%d listener=%p =? this=%p", selItem, fTimerItem, fMenuTimer->getTimerListener(), this);
            if (selItem != fTimerItem || fTimerSlow) {
                fTimerItem = -1;
                if (fMenuTimer)
                    fMenuTimer->stopTimer();
            }
        }
        if (selItem != -1 || app->popup() == this) {
            int submenu = (onCascadeButton(selItem,
                                           motion.x_root - x(),
                                           motion.y_root - y(), false)
                           && !(motion.state & ControlMask)) ? 0 : 1;
            //if (selItem != -1)
            bool canFast = true;

            if (fPopup && activatedX != -1 && SubmenuActivateDelay != 0) {
                int dx = 0;
                int dy = motion.y_root - activatedY;
                int ty = fPopup->y() - activatedY;
                int by = fPopup->y() + fPopup->height() - activatedY;
                int px;

                if (fPopup->x() < activatedX)
                    px = activatedX - (fPopup->x() + fPopup->width());
                else
                    px = fPopup->x() - activatedX;

                if (fPopup->x() > motion.x_root)
                    dx = motion.x_root - activatedX;
                else
                    dx = activatedX - motion.x_root;

                dy = dy * px;

                if (dy >= ty * dx * 2 && dy <= by * dx * 2)
                    canFast = false;
            }

            if (canFast) {
                YPopupWindow *p = fPopup;

                if (MenuActivateDelay != 0 && selItem != -1) {
                    if (fMenuTimer == 0)
                        fMenuTimer = new YTimer();
                    if (fMenuTimer) {
                        fMenuTimer->setInterval(MenuActivateDelay);
                        fMenuTimer->setTimerListener(this);
                        if (!fMenuTimer->isRunning())
                            fMenuTimer->startTimer();
                    }
                    fTimerItem = selItem;
                    fTimerX = motion.x_root;
                    fTimerY = motion.y_root;
                    fTimerSubmenu = submenu;
                    fTimerSlow = false;
                } else {
                    focusItem(selItem, submenu, 1);
                    if (fPopup && p != fPopup) {
                        activatedX = motion.x_root;
                        activatedY = motion.y_root;
                    }
                }
            } else {
                //focusItem(selItem, 0, 1);
                fTimerItem = selItem;
                fTimerX = motion.x_root;
                fTimerY = motion.y_root;
                fTimerSubmenu = submenu;
                fTimerSlow = true;
                if (fMenuTimer == 0)
                    fMenuTimer = new YTimer();
                if (fMenuTimer) {
                    fMenuTimer->setInterval(SubmenuActivateDelay);
                    fMenuTimer->setTimerListener(this);
                    if (!fMenuTimer->isRunning())
                        fMenuTimer->startTimer();
                }
            }
        }
    }

    if (menuFont != NULL) { // ================ autoscrolling of large menus ===
        int const fh(menuFont->height());

        int const sx(motion.x_root < fh ? +fh :
		     motion.x_root >= (int)(desktop->width() - fh - 1) ? -fh :
		     0),
        	  sy(motion.y_root < fh ? +fh :
		     motion.y_root >= (int)(desktop->height() - fh - 1) ? -fh :
		     0);

	autoScroll(sx, sy, motion.x_root, motion.y_root, &motion);
    }

    YPopupWindow::handleMotion(motion); // ========== default implementation ===
}

bool YMenu::handleTimer(YTimer */*timer*/) {
    activatedX = fTimerX;
    activatedY = fTimerY;
    focusItem(fTimerItem, fTimerSubmenu, 1);
    return false;
}

bool YMenu::handleAutoScroll(const XMotionEvent & /*mouse*/) {
    int px = x();
    int py = y();

    if (fAutoScrollDeltaX != 0) {
        if (fAutoScrollDeltaX < 0) {
            if (px + width() > desktop->width())
                px += fAutoScrollDeltaX;
        } else {
            if (px < 0)
                px += fAutoScrollDeltaX;
        }
    }
    if (fAutoScrollDeltaY != 0) {
        if (fAutoScrollDeltaY < 0) {
            if (py + height() > desktop->height())
                py += fAutoScrollDeltaY;
        } else {
            if (py < 0)
                py += fAutoScrollDeltaY;
        }
    }
    setPosition(px, py);
    {
        int mx = fAutoScrollMouseX - x();
        int my = fAutoScrollMouseY - y();

        int selItem = findItem(mx, my);
        focusItem(selItem, 0, 1);
    }
    return true;
}

void YMenu::autoScroll(int deltaX, int deltaY, int mx, int my, const XMotionEvent *motion) {
    fAutoScrollDeltaX = deltaX;
    fAutoScrollDeltaY = deltaY;
    fAutoScrollMouseX = mx;
    fAutoScrollMouseY = my;
    beginAutoScroll((deltaX != 0 || deltaY != 0) ? true : false, motion);
}

YMenuItem *YMenu::addItem(const char *name, int hotCharPos, const char *param, YAction *action) {
    return add(new YMenuItem(name, hotCharPos, param, action, 0));
}

YMenuItem *YMenu::addItem(const char *name, int hotCharPos, YAction *action, YMenu *submenu) {
    return add(new YMenuItem(name, hotCharPos, 0, action, submenu));
}

YMenuItem * YMenu::addSubmenu(const char *name, int hotCharPos, YMenu *submenu) {
    return add(new YMenuItem(name, hotCharPos, 0, 0, submenu));
}

YMenuItem * YMenu::addSeparator() {
    return add(new YMenuItem());
}

YMenuItem * YMenu::addLabel(const char *name) {
    return add(new YMenuItem(name));
}

void YMenu::removeAll() {
    if (fPopup) {
        fPopup->popdown();
        fPopup = 0;
    }
    for (int i = 0; i < itemCount(); i++)
        delete fItems[i];
    FREE(fItems);
    fItems = 0;
    fItemCount = 0;
    paintedItem = selectedItem = -1;
    fPopup = 0;
}

YMenuItem * YMenu::add(YMenuItem *item) {
    if (item) {
        YMenuItem **newItems = (YMenuItem **)REALLOC(fItems, sizeof(YMenuItem *) * ((fItemCount) + 1));
        if (newItems) {
            fItems = newItems;
            fItems[fItemCount++] = item;
            return item;
        } else {
            delete item;
            return 0;
        }
    }
    return item;
}

YMenuItem *YMenu::findAction(const YAction *action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == item(i)->action())
            return item(i);
    return 0;
}

YMenuItem *YMenu::findSubmenu(const YMenu *sub) {
    for (int i = 0; i < itemCount(); i++)
        if (sub == item(i)->submenu())
            return item(i);
    return 0;
}

YMenuItem *YMenu::findName(const char *name, const int first = 0) {
    if (name != NULL)
        for (int i = first; i < itemCount(); i++) {
            const char *iname = item(i)->name();
            if (iname && !strcmp(name, iname))
                return item(i);
        }

    return 0;
}

void YMenu::enableCommand(YAction *action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == 0 || action == item(i)->action())
            item(i)->setEnabled(true);
}

void YMenu::disableCommand(YAction *action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == 0 || action == item(i)->action())
            item(i)->setEnabled(false);
}

int YMenu::getItemHeight(int itemNo, int &h, int &top, int &bottom, int &pad) {
    h = top = bottom = pad = 0;

    if (itemNo < 0 || itemNo > itemCount())
        return -1;

    if (item(itemNo)->name() == 0 && item(itemNo)->submenu() == 0) {
        top = 0;
        bottom = 0;
        pad = 1;
        if (wmLook == lookMetal)
            h = 3;
        else
            h = 4;
    } else {
        int fontHeight = menuFont->height() + 1;
        unsigned int ih = fontHeight;

        if (fontHeight < 16)
            fontHeight = 16;

        if (item(itemNo)->getPixmap() &&
            item(itemNo)->getPixmap()->height() > ih)
            ih = item(itemNo)->getPixmap()->height();

        if (wmLook == lookWarp4 || wmLook == lookWin95) {
            top = bottom = 0;
            pad = 1;
        } else if (wmLook == lookMetal) {
            top = bottom = 1;
            pad = 1;
        } else if (wmLook == lookMotif) {
            top = bottom = 2;
            pad = 0; //1
        } else if (wmLook == lookGtk) {
            top = bottom = 2;
            pad = 0; //1
        } else {
            top = 1;
            bottom = 2;
            pad = 0;//1;
        }
        h = top + pad + ih + pad + bottom;
    }
    return 0;
}

void YMenu::getItemWidth(int i, int &iw, int &nw, int &pw) {
    iw = nw = pw = 0;

    if (item(i)->name() != NULL || item(i)->submenu() != NULL) {
        YPixmap const *p(item(i)->getPixmap());
        if (p) iw = p->height();

        const char *name(item(i)->name());
        if (name) nw = menuFont->textWidth(name);

        const char *param(item(i)->param());
        if (param) pw = menuFont->textWidth(param);
    }
}

void YMenu::getOffsets(int &left, int &top, int &right, int &bottom) {
    if (wmLook == lookMetal) {
        left = 1;
        right = 1;
        top = 2;
        bottom = 2;
    } else {
        left = 2;
        top = 2;
        right = 3;
        bottom = 3;
    }
}

void YMenu::getArea(int &x, int &y, int &w, int &h) {
    getOffsets(x, y, w, h);
    w = width() - 1 - x - w;
    h = height() - 1 - y - h;
}

int YMenu::findItemPos(int itemNo, int &x, int &y) {
    x = -1;
    y = -1;

    if (itemNo < 0 || itemNo > itemCount())
        return -1;

    int w, h;

    getArea(x, y, w, h);
    for (int i = 0; i < itemNo; i++) {
        int ih, top, bottom, pad;

        getItemHeight(i, ih, top, bottom, pad);
        y += ih;
    }
    return 0;
}

int YMenu::findItem(int mx, int my) {
    int x, y, w, h;

    getArea(x, y, w, h);
    for (int i = 0; i < itemCount(); i++) {
        int top, bottom, pad;

        getItemHeight(i, h, top, bottom, pad);
        if (my >= y && my < y + h && mx > 0 && mx < int(width()) - 1)
            return i;

        y += h;
    }
    return -1;
}

void YMenu::sizePopup(int hspace) {
    int width, height;
    int maxName(0);
    int maxParam(0);
    int maxIcon(16);
    int l, t, r, b;
    int padx(1);
    int left(1);

    getOffsets(l, t, r, b);

    width = l;
    height = t;

    for (int i = 0; i < itemCount(); i++) {
        int ih, top, bottom, pad;

        getItemHeight(i, ih, top, bottom, pad);
        height += ih;

        if (pad > padx)
            padx = pad;
        if (top > left)
            left = top;

        int iw, nw, pw;

        getItemWidth(i, iw, nw, pw);

        if (item(i)->submenu())
            pw += 2 + ih;

        if (iw > maxIcon)
            maxIcon = iw;
        if (nw > maxName)
            maxName = nw;
        if (pw > maxParam)
            maxParam = pw;
    }

    maxName = min(maxName, (int)(MenuMaximalWidth ? MenuMaximalWidth
                                 : desktop->width() * 2/3));

    hspace -= 4 + r + maxIcon + l + left + padx + 2 + maxParam + 6 + 2;
    hspace = max(hspace, (int) desktop->width() / 3);

    // !!! not correct, to be fixed
    if (maxName > hspace)
        maxName = hspace;

    namePos = l + left + padx + maxIcon + 2;
    paramPos = namePos + 2 + maxName + 6;
    width = paramPos + maxParam + 4 + r;
    height += b;

    setSize(width, height);
}

void YMenu::paintItems() {
    Graphics &g = getGraphics();
    int l, t, r, b;
    getOffsets(l, t, r, b);

    for (int i = 0; i < itemCount(); i++)
        paintItem(g, i, l, t, r, 0, height(), (i == selectedItem || i == paintedItem) ? 1 : 0);

    paintedItem = selectedItem;
}

void YMenu::drawSeparator(Graphics &g, int x, int y, int w) {
    if (menusepPixmap) {
	g.fillPixmap(menubackPixmap,
		     x, y, w, 2 - menusepPixmap->height()/2);
	g.fillPixmap(menusepPixmap,
		     x, y + 2 - menusepPixmap->height()/2,
		     w, menusepPixmap->height());
	g.fillPixmap(menubackPixmap,
		     x, y + 2 + (menusepPixmap->height()+1)/2,
		     w, 2 - (menusepPixmap->height()+1)/2);
    } else if (wmLook == lookMetal) {
	if (menubackPixmap)
	    g.fillPixmap(menubackPixmap, x, y + 0, w, 1);
        else {
            g.setColor(menuBg);
            g.drawLine(x, y + 0, w, y + 0);
	}

        g.setColor(activeMenuItemBg);
        g.drawLine(x, y + 1, w, y + 1);;
        g.setColor(menuBg->brighter());
        g.drawLine(x, y + 2, w, y + 2);;
        g.drawLine(x, y, x, y + 2);
        g.setColor(menuBg);
    } else {
	if (menubackPixmap) {
	    g.fillPixmap(menubackPixmap, x, y + 0, w, 1);
	    g.fillPixmap(menubackPixmap, x, y + 3, w, 1);
        } else {
            //g.setColor(menuBg); // ASSUMED
	    g.drawLine(x, y + 0, w, y + 0);
            g.drawLine(x, y + 3, w, y + 3);
	}

        g.setColor(menuBg->darker());
        g.drawLine(x, y + 1, w, y + 1);
        g.setColor(menuBg->brighter());
        g.drawLine(x, y + 2, w, y + 2);
        g.setColor(menuBg);
    }
}

void YMenu::paintItem(Graphics &g, int i, int &l, int &t, int &r, int minY, int maxY, int paint) {
    int const fontHeight(menuFont->height() + 1);
    int const fontBaseLine(menuFont->ascent());

    YMenuItem *mitem = item(i);
    const char *name = mitem->name();
    const char *param = mitem->param();
    

    g.setColor(menuBg);
    if (mitem->name() == 0 && mitem->submenu() == 0) {
        if (t + 4 >= minY && t <= maxY) {
            if (paint)
                drawSeparator(g, 1, t, width() - 2);
        }

        t += (wmLook == lookMetal) ? 3 : 4;
    } else {
        bool const active(i == selectedItem && 
		         (mitem->action () || mitem->submenu()));

        int eh, top, bottom, pad, ih;
        getItemHeight(i, eh, top, bottom, pad);
        ih = eh - top - bottom - pad - pad;

        if (t + eh >= minY && t <= maxY) {

	int const cascadePos(width() - r - 2 - ih - pad);

        if (active)
            g.setColor(activeMenuItemBg);

        if (paint) {
            if (active && menuselPixmap)
                g.fillPixmap(menuselPixmap, l, t, width() -r -l, eh);
            else if (menubackPixmap)
                g.fillPixmap(menubackPixmap, l, t, width() -r -l, eh);
            else
                g.fillRect(l, t, width() - r - l, eh);

            if (wmLook == lookMetal && i != selectedItem) {
                g.setColor(menuBg->brighter());
                g.drawLine(1, t, 1, t + eh - 1);
                g.setColor(menuBg);
            }

            if (wmLook != lookWin95 && wmLook != lookWarp4 &&
                active)
            {
                bool raised = false;
#ifdef OLDMOTIF
                if (wmLook == lookMotif)
                    raised = true;
#endif

                g.setColor(menuBg);
                if (wmLook == lookGtk)
                    g.drawBorderW(l, t, width() - r - l - 1, eh - 1, true);
                else if (wmLook == lookMetal) {
                    g.setColor(activeMenuItemBg->darker());
                    g.drawLine(l, t, width() - r - l, t);
                    g.setColor(activeMenuItemBg->brighter());
                    g.drawLine(l, t + eh - 1, width() - r - l, t + eh - 1);
                } else
                    g.draw3DRect(l, t, width() - r - l - 1, eh - 1, raised);


                if (wmLook == lookMotif)
                    g.draw3DRect(l + 1, t + 1,
                                 width() - r - l - 3, eh - 3, raised);
            }

            YColor *fg(mitem->isEnabled() ? active ? activeMenuItemFg
						   : menuItemFg
					  : disabledMenuItemFg);
            g.setColor(fg);
            g.setFont(menuFont);

            int delta = (active) ? 1 : 0;
            if (wmLook == lookMotif || wmLook == lookGtk ||
                wmLook == lookWarp4 || wmLook == lookWin95 ||
                wmLook == lookMetal)
                delta = 0;
            int baseLine = t + top + pad + (ih - fontHeight) / 2 + fontBaseLine + delta;
                //1 + 1 + t + (eh - fontHeight) / 2 + fontBaseLine + delta;

            if (mitem->isChecked()) {
                XPoint points[4];

                points[0].x = l + 3 + (16 - 10) / 2;
                points[1].x = 5;
                points[2].x = 5;
                points[3].x = -5;
                points[0].y = t + top + pad + ih / 2;
                points[1].y = -5;
                points[2].y = 5;
                points[3].y = 5;

                g.fillPolygon(points, 4, Convex, CoordModePrevious);
            } else {
                if (mitem->getPixmap())
                    g.drawPixmap(mitem->getPixmap(),
                                 l + 1 + delta,
                                 t + delta + top + pad + (eh - top - pad * 2 - bottom - mitem->getPixmap()->height()) / 2);
            }

            if (name) {
		int const maxWidth((param ? paramPos - delta :
				    mitem->submenu() != NULL ? cascadePos :
				    width()) - namePos);

                if (!mitem->isEnabled()) {
                    g.setColor(disabledMenuItemSt);
		    g.drawCharsEllipsis(name, strlen(name),
				        1 + delta + namePos, 1 + baseLine, 
					maxWidth);

                    if (mitem->hotCharPos() != -1) {
                        g.drawCharUnderline(1 + delta +  namePos, 1 + baseLine,
                                            name, mitem->hotCharPos());
                    }
                }
                g.setColor(fg);
                g.drawCharsEllipsis(name, strlen(name),
				    delta + namePos, baseLine, maxWidth);

                if (mitem->hotCharPos() != -1) {
                    g.drawCharUnderline(delta + namePos, baseLine,
                                        name, mitem->hotCharPos());
                }
            }

            if (param) {
                if (!mitem->isEnabled()) {
                    g.setColor(disabledMenuItemSt);
                    g.drawChars(param, 0, strlen(param),
                                paramPos + delta + 1,
                                baseLine + 1);
                }
                g.setColor(fg);
                g.drawChars(param, 0, strlen(param),
                            paramPos + delta,
                            baseLine);
            }
            else if (mitem->submenu() != 0) {
//                int active = ((mitem->action() == 0 && i == selectedItem) ||
//                              fPopup == mitem->submenu()) ? 1 : 0;
                if (mitem->action()) {
                    g.setColor(menuBg);
                    if (0) {
                        if (menubackPixmap)
                            g.fillPixmap(menubackPixmap, width() - r - 1 -ih - pad, t + top + pad, ih, ih);
                        else
                            g.fillRect(width() - r - 1 - ih - pad, t + top + pad, ih, ih);
                        g.drawBorderW(width() - r - 1 - ih - pad, t + top + pad, ih - 1, ih - 1,
                                      active ? false : true);
                    } else {
                        g.setColor(menuBg->darker());
                        g.drawLine(cascadePos, t + top + pad,
                                   cascadePos, t + top + pad + ih);
                        g.setColor(menuBg->brighter());
                        g.drawLine(cascadePos + 1, t + top + pad,
                                   cascadePos + 1, t + top + pad + ih);

                    }
                    delta = delta ? active ? 1 : 0 : 0;
                }

                if (wmLook == lookGtk || wmLook == lookMotif) {
                    int asize = 9;
                    int ax = delta + width() - r - 1 - asize * 3 / 2;
                    int ay = delta + t + top + pad + (ih - asize) / 2;
                    g.setColor(menuBg);
                    g.drawArrow(Right, ax, ay, asize, active);
                } else {
                    int asize = 9;
                    int ax = width() - r - 1 - asize;
                    int ay = delta + t + top + pad + (ih - asize) / 2;

                    g.setColor(fg);
		    
		    if (wmLook == lookWarp3) {
			wmLook = lookNice;
			g.drawArrow(Right, ax, ay, asize);
			wmLook = lookWarp3;
		    } else
			g.drawArrow(Right, ax, ay, asize);
                }

            }
        }
        }
        t += eh;
    }
}

void YMenu::paint(Graphics &g, int /*_x*/, int _y, unsigned int /*_width*/, unsigned int _height) {
    if (wmLook == lookMetal) {
        g.setColor(activeMenuItemBg);
        g.drawLine(0, 0, width() - 1, 0);
        g.drawLine(0, 0, 0, height() - 1);
        g.drawLine(width() - 1, 0, width() - 1, height() - 1);
        g.drawLine(0, height() - 1, width() - 1, height() - 1);
        g.setColor(menuBg->brighter());
        g.drawLine(1, 1, width() - 2, 1);
        g.setColor(menuBg);
        g.drawLine(1, height() - 2, width() - 2, height() - 2);
    } else {
        g.setColor(menuBg);
        g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        g.drawLine(1, 1, width() - 3, 1);
        g.drawLine(1, 1, 1, height() - 3);
        g.drawLine(1, height() - 3, width() - 3, height() - 3);
        g.drawLine(width() - 3, 1, width() - 3, height() - 3);
    }

    int l, t, r, b;
    getOffsets(l, t, r, b);

    for (int i = 0; i < itemCount(); i++) {
        paintItem(g, i, l, t, r, _y, _y + _height, 1);
    }
    paintedItem = selectedItem;
}
