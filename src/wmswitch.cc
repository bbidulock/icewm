/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Alt{+Shift}+Tab window switching
 */
#include "config.h"
#include "wmswitch.h"
#include "wpixmaps.h"
#include "wmframe.h"
#include "yxapp.h"
#include "prefs.h"
#include "yprefs.h"
#include "workspaces.h"

class WindowItemsCtrlr : public ISwitchItems
{
    int zTarget;
    YArray<YFrameWindow*> zList;
    YWindowManager *fRoot;
    YFrameWindow *fActiveWindow;
    YFrameWindow *fLastWindow;
    char *fWMClass;

    void append(YFrameWindow* w) {
        if (find(zList, w) < 0) {
            zList.append(w);
        }
    }

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

        if (fActiveWindow != nullptr && find(zList, fActiveWindow) == -1)
            fActiveWindow = nullptr;
        if (fLastWindow != nullptr && find(zList, fLastWindow) == -1)
            fLastWindow = nullptr;
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

                if (hasbit(w->client()->winHints(), WinHintsSkipFocus))
                    continue;

                if (!w->client()->adopted() && !w->visible()) {
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

                if (nonempty(fWMClass)) {
                    if (w->client()->classHint()->match(fWMClass) == false)
                        continue;
                }

                if (w == fRoot->getFocus()) {
                    if (pass == 0) append(w);
                } else if (w->isUrgent()) {
                    if (quickSwitchToUrgent) {
                        if (pass == 1) append(w);
                    } else {
                        if (pass == 2) append(w);
                    }
                } else if (w->frameOption(YFrameWindow::foIgnoreQSwitch)) {
                } else if (w->avoidFocus()) {
                    if (pass == 5) append(w);
                } else if (w->isHidden()) {
                    if (pass == 4)
                        if (quickSwitchToHidden)
                            append(w);
                } else if (w->isMinimized()) {
                    if (pass == 3)
                        if (quickSwitchToMinimized)
                            append(w);
                } else {
                    if (pass == 2) append(w);
                }
            }
        }
    }

    void displayFocusChange(YFrameWindow *frame)  {
        manager->switchFocusTo(frame, false);
    }

public:

    virtual int getCount() override
    {
        return zList.getCount();
    }

    virtual ref<YIcon> getIcon(int itemIdx) override
    {
        if (inrange(itemIdx, 0, getCount() - 1)) {
            YFrameWindow* winItem = zList[itemIdx];
            if (winItem) return winItem->getIcon();
        }
        return null;
    }

    int moveTarget(bool zdown) override {
        const int cnt = getCount();
        return setTarget(cnt < 2 ? 0 : (zTarget + cnt + (zdown ? 1 : -1)) % cnt);
    }
    inline virtual int setTarget(int zPosition) override
    {
        zTarget=zPosition;
        if (inrange(zTarget, 0, getCount() - 1))
            fActiveWindow = zList[zTarget];
        else
            fActiveWindow = nullptr;
        return zPosition;
    }


    WindowItemsCtrlr() :
        zTarget(0), fRoot(manager), fActiveWindow(nullptr), fLastWindow(nullptr),
        fWMClass(nullptr)
    {
    }

    ~WindowItemsCtrlr()
    {
        if (fWMClass)
            free(fWMClass);
    }

    virtual int getActiveItem() override
    {
        return zTarget;
    }

    virtual mstring getTitle(int idx) override
    {
        if (inrange(idx, 0, getCount() - 1))
            return zList[idx]->client()->windowTitle();
        return null;
    }

    virtual void setWMClass(char* wmclass) override {
        if (fWMClass)
            free(fWMClass);
        fWMClass = wmclass;
    }

    virtual void updateList() override {
        freeList();
        getZList();
    }

    void displayFocusChange(int idx) override {
        if (inrange(idx, 0, getCount() - 1))
            displayFocusChange(zList[idx]);
    }

    virtual void begin(bool zdown) override
    {
        fLastWindow = fActiveWindow = manager->getFocus();
        updateList();
        zTarget = 0;
        moveTarget(zdown);
    }

    virtual void reset() override {
        zTarget = 0;
    }

    virtual void cancel() override {
        if (fLastWindow) {
            displayFocusChange(fLastWindow);
        } else if (fActiveWindow) {
            fActiveWindow->activateWindow(true, false);
        }
        freeList();
        fLastWindow = fActiveWindow = nullptr;
    }

    virtual void accept(IClosablePopup *parent) override {
        if (fActiveWindow == nullptr)
            cancel();
        else {
            fActiveWindow->activateWindow(true, false);
            parent->close();
        }
        freeList();
        fLastWindow = fActiveWindow = nullptr;
    }

    virtual void destroyedItem(void *item) override
    {
        if (getCount() == 0)
            return;

        YFrameWindow* frame = (YFrameWindow*) item;
        if (frame == fLastWindow)
            fLastWindow = nullptr;
        updateList();
        if (frame == fActiveWindow || fActiveWindow == nullptr) {
            zTarget = -1;
            moveTarget(true);
        }
        displayFocusChange(fActiveWindow);
    }

    virtual bool isKey(KeySym k, unsigned int vm) override {
        return gKeySysSwitchNext.eq(k, vm) ||
              (gKeySysSwitchClass.eq(k, vm) && fWMClass != nullptr);
    }
};

SwitchWindow::SwitchWindow(YWindow *parent, ISwitchItems *items,
                           bool verticalStyle):
    YPopupWindow(parent),
    m_verticalStyle(verticalStyle),
    m_oldMenuMouseTracking(menuMouseTracking),
    fGradient(null),
    switchFg(&clrQuickSwitchText),
    switchBg(&clrQuickSwitch),
    switchHl(&clrQuickSwitchActive),
    switchMfg(&clrActiveTitleBarText),
    switchFont(YFont::getFont(XFA(switchFontName)))
{
    zItems = items ? items : new WindowItemsCtrlr;
    m_hlItemFromMotion = -1;
    m_hintAreaStart = 0;
    m_hintAreaStep = 1;
    // I prefer clrNormalMenu but some themes use inverted settings where
    // clrNormalMenu is the same as clrQuickSwitch
    if (clrQuickSwitchActive)
        switchMbg = &clrQuickSwitchActive;
    else if (!strcmp(clrNormalMenu, clrQuickSwitch))
        switchMbg = &clrActiveMenuItem;
    else
        switchMbg = &clrNormalMenu;

    modsDown = 0;
    isUp = false;

    setStyle(wsSaveUnder | wsOverrideRedirect | wsPointerMotion | wsNoExpose);
    setTitle("IceSwitch");
    setClassHint("switch", "IceWM");
    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_DIALOG);
}

bool SwitchWindow::close() {
    if (isUp) {
        cancelPopup();
        isUp = false;
        menuMouseTracking = m_oldMenuMouseTracking;
        m_hlItemFromMotion = -1;
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

    desktop->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

    MSG(("got geometry for %d: %d %d %d %d", xiscreen, dx, dy, dw, dh));

    mstring cTitle = zItems->getTitle(zItems->getActiveItem());

    int aWidth =
        quickSwitchSmallWindow ?
        (int) dw * 1/3
        : (quickSwitchVertical ? (int) dw * 2/5 : (int) dw * 3/5);

    int tWidth=0;
    if (quickSwitchMaxWidth) {
        int space = (int) switchFont->textWidth(" ");   /* make entries one space character wider */
        int zCount = zItems->getCount();
        for (int i = 0; i < zCount; i++) {
            mstring title = zItems->getTitle(i);
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
    const int vMargins = quickSwitchVMargin*2;

    if (quickSwitchVertical) {
        w = aWidth;
        if (w >= mWidth)
            w = mWidth;
        w += quickSwitchSepSize;

        m_hintAreaStep = YIcon::largeSize() + quickSwitchIMargin;
        m_hintAreaStart = quickSwitchVMargin - quickSwitchIBorder;
        h = zItems->getCount() * m_hintAreaStep;
        if (h + vMargins > int(dh))
            h = (unsigned(dh) - vMargins) / m_hintAreaStep * m_hintAreaStep;
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
    h += vMargins;
    w += quickSwitchHMargin * 2;

    setGeometry(YRect(dx + ((dw - w) >> 1),
                      dy + ((dh - h) >> 1),
                      w, h));
}

void SwitchWindow::repaint() {
    GraphicsBuffer(this).paint();
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

    iconsDrawn = 0;
    m_verticalStyle ? paintVertical(g) : paintHorizontal(g);
    if (iconsDrawn) {
        g.maxOpacity();
    }
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
                ++iconsDrawn;
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

        mstring cTitle = zItems->getTitle(zItems->getActiveItem());
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

            YColor frameColor = switchHl ? switchHl : switchBg->brighter();
            g.setColor(frameColor);

            const int off(max(1 + curIcon - visIcons, 0));
            const int end(off + visIcons);

            int x((width() - min(visIcons, zItems->getCount()) * dx - ds) /  2 +
                  quickSwitchIMargin);

            m_hintAreaStart = x;
            m_hintAreaStep = dx;

            for (int i = 0, zCount = zItems->getCount (); i < zCount; i++) {
                if (i >= off && i < end) {
                    ref<YIcon> icon = zItems->getIcon(i);
                    if (icon != null) {
                        if (i == m_hlItemFromMotion && i != zItems->getActiveItem()) {
                            g.setColor(frameColor.darker());
                            g.drawRect(x - quickSwitchIBorder,
                                    y - quickSwitchIBorder - ds / 2,
                                    iconSize + 2 * quickSwitchIBorder,
                                    iconSize + 2 * quickSwitchIBorder);
                            g.setColor(frameColor);
                        }
                        if (i == zItems->getActiveItem()) {
                            if (quickSwitchFillSelection)
                                g.fillRect(x - quickSwitchIBorder,
                                        y - quickSwitchIBorder - ds / 2,
                                        iconSize + 2 * quickSwitchIBorder,
                                        iconSize + 2 * quickSwitchIBorder);
                            else
                                g.drawRect(x - quickSwitchIBorder,
                                        y - quickSwitchIBorder - ds / 2,
                                        iconSize + 2 * quickSwitchIBorder,
                                        iconSize + 2 * quickSwitchIBorder);

                            if (icon != null) {
                                icon->draw(g, x, y - ds / 2, iconSize);
                                ++iconsDrawn;
                            }
                        } else {
                            icon->draw(g, x, y, YIcon::largeSize());
                            ++iconsDrawn;
                        }
                        x += ds;
                    }
                    x += dx;
                }
            }
        }
    }
}

int SwitchWindow::calcHintedItem(int x, int y)
{
    if(quickSwitchVertical)
        return (y - m_hintAreaStart) / m_hintAreaStep;
    else if(quickSwitchAllIcons && !quickSwitchHugeIcon)
        return (x - m_hintAreaStart) / m_hintAreaStep;
    else
        return -2;
}

void SwitchWindow::handleMotion(const XMotionEvent& motion) {
    int hintId = calcHintedItem(motion.x, motion.y);
    //printf("hint id: %d\n", hintId);
    if(hintId == m_hlItemFromMotion || hintId == -2)
        return;
    m_hlItemFromMotion = hintId;
    repaint();
}

void SwitchWindow::paintVertical(Graphics &g) {
    // NOTE: quickSwitchHugeIcon not supported in vertical mode. Tried that, looks creepy, not nice (04d53238@code7r)
    const int iconSize = /* quickSwitchHugeIcon ? YIcon::hugeSize() : */ YIcon::largeSize();

    if (zItems->getActiveItem() >= 0) {
        const int maxWid = width() - 2; // reduce due to 3D edge
        const int contentX = quickSwitchHMargin;
        const int titleX = quickSwitchTextFirst ? contentX
                : contentX + iconSize + quickSwitchSepSize;
        const int itemWidth = maxWid - quickSwitchHMargin*2;
        const int frameX = contentX - quickSwitchIBorder;
        const int frameWid = itemWidth + 2 * quickSwitchIBorder;
        const int frameHght = iconSize + 2*quickSwitchIBorder;
        const int strWid = itemWidth - iconSize - quickSwitchSepSize - 2*quickSwitchHMargin;
        const int sepX = quickSwitchTextFirst
                ? maxWid - iconSize - quickSwitchSepSize/2 - 1
                        :  contentX + iconSize + quickSwitchSepSize/2 - 1;

        int contentY = quickSwitchVMargin;

        g.setFont(switchFont);
        g.setColor(switchFg);
        for (int i = 0, zCount=zItems->getCount(); i < zCount; i++) {
            if(contentY + frameHght > (int) height())
                break;
            if (i == zItems->getActiveItem()) {
                g.setColor(switchMbg);
                g.fillRect(frameX, contentY-quickSwitchIBorder, frameWid, frameHght);
                g.setColor(switchMfg);
            }
            else
                g.setColor(switchFg);

            mstring cTitle = zItems->getTitle(i);

            if (cTitle != null) {
                const int titleY = contentY + (iconSize + g.font()->ascent())/2;
                g.drawStringEllipsis(titleX, titleY, cTitle.c_str(), strWid);
            }
            ref<YIcon> icon = zItems->getIcon(i);
            if (icon != null) {
                int iconX = quickSwitchTextFirst
                        ? width() - quickSwitchHMargin - iconSize
                        : contentX;
                icon->draw(g, iconX, contentY, iconSize);
                ++iconsDrawn;
            }

            if(i == m_hlItemFromMotion && i != zItems->getActiveItem())
            {
                g.setColor(switchMbg);
                g.drawRect(frameX, contentY-quickSwitchIBorder, frameWid, frameHght);
            }
            contentY += m_hintAreaStep;
        }
        if (quickSwitchSepSize) {
            g.setColor(switchBg->darker());
            g.drawLine(sepX + 0, 1, sepX + 0, height() - 2);
            g.setColor(switchBg->brighter());
            g.drawLine(sepX + 1, 1, sepX + 1, height() - 2);
        }
    }
}

void SwitchWindow::begin(bool zdown, int mods, char* wmclass) {
    modsDown = mods & (xapp->AltMask | xapp->MetaMask |
                       xapp->HyperMask | xapp->SuperMask |
                       xapp->ModeSwitchMask | ControlMask);
    zItems->setWMClass(wmclass);

    if (close())
        return;

    m_oldMenuMouseTracking = menuMouseTracking;
    menuMouseTracking = true;

    int xiscreen = manager->getSwitchScreen();
    zItems->begin(zdown);

    resize(xiscreen);

    int item = zItems->getActiveItem();
    if (item >= 0) {
        displayFocus(item);
        isUp = popup(nullptr, nullptr, nullptr, xiscreen, YPopupWindow::pfNoPointerChange);
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
        resize(manager->getSwitchScreen());
        repaint();
    }
}

bool SwitchWindow::handleKey(const XKeyEvent &key) {
    KeySym k = keyCodeToKeySym(key.keycode);
    unsigned int m = KEY_MODMASK(key.state);
    unsigned int vm = VMod(m);

    if (key.type == KeyPress) {
        if (zItems->isKey(k, vm)) {
            m_hlItemFromMotion = -1;
            int focused = zItems->moveTarget(true);
            displayFocus(focused);
            return true;
        } else if ((IS_WMKEY(k, vm, gKeySysSwitchLast))) {
            m_hlItemFromMotion = -1;
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
    //printf("got click, hot item: %d\n", m_hintedItem);
    int hintId = calcHintedItem(button.x, button.y);
    if (button.button == Button1 && button.type == ButtonPress) {
        if (hintId >= 0 && hintId < zItems->getCount()) {
            zItems->reset();
            zItems->setTarget(hintId);
            accept();
        }
    }
    YPopupWindow::handleButton(button);
}

// vim: set sw=4 ts=4 et:
