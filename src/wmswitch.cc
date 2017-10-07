/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Windows/OS2 like Alt{+Shift}+Tab window switching
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

YColor *SwitchWindow::switchFg(NULL);
YColor *SwitchWindow::switchBg(NULL);
YColor *SwitchWindow::switchHl(NULL);
YColor *SwitchWindow::switchMbg(NULL);
YColor *SwitchWindow::switchMfg(NULL);

ref<YFont> SwitchWindow::switchFont;

class ISwitchItems
{
public:
    virtual void updateList() =0;
    virtual void freeList() =0;
    virtual int getCount() =0;
    virtual ~ISwitchItems() {}

    // move the focused target up or down and return the new focused element
    virtual void *moveTarget(bool zdown)=0;
    /// Show changed focus preview to user
    virtual void displayFocusChange(void *frame)=0;

    // set target cursor and implementation specific stuff in the beginning
    virtual void begin(bool zdown)=0;
    virtual void cancel()=0;
    virtual void accept(IClosablePopup *parent)=0;

    // XXX: convert to iterator
    virtual void* getActiveItem()=0;
    virtual void* getItemAt(int n) =0;
    virtual ustring getTitle(void *item) =0;
    virtual ref<YIcon> getIcon(void* item) =0;

    // Manager notification about windows disappearing under the fingers
    virtual void destroyedItem(void *item) =0;
};

class WindowItemsCtrlr : public ISwitchItems
{
    int zCount;
    int zTarget;
    YFrameWindow **zList;

    YWindowManager *fRoot;
    YFrameWindow *fActiveWindow;
    YFrameWindow *fLastWindow;

    /// get updated count
    int getZListCount(){
            int count = 0;
            YFrameWindow *w = fRoot->lastFocusFrame();
            while (w) {
                count++;
                w = w->prevFocus();
            }
            return count;
        }
    int getZList(YFrameWindow **list, int max) {

        if (quickSwitchGroupWorkspaces || !quickSwitchToAllWorkspaces) {
            int activeWorkspace = fRoot->activeWorkspace();

            int count = 0;

            count += GetZListWorkspace(list, max, true, activeWorkspace);
            if (quickSwitchToAllWorkspaces) {
                for (int w = 0; w <= workspaceCount; w++) {
                    if (w != activeWorkspace)
                        count += GetZListWorkspace(list + count, max - count, true, w);
                }
            }
            return count;
        } else {
            return GetZListWorkspace(list, max, false, -1);
        }
    }


    int GetZListWorkspace(YFrameWindow **list, int max,
                                        bool workspaceOnly, int workspace)
    {
        int count = 0;
        for (int pass = 0; pass <= 5; pass++) {
            YFrameWindow *w = fRoot->lastFocusFrame();

            while (w) {
                // pass 0: focused window
                // pass 1: urgent windows
                // pass 2: normal windows
                // pass 3: minimized windows
                // pass 4: hidden windows
                // pass 5: unfocusable windows
                if ((w->client() && !w->client()->adopted()) && !w->visible()) {
                    w = w->prevFocus();
                    continue;
                }

                if (!w->isUrgent()) {
                    if (workspaceOnly && w->isSticky() && workspace != fRoot->activeWorkspace()) {
                        w = w->prevFocus();
                        continue;
                    }

                    if (workspaceOnly && !w->visibleOn(workspace)) {
                        w = w->prevFocus();
                        continue;
                    }
                }

                if (w == fRoot->getFocus()) {
                    if (pass == 0) list[count++] = w;
                } else if (w->isUrgent()) {
                    if (quickSwitchToUrgent) {
                        if (pass == 1) list[count++] = w;
                    } else {
                        if (pass == 2) list[count++] = w;
                    }
                } else if (w->frameOptions() & YFrameWindow::foIgnoreQSwitch) {
                } else if (w->avoidFocus()) {
                    if (pass == 5) list[count++] = w;
                } else if (w->isHidden()) {
                    if (pass == 4)
                        if (quickSwitchToHidden)
                            list[count++] = w;
                } else if (w->isMinimized()) {
                    if (pass == 3)
                        if (quickSwitchToMinimized)
                            list[count++] = w;
                } else {
                    if (pass == 2) list[count++] = w;
                }

                w = w->prevFocus();

                if (count > max) {
                    msg("wmswitch BUG: limit=%d pass=%d\n", max, pass);
                    return max;
                }
            }
        }
        return count;
    }


public:


    virtual void* getItemAt(int n)
    {
        return zList[n];
    }

    virtual int getCount()
    {
        return zCount;
    }

    virtual ref<YIcon> getIcon(void* item)
    {
        YFrameWindow* winItem=(YFrameWindow*) item;
        return winItem->getIcon();
    }


    void *moveTarget(bool zdown) {
        if (zdown) {
            zTarget = zTarget + 1;
            if (zTarget >= zCount) zTarget = 0;
        } else {
            zTarget = zTarget - 1;
            if (zTarget < 0) zTarget = zCount - 1;
        }
        if (zTarget >= zCount || zTarget < 0)
            zTarget = -1;

        if (zTarget == -1)
            return 0;
        else
        {
            focus(zList[zTarget]);
            return zList[zTarget];
        }
    }

    WindowItemsCtrlr() :
        zCount(0), zTarget(0), zList(0), fRoot(manager), fActiveWindow(0), fLastWindow(0)
        {

        }
    ~WindowItemsCtrlr()
    {

    }
    void* getActiveItem()
    {
        return fActiveWindow;
    }

    virtual ustring getTitle(void *itemPtr)
    {
        return fActiveWindow ? fActiveWindow->client()->windowTitle() : null;
    }
    void updateList() {
        freeList();

        zCount = getZListCount();

        zList = new YFrameWindow *[zCount + 1]; // for bug hunt
        if (zList == 0)
            zCount = 0;
        else
            zCount = getZList(zList, zCount);
    }

    void freeList() {
        if (zList)
            delete [] zList;
        zCount = 0;
        zList = 0;
    }

    void displayFocusChange(void *frame) {
        manager->switchFocusTo((YFrameWindow*)frame, false);
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
            //manager->activate(fActiveWindow, true);
        }
        freeList();
        fLastWindow = fActiveWindow = 0;
    }

    void destroyedItem(void *item)
    {
        YFrameWindow* frame = (YFrameWindow*) item;
        if (zList == 0)
            return;
        if (frame == fLastWindow)
            fLastWindow = 0;
        updateList();
        if (frame == fActiveWindow) {
            zTarget = -1;
            moveTarget(true);
        }
        displayFocusChange(fActiveWindow);
    }
    // set internal focus on this item, can be accepted or canceled
    virtual void focus(void* item)
    {
        fActiveWindow = (YFrameWindow*) item;
    }

};

SwitchWindow::SwitchWindow(YWindow *parent, ISwitchItems *items):
    YPopupWindow(parent) INIT_GRADIENT(fGradient, null) {

    zItems = items ? items : new WindowItemsCtrlr;

    // why this checks here?
    if (switchBg == 0)
        switchBg = new YColor(clrQuickSwitch);
    if (switchFg == 0)
        switchFg = new YColor(clrQuickSwitchText);
    if (/*switchHl == 0 &&*/ clrQuickSwitchActive)
        switchHl = new YColor(clrQuickSwitchActive);
    if (switchFont == null)
        switchFont = YFont::getFont(XFA(switchFontName));

    // I prefer clrNormalMenu but some themes use inverted settings where
    // clrNormalMenu is the same as clrQuickSwitch
    if (switchHl)
        switchMbg = switchHl;
    else if (!strcmp(clrNormalMenu, clrQuickSwitch))
        switchMbg = new YColor(clrActiveMenuItem);
    else
        switchMbg = new YColor(clrNormalMenu);
    switchMfg = new YColor(clrActiveTitleBarText);

    modsDown = 0;
    isUp = false;

    resize(-1);

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
#ifdef CONFIG_GRADIENTS
    fGradient = null;
#endif
    delete(zItems);
}

void SwitchWindow::resize(int xiscreen) {
    int dx, dy, dw, dh;

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
            void* item = zItems->getItemAt(i);
            if(!item) continue;
            ustring title = zItems->getTitle(item);
            int oWidth = title != null ? (int) switchFont->textWidth(title) + space : 0;
            if (oWidth > tWidth)
                tWidth = oWidth;
        }
    } else {
        tWidth = cTitle != null ? switchFont->textWidth(cTitle) : 0;
    }

#ifndef LITE
    if (quickSwitchVertical || !quickSwitchAllIcons)
        tWidth += 2 * quickSwitchIMargin + YIcon::largeSize() + 3;
#endif
    if (tWidth > aWidth)
        aWidth = tWidth;

    int w = aWidth;
    int h = switchFont->height();
#ifndef LITE
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
#endif
    h += quickSwitchVMargin * 2;
    w += quickSwitchHMargin * 2;

    setGeometry(YRect(dx + ((dw - w) >> 1),
                      dy + ((dh - h) >> 1),
                      w, h));
}

void SwitchWindow::paint(Graphics &g, const YRect &/*r*/) {
#ifdef CONFIG_GRADIENTS
    if (switchbackPixbuf != null &&
        !(fGradient != null &&
          fGradient->width() == width() - 2 &&
          fGradient->height() == height() - 2))
    {
        fGradient = switchbackPixbuf->scale(width() - 2, height() - 2);
    }
#endif

    g.setColor(switchBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);

#ifdef CONFIG_GRADIENTS
    if (fGradient != null)
        g.drawImage(fGradient, 1, 1, width() - 2, height() - 2, 1, 1);
    else
#endif
    if (switchbackPixmap != null)
        g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    else
        g.fillRect(1, 1, width() - 3, height() - 3);

#ifndef LITE
    // for vertical positioning, continue below. Avoid spagheti code.
    if(quickSwitchVertical) goto verticalMode;
#endif

    if (zItems->getActiveItem()) {
        int tOfs(0);

#ifndef LITE
        int ih = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();

        ref<YIcon> icon;
        if (!quickSwitchAllIcons && (icon = zItems->getIcon(zItems->getActiveItem())) != null) {
            int iconSize = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();
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
#endif

        g.setColor(switchFg);
        g.setFont(switchFont);

        ustring cTitle = zItems->getTitle(zItems->getActiveItem());
        if (cTitle != null) {
            const int x = max((width() - tOfs -
                               switchFont->textWidth(cTitle)) >> 1, 0) + tOfs;
            const int y(quickSwitchAllIcons
                        ? quickSwitchTextFirst
                        ? quickSwitchVMargin + switchFont->ascent()
                        : height() - quickSwitchVMargin - switchFont->descent()
                        : ((height() + switchFont->height()) >> 1) -
                        switchFont->descent());

            g.drawChars(cTitle, x, y);

#ifndef LITE
            if (quickSwitchAllIcons && quickSwitchSepSize) {
                int const h(quickSwitchVMargin + ih +
                            quickSwitchIMargin * 2 +
                            quickSwitchSepSize / 2);
                int const y(quickSwitchTextFirst ? height() - h : h);

                g.setColor(switchBg->darker());
                g.drawLine(1, y + 0, width() - 2, y + 0);
                g.setColor(switchBg->brighter());
                g.drawLine(1, y + 1, width() - 2, y + 1);
            }
#endif
        }

#ifndef LITE
        if (quickSwitchAllIcons) {
            int const ds(quickSwitchHugeIcon ? YIcon::hugeSize() -
                         YIcon::largeSize() : 0);
            int const dx(YIcon::largeSize() + 2 * quickSwitchIMargin);

            const int visIcons((width() - 2 * quickSwitchHMargin) / dx);
            int curIcon(-1);

            int const y(quickSwitchTextFirst
                        ? height() - quickSwitchVMargin - ih - quickSwitchIMargin + ds / 2
                        : quickSwitchVMargin + ds + quickSwitchIMargin - ds / 2);

            g.setColor(switchHl ? switchHl : switchBg->brighter());

            const int off(max(1 + curIcon - visIcons, 0));
            const int end(off + visIcons);

            int x((width() - min(visIcons, zItems->getCount()) * dx - ds) /  2 +
                  quickSwitchIMargin);

            for (int i = 0, zCount = zItems->getCount(); i < zCount; i++) {
                void *frame = zItems->getItemAt(i);

                if (icon != null) {
                    if (i >= off && i < end) {
                        ref<YIcon> icon = zItems->getIcon(frame);
                        if (frame == zItems->getActiveItem()) {
                            if (quickSwitchFillSelection)
                                g.fillRect(x - quickSwitchIBorder,
                                           y - quickSwitchIBorder - ds/2,
                                           ih + 2 * quickSwitchIBorder,
                                           ih + 2 * quickSwitchIBorder);
                            else
                                g.drawRect(x - quickSwitchIBorder,
                                           y - quickSwitchIBorder - ds/2,
                                           ih + 2 * quickSwitchIBorder,
                                           ih + 2 * quickSwitchIBorder);

                            int iconSize = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();
                            if (icon != null)
                                icon->draw(g, x, y - ds/2, iconSize);

                            x+= ds;
                        } else {
                            if (icon != null)
                                icon->draw(g, x, y, YIcon::largeSize());
                        }

                        x += dx;
                    }
                }
            }
            //      {} while ((frame = nextWindow(frame, true, true)) != first);
        }
#endif
    }

#ifndef LITE
    return;

verticalMode:
    if (zItems->getActiveItem()) {

        int ih = 0;
        //ih = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();
        ih = YIcon::largeSize();

        int pos = quickSwitchVMargin;
        g.setFont(switchFont);
        g.setColor(switchFg);

        for (int i = 0, zCount=zItems->getCount(); i < zCount; i++) {
            void *frame = zItems->getItemAt(i);

            g.setColor(switchFg);
            if(frame == zItems->getActiveItem()) {
                g.setColor(switchMbg);
                g.fillRect(quickSwitchHMargin, pos + quickSwitchVMargin , width() - quickSwitchHMargin*2, ih + quickSwitchIMargin );
                g.setColor(switchMfg);
            }

            ustring cTitle = zItems->getTitle(frame);

            if (cTitle != null) {
                const int x(1+ih + quickSwitchIMargin *2 + quickSwitchHMargin + quickSwitchSepSize);
                const int y(pos + quickSwitchIMargin +  quickSwitchVMargin + ih/2);

                g.drawChars(cTitle, x, y);

            }
            ref<YIcon> icon = zItems->getIcon(frame);
            if (icon != null) {
                if (quickSwitchTextFirst) {
                    // prepaint icons because of too long strings
                    g.setColor( (frame == zItems->getActiveItem()) ? switchMfg : switchMbg);
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
#endif
}

                if (workspaceOnly && w->isSticky() && workspace != fRoot->activeWorkspace()) {
/*
 YFrameWindow *SwitchWindow::nextWindow(YFrameWindow *from, bool zdown, bool next) {
 if (from == 0) {
 next = false;
 from = zdown ? manager->topLayer() : manager->bottomLayer();
 }
 int flags =
 YFrameWindow::fwfFocusable |
 (quickSwitchToAllWorkspaces ? 0 : YFrameWindow::fwfWorkspace) |
 YFrameWindow::fwfLayers |
 YFrameWindow::fwfSwitchable |
 (next ? YFrameWindow::fwfNext: 0) |
 (zdown ? 0 : YFrameWindow::fwfBackward);

 YFrameWindow *n = 0;

 if (from == 0)
 return 0;

 if (zdown) {
 n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
 if (n == 0 && quickSwitchToMinimized)
 n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
 if (n == 0 && quickSwitchToHidden)
 n = from->findWindow(flags | YFrameWindow::fwfHidden);
 if (n == 0) {
 flags |= YFrameWindow::fwfCycle;
 n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
 if (n == 0 && quickSwitchToMinimized)
 n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
 if (n == 0 && quickSwitchToHidden)
 n = from->findWindow(flags | YFrameWindow::fwfHidden);
 }
 } else {
 if (n == 0 && quickSwitchToHidden)
 n = from->findWindow(flags | YFrameWindow::fwfHidden);
 if (n == 0 && quickSwitchToMinimized)
 n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
 if (n == 0)
 n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
 if (n == 0) {
 flags |= YFrameWindow::fwfCycle;
 if (n == 0 && quickSwitchToHidden)
 n = from->findWindow(flags | YFrameWindow::fwfHidden);
 if (n == 0 && quickSwitchToMinimized)
 n = from->findWindow(flags | YFrameWindow::fwfMinimized | YFrameWindow::fwfNotHidden);
 if (n == 0)
 n = from->findWindow(flags | YFrameWindow::fwfUnminimized | YFrameWindow::fwfNotHidden);
 }
 }
 if (n == 0)
 n = from->findWindow(flags |
 YFrameWindow::fwfVisible |
 (quickSwitchToMinimized) ? 0 : YFrameWindow::fwfUnminimized |
 YFrameWindow::fwfSame);
 if (n == 0)
 n = fLastWindow;

 return n;
 }
 */

void SwitchWindow::begin(bool zdown, int mods) {
    modsDown = mods & (xapp->AltMask | xapp->MetaMask |
                       xapp->HyperMask | xapp->SuperMask |
                       xapp->ModeSwitchMask | ControlMask);

    if(close())
        return;

    int xiscreen = manager->getScreen();
    zItems->begin(zdown);

    resize(xiscreen);

    void* item = zItems->getActiveItem();
    if (item) {
        displayFocus(item);
        isUp = popup(0, 0, 0, xiscreen, YPopupWindow::pfNoPointerChange);
    }
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


void SwitchWindow::displayFocus(void *frame) {
    zItems->displayFocusChange(frame);
    repaint();
}

void SwitchWindow::destroyedFrame(YFrameWindow *frame) {
    zItems->destroyedItem(frame);
}

bool SwitchWindow::handleKey(const XKeyEvent &key) {
    KeySym k = keyCodeToKeySym(key.keycode);
    unsigned int m = KEY_MODMASK(key.state);
    unsigned int vm = VMod(m);

    if (key.type == KeyPress) {
        if ((IS_WMKEY(k, vm, gKeySysSwitchNext))) {
            void* focused = zItems->moveTarget(true);
            displayFocus(focused);
            return true;
        } else if ((IS_WMKEY(k, vm, gKeySysSwitchLast))) {
            void* focused = zItems->moveTarget(false);
            displayFocus(focused);
            return true;
        } else if (k == XK_Escape) {
            cancel();
            return true;
        }
        if ((IS_WMKEY(k, vm, gKeySysSwitchNext)) && !modDown(m)) {
            accept();
            return true;
        }
    } else if (key.type == KeyRelease) {
        if ((IS_WMKEY(k, vm, gKeySysSwitchNext)) && !modDown(m)) {
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

    if ((m & modsDown) != modsDown)
        return false;
    return true;
}

void SwitchWindow::handleButton(const XButtonEvent &button) {
    YPopupWindow::handleButton(button);
}

// vim: set sw=4 ts=4 et:
