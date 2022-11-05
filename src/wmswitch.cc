/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 *
 * Alt{+Shift}+Tab window switching
 */
#include "config.h"
#include "wmframe.h"
#include "wmswitch.h"
#include "wpixmaps.h"
#include "wmmgr.h"
#include "yxapp.h"
#include "prefs.h"
#include "yprefs.h"

struct ZItem {
    int prio;
    int index;
    YFrameWindow* frame;
    YFrameClient* client;

    ZItem(int p, int i, YFrameWindow* f, YFrameClient* c)
        : prio(p), index(i), frame(f), client(c) { }

    void reset() { prio = 0; frame = nullptr; client = nullptr; }

    operator bool() const { return frame && client; }
    bool operator==(YFrameWindow* f) const { return f == frame && f; }
    bool operator==(YFrameClient* c) const { return c == client && c; }

    static int compare(const void* p1, const void* p2) {
        const ZItem* z1 = static_cast<const ZItem*>(p1);
        const ZItem* z2 = static_cast<const ZItem*>(p2);
        if (quickSwitchGroupWorkspaces) {
            int w1 = z1->frame->getWorkspace();
            int w2 = z2->frame->getWorkspace();
            if (w1 == AllWorkspaces)
                w1 = manager->activeWorkspace();
            if (w2 == AllWorkspaces)
                w2 = manager->activeWorkspace();
            if (w1 != w2) {
                int active = manager->activeWorkspace();
                if (w1 == active)
                    return -1;
                else if (w2 == active)
                    return +1;
                else
                    return w1 - w2;
            }
        }
        if (z1->prio != z2->prio)
            return z1->prio - z2->prio;
        else
            return z1->index - z2->index;
    }
};

class WindowItemsCtrlr : public ISwitchItems
{
    int zTarget;
    YArray<ZItem> zList;
    ZItem fActiveItem;
    ZItem fLastItem;
    char* fWMClass;

    void freeList() {
        zList.clear();
    }

    void changeFocusTo(ZItem to)  {
        YRestackLock restack;
        if (to.frame->visible()) {
            if (to.client != to.frame->client())
                to.frame->selectTab(to.client);
            manager->setFocus(to.frame, false, false);
        }
        if (quickSwitchRaiseCandidate)
            manager->restackWindows();
    }

public:

    int getCount() override
    {
        return zList.getCount();
    }

    bool isEmpty() override
    {
        return zList.isEmpty();
    }

    ref<YIcon> getIcon(int idx) override
    {
        ref<YIcon> icon;
        if (inrange(idx, 0, getCount() - 1)) {
            ZItem act = zList[idx];
            if (act.frame->hasTab(act.client)) {
                icon = act.client->getIcon();
                if (icon == null)
                    icon = act.frame->getIcon();
            }
        }
        return icon;
    }

    void moveTarget(bool zdown) override {
        const int cnt = getCount();
        setTarget(cnt < 2 ? 0 : (zTarget + cnt + (zdown ? 1 : -1)) % cnt);
    }

    void setTarget(int zPosition) override
    {
        zTarget = zPosition;
        if (inrange(zTarget, 0, getCount() - 1))
            fActiveItem = zList[zTarget];
        else
            fActiveItem.reset();
    }

    WindowItemsCtrlr() :
        zTarget(0),
        fActiveItem(0, 0, nullptr, nullptr),
        fLastItem(0, 0, nullptr, nullptr),
        fWMClass(nullptr)
    {
    }

    ~WindowItemsCtrlr()
    {
        if (fWMClass)
            free(fWMClass);
    }

    int getActiveItem() override
    {
        return zTarget;
    }

    mstring getTitle(int idx) override
    {
        if (inrange(idx, 0, getCount() - 1)) {
            ZItem act = zList[idx];
            if (act.frame->hasTab(act.client)) {
                return act.client->windowTitle();
            }
        }
        return null;
    }

    int getWorkspace(int idx) override {
        int ws = 0;
        if (inrange(idx, 0, getCount() - 1)) {
            ws = zList[idx].frame->getWorkspace();
            if (ws == AllWorkspaces)
                ws = manager->activeWorkspace();
        }
        return ws;
    }

    bool setWMClass(char* wmclass) override {
        char nil[] = { '\0' };
        bool change = strcmp(Elvis(wmclass, nil), Elvis(fWMClass, nil));
        if (fWMClass)
            free(fWMClass);
        fWMClass = wmclass;
        return change;
    }

    char* getWMClass() override {
        return fWMClass;
    }

    void updateList() override;
    void sort() override;

    void displayFocusChange() override {
        if (inrange(zTarget, 0, getCount() - 1))
            changeFocusTo(zList[zTarget]);
    }

    void begin(bool zdown) override
    {
        YFrameWindow* focus = manager->getFocus();
        fActiveItem = ZItem(0, 0, focus, focus ? focus->client() : nullptr);
        fLastItem = fActiveItem;
        updateList();
        zTarget = 0;
        moveTarget(zdown);
    }

    void cancel() override {
        ZItem last = fLastItem;
        ZItem act = fActiveItem;
        fLastItem.reset();
        fActiveItem.reset();
        freeList();
        if (last) {
            changeFocusTo(last);
        }
        else if (act) {
            if (act.client != act.frame->client())
                act.frame->selectTab(act.client);
            act.frame->activateWindow(true, false);
        }
    }

    void accept() override {
        ZItem active = fActiveItem;
        if (active) {
            active.frame->activateWindow(true, false);
        } else {
            cancel();
        }
    }

    void destroyTarget() override {
        const int idx = getActiveItem();
        if (inrange(idx, 0, getCount() - 1)) {
            bool confirm = false;
            ZItem act = zList[idx];
            if (act.frame->hasTab(act.client)) {
                act.frame->wmCloseClient(act.client, &confirm);
            }
        }
    }

    bool destroyedItem(YFrameWindow* item, YFrameClient* tab) override
    {
        bool removed = false;
        ZItem previous = fActiveItem;

        for (int i = getCount(); 0 <= --i; ) {
            if (zList[i] == item || zList[i] == tab) {
                zList.remove(i);
                removed = true;
                if (i <= zTarget && 0 < zTarget)
                    --zTarget;
            }
        }

        if (fLastItem == item || fLastItem == tab)
            fLastItem.reset();
        if (fActiveItem == item || fActiveItem == tab)
            fActiveItem.reset();

        setTarget(zTarget);
        if (fLastItem == false)
            fLastItem = fActiveItem;
        if (fActiveItem && fActiveItem != previous)
            changeFocusTo(fActiveItem);

        return removed;
    }

    bool createdItem(YFrameWindow* frame, YFrameClient* client) override
    {
        if (notbit(client->winHints(), WinHintsSkipFocus) &&
            (client->adopted() || frame->visible()) &&
            (frame->isUrgent() || quickSwitchToAllWorkspaces ||
             frame->visibleOn(manager->activeWorkspace())) &&
            (::isEmpty(fWMClass) || client->classHint()->match(fWMClass)) &&
            !frame->frameOption(YFrameWindow::foIgnoreQSwitch) &&
            (!frame->isHidden() || quickSwitchToHidden) &&
            (!frame->isMinimized() || quickSwitchToMinimized))
        {
            zList += ZItem(5, zList.getCount(), frame, client);
            return true;
        }
        return false;
    }

    void transfer(YFrameClient* client, YFrameWindow* frame) override {
        for (int i = 0; i < zList.getCount(); i++) {
            if (zList[i].client == client) {
                zList[i].frame = frame;
            }
        }
    }

    YFrameWindow* current() const override {
        return fActiveItem.frame;
    }

    bool isKey(KeySym k, unsigned vm) override {
        return gKeySysSwitchNext.eq(k, vm) || (fWMClass &&
               gKeySysSwitchClass.eq(k, vm));
    }

    unsigned modifiers() override {
        return gKeySysSwitchNext.mod
             | gKeySysSwitchLast.mod
             | gKeySysSwitchClass.mod;
    }

    int lookupClient(YFrameClient* client) {
        for (int i = 0; i < zList.getCount(); ++i)
            if (client == zList[i].client)
                return i;
        return -1;
    }
};

void WindowItemsCtrlr::updateList() {
    YFrameWindow* const focused = manager->getFocus();
    YFrameClient* const fclient = focused ? focused->client() : nullptr;
    int const current = manager->activeWorkspace();
    int index = 0;

    zList.clear();
    for (YFrameIter iter(manager->focusedReverseIterator()); ++iter; ) {
        for (YFrameClient* client : iter->clients()) {
            YFrameWindow* frame = iter;

            if (hasbit(client->winHints(), WinHintsSkipFocus))
                continue;

            if (!client->adopted() && !frame->visible())
                continue;

            if (!quickSwitchToAllWorkspaces) {
                if (!frame->hasState(WinStateUrgent) &&
                    !client->urgencyHint() &&
                    !frame->visibleOn(current))
                    continue;
            }

            if (frame->frameOption(YFrameWindow::foIgnoreQSwitch))
                continue;

            if (nonempty(fWMClass)) {
                if (client->classHint()->match(fWMClass) == false)
                    continue;
            }

            int prio = 0;
            if (frame == focused) {
                if (client == fclient)
                    prio = 1;
                else
                    prio = 2;
            }
            else if (quickSwitchToUrgent && frame->isUrgent()) {
                prio = 3;
            }
            else if (frame->isMinimized()) {
                if (quickSwitchToMinimized)
                    prio = 6;
            }
            else if (frame->isHidden()) {
                if (quickSwitchToHidden)
                    prio = 7;
            }
            else if (frame->avoidFocus()) {
                prio = 8;
            }
            else {
                prio = 4;
            }
            if (prio) {
                zList += ZItem(prio, index, frame, client);
                index += 1;
            }
        }
    }
    sort();
}

void WindowItemsCtrlr::sort() {
    if (1 < zList.getCount())
        qsort(&*zList, size_t(zList.getCount()), sizeof(ZItem), ZItem::compare);

    zTarget = 0;
    if (fActiveItem) {
        int act = lookupClient(fActiveItem.client);
        if (act < 0) {
            fActiveItem.reset();
        }
        else if (act) {
            fActiveItem = zList[act];
            zTarget = act;
        }
    }
    if (fLastItem && lookupClient(fLastItem.client) < 0)
        fLastItem.reset();
}

SwitchWindow::SwitchWindow(YWindow *parent, ISwitchItems *items,
                           bool verticalStyle):
    YPopupWindow(parent),
    zItems(items ? items : new WindowItemsCtrlr),
    m_verticalStyle(verticalStyle),
    m_oldMenuMouseTracking(menuMouseTracking),
    m_hlItemFromMotion(-1),
    m_hintAreaStart(0),
    m_hintAreaStep(1),
    fWorkspace(WorkspaceInvalid),
    switchFg(&clrQuickSwitchText),
    switchBg(&clrQuickSwitch),
    switchHl(&clrQuickSwitchActive),
    switchMfg(&clrActiveTitleBarText),
    switchFont(switchFontName),
    modsDown(0)
{
    // I prefer clrNormalMenu but some themes use inverted settings where
    // clrNormalMenu is the same as clrQuickSwitch
    if (clrQuickSwitchActive)
        switchMbg = &clrQuickSwitchActive;
    else if (!strcmp(clrNormalMenu, clrQuickSwitch))
        switchMbg = &clrActiveMenuItem;
    else
        switchMbg = &clrNormalMenu;

    setStyle(wsSaveUnder | wsOverrideRedirect | wsPointerMotion | wsNoExpose);
    setTitle("IceSwitch");
    setClassHint("switch", "IceWM");
    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_DIALOG);
}

void SwitchWindow::close() {
    if (visible())
        cancelPopup();
}

void SwitchWindow::cancel() {
    close();
    zItems->cancel();
}

void SwitchWindow::accept() {
    close();
    zItems->accept();
}

SwitchWindow::~SwitchWindow() {
    close();
    fGradient = null;
    delete zItems;
}

void SwitchWindow::resize(int xiscreen, bool reposition) {
    int dx, dy;
    unsigned dw, dh;

    desktop->getScreenGeometry(&dx, &dy, &dw, &dh, xiscreen);

    MSG(("got geometry for %d: %d %d %d %d", xiscreen, dx, dy, dw, dh));

    mstring cTitle = zItems->getTitle(zItems->getActiveItem());

    int aWidth =
        quickSwitchSmallWindow ?
        (int) dw * 1/3
        : (m_verticalStyle ? (int) dw * 2/5 : (int) dw * 3/5);

    int tWidth = 0;
    if (quickSwitchMaxWidth && switchFont) {
        int space = (int) switchFont->textWidth(" ");   /* make entries one space character wider */
        int zCount = zItems->getCount();
        for (int i = 0; i < zCount; i++) {
            mstring title = zItems->getTitle(i);
            int oWidth = title != null ? (int) switchFont->textWidth(title) + space : 0;
            if (oWidth > tWidth)
                tWidth = oWidth;
        }
    } else {
        tWidth = cTitle != null && switchFont ? switchFont->textWidth(cTitle) : 0;
    }

    if (m_verticalStyle || !quickSwitchAllIcons)
        tWidth += 2 * quickSwitchIMargin + YIcon::largeSize() + 3;
    if (tWidth > aWidth)
        aWidth = tWidth;

    int w = aWidth;
    int h = switchFont ? switchFont->height() : 1;
    int const mWidth(dw * 6/7);
    const int vMargins = 2 * max(quickSwitchVMargin, quickSwitchIMargin);

    if (m_verticalStyle) {
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

    if (reposition) {
        setGeometry(YRect(dx + ((dw - w) >> 1),
                          dy + ((dh - h) >> 1),
                          w, h));
    } else {
        setSize(w, h);
    }
}

void SwitchWindow::repaint() {
    if (visible()) {
        GraphicsBuffer(this).paint();
    }
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

    m_verticalStyle ? paintVertical(g) : paintHorizontal(g);
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
                g.drawLine(x + 0, 1, x + 0, height() - 2);
                g.setColor(switchBg->brighter());
                g.drawLine(x + 1, 1, x + 1, height() - 2);
            }
        }

        g.setColor(switchFg);
        if (switchFont) {
            g.setFont(switchFont);
        }

        mstring cTitle = zItems->getTitle(zItems->getActiveItem());
        if (cTitle != null && switchFont) {
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
                        if (i == m_hlItemFromMotion &&
                            i != zItems->getActiveItem())
                        {
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
                            }
                        } else {
                            icon->draw(g, x, y, YIcon::largeSize());
                        }
                        x += ds;
                    }
                    x += dx;
                }
            }
        }
    }
}

int SwitchWindow::hintedItem(int x, int y)
{
    int ends = m_hintAreaStart + m_hintAreaStep * zItems->getCount();
    if (m_verticalStyle) {
        if (x >= 0 && x < int(width()) &&
            y >= m_hintAreaStart && y < ends)
            return (y - m_hintAreaStart) / m_hintAreaStep;
    }
    else if (quickSwitchAllIcons && !quickSwitchHugeIcon) {
        if (y >= 0 && y < int(height()) &&
            x >= m_hintAreaStart && x < ends)
            return (x - m_hintAreaStart) / m_hintAreaStep;
    }
    return -2;
}

void SwitchWindow::handleMotion(const XMotionEvent& motion) {
    int hint = hintedItem(motion.x, motion.y);
    if (m_hlItemFromMotion != hint) {
        m_hlItemFromMotion = hint;
        repaint();
    }
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

        int contentY = quickSwitchVMargin + quickSwitchIBorder;

        if (switchFont) {
            g.setFont(switchFont);
        }
        g.setColor(switchFg);
        for (int i = 0, zCount = zItems->getCount(); i < zCount; i++) {
            if (contentY + frameHght > int(quickSwitchIBorder + height()))
                break;
            if (i > 0 && zItems->getWorkspace(i) != zItems->getWorkspace(i-1)) {
                g.setColor(switchBg->darker());
                g.drawLine(1, contentY - 4, width() - 2, contentY - 4);
                g.setColor(switchBg->brighter());
                g.drawLine(1, contentY - 3, width() - 2, contentY - 3);
            }
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
            }

            if (i == m_hlItemFromMotion && i != zItems->getActiveItem())
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

void SwitchWindow::begin(bool zdown, unsigned mods, char* wmclass) {
    bool change = zItems->setWMClass(wmclass);
    modsDown = KEY_MODMASK(mods);
    m_oldMenuMouseTracking = menuMouseTracking;
    menuMouseTracking = true;

    if (zItems->isEmpty() || change ||
        (fWorkspace != manager->activeWorkspace() &&
         !quickSwitchToAllWorkspaces)) {
        zItems->begin(zdown);
    } else {
        zItems->sort();
        zItems->moveTarget(zdown);
    }
    fWorkspace = manager->activeWorkspace();

    if (zItems->getCount()) {
        int screen = manager->getSwitchScreen();
        resize(screen, true);
        if (popup(nullptr, nullptr, manager, screen, pfNoPointerChange)
            && visible()) {
            unsigned mask;
            if (xapp->queryMask(handle(), &mask) && !modDown(mask))
                accept();
        }
    }
}

void SwitchWindow::activatePopup(int flags) {
    displayFocus();
}

void SwitchWindow::deactivatePopup() {
    menuMouseTracking = m_oldMenuMouseTracking;
    m_hlItemFromMotion = -1;
}

void SwitchWindow::displayFocus() {
    zItems->displayFocusChange();
    repaint();
}

void SwitchWindow::destroyedFrame(YFrameWindow *frame) {
    int active = zItems->getActiveItem();
    if (zItems->destroyedItem(frame, nullptr)) {
        if (zItems->isEmpty()) {
            cancel();
        }
        else if (visible()) {
            resize(getScreen(), active > zItems->getActiveItem());
            repaint();
        }
    }
}

void SwitchWindow::destroyedClient(YFrameClient* client) {
    int active = zItems->getActiveItem();
    if (zItems->destroyedItem(nullptr, client)) {
        if (zItems->isEmpty()) {
            cancel();
        }
        else if (visible()) {
            resize(getScreen(), active > zItems->getActiveItem());
            repaint();
        }
    }
}

void SwitchWindow::createdFrame(YFrameWindow *frame) {
    createdClient(frame, frame->client());
}

void SwitchWindow::createdClient(YFrameWindow* frame, YFrameClient* client) {
    if (zItems->createdItem(frame, client)) {
        resize(getScreen(), false);
        repaint();
    }
}

void SwitchWindow::transfer(YFrameClient* client, YFrameWindow* frame) {
    zItems->transfer(client, frame);
}

YFrameWindow* SwitchWindow::current() {
    return zItems->current();
}

void SwitchWindow::target(int delta) {
    if (delta) {
        m_hlItemFromMotion = -1;
        int active = zItems->getActiveItem();
        int count = zItems->getCount();
        int index = (1 < count) ? (active + delta + count) % count : 0;
        zItems->setTarget(index);
        displayFocus();
    }
}

bool SwitchWindow::handleKey(const XKeyEvent &key) {
    KeySym k = keyCodeToKeySym(key.keycode);
    unsigned m = KEY_MODMASK(key.state);
    unsigned vm = VMod(m);

    if (key.type == KeyPress) {
        if (isKey(k, vm)) {
            target(+1);
        }
        else if (gKeySysSwitchLast.eq(k, vm)) {
            target(-1);
        }
        else if (k == XK_Escape) {
            cancel();
        }
        else if (gKeyWinClose.eq(k, vm)) {
            zItems->destroyTarget();
        }
        else if (k == XK_Return) {
            accept();
        }
        else if (manager->handleSwitchWorkspaceKey(key, k, vm)) {
            bool change = (fWorkspace != manager->activeWorkspace());
            fWorkspace = manager->activeWorkspace();
            if (change && !quickSwitchToAllWorkspaces) {
                zItems->updateList();
            } else {
                zItems->sort();
            }
            if (zItems->getCount()) {
                resize(getScreen(), true);
                repaint();
            } else {
                cancel();
            }
        }
        else if (k == XK_Down && m_verticalStyle) {
            target(+1);
        }
        else if (k == XK_Up && m_verticalStyle) {
            target(-1);
        }
        else if (k == XK_Right && !m_verticalStyle) {
            target(+1);
        }
        else if (k == XK_Left && !m_verticalStyle) {
            target(-1);
        }
        else if (k == XK_End) {
            int num = zItems->getCount();
            int act = zItems->getActiveItem();
            target(num - 1 - act);
        }
        else if (k == XK_Home) {
            target(-zItems->getActiveItem());
        }
        else if (k == XK_Delete) {
            zItems->destroyTarget();
        }
        else if (k >= '1' && k <= '9') {
            int index = int(k - '1');
            if (index < zItems->getCount())
                target(index - zItems->getActiveItem());
        }
        else if (zItems->isKey(k, vm) && !modDown(m)) {
            accept();
        }
    }
    else if (key.type == KeyRelease) {
        if ((isKey(k, vm) && !modDown(m)) || isModKey(key.keycode)) {
            accept();
        }
    }
    return true;
}

bool SwitchWindow::isKey(KeySym k, unsigned vm) {
    return zItems->isKey(k, vm);
}

unsigned SwitchWindow::modifiers() {
    return zItems->modifiers();
}

bool SwitchWindow::isModKey(KeyCode c) {
    KeySym k = keyCodeToKeySym(c);

    if (k == XK_Shift_L || k == XK_Shift_R)
        return hasbit(modsDown, ShiftMask)
            && hasbit(modifiers(), kfShift)
            && modsDown == ShiftMask;

    if (k == XK_Control_L || k == XK_Control_R)
        return hasbit(modsDown, ControlMask)
            && hasbit(modifiers(), kfCtrl);

    if (k == XK_Alt_L     || k == XK_Alt_R)
        return hasbit(modsDown, xapp->AltMask)
            && hasbit(modifiers(), kfAlt);

    if (k == XK_Meta_L    || k == XK_Meta_R)
        return hasbit(modsDown, xapp->MetaMask)
            && hasbit(modifiers(), kfMeta);

    if (k == XK_Super_L   || k == XK_Super_R)
        return hasbit(modsDown, xapp->SuperMask)
            && (modSuperIsCtrlAlt
                ? hasbits(modifiers(), kfCtrl + kfAlt)
                : hasbit(modifiers(), kfSuper));

    if (k == XK_Hyper_L   || k == XK_Hyper_R)
        return hasbit(modsDown, xapp->HyperMask)
            && hasbit(modifiers(), kfHyper);

    if (k == XK_ISO_Level3_Shift || k == XK_Mode_switch)
        return hasbit(modsDown, xapp->ModeSwitchMask)
            && hasbit(modifiers(), kfAltGr);

    return false;
}

bool SwitchWindow::modDown(unsigned mod) {
    return modsDown == ShiftMask
        ? hasbit(KEY_MODMASK(mod), ShiftMask)
        : hasbits(KEY_MODMASK(mod), modsDown & ~ShiftMask);
}

void SwitchWindow::handleButton(const XButtonEvent &button) {
    int hint;
    switch (button.button * (button.type == ButtonPress ? 1 : -1)) {
    case Button1:
        m_hlItemFromMotion = -1;
        if ((hint = hintedItem(button.x, button.y)) >= 0) {
            zItems->setTarget(hint);
            accept();
        }
        else if (!geometry().contains(button.x_root, button.y_root)) {
            cancel();
        }
        break;
    case Button2:
        m_hlItemFromMotion = -1;
        if ((hint = hintedItem(button.x, button.y)) >= 0) {
            target(hint - zItems->getActiveItem());
            zItems->destroyTarget();
        }
        break;
    case Button4:
        target(-1);
        break;
    case Button5:
        target(+1);
        break;
    default:
        break;
    }
}

// vim: set sw=4 ts=4 et:
