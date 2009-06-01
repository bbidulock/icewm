/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"

#include "yfull.h"
#include "ykey.h"
#include "ymenu.h"
#include "yaction.h"
#include "ymenuitem.h"
#include "yrect.h"

#include "yxapp.h"
#include "prefs.h"
#include "yprefs.h"

#include "ascii.h"
#include "ypixbuf.h"

#include <string.h>

YColor *menuBg = 0;
YColor *menuItemFg = 0;
YColor *activeMenuItemBg = 0;
YColor *activeMenuItemFg = 0;
YColor *disabledMenuItemFg = 0;
YColor *disabledMenuItemSt = 0;

ref<YFont> menuFont;

int YMenu::fAutoScrollDeltaX = 0;
int YMenu::fAutoScrollDeltaY = 0;
int YMenu::fAutoScrollMouseX = -1;
int YMenu::fAutoScrollMouseY = -1;
YMenu *YMenu::fPointedMenu = 0;

void YMenu::setActionListener(YActionListener *actionListener) {
    fActionListener = actionListener;
}

void YMenu::finishPopup(YMenuItem *item, YAction *action,
                        unsigned int modifiers)
{
    YActionListener *cmd = fActionListener;

    YPopupWindow::finishPopup();

    if (item)
        item->actionPerformed(cmd, action, modifiers);
}

YTimer *YMenu::fMenuTimer = 0;

YMenu::YMenu(YWindow *parent):
    YPopupWindow(parent) INIT_GRADIENT(fGradient, NULL) {
    if (menuFont == null)
        menuFont = YFont::getFont(XFA(menuFontName));
    if (menuBg == 0)
        menuBg = new YColor(clrNormalMenu);
    if (menuItemFg == 0)
        menuItemFg = new YColor(clrNormalMenuItemText);
    if (*clrActiveMenuItem && activeMenuItemBg == 0)
        activeMenuItemBg = new YColor(clrActiveMenuItem);
    if (activeMenuItemFg == 0)
        activeMenuItemFg = new YColor(clrActiveMenuItemText);
    if (disabledMenuItemFg == 0)
        disabledMenuItemFg = new YColor(clrDisabledMenuItemText);
    if (disabledMenuItemSt == 0)
        disabledMenuItemSt = *clrDisabledMenuItemShadow
                           ? new YColor(clrDisabledMenuItemShadow)
                           : menuBg->brighter();

    paintedItem = selectedItem = -1;
    submenuItem = -1;
    fPopup = 0;
    fActionListener = 0;
    fPopupActive = 0;
    fShared = false;
    activatedX = -1;
    activatedY = -1;
    fTimerX = 0;
    fTimerY = 0;
    fTimerSubmenuItem = -1;
}

YMenu::~YMenu() {
    if (fMenuTimer && fMenuTimer->getTimerListener() == this) {
        fMenuTimer->setTimerListener(0);
        fMenuTimer->stopTimer();
    }
    hideSubmenu();
#ifdef CONFIG_GRADIENTS
    fGradient = null;
#endif
}

void YMenu::activatePopup(int flags) {
    if (popupFlags() & pfButtonDown)
        focusItem(-1);
    else {
        if (menuMouseTracking && (flags & pfButtonDown))
            focusItem(-1);
        else
            focusItem(findActiveItem(itemCount() - 1, 1));
    }
}

void YMenu::deactivatePopup() {
    hideSubmenu();
    if (fPointedMenu == this)
        fPointedMenu = 0;
    if (fMenuTimer && fMenuTimer->getTimerListener() == this) {
        fMenuTimer->setTimerListener(0);
        fMenuTimer->stopTimer();
    }
}

void YMenu::donePopup(YPopupWindow *popup) {
    PRECONDITION(popup != 0);
    PRECONDITION(fPopup != 0);
    if (fPointedMenu == this)
        fPointedMenu = 0;
    if (fPopup) {
        hideSubmenu();
        if (selectedItem != -1)
            if (getItem(selectedItem)->getSubmenu() == popup)
                paintItems();
    }
}

bool YMenu::isCondCascade(int selItem) {
    if (selItem != -1 &&
        getItem(selItem)->getAction() &&
        getItem(selItem)->getSubmenu())
    {
        return true;
    }
    return false;
}

int YMenu::onCascadeButton(int selItem, int x, int /*y*/, bool /*checkPopup*/) {
    if (isCondCascade(selItem)) {
#if 1
        ///hmm
        if (fPopup && fPopup == getItem(selItem)->getSubmenu())
            return 0;
#endif

        int fontHeight = menuFont->height() + 1;
        int h = fontHeight;

#ifndef LITE
        if (getItem(selItem)->getIcon() != null &&
            YIcon::smallSize() > h)
            h = YIcon::smallSize();
#endif

        if (x <= int(width() - h - 4))
            return 1;
    }
    return 0;
}

void YMenu::focusItem(int itemNo) {
    if (itemNo != selectedItem) {
        selectedItem = itemNo;

        int dx, dy, dw, dh;
        desktop->getScreenGeometry(&dx, &dy, &dw, &dh, getXiScreen());

        if (selectedItem != -1) {
            if (x() < dx || y() < dy ||
                x() + width() > dx + dw ||
                y() + height() > dy + dh)
            {
                int ix, iy, ih;
                int ny = y();

                findItemPos(selectedItem, ix, iy, ih);

                if (y() + iy + ih > dy + dh)
                    ny = dx + dh - ih - iy;
                else if (y() + iy < dy)
                    ny = -iy;
                setPosition(x(), ny);
            }
        }
    }
    paintItems();
}

void YMenu::activateSubMenu(int item, bool byMouse) {
    YMenu *sub = 0;
    if (item != -1 && getItem(item)->isEnabled())
        sub = getItem(item)->getSubmenu();

    if (sub != fPopup) {
        int repaint = 0;

        hideSubmenu();
        repaint = 1;

        if (sub) {
            int xp, yp, ih;
            int l, t, r, b;

            getOffsets(l, t, r, b);
            findItemPos(item, xp, yp, ih);
            YRect rect(x(), y(), width(), height());
            sub->setActionListener(getActionListener());
            sub->popup(0, this, 0,
                       x() + width() - r, y() + yp - t,
                       width() - r - l, -1,
                       getXiScreen(),
                       YPopupWindow::pfCanFlipHorizontal |
                       (popupFlags() & YPopupWindow::pfFlipHorizontal) |
                       (byMouse ? (unsigned int)YPopupWindow::pfButtonDown : 0U));
            fPopup = sub;
            repaint = 1;
            submenuItem = item;
        }
        paintItems();
    }
}

int YMenu::findActiveItem(int cur, int direction) {
    PRECONDITION(direction == -1 || direction == 1);

    if (itemCount() == 0)
        return -1;

    if (cur == -1) {
        if (direction == 1)
            cur = itemCount() - 1;
        else
            cur = 0;
    }

    PRECONDITION(cur >= 0 && cur < itemCount());

    int c = cur;
    do {
        c += direction;
        if (c < 0) c = itemCount() - 1;
        if (c >= itemCount()) c = 0;
    } while (c != cur && (!getItem(c)->getAction() &&
                          !getItem(c)->getSubmenu()));
    return c;
}

int YMenu::activateItem(int modifiers, bool byMouse) {
    PRECONDITION(selectedItem != -1);
    if (getItem(selectedItem)->isEnabled()) {
        if (getItem(selectedItem)->getAction() == 0 &&
            getItem(selectedItem)->getSubmenu() != 0)
        {
            focusItem(selectedItem);
            activateSubMenu(selectedItem, byMouse);
        } else if (getItem(selectedItem)->getAction()) {
            finishPopup(getItem(selectedItem), getItem(selectedItem)->getAction(), modifiers);
        }
    } else {
        return -1;
    }
    return 0;
}

int YMenu::findHotItem(char k) {
    int count = 0;

    for (int i = 0; i < itemCount(); i++) {
        int hot = getItem(i)->getHotChar();

        const YMenuItem *mitem = getItem(i);
        if (mitem->getAction() ||
            mitem->getSubmenu())
        {
            if (hot != -1 && ASCII::toUpper(hot) == k)
                count++;
        }
    }
    if (count == 0)
        return 0;

    int cur = selectedItem;
    if (cur == -1)
        cur = itemCount() - 1;

    for (int c = cur + 1; ; ++c) {
        if (c >= itemCount())
            c = 0;

        const YMenuItem *mitem = getItem(c);
        if (mitem->getAction() ||
            mitem->getSubmenu())
        {
            int hot = mitem->getHotChar();

            if (hot != -1 && ASCII::toUpper(hot) == k) {
                focusItem(c);
                break;
            }
        }
        if (c == cur)
            break;
    }
    return count;
}

bool YMenu::handleKey(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(xapp->display(), (KeyCode)key.keycode, 0);
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
                    focusItem(findActiveItem(selectedItem, -1));
                else if (k == XK_Down || k == XK_KP_Down)
                    focusItem(findActiveItem(selectedItem, 1));
                else if (k == XK_Home || k == XK_KP_Home)
                    focusItem(findActiveItem(itemCount() - 1, 1));
                else if (k == XK_End || k == XK_KP_End)
                    focusItem(findActiveItem(0, -1));
                else if (k == XK_Right || k == XK_KP_Right) {
                    focusItem(selectedItem);
                    activateSubMenu(selectedItem, false);
                } else if (k == XK_Return || k == XK_KP_Enter) {
                    if (selectedItem != -1 &&
                        (getItem(selectedItem)->getAction() != 0 ||
                         getItem(selectedItem)->getSubmenu() != 0))
                    {
                        activateItem(key.state, false);
                        return true;
                    }
                } else if ((k < 256) && ((m & ~ShiftMask) == 0)) {
                    if (findHotItem(ASCII::toUpper((char)k)) == 1) {
                        if (!(m & ShiftMask))
                            activateItem(key.state, false);
                    }
                    return true;
                }
            }
        }
    }
    return YPopupWindow::handleKey(key);
}

void YMenu::handleButton(const XButtonEvent &button) {
    if (button.button == Button5) {
        if (button.type == ButtonPress) {
            if (button.x_root >= x() && button.x_root < (int)(x() + width())) {
                hideSubmenu();
                setPosition(x(), clamp(y() - (int)(button.state & ShiftMask ?
                                                   menuFont->height() * 5/2 :
                                                   menuFont->height()),
                                       button.y_root - (int)height() + 1,
                                       button.y_root));
                if (menuMouseTracking)
                    trackMotion(clamp(button.x_root, x() + 2, x() + (int)width() - 3),
                                button.y_root, button.state, true);
            }
        }
    } else if (button.button == Button4) {
        if (button.type == ButtonPress) {
            if (button.x_root >= x() && button.x_root < (int)(x() + width())) {
                hideSubmenu();
                setPosition(x(), clamp(y() + (int)(button.state & ShiftMask ?
                                                   menuFont->height() * 5/2 :
                                                   menuFont->height()),
                                       button.y_root - (int)height() + 1,
                                       button.y_root));
                if (menuMouseTracking)
                    trackMotion(clamp(button.x_root, x() + 2, x() + (int)width() - 3),
                                button.y_root, button.state, true);
            }
        }
    } else if (button.button) {
        int const selItem = findItem(button.x_root - x(),
                                     button.y_root - y());
        trackMotion(button.x_root, button.y_root, button.state, false);
        bool const nocascade(!onCascadeButton(selItem,
                                              button.x_root - x(),
                                              button.y_root - y(), true) ||
                             (button.state & ControlMask));

        if (button.type == ButtonPress) {
            fPopupActive = fPopup;
        } else if (button.type == ButtonRelease) {
            if (fPopupActive == fPopup && fPopup != 0 && nocascade) {
                hideSubmenu();
                focusItem(selItem);
                paintItems();
                return;
            }
        }
        focusItem(selItem);
        activatedX = button.x_root;
        activatedY = button.y_root;
        activateSubMenu(nocascade ? selectedItem : -1, true);

        if (button.type == ButtonRelease)
        {
            bool noAction = true;
            if (selectedItem != -1) {
                noAction =
                getItem(selectedItem)->getAction() == 0 &&
                getItem(selectedItem)->getSubmenu() == 0;
            }

            if (selectedItem == -1 || noAction) {
                if (menuMouseTracking)
                    focusItem(-1);
                else
                    focusItem(findActiveItem(itemCount() - 1, 1));
            } else {
                if ((getItem(selectedItem)->getAction() != 0 ||
                     getItem(selectedItem)->getSubmenu() != 0)
                    &&
                    (getItem(selectedItem)->getAction() == 0 ||
                     getItem(selectedItem)->getSubmenu() == 0 || !nocascade)
                   )
                {

                    activateItem(button.state, true);
                    return;
                }
            }
        }
    }
    YPopupWindow::handleButton(button);
}

void YMenu::handleMotion(const XMotionEvent &motion) {
    if (motion.x_root >= x() &&
        motion.y_root >= y() &&
        motion.x_root < int (x() + width()) &&
        motion.y_root < int (y() + height()))
    {
        bool isButton =
            (motion.state & (Button1Mask |
                             Button2Mask |
                             Button3Mask |
                             Button4Mask |
                             Button5Mask)) ? true : false;

        if (menuMouseTracking || isButton)
            trackMotion(motion.x_root, motion.y_root, motion.state, true);

        if (menuFont != null) { // ================ autoscrolling of large menus ===
            int const fh(menuFont->height());
    
            int dx, dy, dw, dh;
            desktop->getScreenGeometry(&dx, &dy, &dw, &dh, getXiScreen());

            int const sx(motion.x_root < fh ? +fh :
                         motion.x_root >= (dx + dw - fh - 1) ? -fh :
                         0),
                sy(motion.y_root < fh ? +fh :
                   motion.y_root >= (dy + dh - fh - 1) ? -fh :
                   0);

            if (motion.y_root >= y() && motion.y_root < (y() + height()) &&
                motion.x_root >= x() && motion.x_root < (x() + width()))
            {
                autoScroll(sx, sy, motion.x_root, motion.y_root, &motion);
            }
        }
    }
    YPopupWindow::handleMotion(motion); // ========== default implementation ===
}

void YMenu::trackMotion(const int x_root, const int y_root,
                        const unsigned state, bool trackSubmenu)
{
    int selItem = findItem(x_root - x(), y_root - y());
    if (fMenuTimer &&
        fMenuTimer->getTimerListener() == this &&
        (selItem != fTimerSubmenuItem)) /// ok?
    {
        fMenuTimer->stopTimer();
    }
    if (selItem != -1) {
        focusItem(selItem);

        const bool submenu =
            state & ControlMask ||
            !onCascadeButton(selItem,
                             x_root - x(), y_root - y(), false);
        if (trackSubmenu) {
            bool canFast = true;

            if (fPopup) {
                int dx = 0;
                int dy = y_root - activatedY;
                int ty = fPopup->y() - activatedY;
                int by = fPopup->y() + fPopup->height() - activatedY;
                int px = 0;

                if (fPopup->x() < activatedX)
                    px = activatedX - (fPopup->x() + fPopup->width());
                else
                    px = fPopup->x() - activatedX;

                if (fPopup->x() > x_root)
                    dx = x_root - activatedX;
                else
                    dx = activatedX - x_root;

                dy = dy * px;

                if (dy >= ty * dx && dy <= by * dx)
                    canFast = false;
            }

            if (fMenuTimer == 0)
                fMenuTimer = new YTimer();
            if (!fMenuTimer)
                return;
            fMenuTimer->setTimerListener(this);
            fTimerX = x_root;
            fTimerY = y_root;
            fTimerSubmenuItem = submenu ? selectedItem : -1;
            if (canFast) {
                fMenuTimer->setInterval(MenuActivateDelay);
//                if (selectedItem != -1 &&
//                    getItem(selectedItem)->getSubmenu() != 0 &&
//                    submenu)
//                {
//                    activatedX = x_root;
//                    activatedY = y_root;
//                }
            } else
                fMenuTimer->setInterval(SubmenuActivateDelay);
            fMenuTimer->setTimerListener(this);
            if (!fMenuTimer->isRunning())
                fMenuTimer->startTimer();
        }
    }
}

void YMenu::handleMotionOutside(bool top, const XMotionEvent &motion) {
    bool isButton =
        (motion.state & (Button1Mask |
                         Button2Mask |
                         Button3Mask |
                         Button4Mask |
                         Button5Mask)) ? true : false;
    if (!top || isButton || menuMouseTracking)
        focusItem(-1);
}

bool YMenu::handleTimer(YTimer *timer) {
    if (timer == fMenuTimer) {
        activatedX = fTimerX;
        activatedY = fTimerY;
        activateSubMenu(fTimerSubmenuItem, true);
    }
    return false;
}

bool YMenu::handleAutoScroll(const XMotionEvent & /*mouse*/) {
    int px = x();
    int py = y();

    int dx, dy, dw, dh;
    desktop->getScreenGeometry(&dx, &dy, &dw, &dh, getXiScreen());

    if (fAutoScrollDeltaX != 0) {
        if (fAutoScrollDeltaX < 0) {
            if (px + width() > dx + dw)
                px += fAutoScrollDeltaX + 1;
        } else {
            if (px < dx)
                px += fAutoScrollDeltaX + 1;
        }
    }
    if (fAutoScrollDeltaY != 0) {
        if (fAutoScrollDeltaY < 0) {
            if (py + height() > dy + dh)
                py += fAutoScrollDeltaY + 1;
        } else {
            if (py < dy)
                py += fAutoScrollDeltaY + 1;
        }
    }
    if (px != x() || py != y()) {
        setPosition(px, py);
        {
            int mx = fAutoScrollMouseX - x();
            int my = fAutoScrollMouseY - y();
    
            int selItem = findItem(mx, my);
            focusItem(selItem);
        }
    }
    return true;
}

void YMenu::autoScroll(int deltaX, int deltaY, int mx, int my, const XMotionEvent *motion) {
    fAutoScrollDeltaX = deltaX;
    fAutoScrollDeltaY = deltaY;
    fAutoScrollMouseX = mx;
    fAutoScrollMouseY = my;
    beginAutoScroll(deltaX || deltaY, motion);
}

YMenuItem *YMenu::addItem(const ustring &name, int hotCharPos, const ustring &param, YAction *action) {
    return add(new YMenuItem(name, hotCharPos, param, action, 0));
}

YMenuItem *YMenu::addItem(const ustring &name, int hotCharPos, YAction *action, YMenu *submenu) {
    return add(new YMenuItem(name, hotCharPos, null, action, submenu));
}

YMenuItem *YMenu::addSubmenu(const ustring &name, int hotCharPos, YMenu *submenu) {
    return add(new YMenuItem(name, hotCharPos, null, 0, submenu));
}

YMenuItem * YMenu::addSeparator() {
    return add(new YMenuItem());
}

YMenuItem *YMenu::addLabel(const ustring &name) {
    return add(new YMenuItem(name));
}

void YMenu::removeAll() {
    hideSubmenu();
    fItems.clear();
    paintedItem = selectedItem = -1;
}

YMenuItem * YMenu::add(YMenuItem *item) {
    if (item) fItems.append(item);
    return item;
}

YMenuItem * YMenu::addSorted(YMenuItem *item, bool duplicates) {
    for (int i = 0; i < itemCount(); i++) {
        if (item->getName() == null || fItems[i]->getName() == null)
            continue;

        int cmp = item->getName().compareTo(fItems[i]->getName());
        if (cmp > 0)
            continue;
        else if (cmp != 0 || duplicates) {
            fItems.insert(i, item);
            return item;
        } else {
            return 0;
        }
    }
    if (item) fItems.append(item);
    return item;
}

YMenuItem *YMenu::findAction(const YAction *action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == getItem(i)->getAction()) return getItem(i);
    return 0;
}

YMenuItem *YMenu::findSubmenu(const YMenu *sub) {
    for (int i = 0; i < itemCount(); i++)
        if (sub == getItem(i)->getSubmenu()) return getItem(i);
    return 0;
}

YMenuItem *YMenu::findName(const ustring &name, const int first) {
    if (name != null)
        for (int i = first; i < itemCount(); i++) {
            ustring iname = getItem(i)->getName();
            if (iname != null && iname.equals(name))
                return getItem(i);
    }

    return 0;
}

int YMenu::findFirstLetRef(char firstLet, const int first, const int ignCase) {
    if (ignCase) 
        firstLet = ASCII::toUpper(firstLet);
    for (int i = first; i < itemCount(); i++) {
        YMenuItem *temp = getItem(i);
        ustring iLetterRef = temp->getName();
        if (iLetterRef != null) {
            int iLetter = iLetterRef.charAt(0);
            if (ignCase) 
                iLetter = ASCII::toUpper(iLetter);
            if (iLetter == firstLet)
                return i;
        }
    }
    return -1;
}


void YMenu::enableCommand(YAction *action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == 0 || action == getItem(i)->getAction())
            getItem(i)->setEnabled(true);
    }

void YMenu::disableCommand(YAction *action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == 0 || action == getItem(i)->getAction())
            getItem(i)->setEnabled(false);
    }

void YMenu::getOffsets(int &left, int &top, int &right, int &bottom) {
    if (wmLook == lookMetal || wmLook == lookFlat) {
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

int YMenu::findItemPos(int itemNo, int &x, int &y, int &ih) {
    x = -1;
    y = -1;
    ih = 0;

    if (itemNo < 0 || itemNo > itemCount())
        return -1;

    int w, h;
    int top, bottom, pad;

    getArea(x, y, w, h);
    for (int i = 0; i < itemNo; i++) {
        y += getItem(i)->queryHeight(top, bottom, pad);
    }
    if (itemNo < itemCount())
        ih = getItem(itemNo)->queryHeight(top, bottom, pad);

    return 0;
}

int YMenu::findItem(int mx, int my) {
    int x, y, w, h;

    getArea(x, y, w, h);
    for (int i = 0; i < itemCount(); i++) {
        int top, bottom, pad;

        h = fItems[i]->queryHeight(top, bottom, pad);

        if (my >= y && my < y + h && mx > 0 && mx < int(width()) - 1) {
            if (!fItems[i]->isSeparator())
                return i;
            else
                return -1;
        }

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
    int dx, dy, dw, dh;
    desktop->getScreenGeometry(&dx, &dy, &dw, &dh, getXiScreen());

    width = l;
    height = t;

    for (int i = 0; i < itemCount(); i++) {
        const YMenuItem *mitem = getItem(i);

        int ih, top, bottom, pad;
        height+= (ih = mitem->queryHeight(top, bottom, pad));

        if (pad > padx) padx = pad;
        if (top > left) left = top;

        maxIcon = max(maxIcon, mitem->getIconWidth());
        maxName = max(maxName, mitem->getNameWidth());
        maxParam = max(maxParam, mitem->getParamWidth() +
                                (mitem->getSubmenu() ? 2 + ih : 0));
    }

    maxName = min(maxName, MenuMaximalWidth ? MenuMaximalWidth : dw * 2/3);

    hspace -= 4 + r + maxIcon + l + left + padx + 2 + maxParam + 6 + 2;
    hspace = max(hspace, dw / 3);

    // !!! not correct, to be fixed
    if (maxName > hspace)
        maxName = hspace;

    namePos = l + left + padx + maxIcon + 2;
    paramPos = namePos + 2 + maxName + 6;
    width = paramPos + maxParam + 4 + r;
    height += b;

#ifdef CONFIG_GRADIENTS
    if (menubackPixbuf != null
        && !(fGradient != null &&
             fGradient->width() == width &&
             fGradient->height() == height)) {
        fGradient = menubackPixbuf->scale(width, height);
    }
#endif

    setSize(width, height);
}

void YMenu::repaintItem(int item) {
    int x, y, h;

    if (findItemPos(item, x, y, h) != -1)
        XClearArea(xapp->display(), handle(), 0, y, width(), h, True);
}

void YMenu::paintItems() {
    int highlightItem = selectedItem;
    if (highlightItem == -1)
        highlightItem = submenuItem;

    if (paintedItem != highlightItem) {
        int oldPaintedItem = paintedItem;
        paintedItem = highlightItem;
        if (oldPaintedItem != -1)
            repaintItem(oldPaintedItem);
        if (paintedItem != -1)
            repaintItem(paintedItem);
    }
}

void YMenu::drawBackground(Graphics &g, int x, int y, int w, int h) {
#ifdef CONFIG_GRADIENTS
    if (fGradient != null)
        g.drawImage(fGradient, x, y, w, h, x, y);
    else
#endif
    if (menubackPixmap != null)
        g.fillPixmap(menubackPixmap, x, y, w, h);
    else
        g.fillRect(x, y, w, h);
}

void YMenu::drawSeparator(Graphics &g, int x, int y, int w) {
    g.setColor(menuBg);

#ifdef CONFIG_GRADIENTS
    if (menusepPixbuf != null) {
        drawBackground(g, x, y, w, 2 - menusepPixbuf->height()/2);

        g.drawGradient(menusepPixbuf,
                       x, y + 2 - menusepPixbuf->height()/2,
                       w, menusepPixbuf->height());

        drawBackground(g, x, y + 2 + (menusepPixbuf->height()+1)/2,
                       w, 2 - (menusepPixbuf->height()+1)/2);
    } else
#endif
    if (menusepPixmap != null) {
        drawBackground(g, x, y, w, 2 - menusepPixmap->height()/2);

        g.fillPixmap(menusepPixmap,
                     x, y + 2 - menusepPixmap->height()/2,
                     w, menusepPixmap->height());

        drawBackground(g, x, y + 2 + (menusepPixmap->height()+1)/2,
                       w, 2 - (menusepPixmap->height()+1)/2);
    } else if (wmLook == lookMetal || wmLook == lookFlat) {
        drawBackground(g, x, y + 0, w, 1);

        if (activeMenuItemBg)
            g.setColor(activeMenuItemBg);

        g.drawLine(x, y + 1, w, y + 1);;
        g.setColor(menuBg->brighter());
        g.drawLine(x, y + 2, w, y + 2);;
        g.drawLine(x, y, x, y + 2);
    } else {
        drawBackground(g, x, y + 0, w, 1);

        g.setColor(menuBg->darker());
        g.drawLine(x, y + 1, w, y + 1);
        g.setColor(menuBg->brighter());
        g.drawLine(x, y + 2, w, y + 2);
        g.setColor(menuBg);

        drawBackground(g, x, y + 3, w, 1);
    }
}

void YMenu::paintItem(Graphics &g, const int i, const int l, const int t, const int r,
                      const int minY, const int maxY, bool draw) {
    int const fontHeight(menuFont->height() + 1);
    int const fontBaseLine(menuFont->ascent());

    YMenuItem *mitem = getItem(i);
    ustring name = mitem->getName();
    ustring param = mitem->getParam();

    if (mitem->getName() == null && mitem->getSubmenu() == 0) {
        if (draw && t + 4 >= minY && t <= maxY)
            drawSeparator(g, 1, t, width() - 2);
    } else {
        bool const active(i == paintedItem &&
                          (mitem->getAction() || mitem->getSubmenu()));

        int eh, top, bottom, pad, ih;
        eh = getItem(i)->queryHeight(top, bottom, pad);
        ih = eh - top - bottom - pad - pad;

        if (t + eh >= minY && t <= maxY) {
            int const cascadePos(width() - r - 2 - ih - pad);
            g.setColor((active && activeMenuItemBg) ? activeMenuItemBg : menuBg);

            if (draw) {
                if (active) {
#ifdef CONFIG_GRADIENTS
                    if (menuselPixbuf != null) {
                        g.drawGradient(menuselPixbuf, l, t, width() - r - l, eh);
                    } else
#endif
                        if (menuselPixmap != null) {
                            g.fillPixmap(menuselPixmap, l, t, width() - r - l, eh);
                        } else if (activeMenuItemBg) {
                            g.setColor(activeMenuItemBg);
                            g.fillRect(l, t, width() - r - l, eh);
                        } else {
                            g.setColor(menuBg);
                            drawBackground(g, l, t, width() - r - l, eh);
                        }
                } else {
                    g.setColor(menuBg);
                    drawBackground(g, l, t, width() - r - l, eh);
                }

                if ((wmLook == lookMetal || wmLook == lookFlat) && i != selectedItem) {
                    g.setColor(menuBg->brighter());
                    g.drawLine(1, t, 1, t + eh - 1);
                    g.setColor(menuBg);
                }

                if (wmLook != lookWin95 && wmLook != lookWarp4 && active) {
#ifdef OLDMOTIF
                    bool raised(wmLook == lookMotif);
#else
                    bool raised(false);
#endif
                    g.setColor(menuBg);
                    if (wmLook == lookGtk) {
                        g.drawBorderW(l, t, width() - r - l - 1, eh - 1, true);
                    } else if (wmLook == lookMetal || wmLook == lookFlat) {
                        g.setColor((activeMenuItemBg ? activeMenuItemBg
                                    : menuBg)->darker());
                        g.drawLine(l, t, width() - r - l, t);
                        g.setColor((activeMenuItemBg ? activeMenuItemBg
                                    : menuBg)->brighter());
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
                    wmLook == lookMetal || wmLook == lookFlat)
                    delta = 0;
                int baseLine = t + top + pad + (ih - fontHeight) / 2 + fontBaseLine + delta;

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
                } else if (mitem->getIcon() != null) {
#ifndef LITE
                    mitem->getIcon()->draw(g,
                               l + 1 + delta, t + delta + top + pad +
                               (eh - top - pad * 2 - bottom -
                                YIcon::smallSize()) / 2,
                                YIcon::smallSize());
#endif
                }

                if (name != null) {
                    int const maxWidth =
                        (param != null ? paramPos - delta
                         : mitem->getSubmenu() ? cascadePos : width()) -
                        namePos;

                    if (!mitem->isEnabled()) {
                        g.setColor(disabledMenuItemSt);
                        g.drawStringEllipsis(1 + delta + namePos, 1 + baseLine,
                                             name, maxWidth);

                        if (mitem->getHotCharPos() != -1)
                            g.drawCharUnderline(1 + delta +  namePos, 1 + baseLine,
                                                name, mitem->getHotCharPos());
                    }
                    g.setColor(fg);
                    g.drawStringEllipsis(delta + namePos, baseLine, name, maxWidth);

                    if (mitem->getHotCharPos() != -1)
                        g.drawCharUnderline(delta + namePos, baseLine,
                                            name, mitem->getHotCharPos());
                }

                if (param != null) {
                    if (!mitem->isEnabled()) {
                        g.setColor(disabledMenuItemSt);
                        g.drawChars(param,
                                    paramPos + delta + 1,
                                    baseLine + 1);
                    }
                    g.setColor(fg);
                    g.drawChars(param,
                                paramPos + delta, baseLine);
                } else if (mitem->getSubmenu() != 0) {
                    if (mitem->getAction()) {
                        g.setColor(menuBg);
                        if (0) {
                            drawBackground(g, width() - r - 1 -ih - pad, t + top + pad, ih, ih);
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
                        delta = (delta && active ? 1 : 0);
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

                        g.drawArrow(Right, ax, ay, asize);
                    }
                }
            }
        }
    }
}

void YMenu::paint(Graphics &g, const YRect &r1) {
    if (wmLook == lookMetal || wmLook == lookFlat) {
        g.setColor(activeMenuItemBg ? activeMenuItemBg : menuBg);
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
    int x, y, w, h;
    getArea(x, y, w, h);
    int iy = y;
    int top, bottom, pad;

    for (int i = 0; i < itemCount(); i++) {
        int ih = getItem(i)->queryHeight(top, bottom, pad);

        if (iy < r1.y() + r1.height() &&
            iy + ih > r1.y())
        {
            paintItem(g, i, l, iy, r, r1.y(), r1.y() + r1.height(), 1);
        }
        iy += ih;
    }
}

void YMenu::hideSubmenu() {
    if (fPopup)
        fPopup->popdown();
    fPopup = 0;
    fPopupActive = 0;
    submenuItem = -1;
}
