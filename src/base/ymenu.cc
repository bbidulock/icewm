/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#include "ykey.h"
#include "ymenu.h"
#include "yaction.h"
#include "ymenuitem.h"

#include "yapp.h"
#include "default.h"

#include <string.h>
#include <stdio.h>
#include "ycstring.h"

#define wmLook ------

YNumPrefProperty YMenu::gSubmenuActivateDelay("system", "SubmenuActivateDelay", 300);
YNumPrefProperty YMenu::gMenuActivateDelay("system", "MenuActivateDelay", 40);
YBoolPrefProperty YMenu::gMenuMouseTracking("system", "MenuMouseTracking", true);
YColorPrefProperty YMenu::gMenuBg("system", "ColorMenu", "rgb:C0/C0/C0");
YColorPrefProperty YMenu::gMenuItemFg("system", "ColorMenuItemText", "rgb:00/00/00");
YColorPrefProperty YMenu::gActiveMenuItemBg("system", "ColorActiveMenuItem", "rgb:A0/A0/A0");
YColorPrefProperty YMenu::gActiveMenuItemFg("system", "ColorActiveMenuItemText", "rgb:00/00/00");
YColorPrefProperty YMenu::gDisabledMenuItemFg("system", "ColorDisabledMenuItemText", "rgb:80/80/80");
YFontPrefProperty YMenu::gMenuFont("system", "MenuFontName", BOLDFONT(120));;
YPixmapPrefProperty YMenu::gPixmapBackground("system", "MenuBackgroundPixmap", 0, 0); // !!!"menubg.xpm", LIBDIR);

// MenuStyle
static int menustyle = YMenu::msWindows;
// MenuStyleActiveRaised
static int menuitemraised = 1;
// MenuStyleActiveMoves
static int menustyle_delta = 0;

int YMenu::fAutoScrollDeltaX = 0;
int YMenu::fAutoScrollDeltaY = 0;

void YMenu::setActionListener(YActionListener *actionListener) {
    fActionListener = actionListener;
}

void YMenu::finishPopup(YMenuItem *item, YAction *action, unsigned int modifiers) {
    YActionListener *cmd = fActionListener;

    YPopupWindow::finishPopup();

    if (item)
        item->actionPerformed(cmd, action, modifiers);
}

YTimer *YMenu::fMenuTimer = 0;
int YMenu::fTimerX = 0, YMenu::fTimerY = 0;
//int YMenu::fTimerItem = 0;
int YMenu::fTimerSubmenu = 0;
bool YMenu::fTimerSlow = false;

YMenu::YMenu(YWindow *parent): YPopupWindow(parent) {
    //if (menuFont == 0)
    //    menuFont = YFont::getFont(menuFontName);
    //if (menuBg == 0)
    //    menuBg = new YColor(clrNormalMenu);
    //if (menuItemFg == 0)
    //    menuItemFg = new YColor(clrNormalMenuItemText);
    //if (activeMenuItemBg == 0)
    //    activeMenuItemBg = new YColor(clrActiveMenuItem);
    //if (activeMenuItemFg == 0)
    //    activeMenuItemFg = new YColor(clrActiveMenuItemText);
    //if (disabledMenuItemFg == 0)
    //    disabledMenuItemFg = new YColor(clrDisabledMenuItemText);
    fItems = 0;
    fItemCount = 0;
    paintedItem = selectedItem = -1;
    submenuItem = -1;
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
        focusItem(findActiveItem(itemCount() - 1, 1));
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

        int fontHeight = gMenuFont.getFont()->height() + 1;

        unsigned int h = fontHeight;

        if (item(selItem)->getPixmap() && item(selItem)->getPixmap()->height() > h)
            h = item(selItem)->getPixmap()->height();

        if (x <= int(width() - h - 4))
            return 1;
    }
    return 0;
}

void YMenu::focusItem(int itemNo) {
    if (itemNo != selectedItem) {
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

        if (paintedItem != selectedItem)
            paintItems();
    }
}

void YMenu::activateSubMenu(bool submenu, bool byMouse) {
   YMenu *sub = 0;
    if (selectedItem != -1)
        sub = item(selectedItem)->submenu();

    if (sub != fPopup) {
        int repaint = 0;

        if (fPopup) {
            fPopup->popdown();
            fPopup = 0;
            submenuItem = -1;
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
            submenuItem = selectedItem;
            fPopup = sub;
            repaint = 1;
        }


        if (repaint && selectedItem != -1 && paintedItem == selectedItem &&
            item(selectedItem)->action())
            paintItems();
    }
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

int YMenu::activateItem(int no, int byMouse, unsigned int modifiers) {
    PRECONDITION(selectedItem == no && selectedItem != -1);
    if (item(selectedItem)->isEnabled()) {
        if (item(selectedItem)->action() == 0 &&
            item(selectedItem)->submenu() != 0)
        {
            focusItem(selectedItem); //, byMouse); // !!! 1
        } else if (item(selectedItem)->action()) {
            finishPopup(item(selectedItem), item(selectedItem)->action(), modifiers);
        }
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
                focusItem(c);
                break;
            }
        }
    } while (c != cur);
    return count;
}

bool YMenu::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    //KeySym k = XKeycodeToKeysym(app->display(), key.keycode, 0);
    //int m = KEY_MODMASK(key.state);

    if (key.type == KeyPress) {
        if ((vmod & ~kfShift) == 0) {
            if (ksym == XK_Escape) {
                cancelPopup();
            } else if (ksym == XK_Left || ksym == XK_KP_Left) {
                if (prevPopup())
                    cancelPopup();
            } else if (itemCount() > 0) {
                if (ksym == XK_Up || ksym == XK_KP_Up)
                    focusItem(findActiveItem(selectedItem, -1));
                else if (ksym == XK_Down || ksym == XK_KP_Down)
                    focusItem(findActiveItem(selectedItem, 1));
                else if (ksym == XK_Home || ksym == XK_KP_Home)
                    focusItem(findActiveItem(itemCount() - 1, 1));
                else if (ksym == XK_End || ksym == XK_KP_End)
                    focusItem(findActiveItem(0, -1));
                else if (ksym == XK_Right || ksym == XK_KP_Right) {
                    focusItem(selectedItem);
                    activateSubMenu(true, false);
                } else if (ksym == XK_Return || ksym == XK_KP_Enter) {
                    if (selectedItem != -1 &&
                        (item(selectedItem)->action() != 0 ||
                         item(selectedItem)->submenu() != 0))
                    {
                        activateItem(selectedItem, 0, key.state);
                        return true;
                    }
                } else if ((ksym < 256) && ((vmod & ~kfShift) == 0)) {
                    if (findHotItem(TOUPPER(char(ksym))) == 1) {
                        if (!(vmod & kfShift))
                            activateItem(selectedItem, 0, key.state);
                    }
                    return true;
                }
            }
        }
    }
    return YPopupWindow::handleKeySym(key, ksym, vmod);
}

void YMenu::handleButton(const XButtonEvent &button) {
    if (button.button != 0) {
        int selItem = findItem(button.x_root - x(), button.y_root - y());
        int nocascade = (onCascadeButton(selItem,
                                         button.x_root - x(),
                                         button.y_root - y(), true) &&
                         !(button.state & ControlMask)) ? 0 : 1;
        if (button.type == ButtonRelease && fPopupActive == fPopup && fPopup != 0 && nocascade) {
            fPopup->popdown();
            fPopupActive = fPopup = 0;
            focusItem(selItem); // byMouse=1
            if (nocascade)
                paintItems();
            return ;
        } else if (button.type == ButtonPress) {
            fPopupActive = fPopup;
        }
        focusItem(selItem); // !!! nocascade, byMouse=1
        activateSubMenu(nocascade, true);
        if (selectedItem != -1 &&
            button.type == ButtonRelease &&
            (item(selectedItem)->submenu() != 0 ||
             item(selectedItem)->action() != 0)
            &&
            (item(selectedItem)->action() == 0 ||
             !item(selectedItem)->submenu() || !nocascade) // ??? !!! ??? WTF
           )
        {
            activatedX = button.x_root;
            activatedY = button.y_root;
            activateItem(selectedItem, 1, button.state);
            return ;
        }
        if (button.type == ButtonRelease &&
            (selectedItem == -1 || (item(selectedItem)->action() == 0 && item(selectedItem)->submenu() == 0)))
            focusItem(findActiveItem(itemCount() - 1, 1));
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


    if (gMenuMouseTracking.getBool() || isButton) {
        int selItem = findItem(motion.x_root - x(), motion.y_root - y());

        if (fMenuTimer && fMenuTimer->getTimerListener() == this) {
            //printf("sel=%d timer=%d listener=%p =? this=%p\n", selItem, fTimerItem, fMenuTimer->getTimerListener(), this);
            if (selItem != selectedItem && selItem != -1) // ok && ???
                if (fMenuTimer)
                    fMenuTimer->stopTimer();
        }

        if (selItem != -1 && item(selItem)->name() == 0)
            selItem = -1;

        if (selItem == -1)
            selItem = submenuItem;

        if (selItem != -1 /*&& app->popup() == this*/) {
            focusItem(selItem);// submenu, 1

#if 1
            int submenu = (onCascadeButton(selItem,
                                           motion.x_root - x(),
                                           motion.y_root - y(), false)
                           && !(motion.state & ControlMask)) ? 0 : 1;
#endif
            //if (selItem != -1)
            bool canFast = true;

            if (fPopup /*&& activatedX != -1 && gSubmenuActivateDelay.getNum() != 0*/) {
                int dx = 0;
                int dy = motion.y_root - activatedY;
                int ty = fPopup->y() - activatedY;
                int by = fPopup->y() + fPopup->height() - activatedY;
                int px = 0;

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

            if (fMenuTimer == 0)
                fMenuTimer = new YTimer(this, gMenuActivateDelay.getNum());

            if (fMenuTimer) {
                fTimerSubmenu = submenu;
                if (canFast) {
                    fMenuTimer->setInterval(gMenuActivateDelay.getNum());
                    if (selectedItem != -1 &&
                        item(selectedItem)->submenu() != 0 &&
                        submenu)
                    {
                        activatedX = motion.x_root;
                        activatedY = motion.y_root;
                    }
                } else
                    fMenuTimer->setInterval(gSubmenuActivateDelay.getNum());

                fMenuTimer->setTimerListener(this);
                if (!fMenuTimer->isRunning())
                    fMenuTimer->startTimer();
            }
        }
    }

    // !!! should this be moved to YPopupWindow?
    int sx = 0;
    int sy = 0;

    if (motion.x_root == 0)
        sx = 10;
    else if (motion.x_root == int(desktop->width()) - 1)
        sx = -10;

    if (motion.y_root == 0)
        sy = 10;
    else if (motion.y_root == int(desktop->height()) - 1)
        sy = -10;

    autoScroll(sx, sy, &motion);

    YPopupWindow::handleMotion(motion);
}

void YMenu::handleCrossing(const XCrossingEvent &crossing) { // !!! doesn't work
    if (crossing.type == LeaveNotify) {
        if (selectedItem == -1)
            focusItem(submenuItem);
    }
}

bool YMenu::handleTimer(YTimer *timer) {
    if (timer == fMenuTimer) {
        activatedX = fTimerX;
        activatedY = fTimerY;
        activateSubMenu(fTimerSubmenu, true);
#if 0
        focusItem(fTimerItem, fTimerSubmenu, 1);
#endif
    }
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
    return true;
}

void YMenu::autoScroll(int deltaX, int deltaY, const XMotionEvent *motion) {
    fAutoScrollDeltaX = deltaX;
    fAutoScrollDeltaY = deltaY;
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

YMenuItem *YMenu::findAction(YAction *action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == item(i)->action())
            return item(i);
    return 0;
}

YMenuItem *YMenu::findSubmenu(YMenu *sub) {
    for (int i = 0; i < itemCount(); i++)
        if (sub == item(i)->submenu())
            return item(i);
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

    if (item(itemNo)->action() == 0 && item(itemNo)->submenu() == 0) {
        top = 0;
        bottom = 0;
        pad = 1;
        if (menustyle == msMetal)
            h = 3;
        else
            h = 4;
    } else {
        int fontHeight = gMenuFont.getFont()->height() + 1;
        unsigned int ih = fontHeight;

        if (fontHeight < 16)
            fontHeight = 16;

        if (item(itemNo)->getPixmap() &&
            item(itemNo)->getPixmap()->height() > ih)
            ih = item(itemNo)->getPixmap()->height();

        if (menustyle == YMenu::msWindows) {
            top = bottom = 0;
            pad = 1;
        } else if (menustyle == YMenu::msMetal) {
            top = bottom = 1;
            pad = 1;
        } else { // if (wmLook == YMenu::msMotif) {
            top = bottom = 2;
            pad = 0;
        }
        h = top + pad + ih + pad + bottom;
    }
    return 0;
}

void YMenu::getItemWidth(int i, int &iw, int &nw, int &pw) {
    iw = nw = pw = 0;

    if (item(i)->action() == 0 && item(i)->submenu() == 0) {
        iw = 0;
        nw = 0;
        pw = 0;
    } else {
        YPixmap *p = item(i)->getPixmap();
        if (p)
            iw = p->height();

        const CStr *name = item(i)->name();
        if (name)
            nw = gMenuFont.getFont()->textWidth(name->c_str());

        const CStr *param = item(i)->param();
        if (param)
            pw = gMenuFont.getFont()->textWidth(param->c_str());
    }
}

void YMenu::getOffsets(int &left, int &top, int &right, int &bottom) {
    if (menustyle == YMenu::msMetal) {
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

void YMenu::sizePopup() {
    int width, height;
    int maxName = 0;
    int maxParam = 0;
    int maxIcon = 16;
    int l, t, r, b;
    int padx = 1;
    int left = 1;

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

    //maxWidth = maxName + maxParam + (maxParam ? 8 : 0);
    //width += 3 + maxh + 2 + 2 + maxWidth + 4 + 4;

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
        paintItem(g, i, l, t, r, (i == selectedItem || i == paintedItem) ? 1 : 0);
    paintedItem = selectedItem;
}

void YMenu::drawSeparator(Graphics &g, int x, int y, int w) {
    if (menustyle == msMetal) {
        g.setColor(gMenuBg);
        g.drawLine(x, y + 0, w, y + 0);
        g.setColor(gActiveMenuItemBg);
        g.drawLine(x, y + 1, w, y + 1);;
        g.setColor(YColor::white);
        g.drawLine(x, y + 2, w, y + 2);;
        g.drawLine(x, y, x, y + 2);
        g.setColor(gMenuBg);
    } else {
        //g.setColor(menuBg); // ASSUMED
        g.drawLine(x, y + 0, w, y + 0);
        g.setColor(gMenuBg.getColor()->darker());
        g.drawLine(x, y + 1, w, y + 1);
        g.setColor(gMenuBg.getColor()->brighter());
        g.drawLine(x, y + 2, w, y + 2);
        g.setColor(gMenuBg.getColor());
        g.drawLine(x, y + 3, w, y + 3); y++;
    }
}

void YMenu::paintItem(Graphics &g, int i, int &l, int &t, int &r, int paint) {
    int fontHeight = gMenuFont.getFont()->height() + 1;
    int fontBaseLine = gMenuFont.getFont()->ascent();
    YMenuItem *mitem = item(i);
    const CStr *name = mitem->name();
    const CStr *param = mitem->param();
    YPixmap *menubackPixmap = gPixmapBackground.getPixmap();

    g.setColor(gMenuBg);
    if (mitem->action() == 0 && mitem->submenu() == 0) {
        if (paint)
            drawSeparator(g, 1, t, width() - 2);
        t += (menustyle == msMetal) ? 3 : 4;
    } else {
        int eh, top, bottom, pad, ih;

        getItemHeight(i, eh, top, bottom, pad);
        ih = eh - top - bottom - pad - pad;

        if (i == selectedItem)
            g.setColor(gActiveMenuItemBg);

        if (paint) {
            if (menubackPixmap)
                g.fillPixmap(menubackPixmap, l, t, width() -r -l, eh);
            else
                g.fillRect(l, t, width() - r - l, eh);

            if (menustyle == msMetal && i != selectedItem) {
                g.setColor(YColor::white);
                g.drawLine(1, t, 1, t + eh - 1);
                g.setColor(gMenuBg);
            }

            if (menustyle != msWindows &&
                i == selectedItem)
            {
                bool raised = false;
#ifdef OLDMOTIF
                if (wmLook == lookMotif)
                    raised = true;
#endif

                g.setColor(gMenuBg);
                if (menustyle == msGtk)
                    g.drawBorderW(l, t, width() - r - l - 1, eh - 1, true);
                else
                    if (menustyle == msMetal) {
                    g.setColor(gActiveMenuItemBg.getColor()->darker());
                    g.drawLine(l, t, width() - r - l, t);
                    g.setColor(gActiveMenuItemBg.getColor()->brighter());
                    g.drawLine(l, t + eh - 1, width() - r - l, t + eh - 1);
                } else
                    g.draw3DRect(l, t, width() - r - l - 1, eh - 1, raised);


                if (menustyle == msMotif)
                    g.draw3DRect(l + 1, t + 1,
                                 width() - r - l - 3, eh - 3, raised);
            }

            YColor *fg = 0;
            if (!mitem->isEnabled())
                fg = gDisabledMenuItemFg.getColor();
            else if (i == selectedItem)
                fg = gActiveMenuItemFg.getColor();
            else
                fg = gMenuItemFg.getColor();
            g.setColor(fg);
            g.setFont(gMenuFont.getFont());

            // !!! amke
            int delta = (i == selectedItem) ? 1 : 0;
            if (delta)
                delta = menustyle_delta;
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
                if (!mitem->isEnabled()) {
                    g.setColor(YColor::white);
                    g.drawChars(name->c_str(), 0, name->length(),
                                1 + delta + namePos, 1 + baseLine);

                    if (mitem->hotCharPos() != -1) {
                        g.drawCharUnderline(1 + delta +  namePos, 1 + baseLine,
                                            name->c_str(), mitem->hotCharPos());
                    }
                }
                g.setColor(fg);
                g.drawChars(name->c_str(), 0, name->length(),
                            delta + namePos, baseLine);

                if (mitem->hotCharPos() != -1) {
                    g.drawCharUnderline(delta + namePos, baseLine,
                                        name->c_str(), mitem->hotCharPos());
                }
            }

            if (param) {
                if (!mitem->isEnabled()) {
                    g.setColor(YColor::white);
                    g.drawChars(param->c_str(), 0, param->length(),
                                paramPos + delta + 1,
                                baseLine + 1);
                }
                g.setColor(fg);
                g.drawChars(param->c_str(), 0, param->length(),
                            paramPos + delta,
                            baseLine);
            }
            else if (mitem->submenu() != 0) {
                int active = ((mitem->action() == 0 && i == selectedItem) ||
                              fPopup == mitem->submenu()) ? 1 : 0;
                if (mitem->action()) {
                    g.setColor(gMenuBg);
                    if (0) {
                        if (menubackPixmap)
                            g.fillPixmap(menubackPixmap, width() - r - 1 -ih - pad, t + top + pad, ih, ih);
                        else
                            g.fillRect(width() - r - 1 - ih - pad, t + top + pad, ih, ih);
                        g.drawBorderW(width() - r - 1 - ih - pad, t + top + pad, ih - 1, ih - 1,
                                      active ? false : true);
                    } else {
                        g.setColor(gMenuBg.getColor()->darker());
                        g.drawLine(width() - r - 2 - ih - pad, t + top + pad,
                                   width() - r - 2 - ih - pad, t + top + pad + ih);
                        g.setColor(gMenuBg.getColor()->brighter());
                        g.drawLine(width() - r - 2 - ih - pad + 1, t + top + pad,
                                   width() - r - 2 - ih - pad + 1, t + top + pad + ih);

                    }
                    delta = delta ? active ? 1 : 0 : 0;
                }


                // !!! make arrow in/out depending on menu visibility
                // not item being active
                if (menustyle == msGtk || menustyle == msMotif) {
                    int asize = 9;
                    int ax = delta + width() - r - 1 - asize * 3 / 2;
                    int ay = delta + t + top + pad + (ih - asize) / 2;
                    g.setColor(gMenuBg);
                    g.drawArrow(0, active ? 1 : -1, ax, ay, asize);
                } else {
                    int asize = 9;
                    int ax = width() - r - 1 - asize;
                    int ay = delta + t + top + pad + (ih - asize) / 2;

                    g.setColor(fg);
                    g.drawArrow(0, 0, ax, ay, asize);
                }

            }
        }
        t += eh;
    }
}

void YMenu::paint(Graphics &g, int /*_x*/, int /*_y*/, unsigned int /*_width*/, unsigned int /*_height*/) {
    if (menustyle == msMetal) {
        g.setColor(gActiveMenuItemBg);
        g.drawLine(0, 0, width() - 1, 0);
        g.drawLine(0, 0, 0, height() - 1);
        g.drawLine(width() - 1, 0, width() - 1, height() - 1);
        g.drawLine(0, height() - 1, width() - 1, height() - 1);
        g.setColor(YColor::white);
        g.drawLine(1, 1, width() - 2, 1);
        g.setColor(gMenuBg);
        g.drawLine(1, height() - 2, width() - 2, height() - 2);
    } else {
        g.setColor(gMenuBg);
        g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        g.drawLine(1, 1, width() - 3, 1);
        g.drawLine(1, 1, 1, height() - 3);
        g.drawLine(1, height() - 3, width() - 3, height() - 3);
        g.drawLine(width() - 3, 1, width() - 3, height() - 3);
    }

    int l, t, r, b;
    getOffsets(l, t, r, b);

    for (int i = 0; i < itemCount(); i++)
        paintItem(g, i, l, t, r, 1);
    paintedItem = selectedItem;
}
