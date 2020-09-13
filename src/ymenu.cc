/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "yxapp.h"
#include "prefs.h"
#include "yprefs.h"
#include "ascii.h"
#include <string.h>

static YColorName menuBg(&clrNormalMenu);
static YColorName menuItemFg(&clrNormalMenuItemText);
static YColorName activeMenuItemBg(&clrActiveMenuItem);
static YColorName activeMenuItemFg(&clrActiveMenuItemText);
static YColorName disabledMenuItemFg(&clrDisabledMenuItemText);
static YColorName disabledMenuItemSt(&clrDisabledMenuItemShadow);

ref<YFont> menuFont;

ref<YPixmap> menusepPixmap;
ref<YPixmap> menuselPixmap;
ref<YPixmap> menubackPixmap;

ref<YImage> menuselPixbuf;
ref<YImage> menusepPixbuf;
ref<YImage> menubackPixbuf;

int YMenu::fAutoScrollDeltaX = 0;
int YMenu::fAutoScrollDeltaY = 0;
int YMenu::fAutoScrollMouseX = -1;
int YMenu::fAutoScrollMouseY = -1;
int YMenu::fMenuObjectCount;
YMenu *YMenu::fPointedMenu = nullptr;

void YMenu::setActionListener(YActionListener *actionListener) {
    fActionListener = actionListener;
}

void YMenu::finishPopup(YMenuItem *item, YAction action,
                        unsigned int modifiers)
{
    YActionListener *cmd = fActionListener;

    YPopupWindow::finishPopup();

    if (item)
        item->actionPerformed(cmd, action, modifiers);
}

lazy<YTimer> YMenu::fMenuTimer;

YMenu::YMenu(YWindow *parent):
    YPopupWindow(parent),
    fGraphics(this),
    fGradient(null),
    fMenusel(null)
{
    paintedItem = selectedItem = -1;
    submenuItem = -1;
    fPopup = nullptr;
    fActionListener = nullptr;
    fPopupActive = nullptr;
    fShared = false;
    fRaised = false;
    activatedX = -1;
    activatedY = -1;
    fTimerX = 0;
    fTimerY = 0;
    fTimerSubmenuItem = -1;
    addStyle(wsNoExpose);

    if (menuFont == null)
        menuFont = YFont::getFont(XFA(menuFontName));
    ++fMenuObjectCount;
}

void YMenu::raise() {
    if (fRaised == false) {
        fRaised = true;
        if (visible() == false && parent() == desktop) {
            setTitle("IceMenu");
            setClassHint("icemenu", "IceWM");
            setNetWindowType(_XA_NET_WM_WINDOW_TYPE_POPUP_MENU);
        }
    }
    YPopupWindow::raise();
}

YMenu::~YMenu() {
    if (fMenuTimer)
        fMenuTimer->disableTimerListener(this);
    hideSubmenu();
    fGradient = null;
    fMenusel = null;
    removeAll();
    if (--fMenuObjectCount == 0)
        menuFont = null;
}

void YMenu::activatePopup(int flags) {
    repaint();
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
        fPointedMenu = nullptr;
    if (fMenuTimer)
        fMenuTimer->disableTimerListener(this);
}

void YMenu::donePopup(YPopupWindow *popup) {
    PRECONDITION(popup != 0);
    PRECONDITION(fPopup != 0);
    if (fPointedMenu == this)
        fPointedMenu = nullptr;
    if (fPopup) {
        hideSubmenu();
        if (selectedItem != -1)
            if (getItem(selectedItem)->getSubmenu() == popup)
                paintItems();
    }
}

bool YMenu::isCondCascade(int selItem) {
    if (selItem != -1 &&
        getItem(selItem)->getAction() != actionNull &&
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

        unsigned fontHeight = menuFont->height() + 1;
        unsigned h = fontHeight;

        if (getItem(selItem)->getIcon() != null &&
            YIcon::menuSize() > h)
            h = YIcon::menuSize();

        if (x <= int(width() - h - 4))
            return 1;
    }
    return 0;
}

void YMenu::focusItem(int itemNo) {
    if (itemNo != selectedItem) {
        selectedItem = itemNo;

        int dx, dy;
        unsigned uw, uh;
        desktop->getScreenGeometry(&dx, &dy, &uw, &uh, getXiScreen());
        const int dw = int(uw), dh = int(uh);

        if (selectedItem != -1) {
            if (x() < dx || y() < dy ||
                x() + int(width()) > dx + dw ||
                y() + int(height()) > dy + dh)
            {
                int ix, iy;
                unsigned uh;

                findItemPos(selectedItem, ix, iy, uh);
                const int ih = int(uh);

                int ny = y();
                if (ny + iy + ih > dy + dh)
                    ny = dx + dh - ih - iy;
                else if (ny + iy < dy)
                    ny = -iy;
                setPosition(x(), ny);
            }
        }
    }
    paintItems();
}

void YMenu::activateSubMenu(int item, bool byMouse) {
    YMenu *sub = nullptr;
    if (item != -1 && getItem(item)->isEnabled())
        sub = getItem(item)->getSubmenu();

    if (sub != fPopup) {
        hideSubmenu();

        if (sub) {
            int xp, yp;
            unsigned ih;
            int l, t, r, b;

            getOffsets(l, t, r, b);
            findItemPos(item, xp, yp, ih);
            YRect rect(x(), y(), width(), height());
            if (sub->getActionListener() == nullptr)
                sub->setActionListener(getActionListener());
            sub->popup(nullptr, this, nullptr,
                       x() + int(width()) - r, y() + yp - t,
                       int(width()) - r - l, -1,
                       getXiScreen(),
                       YPopupWindow::pfCanFlipHorizontal |
                       (popupFlags() & YPopupWindow::pfFlipHorizontal) |
                       (byMouse ? (unsigned int)YPopupWindow::pfButtonDown : 0U));
            fPopup = sub;
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
    } while (c != cur && (getItem(c)->getAction() == actionNull &&
                          !getItem(c)->getSubmenu()));
    return c;
}

int YMenu::activateItem(int modifiers, bool byMouse) {
    PRECONDITION(selectedItem != -1);

    YMenuItem *item = getItem(selectedItem);
    if (item->isEnabled()) {
        if (item->getAction() != actionNull) {
            finishPopup(item, item->getAction(), modifiers);
        }
        else if (item->getSubmenu() != nullptr) {
            focusItem(selectedItem);
            activateSubMenu(selectedItem, byMouse);
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
        if (mitem->getAction() != actionNull ||
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
        if (mitem->getAction() != actionNull ||
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
    KeySym k = keyCodeToKeySym(key.keycode);
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
                else if (k == XK_Page_Up || k == XK_KP_Page_Up) {
                    int item = selectedItem;
                    while (0 < item && selectedItem < item + 10) {
                        int found = findActiveItem(item, -1);
                        if (0 <= found && found < item)
                            item = found;
                        else
                            break;
                    }
                    if (inrange(item, 0, selectedItem - 1))
                        focusItem(item);
                }
                else if (k == XK_Page_Down || k == XK_KP_Page_Down) {
                    int item = selectedItem;
                    while (item + 1 < itemCount() && item < selectedItem + 10) {
                        int found = findActiveItem(item, +1);
                        if (found < itemCount() && item < found)
                            item = found;
                        else
                            break;
                    }
                    if (inrange(item, selectedItem + 1, itemCount() - 1))
                        focusItem(item);
                }
                else if (k == XK_Home || k == XK_KP_Home)
                    focusItem(findActiveItem(itemCount() - 1, 1));
                else if (k == XK_End || k == XK_KP_End)
                    focusItem(findActiveItem(0, -1));
                else if (k == XK_Right || k == XK_KP_Right) {
                    focusItem(selectedItem);
                    activateSubMenu(selectedItem, false);
                } else if (k == XK_Return || k == XK_KP_Enter) {
                    if (selectedItem != -1 &&
                        (getItem(selectedItem)->getAction() != actionNull ||
                         getItem(selectedItem)->getSubmenu() != nullptr))
                    {
                        activateItem(key.state, false);
                        return true;
                    }
                } else if (k < 256) {
                    if (findHotItem(ASCII::toUpper((char)k)) == 1) {
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
        if (button.type == ButtonPress && itemCount() > 0) {
            if (inrange(button.x_root, x(), x() + int(width()) - 1)) {
                const int itemHeight = height() / itemCount();
                const int stepSize = max(menuFont->height(), itemHeight);
                hideSubmenu();
                setPosition(x(), clamp(y() - (int)(button.state & ShiftMask ?
                                                   3 * stepSize : stepSize),
                                       button.y_root - (int)height() + 1,
                                       button.y_root));
                if (menuMouseTracking)
                    trackMotion(clamp(button.x_root, x() + 2, x() + (int)width() - 3),
                                button.y_root, button.state, true);
                focusItem(findItem(button.x_root - x(),
                                   button.y_root - y()));
            }
        }
    } else if (button.button == Button4) {
        if (button.type == ButtonPress && itemCount() > 0) {
            if (inrange(button.x_root, x(), x() + int(width()) - 1)) {
                const int itemHeight = height() / itemCount();
                const int stepSize = max(menuFont->height(), itemHeight);
                hideSubmenu();
                setPosition(x(), clamp(y() + (int)(button.state & ShiftMask ?
                                                   3 * stepSize : stepSize),
                                       button.y_root - (int)height() + 1,
                                       button.y_root));
                if (menuMouseTracking)
                    trackMotion(clamp(button.x_root, x() + 2, x() + (int)width() - 3),
                                button.y_root, button.state, true);
                focusItem(findItem(button.x_root - x(),
                                   button.y_root - y()));
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
            if (fPopupActive == fPopup && fPopup != nullptr && nocascade) {
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
                getItem(selectedItem)->getAction() == actionNull &&
                getItem(selectedItem)->getSubmenu() == nullptr;
            }

            if (selectedItem == -1 || noAction) {
                if (menuMouseTracking)
                    focusItem(-1);
                else
                    focusItem(findActiveItem(itemCount() - 1, 1));
            } else {
                if ((getItem(selectedItem)->getAction() != actionNull ||
                     getItem(selectedItem)->getSubmenu() != nullptr)
                    &&
                    (getItem(selectedItem)->getAction() == actionNull ||
                     getItem(selectedItem)->getSubmenu() == nullptr || !nocascade)
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

            int dx, dy;
            unsigned uw, uh;
            desktop->getScreenGeometry(&dx, &dy, &uw, &uh, getXiScreen());
            const int dw = int(uw), dh = int(uh);

            int const sx(motion.x_root < fh ? +fh :
                         motion.x_root >= (dx + dw - fh - 1) ? -fh :
                         0),
                      sy(motion.y_root < fh ? +fh :
                         motion.y_root >= (dy + dh - fh - 1) ? -fh :
                         0);

            if (motion.y_root >= y() && motion.y_root < (y() + (int) height()) &&
                motion.x_root >= x() && motion.x_root < (x() + (int) width()))
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
        (selItem != fTimerSubmenuItem)) /// ok?
    {
        fMenuTimer->disableTimerListener(this);
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

            fTimerX = x_root;
            fTimerY = y_root;
            fTimerSubmenuItem = submenu ? selectedItem : -1;
            long delay = canFast ? MenuActivateDelay : SubmenuActivateDelay;
            fMenuTimer->setTimer(delay, this, true);
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

    int dx, dy;
    unsigned uw, uh;
    desktop->getScreenGeometry(&dx, &dy, &uw, &uh, getXiScreen());
    const int dw = int(uw), dh = int(uh);

    if (fAutoScrollDeltaX != 0) {
        if (fAutoScrollDeltaX < 0) {
            if (px + int(width()) > dx + dw)
                px += fAutoScrollDeltaX + 1;
        } else {
            if (px < dx)
                px += fAutoScrollDeltaX + 1;
        }
    }
    if (fAutoScrollDeltaY != 0) {
        if (fAutoScrollDeltaY < 0) {
            if (py + int(height()) > dy + dh)
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

YMenuItem *YMenu::addItem(const mstring &name, int hotCharPos, const mstring &param, YAction action, const char *icons) {
    return add(new YMenuItem(name, hotCharPos, param, action, nullptr), icons);
}

YMenuItem *YMenu::addItem(const mstring &name, int hotCharPos, YAction action, YMenu *submenu, const char *icons) {
    return add(new YMenuItem(name, hotCharPos, null, action, submenu), icons);
}

YMenuItem *YMenu::addSubmenu(const mstring &name, int hotCharPos, YMenu *submenu, const char *icons) {
    return add(new YMenuItem(name, hotCharPos, null, actionNull, submenu), icons);
}

YMenuItem *YMenu::addItem(const mstring &name, int hotCharPos, const mstring &param, YAction action) {
    return add(new YMenuItem(name, hotCharPos, param, action, nullptr));
}

YMenuItem *YMenu::addItem(const mstring &name, int hotCharPos, YAction action, YMenu *submenu) {
    return add(new YMenuItem(name, hotCharPos, null, action, submenu));
}

YMenuItem *YMenu::addSubmenu(const mstring &name, int hotCharPos, YMenu *submenu) {
    return add(new YMenuItem(name, hotCharPos, null, actionNull, submenu));
}

YMenuItem * YMenu::addSeparator() {
    return add(new YMenuItem());
}

YMenuItem *YMenu::addLabel(const mstring &name) {
    return add(new YMenuItem(name));
}

void YMenu::removeAll() {
    hideSubmenu();
    const int n = itemCount();
    for (int i = 0; i < n; ++i) {
        YMenu* sub = fItems[i]->getSubmenu();
        if (sub && sub->isShared()) {
            fItems[i]->setSubmenu(nullptr);
        }
    }
    fItems.clear();
    // paintedItem = selectedItem = -1;
}

YMenuItem * YMenu::add(YMenuItem *item) {
    if (item) fItems.append(item);
    return item;
}

YMenuItem * YMenu::add(YMenuItem *item, const char *icons) {
    if (item) {
        ref<YIcon> icon(YIcon::getIcon(icons));
        if (icon != null && icon->isCached()) {
            item->setIcon(icon);
        }
        fItems.append(item);
    }
    return item;
}

YMenuItem * YMenu::addSorted(YMenuItem *item, bool duplicates, bool ignoreCase) {
    for (int i = 0; i < itemCount(); i++) {
        if (item->getName() == null || fItems[i]->getName() == null)
            continue;

        int cmp = item->getName().collate(fItems[i]->getName(), ignoreCase);
        if (cmp > 0)
            continue;
        else if (cmp != 0 || duplicates) {
            fItems.insert(i, item);
            return item;
        } else {
            return nullptr;
        }
    }
    if (item) fItems.append(item);
    return item;
}

YMenuItem *YMenu::findAction(YAction action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == getItem(i)->getAction()) return getItem(i);
    return nullptr;
}

YMenuItem *YMenu::findSubmenu(const YMenu *sub) {
    for (int i = 0; i < itemCount(); i++)
        if (sub == getItem(i)->getSubmenu()) return getItem(i);
    return nullptr;
}

YMenuItem *YMenu::findName(const mstring &name, const int first) {
    if (name != null)
        for (int i = first; i < itemCount(); i++) {
            mstring iname = getItem(i)->getName();
            if (iname != null && iname.equals(name))
                return getItem(i);
    }

    return nullptr;
}

int YMenu::findFirstLetRef(char firstLet, const int first, const int ignCase) {
    if (ignCase)
        firstLet = ASCII::toUpper(firstLet);
    for (int i = first; i < itemCount(); i++) {
        YMenuItem *temp = getItem(i);
        mstring iLetterRef = temp->getName();
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


void YMenu::enableCommand(YAction action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == actionNull || action == getItem(i)->getAction())
            getItem(i)->setEnabled(true);
    }

void YMenu::disableCommand(YAction action) {
    for (int i = 0; i < itemCount(); i++)
        if (action == actionNull || action == getItem(i)->getAction())
            getItem(i)->setEnabled(false);
    }

void YMenu::checkCommand(YAction action, bool check) {
    for (int i = 0; i < itemCount(); i++)
        if (action == actionNull || action == getItem(i)->getAction())
            getItem(i)->setChecked(check);
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

void YMenu::getArea(int &x, int &y, unsigned &w, unsigned &h) {
    int right = 0, bottom = 0;
    getOffsets(x, y, right, bottom);
    w = int(width()) - 1 - x - right;
    h = int(height()) - 1 - y - bottom;
}

int YMenu::findItemPos(int itemNo, int &x, int &y, unsigned &ih) {
    x = -1;
    y = -1;
    ih = 0;

    if (itemNo < 0 || itemNo > itemCount())
        return -1;

    unsigned w, h;
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
    int x, y;
    unsigned w, h;

    getArea(x, y, w, h);
    for (int i = 0; i < itemCount(); i++, y += int(h)) {
        int top, bottom, pad;

        h = fItems[i]->queryHeight(top, bottom, pad);

        if (inrange(my, y, y + int(h)) && inrange(mx, 1, int(width()) - 1)) {
            if (!fItems[i]->isSeparator())
                return i;
            else
                return -1;
        }
    }

    return -1;
}

void YMenu::sizePopup(int hspace) {
    int maxName(0);
    int maxParam(0);
    int maxIcon(16);
    int l, t, r, b;
    int padx(1);
    int left(1);

    getOffsets(l, t, r, b);
    int dx, dy;
    unsigned uw, uh;
    desktop->getScreenGeometry(&dx, &dy, &uw, &uh, getXiScreen());
    int dw = int(uw);

    int height = t;

    for (int i = 0; i < itemCount(); i++) {
        const YMenuItem *mitem = getItem(i);

        int top, bottom, pad;
        int ih = mitem->queryHeight(top, bottom, pad);

        if (height + ih >= 16000) {
            // evade library bug
            TLOG(("truncating menu to %d items at height %d", i, height));
            fItems.shrink(i);
            break;
        }

        height += ih;

        if (pad > padx) padx = pad;
        if (top > left) left = top;

        maxIcon = max(maxIcon, mitem->getIconWidth());
        maxName = max(maxName, mitem->getNameWidth());
        maxParam = max(maxParam, mitem->getParamWidth() +
                                (mitem->getSubmenu() ? 2 + ih : 0));
    }

    maxName = min(maxName, int(MenuMaximalWidth ? MenuMaximalWidth : dw * 2/3));

    hspace -= 4 + r + maxIcon + l + left + padx + 2 + maxParam + 6 + 2;
    hspace = max(hspace, int(dw / 3));

    // !!! not correct, to be fixed
    if (maxName > hspace)
        maxName = hspace;

    namePos = l + left + padx + maxIcon + 2;
    paramPos = namePos + 2 + maxName + 6;
    int width = paramPos + maxParam + 4 + r + 10;
    height += b;

    if (menubackPixbuf != null) {
        if (fGradient == null ||
            int(fGradient->width()) != width ||
            int(fGradient->height()) != height)
        {
            ref<YImage> scaled = menubackPixbuf->scale(width, height);
            if (scaled != null) {
                fGradient = scaled->renderToPixmap(depth());
            }
        }
    }

    setSize(unsigned(width), unsigned(height));
}

void YMenu::repaintItem(int item) {
    int x, y;
    unsigned h;

    if (findItemPos(item, x, y, h) != -1)
        fGraphics.paint(YRect(0, y, width(), h));
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
    if (w < 1 || h < 1)
        return;

    PRECONDITION(w < SHRT_MAX && h < SHRT_MAX);
    g.clearArea(x, y, w, h);
    if (fGradient != null)
        g.copyPixmap(fGradient, x, y, w, h, x, y);
    else
    if (menubackPixmap != null)
        g.fillPixmap(menubackPixmap, x, y, w, h);
    else {
        g.setColor(menuBg);
        g.fillRect(x, y, w, h);
    }
}

void YMenu::drawSeparator(Graphics &g, int x, int y, unsigned w) {
    g.setColor(menuBg);

    if (menusepPixbuf != null) {
        drawBackground(g, x, y, w, 2 - int(menusepPixbuf->height()/2));

        g.drawGradient(menusepPixbuf,
                       x, y + 2 - int(menusepPixbuf->height()/2),
                       w, menusepPixbuf->height());

        drawBackground(g, x, y + 2 + int(menusepPixbuf->height()+1)/2,
                       w, 2 - int(menusepPixbuf->height()+1)/2);
    } else
    if (menusepPixmap != null) {
        drawBackground(g, x, y, w, 2 - int(menusepPixmap->height()/2));

        g.fillPixmap(menusepPixmap,
                     x, y + 2 - int(menusepPixmap->height()/2),
                     w, menusepPixmap->height());

        drawBackground(g, x, y + 2 + int(menusepPixmap->height()+1)/2,
                       w, 2 - int(menusepPixmap->height()+1)/2);
    } else if (wmLook == lookMetal || wmLook == lookFlat) {
        drawBackground(g, x, y + 0, w, 1);

        if (activeMenuItemBg)
            g.setColor(activeMenuItemBg);

        g.drawLine(x, y + 1, w, y + 1);
        g.setColor(menuBg->brighter());
        g.drawLine(x, y + 2, w, y + 2);
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
    mstring name = mitem->getName();
    mstring param = mitem->getParam();

    if (mitem->getName() == null && mitem->getSubmenu() == nullptr) {
        if (draw && t + 4 >= minY && t <= maxY)
            drawSeparator(g, 1, t, width() - 2);
    } else {
        bool const active(i == paintedItem &&
                          (mitem->getAction() != actionNull ||
                           mitem->getSubmenu()));

        int eh, top, bottom, pad, ih;
        eh = getItem(i)->queryHeight(top, bottom, pad);
        ih = eh - top - bottom - pad - pad;

        if (t + eh >= minY && t <= maxY) {
            int const cascadePos(width() - r - 2 - ih - pad);
            g.setColor((active && activeMenuItemBg) ? activeMenuItemBg : menuBg);

            if (draw) {
                if (active) {
                    if (menuselPixbuf != null) {
                        int ew = width() - r - l;
                        if (fMenusel == null ||
                            fMenusel->width() != unsigned(ew) ||
                            fMenusel->height() != unsigned(eh))
                        {
                            ref<YImage> im = menuselPixbuf->scale(ew, eh);
                            if (im != null) {
                                fMenusel = im->renderToPixmap(depth());
                            }
                        }
                        g.copyPixmap(fMenusel, 0, 0, ew, eh, l, t);
                    }
                    else if (menuselPixmap != null) {
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

                YColor fg(mitem->isEnabled() ? active ? activeMenuItemFg
                           : menuItemFg
                           : disabledMenuItemFg);
                g.setColor(fg);
                g.setFont(menuFont);

                int delta = active;
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
                    int size = YIcon::menuSize();
                    int dx = l + 1 + delta;
                    int dy = t + delta + top + pad +
                               (eh - top - pad * 2 - bottom - size) / 2;
                    mitem->getIcon()->draw(g, dx, dy, size);
                }

                if (name != null) {
                    int const maxWidth =
                        (param != null ? paramPos - delta
                         : mitem->getSubmenu() ? cascadePos : width()) -
                        namePos;

                    if (!mitem->isEnabled()) {
                        g.setColor(disabledMenuItemSt ? disabledMenuItemSt :
                                   menuBg->brighter());
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
                        g.setColor(disabledMenuItemSt ? disabledMenuItemSt :
                                   menuBg->brighter());
                        g.drawChars(param,
                                    paramPos + delta + 1,
                                    baseLine + 1);
                    }
                    g.setColor(fg);
                    g.drawChars(param,
                                paramPos + delta, baseLine);
                } else if (mitem->getSubmenu() != nullptr) {
                    if (mitem->getAction() != actionNull) {
                        g.setColor(menuBg);
                        if (false) {
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
                        delta = (delta && active);
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
    if (g.drawable() == None || int(r1.width()) < 1 || int(r1.height()) < 1)
        return;

    drawBackground(g, r1.x(), r1.y(), r1.width(), r1.height());

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
    int x, y;
    unsigned w, h;
    getArea(x, y, w, h);
    int iy = y;
    int top, bottom, pad;

    for (int i = 0; i < itemCount(); i++) {
        int ih = getItem(i)->queryHeight(top, bottom, pad);

        if (iy < r1.y() + (int) r1.height() &&
            iy + (int) ih > r1.y())
        {
            paintItem(g, i, l, iy, r, r1.y(), r1.y() + r1.height(), true);
        }
        iy += ih;
    }
}

void YMenu::hideSubmenu() {
    if (fPopup)
        fPopup->popdown();
    fPopup = nullptr;
    fPopupActive = nullptr;
    submenuItem = -1;
}

void YMenu::configure(const YRect2& r) {
    if (r.resized()) {
        repaint();
    }
}

void YMenu::repaint() {
    fGraphics.release();
    fGraphics.paint();
}

// vim: set sw=4 ts=4 et:
