/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Alt{+Shift}+Tab window switching
 */
#include "config.h"

#include "yimage.h"
#include "ykey.h"
#include "wmswitch.h"
#include "wpixmaps.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "yxapp.h"
#include "prefs.h"
#include "yrect.h"
#include "yicon.h"
#include "wmwinlist.h"

class WindowItemsCtrlr : public ISwitchItems
{
    int zTarget;
    YArray<YFrameWindow*> zList;
    YWindowManager *fRoot;
    YFrameWindow *fActiveWindow;
    YFrameWindow *fLastWindow;

    void getZList() {

        if (quickSwitchGroupWorkspaces || !quickSwitchToAllWorkspaces) {
            int activeWorkspace = fRoot->activeWorkspace();
            GetZListWorkspace(true, activeWorkspace);
            if (quickSwitchToAllWorkspaces) {
                for (int w = 0; w < workspaceCount; ++w) {
                    if (w != activeWorkspace)
                        GetZListWorkspace(true, w);
                }
            }
        } else
            GetZListWorkspace(false, -1);

        if (fActiveWindow != 0 && find(zList, fActiveWindow) == -1)
            fActiveWindow = 0;
        if (fLastWindow != 0 && find(zList, fLastWindow) == -1)
            fLastWindow = 0;
    }

    void freeList() {
        zList.clear();
    }

    void GetZListWorkspace(bool workspaceOnly, int workspace)
    {
        for (int pass = 0; pass <= 5; pass++) {
            YFrameIter w = fRoot->focusedReverseIterator();

            while (++w) {
                // pass 0: focused window
                // pass 1: urgent windows
                // pass 2: normal windows
                // pass 3: minimized windows
                // pass 4: hidden windows
                // pass 5: unfocusable windows
                if ((w->client() && !w->client()->adopted()) && !w->visible()) {
                    continue;
                }

                if (!w->isUrgent()) {
                    if (workspaceOnly && w->isSticky() &&
                        workspace != fRoot->activeWorkspace()) {
                        continue;
                    }

                    if (workspaceOnly && !w->visibleOn(workspace)) {
                        continue;
                    }
                }

                if (w == fRoot->getFocus()) {
                    if (pass == 0) zList.append(w);
                } else if (w->isUrgent()) {
                    if (quickSwitchToUrgent) {
                        if (pass == 1) zList.append(w);
                    } else {
                        if (pass == 2) zList.append(w);
                    }
                } else if (w->frameOptions() & YFrameWindow::foIgnoreQSwitch) {
                } else if (w->avoidFocus()) {
                    if (pass == 5) zList.append(w);
                } else if (w->isHidden()) {
                    if (pass == 4)
                        if (quickSwitchToHidden)
                            zList.append(w);
                } else if (w->isMinimized()) {
                    if (pass == 3)
                        if (quickSwitchToMinimized)
                            zList.append(w);
                } else {
                    if (pass == 2) zList.append(w);
                }
            }
        }
    }

    void displayFocusChange(YFrameWindow *frame)  {
        manager->switchFocusTo(frame, false);
    }

public:

    virtual int getCount()
    {
        return zList.getCount();
    }

    virtual ref<YIcon> getIcon(int itemIdx) OVERRIDE
    {
        if (inrange(itemIdx, 0, getCount() - 1)) {
            YFrameWindow* winItem = zList[itemIdx];
            if (winItem) return winItem->getIcon();
        }
        return null;
    }


    int moveTarget(bool zdown) OVERRIDE {
        if (getCount() > 1)
            zTarget = (zTarget + getCount() + (zdown ? 1 : -1)) % getCount();
        else
            zTarget = 0;
        if (inrange(zTarget, 0, getCount() - 1))
            fActiveWindow = zList[zTarget];
        else
            fActiveWindow = 0;
        return zTarget;
    }

    WindowItemsCtrlr() :
        zTarget(0), fRoot(manager), fActiveWindow(0), fLastWindow(0)
    {
    }

    ~WindowItemsCtrlr()
    {
    }

    int getActiveItem()
    {
        return zTarget;
    }

    virtual ustring getTitle(int idx)
    {
        if (inrange(idx, 0, getCount() - 1))
            return zList[idx]->client()->windowTitle();
        return null;
    }

    void updateList() {
        freeList();
        getZList();
    }

    void displayFocusChange(int idx) OVERRIDE {
        if (inrange(idx, 0, getCount() - 1))
            displayFocusChange(zList[idx]);
    }

    void begin(bool zdown)
    {
        fLastWindow = fActiveWindow = manager->getFocus();
        updateList();
        zTarget = 0;
        moveTarget(zdown);
    }

    void cancel() {
        if (fLastWindow) {
            displayFocusChange(fLastWindow);
        } else if (fActiveWindow) {
            fRoot->activate(fActiveWindow, false, true);
        }
        freeList();
        fLastWindow = fActiveWindow = 0;
    }

    void accept(IClosablePopup *parent) {
        if (fActiveWindow == 0)
            cancel();
        else {
            fRoot->activate(fActiveWindow, true, true);
            parent->close();
            fActiveWindow->wmRaise();
        }
        freeList();
        fLastWindow = fActiveWindow = 0;
    }

    void destroyedItem(void *item)
    {
        if (getCount() == 0)
            return;

        YFrameWindow* frame = (YFrameWindow*) item;
        if (frame == fLastWindow)
            fLastWindow = 0;
        updateList();
        if (frame == fActiveWindow || fActiveWindow == 0) {
            zTarget = -1;
            moveTarget(true);
        }
        displayFocusChange(fActiveWindow);
    }

    virtual bool isKey(KeySym k, unsigned int vm) OVERRIDE {
        return (IS_WMKEY(k, vm, gKeySysSwitchNext));
    }
};

SwitchWindow::SwitchWindow(YWindow *parent, ISwitchItems *items,
                           bool verticalStyle):
    YPopupWindow(parent),
    fGradient(null),
    switchFg(new YColor(clrQuickSwitchText)),
    switchBg(new YColor(clrQuickSwitch)),
    switchHl(clrQuickSwitchActive ? new YColor(clrQuickSwitchActive) : 0),
    switchMfg(new YColor(clrActiveTitleBarText)),
    switchFont(YFont::getFont(XFA(switchFontName)))
{
    zItems = items ? items : new WindowItemsCtrlr;
    m_verticalStyle = verticalStyle;

    // I prefer clrNormalMenu but some themes use inverted settings where
    // clrNormalMenu is the same as clrQuickSwitch
    if (clrQuickSwitchActive)
        switchMbg = new YColor(clrQuickSwitchActive);
    else if (!strcmp(clrNormalMenu, clrQuickSwitch))
        switchMbg = new YColor(clrActiveMenuItem);
    else
        switchMbg = new YColor(clrNormalMenu);

    modsDown = 0;
    isUp = false;

    //resize(-1);

    setStyle(wsSaveUnder | wsOverrideRedirect);
}

bool SwitchWindow::close() {
    if (isUp) {
        cancelPopup();
        isUp = false;
        return true;
    }
    return false;
}

void SwitchWindow::cancel() {
    close();
    zItems->cancel();
}

void SwitchWindow::accept() {
    zItems->accept(this);
}

SwitchWindow::~SwitchWindow() {
    close();
    fGradient = null;
    delete zItems;
}

void SwitchWindow::resize(int xiscreen) {
    int dx, dy;
    unsigned dw, dh;

    manager->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

    MSG(("got geometry for %d: %d %d %d %d", xiscreen, dx, dy, dw, dh));

    ustring cTitle = zItems->getTitle(zItems->getActiveItem());

    int aWidth =
        quickSwitchSmallWindow ?
        (int) dw * 1/3 : (int) dw * 3/5;

    int tWidth=0;
    if (quickSwitchMaxWidth) {
        int space = (int) switchFont->textWidth(" ");   /* make entries one space character wider */
        int zCount = zItems->getCount();
        for (int i = 0; i < zCount; i++) {
            ustring title = zItems->getTitle(i);
            int oWidth = title != null ? (int) switchFont->textWidth(title) + space : 0;
            if (oWidth > tWidth)
                tWidth = oWidth;
        }
    } else {
        tWidth = cTitle != null ? switchFont->textWidth(cTitle) : 0;
    }

    if (quickSwitchVertical || !quickSwitchAllIcons)
        tWidth += 2 * quickSwitchIMargin + YIcon::largeSize() + 3;
    if (tWidth > aWidth)
        aWidth = tWidth;

    int w = aWidth;
    int h = switchFont->height();
    int const mWidth(dw * 6/7);

    if (quickSwitchVertical) {
        w = aWidth;
        if (w >= mWidth)
            w = mWidth;
        w += quickSwitchSepSize;

        int step = (YIcon::largeSize() + 2 * quickSwitchIMargin);
        int maxHeight = (int) dh - YIcon::largeSize();
        h = zItems->getCount() * step;

        if (h > maxHeight)
            h= maxHeight - (maxHeight % step);
    } else {

        int iWidth =
            zItems->getCount() * (YIcon::largeSize() + 2 * quickSwitchIMargin) +
            (quickSwitchHugeIcon ? YIcon::hugeSize() - YIcon::largeSize() : 0);

        int const iHeight =
            (quickSwitchHugeIcon ?
             YIcon::hugeSize() : YIcon::largeSize()) + quickSwitchIMargin * 2;

        if (iWidth > aWidth)
            aWidth = iWidth;

        if (quickSwitchAllIcons) {
            if (aWidth > w)
                w = aWidth;
        }
        if (w >= mWidth)
            w = mWidth;

        if (quickSwitchAllIcons)
            h += quickSwitchSepSize + iHeight;
        else {
            if (iHeight > h)
                h = iHeight;
        }
    }
    h += quickSwitchVMargin * 2;
    w += quickSwitchHMargin * 2;

    setGeometry(YRect(dx + ((dw - w) >> 1),
                      dy + ((dh - h) >> 1),
                      w, h));
}

void SwitchWindow::paint(Graphics &g, const YRect &/*r*/) {
    if (switchbackPixbuf != null &&
        (fGradient == null ||
         fGradient->width() != width() - 2 ||
         fGradient->height() != height() - 2))
    {
        fGradient = switchbackPixbuf->scale(width() - 2, height() - 2);
    }

    g.setColor(switchBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);

    if (fGradient != null)
        g.drawImage(fGradient, 1, 1, width() - 2, height() - 2, 1, 1);
    else
    if (switchbackPixmap != null)
        g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    else
        g.fillRect(1, 1, width() - 3, height() - 3);

    return m_verticalStyle
        ? paintVertical(g)
        : paintHorizontal(g);
}

void SwitchWindow::paintHorizontal(Graphics &g) {
    if (zItems->getActiveItem() >= 0) {
        int tOfs(0);
        int iconSize = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();

        ref<YIcon> icon;
        if (!quickSwitchAllIcons &&
                (icon = zItems->getIcon(zItems->getActiveItem())) != null) {
            int iconWidth = iconSize, iconHeight = iconSize;

            if (icon != null) {
                if (quickSwitchTextFirst) {
                    icon->draw(g,
                               width() - iconWidth - quickSwitchIMargin,
                               (height() - iconHeight - quickSwitchIMargin) / 2,
                               iconSize);
                } else {
                    icon->draw(g,
                               quickSwitchIMargin,
                               (height() - iconHeight - quickSwitchIMargin) / 2,
                               iconSize);

                    tOfs = iconWidth + quickSwitchIMargin
                        + quickSwitchSepSize;
                }
            }

            if (quickSwitchSepSize) {
                const int ip(iconWidth + 2 * quickSwitchIMargin +
                             quickSwitchSepSize/2);
                const int x(quickSwitchTextFirst ? width() - ip : ip);

                g.setColor(switchBg->darker());
                g.drawLine(x + 0, 1, x + 0, width() - 2);
                g.setColor(switchBg->brighter());
                g.drawLine(x + 1, 1, x + 1, width() - 2);
            }
        }

        g.setColor(switchFg);
        g.setFont(switchFont);

        ustring cTitle = zItems->getTitle(zItems->getActiveItem());
        if (cTitle != null) {
            const int x = max((width() - tOfs -
                               switchFont->textWidth(cTitle)) >> 1, 0U) + tOfs;
            const int y(quickSwitchAllIcons
                        ? quickSwitchTextFirst
                        ? quickSwitchVMargin + switchFont->ascent()
                        : height() - quickSwitchVMargin - switchFont->descent()
                        : ((height() + switchFont->height()) >> 1) -
                        switchFont->descent());

            g.drawChars(cTitle, x, y);

            if (quickSwitchAllIcons && quickSwitchSepSize) {
                int const h(quickSwitchVMargin + iconSize +
                            quickSwitchIMargin * 2 +
                            quickSwitchSepSize / 2);
                int const y(quickSwitchTextFirst ? height() - h : h);

                g.setColor(switchBg->darker());
                g.drawLine(1, y + 0, width() - 2, y + 0);
                g.setColor(switchBg->brighter());
                g.drawLine(1, y + 1, width() - 2, y + 1);
            }
        }

        if (quickSwitchAllIcons) {
            int const ds(quickSwitchHugeIcon ? YIcon::hugeSize() -
                         YIcon::largeSize() : 0);
            int const dx(YIcon::largeSize() + 2 * quickSwitchIMargin);

            const int visIcons((width() - 2 * quickSwitchHMargin) / dx);
            int curIcon(-1);

            int const y(quickSwitchTextFirst
                        ? height() - quickSwitchVMargin - iconSize - quickSwitchIMargin + ds / 2
                        : quickSwitchVMargin + ds + quickSwitchIMargin - ds / 2);

            g.setColor(switchHl ? switchHl : switchBg->brighter());

            const int off(max(1 + curIcon - visIcons, 0));
            const int end(off + visIcons);

            int x((width() - min(visIcons, zItems->getCount()) * dx - ds) /  2 +
                  quickSwitchIMargin);

            for (int i = 0, zCount = zItems->getCount (); i < zCount; i++) {
                if (i >= off && i < end) {
                    ref<YIcon> icon = zItems->getIcon (i);
                    if (icon != null && i == zItems->getActiveItem ()) {
                        if (quickSwitchFillSelection)
                            g.fillRect (x - quickSwitchIBorder,
                                        y - quickSwitchIBorder - ds / 2,
                                        iconSize + 2 * quickSwitchIBorder,
                                        iconSize + 2 * quickSwitchIBorder);
                        else
                            g.drawRect (x - quickSwitchIBorder,
                                        y - quickSwitchIBorder - ds / 2,
                                        iconSize + 2 * quickSwitchIBorder,
                                        iconSize + 2 * quickSwitchIBorder);

                        if (icon != null)
                            icon->draw (g, x, y - ds / 2, iconSize);

                        x += ds;
                    } else {
                        if (icon != null)
                            icon->draw (g, x, y, YIcon::largeSize ());
                    }
                    x += dx;
                }
            }
        }
    }
}

void SwitchWindow::paintVertical(Graphics &g) {
    if (zItems->getActiveItem() >= 0) {

        int ih = 0;
        //ih = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();
        ih = YIcon::largeSize();

        int pos = quickSwitchVMargin;
        g.setFont(switchFont);
        g.setColor(switchFg);

        for (int i = 0, zCount=zItems->getCount(); i < zCount; i++) {

            g.setColor(switchFg);
            if (i == zItems->getActiveItem()) {
                g.setColor(switchMbg);
                g.fillRect(quickSwitchHMargin, pos + quickSwitchVMargin , width() - quickSwitchHMargin*2, ih + quickSwitchIMargin );
                g.setColor(switchMfg);
            }

            ustring cTitle = zItems->getTitle(i);

            if (cTitle != null) {
                const int x(1+ih + quickSwitchIMargin *2 + quickSwitchHMargin + quickSwitchSepSize);
                const int y(pos + quickSwitchIMargin +  quickSwitchVMargin + ih/2);

                g.drawChars(cTitle, x, y);

            }
            ref<YIcon> icon = zItems->getIcon(i);
            if (icon != null) {
                if (quickSwitchTextFirst) {
                    // prepaint icons because of too long strings
                    g.setColor( (i == zItems->getActiveItem()) ? switchMfg : switchMbg);
                    g.fillRect(
                               width() - ih - quickSwitchIMargin *2 - quickSwitchHMargin,
                               pos + quickSwitchVMargin,
                               ih + 2 * quickSwitchIMargin,
                               ih + quickSwitchIMargin);

                    icon->draw(g,
                               width() - ih - quickSwitchIMargin - quickSwitchHMargin,
                               pos + quickSwitchIMargin,
                               YIcon::largeSize());
                } else {
                    icon->draw(g,
                               quickSwitchIMargin,
                               pos + quickSwitchIMargin,
                               YIcon::largeSize());
                }
            }

            pos += ih + 2* quickSwitchIMargin;
        }

        if (quickSwitchSepSize) {
            const int ip(ih + 2 * quickSwitchIMargin +
                         quickSwitchSepSize/2);
            const int x(quickSwitchTextFirst ? width() - ip : ip);

            g.setColor(switchBg->darker());
            g.drawLine(x + 0, 1, x + 0, height() - 2);
            g.setColor(switchBg->brighter());
            g.drawLine(x + 1, 1, x + 1, height() - 2);
        }
    }
}

void SwitchWindow::begin(bool zdown, int mods) {
    modsDown = mods & (xapp->AltMask | xapp->MetaMask |
                       xapp->HyperMask | xapp->SuperMask |
                       xapp->ModeSwitchMask | ControlMask);

    if (close())
        return;

    int xiscreen = manager->getScreen();
    zItems->begin(zdown);

    resize(xiscreen);

    int item = zItems->getActiveItem();
    if (item >= 0) {
        displayFocus(item);
        isUp = popup(0, 0, 0, xiscreen, YPopupWindow::pfNoPointerChange);
    }

    if (zItems->getCount() < 1) {
        close();
    }
    else if (isUp)
    {
        Window root, child;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;

        XQueryPointer(xapp->display(), handle(), &root, &child,
                      &root_x, &root_y, &win_x, &win_y, &mask);

        if (!modDown(mask))
            accept();
    }
}

void SwitchWindow::activatePopup(int /*flags*/) {
}

void SwitchWindow::deactivatePopup() {
}

void SwitchWindow::displayFocus(int itemIdx) {
    zItems->displayFocusChange(itemIdx);
    repaint();
}

void SwitchWindow::destroyedFrame(YFrameWindow *frame) {
    zItems->destroyedItem(frame);
    if (zItems->getCount() == 0) {
        cancel();
    }
    else if (isUp) {
        resize(manager->getScreen());
        repaint();
    }
}

bool SwitchWindow::handleKey(const XKeyEvent &key) {
    KeySym k = keyCodeToKeySym(key.keycode);
    unsigned int m = KEY_MODMASK(key.state);
    unsigned int vm = VMod(m);

    if (key.type == KeyPress) {
        if (zItems->isKey(k, vm)) {
            int focused = zItems->moveTarget(true);
            displayFocus(focused);
            return true;
        } else if ((IS_WMKEY(k, vm, gKeySysSwitchLast))) {
            // XXX: what to do with the swich-last key...
            int focused = zItems->moveTarget(false);
            displayFocus(focused);
            return true;
        } else if (k == XK_Escape) {
            cancel();
            return true;
        }
        if (zItems->isKey(k, vm) && !modDown(m)) {
            accept();
            return true;
        }
    } else if (key.type == KeyRelease) {
        if (zItems->isKey(k, vm) && !modDown(m)) {
            accept();
            return true;
        } else if (isModKey((KeyCode)key.keycode)) {
            accept();
            return true;
        }
    }
    return YPopupWindow::handleKey(key);
}

bool SwitchWindow::isModKey(KeyCode c) {
    KeySym k = keyCodeToKeySym(c);

    if (k == XK_Control_L || k == XK_Control_R ||
        k == XK_Alt_L     || k == XK_Alt_R     ||
        k == XK_Meta_L    || k == XK_Meta_R    ||
        k == XK_Super_L   || k == XK_Super_R   ||
        k == XK_Hyper_L   || k == XK_Hyper_R   ||
        k == XK_ISO_Level3_Shift || k == XK_Mode_switch)

        return true;

    return false;
}

bool SwitchWindow::modDown(int mod) {
   int m = mod & (xapp->AltMask | xapp->MetaMask | xapp->HyperMask |
         xapp->SuperMask | xapp->ModeSwitchMask | ControlMask);

    return hasbits(m, modsDown);
}

void SwitchWindow::handleButton(const XButtonEvent &button) {
    YPopupWindow::handleButton(button);
}

// vim: set sw=4 ts=4 et:
