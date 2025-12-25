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
#include "keysyms.h"
#include <ctype.h>

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
    bool operator==(bool b) const { return bool(*this) == b; }
    bool operator!=(bool b) const { return bool(*this) != b; }
    bool operator!() const { return bool(*this) == false; }

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

        if (quickSwitchPersistence) {
            const WindowOption* wo1 = z1->client->getWindowOption();
            const WindowOption* wo2 = z1->client->getWindowOption();
            int order = (wo1 ? wo1->order : 0) - (wo2 ? wo2->order : 0);
            if (order)
                return order;

            const char* c1 = z1->client->classHint()->res_class;
            const char* c2 = z2->client->classHint()->res_class;
            if (nonempty(c1)) {
                if (nonempty(c2)) {
                    int sc = strcmp(c1, c2);
                    if (sc)
                        return sc;
                } else {
                    return -1;
                }
            } else if (nonempty(c2)) {
                return +1;
            }

            const char* n1 = z1->client->classHint()->res_name;
            const char* n2 = z2->client->classHint()->res_name;
            if (nonempty(n1)) {
                if (nonempty(n2)) {
                    int sc = strcmp(n1, n2);
                    if (sc)
                        return sc;
                } else {
                    return -1;
                }
            } else if (nonempty(n2)) {
                return +1;
            }
        }

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
        YFullscreenLock lock;
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

    void cancelItem() override {
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

    void acceptItem() override {
        ZItem act = fActiveItem;
        if (act) {
            if (act.client != act.frame->client())
                act.frame->selectTab(act.client);
            act.frame->activateWindow(true, false);
            if (act.frame->isFullscreen())
                act.frame->updateLayer();
        } else {
            cancelItem();
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
        if ( !fLastItem)
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
            !client->frameOption(YFrameWindow::foIgnoreQSwitch) &&
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

    bool isKey(const XKeyEvent& x) override {
        return gKeySysSwitchNext == x || (fWMClass && gKeySysSwitchClass == x);
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

    int nextKey(KeySym key) override;
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

            if (frame->frameOption(YFrameWindow::foIgnoreQSwitch) ||
                client->frameOption(YFrameWindow::foIgnoreQSwitch))
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
        int act = fActiveItem.frame->visible()
                ? lookupClient(fActiveItem.client) : -1;
        if (act < 0) {
            fActiveItem.reset();
        }
        else if (act) {
            fActiveItem = zList[act];
            if (quickSwitchPersistence)
                zTarget = act;
            else if (act == 1)
                zList.swap(0, 1);
            else
                zList.moveto(act, 0);
        }
    }
    if (fLastItem && lookupClient(fLastItem.client) < 0)
        fLastItem.reset();
}

int WindowItemsCtrlr::nextKey(KeySym keysym) {
    if ((keysym & 0xff) == keysym && isalpha(keysym & 0xff)) {
        unsigned char lower = tolower((unsigned char) keysym);
        for (int i = zTarget + 1; i < zList.getCount(); ++i) {
            ZItem act = zList[i];
            ClassHint* hint = act.client->classHint();
            if (hint) {
                if (hint->res_class && lower ==
                    tolower((unsigned char) *hint->res_class)) {
                    return i;
                }
                if (hint->res_name && lower ==
                    tolower((unsigned char) *hint->res_name)) {
                    return i;
                }
            }
        }
    }
    return -1;
}

Switcher* Switcher::newSwitchWindow(YWindow* parent, ISwitchItems* items,
                                    bool verticalStyle) {
    return new SwitchWindow(parent, items, verticalStyle);
}

SwitchWindow::SwitchWindow(YWindow *parent, ISwitchItems *items,
                           bool verticalStyle):
    Switcher(parent),
    zItems(items ? items : new WindowItemsCtrlr),
    m_verticalStyle(verticalStyle),
    m_hlItemFromMotion(-1),
    m_hintAreaStart(0),
    m_hintAreaStep(1),
    m_hintAreaFirst(0),
    m_hintAreaLimit(1),
    m_hintAreaBanks(1),
    m_hintFirstBank(0),
    m_hintAreaOther(0),
    m_hintOtherEnds(0),
    fWorkspace(WorkspaceInvalid),
    switchFg(&clrQuickSwitchText),
    switchBg(&clrQuickSwitch),
    switchBc(&clrQuickSwitchBorder),
    switchHl(&clrQuickSwitchActive),
    switchMfg(&clrActiveTitleBarText),
    switchFont(switchFontName),
    keyPressed(0),
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
    YRestackLock restack;
    close();
    zItems->cancelItem();

    YFrameWindow* f = manager->getFocus();
    if (f && f->isFullscreen() && f->getActiveLayer() != WinLayerFullscreen)
        f->updateLayer();
    manager->restackWindows();
}

void SwitchWindow::accept() {
    YRestackLock restack;
    close();
    zItems->acceptItem();
    manager->restackWindows();
}

SwitchWindow::~SwitchWindow() {
    close();
    fGradient = null;
    delete zItems;
}

void SwitchWindow::resize(int xiscreen, bool reposition) {
    const int zCount = zItems->getCount();
    const int active = zItems->getActiveItem();
    const int vMargins = 2 * max(quickSwitchVMargin, quickSwitchIMargin);
    const YRect geom(desktop->getScreenGeometry(xiscreen));
    const int dx = geom.xx, dy = geom.yy, dw = int(geom.ww), dh = int(geom.hh);
    const int maxWidth = dw * 6/7;

    MSG(("got geometry for %d: %d %d %d %d", xiscreen, dx, dy, dw, dh));

    int assumeWidth = quickSwitchSmallWindow ? dw * 1/3 :
                             m_verticalStyle ? dw * 2/5 : dw * 3/5;
    int txtWidth = min(maxWidth, 10 * int(YIcon::largeSize()));
    if (switchFont) {
        const int limit = min(zCount, 99);
        const int space = int(switchFont->textWidth(" "));
        mstring a = zItems->getTitle(active), b, c;
        for (int i = 0; i < limit; i++) {
            if (i != active) {
                mstring title = zItems->getTitle(i);
                if (title.length() > a.length()) {
                    c = b, b = a, a = title;
                }
                else if (title.length() > b.length()) {
                    if (title != a)
                        c = b, b = title;
                }
                else if (title.length() > c.length()) {
                    if (title != a && title != b)
                        c = title;
                }
            }
        }
        for (mstring d : { a, b, c } ) {
            if (d.nonempty()) {
                int titleWidth = switchFont->textWidth(d) + 2*space;
                if (txtWidth < titleWidth)
                    txtWidth = titleWidth;
            }
        }
    }

    if (m_verticalStyle || !quickSwitchAllIcons)
        txtWidth += 2 * quickSwitchIMargin + YIcon::largeSize() + 3;
    if (assumeWidth < txtWidth)
        assumeWidth = txtWidth;

    int w = assumeWidth;
    int h = switchFont ? switchFont->height() : 1;

    if (m_verticalStyle) {
        m_hintAreaBanks = 1;
        m_hintAreaStep = YIcon::largeSize() + quickSwitchIMargin;
        m_hintAreaStart = quickSwitchVMargin - quickSwitchIBorder;
        h = zCount * m_hintAreaStep;
        const int vertical = (dh - (reposition ? 0 : (y() - dy)));
        if (h > vertical - m_hintAreaStep - vMargins) {
            h = (vertical - m_hintAreaStep - vMargins)
                / m_hintAreaStep * m_hintAreaStep;
            if (w * 3/2 < maxWidth) {
                int n = h / m_hintAreaStep;
                m_hintAreaBanks = max(2, min(maxWidth / txtWidth,
                                      (zCount + n - 1) / n));
                w = min(maxWidth, m_hintAreaBanks * txtWidth);
                int k = (zCount + (m_hintAreaBanks - 1))
                        / m_hintAreaBanks * m_hintAreaStep;
                if (h > k)
                    h = k;
            }
        }

        if (w > maxWidth)
            w = maxWidth;
        w += quickSwitchSepSize * m_hintAreaBanks;
    } else {
        m_hintAreaBanks = 1;
        int perIcon = YIcon::largeSize() + 2 * quickSwitchIMargin;
        int perHuge = quickSwitchHugeIcon
                    ? YIcon::hugeSize() - YIcon::largeSize() : 0;
        int iconWidth = zCount * perIcon + perHuge;
        int iconHeight = perIcon + perHuge;

        if (assumeWidth < iconWidth)
            w = iconWidth;
        else if (assumeWidth > max(iconWidth, txtWidth))
            w = max(max(iconWidth, txtWidth), dw * 1/3);
        else
            w = assumeWidth;

        if (quickSwitchAllIcons)
            h += quickSwitchSepSize + iconHeight;
        else if (h < iconHeight)
            h = iconHeight;

        if (w > maxWidth) {
            int num = max(1,
                      int(maxWidth - perHuge - 2*quickSwitchHMargin) / perIcon);
            int banks = (zCount + (num - 1)) / num;
            num = (zCount + (banks - 1)) / banks;
            w = num * perIcon + perHuge + 2*quickSwitchHMargin;
            int maxBanks = (dh - vMargins - perHuge - perIcon
                            - quickSwitchSepSize) / perIcon;
            m_hintAreaBanks = max(1, min(banks, maxBanks));
        }
        if (w > maxWidth) {
            w = maxWidth;
        }
        if (m_hintAreaBanks > 1) {
            h += (m_hintAreaBanks - 1) * iconHeight;
        }
    }
    h += vMargins;
    w += 2*quickSwitchHMargin;

    if (reposition) {
        setGeometry(YRect(dx + (dw - w)/2, dy + (dh - h)/2, w, h));
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

    if (switchBc) {
        g.setColor(switchBc);
        g.setLineWidth(1);
        g.drawRect(0, 0, width() - 1, height() - 1);
    } else {
        g.setColor(switchBg);
        g.drawBorderW(0, 0, width() - 1, height() - 1, true);
    }

    if (fGradient != null)
        g.drawImage(fGradient, 0, 0, width() - 2, height() - 2, 1, 1);
    else if (switchbackPixmap != null)
        g.fillPixmap(switchbackPixmap, 1, 1, width() - 2, height() - 2);
    else {
        g.setColor(switchBg);
        g.fillRect(1, 1, width() - 3, height() - 3);
    }

    m_verticalStyle ? paintVertical(g) : paintHorizontal(g);
}

void SwitchWindow::paintHorizontal(Graphics &g) {
    const int active = zItems->getActiveItem();
    if (active >= 0) {
        const int iconSize = quickSwitchHugeIcon
                           ? YIcon::hugeSize()
                           : YIcon::largeSize();
        int tOfs = 0;

        ref<YIcon> icon = zItems->getIcon(active);
        if ( !quickSwitchAllIcons && icon != null) {
            const int iconWidth = iconSize, iconHeight = iconSize;
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

                tOfs = iconWidth + quickSwitchIMargin + quickSwitchSepSize;
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
        g.setFont(switchFont);

        mstring cTitle = zItems->getTitle(active);
        if (cTitle != null && switchFont) {
            const int x = max((width() - tOfs -
                               switchFont->textWidth(cTitle)) >> 1, 0U) + tOfs;
            const int y(quickSwitchAllIcons
                        ? quickSwitchTextFirst
                        ? quickSwitchVMargin + switchFont->ascent() + 2
                        : height() - quickSwitchVMargin - switchFont->descent()
                        : ((height() + switchFont->height()) >> 1) -
                            switchFont->descent());

            g.drawChars(cTitle, x, y);

            if (quickSwitchAllIcons && quickSwitchSepSize) {
                int const h(quickSwitchVMargin +
                            (iconSize + 2*quickSwitchIMargin) * m_hintAreaBanks
                            + quickSwitchSepSize / 2);
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

            const int bankSize = (width() - 2 * quickSwitchHMargin - ds) / dx;

            int y = quickSwitchTextFirst
                  ? quickSwitchVMargin
                    + (switchFont ? switchFont->height() : 1)
                    + quickSwitchSepSize
                    + quickSwitchIMargin + ds / 2
                  : quickSwitchVMargin + ds + quickSwitchIMargin - ds / 2;

            YColor frameColor = switchHl ? switchHl : switchBg->brighter();
            g.setColor(frameColor);

            const int zCount = zItems->getCount();

            int x = (width() - min(bankSize, zCount) * dx - ds) / 2 +
                     quickSwitchIMargin;

            m_hintAreaStart = x;
            m_hintAreaStep = dx;
            m_hintAreaFirst = bankSize * m_hintFirstBank;
            m_hintAreaLimit = min(zCount, bankSize *
                                  (m_hintFirstBank + m_hintAreaBanks));
            m_hintAreaOther = m_hintOtherEnds = y;

            int xcopy = x;
            for (int b = 0; b < m_hintAreaBanks; ++b) {
                int first = (b + m_hintFirstBank) * bankSize;
                int limit = min((b + 1) * bankSize, zCount);
                x = xcopy;
                for (int i = first; i < limit; i++) {
                    ref<YIcon> icon = zItems->getIcon(i);
                    if (icon != null) {
                        if (i == m_hlItemFromMotion &&
                            i != active)
                        {
                            g.setColor(frameColor.darker());
                            g.drawRect(x - quickSwitchIBorder,
                                    y - quickSwitchIBorder - ds / 2,
                                    YIcon::largeSize() + 2 * quickSwitchIBorder,
                                    iconSize + 2 * quickSwitchIBorder);
                            g.setColor(frameColor);
                        }
                        if (i == active) {
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
                            x += ds;
                        } else {
                            icon->draw(g, x, y, YIcon::largeSize());
                        }
                        x += dx;
                        if (quickSwitchGroupWorkspaces && i + 1 < limit &&
                            zItems->getWorkspace(i) != zItems->getWorkspace(i+1))
                        {
                            int xoffset = x - quickSwitchIMargin;
                            int y2 = (i == active || i + 1 == active)
                                     ? ds / 4 : 0;
                            g.setColor(switchBg->darker());
                            g.drawLine(xoffset - 1, y - y2 - quickSwitchIMargin,
                                       xoffset - 1, y + y2 + dx);
                            g.setColor(switchBg->brighter());
                            g.drawLine(xoffset, y - y2 - quickSwitchIMargin,
                                       xoffset, y + y2 + dx);
                            g.setColor(frameColor);
                        }
                    }
                }
                y += dx + ds;
            }
            m_hintOtherEnds = y;
        }
    }
}

int SwitchWindow::hintedItem(int x, int y)
{
    if (m_verticalStyle) {
        int count = (int(height()) - m_hintAreaStart - quickSwitchVMargin)
                  / m_hintAreaStep;
        int ends = m_hintAreaStart + m_hintAreaStep * count;
        if (x >= 0 && x < int(width()) &&
            y >= m_hintAreaStart && y < ends) {
            int i = (y - m_hintAreaStart) / m_hintAreaStep + m_hintAreaFirst;
            i += count * (x / (int(width()) / m_hintAreaBanks));
            if (i >= 0 && i < zItems->getCount())
                return i;
        }
    }
    else if (quickSwitchAllIcons) {
        int iconsize = YIcon::largeSize();
        int active = zItems->getActiveItem();
        int ds = quickSwitchHugeIcon ? YIcon::hugeSize() - iconsize : 0;
        int count = (int(width()) - 2*quickSwitchHMargin - ds) / m_hintAreaStep;
        int ends = m_hintAreaStart + m_hintAreaStep * count;
        if (y >= m_hintAreaOther - ds/2 &&
            y < m_hintOtherEnds + iconsize &&
            x >= m_hintAreaStart && x < ends + ds) {
            int b = (y - m_hintAreaOther + ds/2) / (m_hintAreaStep + ds);
            int o = (x - m_hintAreaStart) / m_hintAreaStep;
            int i = (b + m_hintFirstBank) * count + o;
            if (i > active && quickSwitchHugeIcon &&
                active / count == b + m_hintFirstBank) {
                o = (x - m_hintAreaStart - ds) / m_hintAreaStep;
                int t = (b + m_hintFirstBank) * count + o;
                i = (t >= active) ? t : active;
            }
            if (i >= 0 && i < zItems->getCount())
                return i;
        }
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
    const int iconSize = YIcon::largeSize();
    const int active = zItems->getActiveItem();
    const int maxWid = width() - 2; // reduce due to 3D edge
    const int bankWidth = maxWid / m_hintAreaBanks;
    const int itemWidth = bankWidth - 2*quickSwitchHMargin;
    const int frameWid = itemWidth + 2*quickSwitchIBorder;
    const int frameHght = iconSize + 2*quickSwitchIBorder;
    const int vMargins = 2 * max(quickSwitchVMargin, quickSwitchIMargin);
    const int bankCount = (height() - vMargins) / m_hintAreaStep;

    for (int bank = 0; bank < m_hintAreaBanks && 0 <= active; ++bank) {
        const int bankStart = bank * bankWidth;
        const int contentX = bankStart + quickSwitchHMargin;
        const int titleX = quickSwitchTextFirst ? contentX :
                           contentX + iconSize + quickSwitchSepSize;
        const int frameX = contentX - quickSwitchIBorder;
        const int strWid = itemWidth - iconSize - quickSwitchSepSize
                           - 2*quickSwitchHMargin;
        const int sepX = quickSwitchTextFirst
                         ? bankStart + bankWidth
                                    - iconSize - quickSwitchSepSize/2 - 1
                         : contentX + iconSize + quickSwitchSepSize/2 - 1;
        const int first = (bank + m_hintFirstBank) * bankCount;
        const int zCount = min(first + bankCount, zItems->getCount()) - first;
        m_hintAreaFirst = m_hintFirstBank * bankCount;
        m_hintAreaLimit = first + zCount;
        m_hintAreaOther = m_hintOtherEnds = 0;

        int contentY = quickSwitchVMargin + quickSwitchIBorder;

        if (switchFont) {
            g.setFont(switchFont);
        }
        g.setColor(switchFg);
        for (int i = first; i - first < zCount; i++) {
            if (contentY + frameHght > int(quickSwitchIBorder + height()))
                break;
            if (i > 0 && zItems->getWorkspace(i) != zItems->getWorkspace(i-1)) {
                int x0 = 1 + bank * bankWidth;
                int x1 = x0 + bankWidth - 2;
                g.setColor(switchBg->darker());
                g.drawLine(x0, contentY - 4, x1, contentY - 4);
                g.setColor(switchBg->brighter());
                g.drawLine(x0, contentY - 3, x1, contentY - 3);
            }
            if (i == active) {
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
                        ? bankStart + bankWidth - quickSwitchHMargin - iconSize
                        : contentX;
                icon->draw(g, iconX, contentY, iconSize);
            }

            if (i == m_hlItemFromMotion && i != active)
            {
                g.setColor(switchMbg);
                g.drawRect(frameX, contentY-quickSwitchIBorder, frameWid, frameHght);
            }
            contentY += m_hintAreaStep;
        }
        if (quickSwitchSepSize) {
            const int margin = quickSwitchVMargin;
            const int bottom = height() - margin - 2;
            g.setColor(switchBg->darker());
            g.drawLine(sepX + 0, margin, sepX + 0, bottom);
            g.setColor(switchBg->brighter());
            g.drawLine(sepX + 1, margin, sepX + 1, bottom);
        }
        if (bank) {
            g.setColor(switchBg->darker());
            g.drawLine(bankStart - 1, 1, bankStart - 1, height() - 3);
            g.setColor(switchBg->brighter());
            g.drawLine(bankStart, 1, bankStart, height() - 3);
        }
    }
}

void SwitchWindow::begin(bool zdown, unsigned mods, char* wmclass) {
    bool change = zItems->setWMClass(wmclass);
    modsDown = KEY_MODMASK(mods);
    setPointerMotion(true);

    if (zItems->isEmpty() || change || !quickSwitchPersistence ||
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
    m_hlItemFromMotion = -1;
}

void SwitchWindow::displayFocus() {
    zItems->displayFocusChange();
    repaint();
}

void SwitchWindow::destroyedFrame(YFrameWindow *frame) {
    int active = zItems->getActiveItem();
    if (zItems->destroyedItem(frame, nullptr)) {
        m_hlItemFromMotion = -1;
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
        m_hlItemFromMotion = -1;
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


int add_modulo(int active, int delta, int count) {
    return ((active + delta) % count + count) % count;
}

void SwitchWindow::target(int delta) {
    if (delta) {
        m_hlItemFromMotion = -1;
        int active = zItems->getActiveItem();
        int count = zItems->getCount();
        int index = (1 < count) ? add_modulo(active, delta, count) : 0;
        int banksize = 0;

        if (m_verticalStyle) {
            banksize = (int(height()) - m_hintAreaStart) / m_hintAreaStep;
        }
        else if (quickSwitchAllIcons) {
            int dx = YIcon::largeSize() + 2*quickSwitchIMargin;
            int ds = quickSwitchHugeIcon ? YIcon::hugeSize() -
                      YIcon::largeSize() : 0;
            banksize = (width() - 2*quickSwitchHMargin - ds) / dx;
        }

        if (banksize) {
            if (index < active) {
                while (index < m_hintFirstBank * banksize) {
                    m_hintFirstBank--;
                }
            }
            else {
                while (index >=
                       (m_hintFirstBank + m_hintAreaBanks) * banksize) {
                    m_hintFirstBank++;
                }
            }
        }

        zItems->setTarget(index);
        displayFocus();
    }
}

bool SwitchWindow::handleKey(const XKeyEvent &key) {
    KeySym k = keyCodeToKeySym(key.keycode);
    k = mapKeypad(k);
    if (key.type == KeyPress) {
        keyPressed = k;
        if (isKey(key)) {
            target(+1);
        }
        else if (gKeySysSwitchLast == key) {
            target(-1);
        }
        else if (gKeyWinClose == key) {
            zItems->destroyTarget();
        }
        else if (k == XK_Return) {
            accept();
        }
        else if (manager->handleSwitchWorkspaceKey(key)) {
            bool change = (fWorkspace != manager->activeWorkspace());
            fWorkspace = manager->activeWorkspace();
            if ((change && !quickSwitchToAllWorkspaces) ||
                !quickSwitchPersistence) {
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
        else if (k == XK_Down) {
            if (m_verticalStyle) {
                target(+1);
            } else {
                int dx = YIcon::largeSize() + 2*quickSwitchIMargin;
                int ds = quickSwitchHugeIcon ? YIcon::hugeSize() -
                          YIcon::largeSize() : 0;
                int count = (width() - 2*quickSwitchHMargin - ds) / dx;
                int activ = zItems->getActiveItem();
                if (activ + count < zItems->getCount()) {
                    target(+count);
                }
            }
        }
        else if (k == XK_Up) {
            if (m_verticalStyle) {
                target(-1);
            } else {
                int dx = YIcon::largeSize() + 2*quickSwitchIMargin;
                int ds = quickSwitchHugeIcon ? YIcon::hugeSize() -
                          YIcon::largeSize() : 0;
                int count = (width() - 2*quickSwitchHMargin - ds) / dx;
                int activ = zItems->getActiveItem();
                if (activ >= count) {
                    target(-count);
                }
            }
        }
        else if (k == XK_Right) {
            if ( !m_verticalStyle) {
                target(+1);
            } else {
                int count = (int(height()) - m_hintAreaStart) / m_hintAreaStep;
                int activ = zItems->getActiveItem();
                if (activ + count < zItems->getCount()) {
                    target(+count);
                }
            }
        }
        else if (k == XK_Left) {
            if ( !m_verticalStyle) {
                target(-1);
            } else {
                int count = (int(height()) - m_hintAreaStart) / m_hintAreaStep;
                int activ = zItems->getActiveItem();
                if (activ >= count) {
                    target(-count);
                }
            }
        }
        else if (k == XK_End) {
            int num = zItems->getCount();
            int act = zItems->getActiveItem();
            target(num - 1 - act);
        }
        else if (k == XK_Home) {
            m_hintFirstBank = 0;
            target(-zItems->getActiveItem());
        }
        else if (k == XK_Next) {
            int num = zItems->getCount();
            int act = zItems->getActiveItem();
            target(min(act + 10, num - 1) - act);
        }
        else if (k == XK_Prior) {
            int act = zItems->getActiveItem();
            target(max(act - 10, 0) - act);
        }
        else if (k == XK_Delete) {
            zItems->destroyTarget();
        }
        else if (k >= '1' && k <= '9') {
            if (0 <= m_hintAreaFirst && m_hintAreaLimit <= zItems->getCount()) {
                int index = int(k - '1') + m_hintAreaFirst;
                if (index < zItems->getCount())
                    target(index - zItems->getActiveItem());
            }
        }
        else if (zItems->isKey(key) && !modDown(key.state)) {
            accept();
        }
        else {
            int i = zItems->nextKey(k);
            if (i >= 0) {
                target(i - zItems->getActiveItem());
            }
        }
    }
    else if (key.type == KeyRelease) {
        if ((isKey(key) && !modDown(key.state)) || isModKey(key.keycode)) {
            accept();
        }
        else if (k == XK_Escape && k == keyPressed) {
            zItems->setTarget(-1);
            cancel();
        }
    }
    return true;
}

bool SwitchWindow::isKey(const XKeyEvent& key) {
    return zItems->isKey(key);
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
    case Button3:
        m_hlItemFromMotion = -1;
        if ((hint = hintedItem(button.x, button.y)) >= 0) {
            target(hint - zItems->getActiveItem());
            YFrameWindow* frame = zItems->current();
            if (frame) {
                frame->popupSystemMenu(this, button.x_root, button.y_root,
                                       YPopupWindow::pfCanFlipVertical |
                                       YPopupWindow::pfCanFlipHorizontal);
            }
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
