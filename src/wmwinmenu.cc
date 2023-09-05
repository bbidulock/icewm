/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "prefs.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "wmwinmenu.h"
#include "intl.h"

void WindowListMenu::actionPerformed(YAction action, unsigned modifiers) {
    YFrameClient* tab = manager->findClient(Window(action.ident()));
    if (tab) {
        YFrameWindow* f = tab->obtainFrame();
        if (f) {
            f->selectTab(tab);
            if (modifiers & ShiftMask)
                f->wmOccupyWorkspace(manager->activeWorkspace());
            f->activateWindow(true, false);
        }
        return ;
    }

    for (int w = 0; w < workspaceCount; w++) {
        if (workspaceActionActivate[w] == action) {
            manager->activateWorkspace(w);
            return ;
        }
    }
    if (action == actionWindowList) {
        defer->actionPerformed(action, modifiers);
    }
}

WindowListMenu::WindowListMenu(YActionListener *app, YWindow *parent):
    YMenu(parent)
    , defer(app)
{
    setActionListener(this);
    setShared(true);
}

void WindowListMenu::updatePopup() {
    removeAll();

    int activeWorkspace = manager->activeWorkspace();

    struct entry {
        YFrameWindow *frame;
        int space;
        int layer;
        int level;
        entry(YFrameWindow* f, int s, int la, int le)
            : frame(f), space(s), layer(la), level(le) { }
        bool operator<(const entry& e) {
            return (space != e.space) ? space < e.space :
                   (layer != e.layer) ? layer < e.layer : level < e.level;
        }
    };

    YArray<entry> entries;

    for (int layer = 0 ; layer < WinLayerCount; layer++) {
        YFrameWindow *frame = manager->top(layer);
        for (; frame; frame = frame->next()) {
            if (frame->client()->adopted() &&
                !frame->frameOption(YFrameWindow::foIgnoreWinList))
            {
                int level =
                    frame->isHidden() ? 3 :
                    frame->isMinimized() ? 2 :
                    frame->isRollup() ? 1 : 0;
                int space = (frame->getWorkspace() == AllWorkspaces ||
                             frame->getWorkspace() == activeWorkspace)
                            ? 0 : 1 + frame->getWorkspace();
                entry et(frame, space, layer, level);
                int lo = 0, hi = entries.getCount();
                while (lo < hi) {
                    int pv = (lo + hi) / 2;
                    if (et < entries[pv])
                        hi = pv;
                    else
                        lo = pv + 1;
                }
                entries.insert(lo, et);
            }
        }
    }

    int k = 0;
    for (int space = 0; space <= workspaceCount; space++) {
        if (space - 1 != activeWorkspace) {
            YMenu* addTo = this;
            if (0 < space) {
                char buf[128];
                snprintf(buf, sizeof buf, _("%lu. Workspace %-.32s"),
                         (unsigned long) space, workspaceNames[space - 1]);
                YMenu* sub = nullptr;
                if (k < entries.getCount() && entries[k].space == space) {
                    sub = addTo = new YMenu();
                    if (sub) {
                        setActionListener(this);
                    }
                }
                if (space == 1 + (activeWorkspace == 0)) {
                    addSeparator();
                }
                addItem(buf, (space < 10) ? 0 : -1,
                        workspaceActionActivate[space - 1], sub);
            }
            for (; k < entries.getCount() && entries[k].space <= space; ++k) {
                if (entries[k].space == space && addTo) {
                    if (0 < k && 0 < addTo->itemCount()) {
                        if (entries[k - 1] < entries[k])
                            addTo->addSeparator();
                    }
                    YFrameWindow* frame = entries[k].frame;
                    for (YFrameClient* tab : frame->clients()) {
                        YAction action(EAction(int(tab->handle())));
                        mstring title(tab->windowTitle());
                        YMenuItem* item = new YMenuItem(title, -1, null,
                                                        action, nullptr);
                        if (item) {
                            ref<YIcon> icon = tab->getIcon();
                            if (icon == null)
                                icon = frame->getIcon();
                            if (icon != null)
                                item->setIcon(icon);
                            addTo->add(item);
                        }
                    }
                }
            }
        }
    }
    addSeparator();
    addItem(_("_Window list"), -2, KEY_NAME(gKeySysWindowList), actionWindowList);
    setActionListener(this);
}

void WindowListMenu::activatePopup(int flags) {
    super::activatePopup(flags);
    repaint();
}

// vim: set sw=4 ts=4 et:
