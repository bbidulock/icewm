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

SwitchWindow * switchWindow(NULL);

SwitchWindow::SwitchWindow(YWindow *parent):
    YPopupWindow(parent) INIT_GRADIENT(fGradient, NULL) {
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

    fActiveWindow = 0;
    fLastWindow = 0;
    modsDown = 0;
    isUp = false;
    fRoot = manager;
    zCount = 0;
    zList = 0;

    resize(-1);

    setStyle(wsSaveUnder | wsOverrideRedirect);
}

SwitchWindow::~SwitchWindow() {
    if (isUp) {
        cancelPopup();
        isUp = false;
    }
#ifdef CONFIG_GRADIENTS
    fGradient = null;
#endif
}

void SwitchWindow::resize(int xiscreen) {
    int dx, dy, dw, dh;

    manager->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

    MSG(("got geometry for %d: %d %d %d %d", xiscreen, dx, dy, dw, dh));

    ustring cTitle = fActiveWindow ? fActiveWindow->client()->windowTitle() : null;

    int aWidth =
        quickSwitchSmallWindow ?
        (int) dw * 1/3 : (int) dw * 3/5;

    int tWidth=0;
    if (quickSwitchMaxWidth) {
        int space = (int) switchFont->textWidth(" ");   /* make entries one space character wider */
        for (int i = 0; i < zCount; i++) {
            if(zList[i]) {
                ustring title = zList[i]->client()->windowTitle();
                int oWidth = title != null ? (int) switchFont->textWidth(title) + space : 0;
                if(oWidth > tWidth) tWidth = oWidth;
            }
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

    int const mWidth(dw * 6/7);
    int w = aWidth;
    int h = switchFont->height();
#ifndef LITE

    if (quickSwitchVertical) {
        w = aWidth;
        if (w >= mWidth)
            w = mWidth;
        w += quickSwitchSepSize;

        int step = (YIcon::largeSize() + 2 * quickSwitchIMargin);
        int maxHeight = (int) dh - YIcon::largeSize();
        h = zCount * step;

        if (h > maxHeight)
            h= maxHeight - (maxHeight % step);
    } else {

        int iWidth =
            zCount * (YIcon::largeSize() + 2 * quickSwitchIMargin) +
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

    if (fActiveWindow) {
        int tOfs(0);

        int ih = 0;
#ifndef LITE
        ih = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();

        if (!quickSwitchAllIcons && fActiveWindow->clientIcon() != null) {
            int iconSize = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();
            ref<YIcon> icon = fActiveWindow->clientIcon();
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

        ustring cTitle = fActiveWindow->client()->windowTitle();

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

            int x((width() - min(visIcons, zCount) * dx - ds) /  2 +
                  quickSwitchIMargin);

            for (int i = 0; i < zCount; i++) {
                YFrameWindow *frame = zList[i];

                if (frame->clientIcon() != null) {
                    if (i >= off && i < end) {
                        if (frame == fActiveWindow) {
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
                            ref<YIcon> icon = fActiveWindow->clientIcon();

                            if (icon != null)
                                icon->draw(g, x, y - ds/2, iconSize);

                            x+= ds;
                        } else {
                            ref<YIcon> icon = frame->clientIcon();
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
    if (fActiveWindow) {

        int ih = 0;
        //ih = quickSwitchHugeIcon ? YIcon::hugeSize() : YIcon::largeSize();
        ih = YIcon::largeSize();

        int pos = quickSwitchVMargin;
        g.setFont(switchFont);
        g.setColor(switchFg);

        for (int i = 0; i < zCount; i++) {
            YFrameWindow *frame = zList[i];

            g.setColor(switchFg);
            if(frame == fActiveWindow) {
                g.setColor(switchMbg);
                g.fillRect(quickSwitchHMargin, pos + quickSwitchVMargin , width() - quickSwitchHMargin*2, ih + quickSwitchIMargin );
                g.setColor(switchMfg);
            }

            ustring cTitle = frame->client()->windowTitle();

            if (cTitle != null) {
                const int x(1+ih + quickSwitchIMargin *2 + quickSwitchHMargin + quickSwitchSepSize);
                const int y(pos + quickSwitchIMargin +  quickSwitchVMargin + ih/2);

                g.drawChars(cTitle, x, y);

            }
            if (frame->clientIcon() != null) {
                ref<YIcon> icon = frame->clientIcon();

                if (quickSwitchTextFirst) {

                    // prepaint icons because of too long strings
                    g.setColor( (frame == fActiveWindow) ? switchMfg : switchMbg);
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

int SwitchWindow::getZListCount() {
    int count = 0;

    YFrameWindow *w = fRoot->lastFocusFrame();
    while (w) {
        count++;
        w = w->prevFocus();
    }
    return count;
}

int SwitchWindow::getZList(YFrameWindow **list, int max) {

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

int SwitchWindow::GetZListWorkspace(YFrameWindow **list, int max,
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

void SwitchWindow::updateZList() {
    freeZList();

    zCount = getZListCount();

    zList = new YFrameWindow *[zCount + 1]; // for bug hunt
    if (zList == 0)
        zCount = 0;
    else
        zCount = getZList(zList, zCount);
}

void SwitchWindow::freeZList() {
    if (zList)
        delete [] zList;
    zCount = 0;
    zList = 0;
}

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

YFrameWindow *SwitchWindow::nextWindow(bool zdown) {
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
        return zList[zTarget];
}

void SwitchWindow::begin(bool zdown, int mods) {
    modsDown = mods & (xapp->AltMask | xapp->MetaMask |
                       xapp->HyperMask | xapp->SuperMask |
                       xapp->ModeSwitchMask | ControlMask);


    if (isUp) {
        cancelPopup();
        isUp = false;
        return;
    }

    int xiscreen = manager->getScreen();

    fLastWindow = fActiveWindow = manager->getFocus();
    updateZList();
    zTarget = 0;
    fActiveWindow = nextWindow(zdown);

    resize(xiscreen);

    if (fActiveWindow) {
        displayFocus(fActiveWindow);
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

void SwitchWindow::cancel() {
    if (isUp) {
        cancelPopup();
        isUp = false;
    }
    if (fLastWindow) {
        displayFocus(fLastWindow);
    } else if (fActiveWindow) {
        fRoot->activate(fActiveWindow, false, true);
    }
    freeZList();
    fLastWindow = fActiveWindow = 0;
}

void SwitchWindow::accept() {
    if (fActiveWindow == 0)
        cancel();
    else {
        fRoot->activate(fActiveWindow, true, true);
        if (isUp) {
            cancelPopup();
            isUp = false;
        }
        fActiveWindow->wmRaise();
        //manager->activate(fActiveWindow, true);
    }
    freeZList();
    fLastWindow = fActiveWindow = 0;
}

void SwitchWindow::displayFocus(YFrameWindow *frame) {
    manager->switchFocusTo(frame, false);
    repaint();
}

void SwitchWindow::destroyedFrame(YFrameWindow *frame) {
    if (zList == 0)
        return;
    if (frame == fLastWindow)
        fLastWindow = 0;
    updateZList();
    if (frame == fActiveWindow) {
        zTarget = -1;
        fActiveWindow = nextWindow(true);
    }
    displayFocus(fActiveWindow);
}

bool SwitchWindow::handleKey(const XKeyEvent &key) {
    KeySym k = XKeycodeToKeysym(xapp->display(), (KeyCode)key.keycode, 0);
    unsigned int m = KEY_MODMASK(key.state);
    unsigned int vm = VMod(m);

    if (key.type == KeyPress) {
        if ((IS_WMKEY(k, vm, gKeySysSwitchNext))) {
            fActiveWindow = nextWindow(true);
            displayFocus(fActiveWindow);
            return true;
        } else if ((IS_WMKEY(k, vm, gKeySysSwitchLast))) {
            fActiveWindow = nextWindow(false);
            displayFocus(fActiveWindow);
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
    KeySym k = XKeycodeToKeysym(xapp->display(), c, 0);

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
