/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#include "wmaction.h"
#include "ylib.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "yaction.h"
#include "sysdep.h"
#include "prefs.h"
#include "yicon.h"

#include "wmmgr.h"
#include "wmframe.h"
#include "wmwinmenu.h"
#include "workspaces.h"

#include "intl.h"

class ActivateWindowMenuItem: public YMenuItem {
public:
    ActivateWindowMenuItem(YFrameWindow *frame):
        YMenuItem(frame->getTitle(), -1, null, YAction(), nullptr),
        fFrame(frame)
    {
        if (fFrame->clientIcon() != null)
            setIcon(fFrame->clientIcon());
    }

    void actionPerformed(YActionListener *, YAction, unsigned modifiers) override {
        for (YFrameWindow *f = manager->topLayer(); f; f = f->nextLayer()) {
            if (f == fFrame) {
                if (modifiers & ShiftMask)
                    f->wmOccupyOnlyWorkspace(manager->activeWorkspace());
                f->activateWindow(true, false);
                return ;
            }
        }
    }
private:
    YFrameWindow *fFrame;
};


YMenu *YWindowManager::createWindowMenu(YMenu *menu, long workspace) {
    if (!menu)
        menu = new YMenu();

    int level, levelCount, windowLevel, layerCount;
    YFrameWindow *frame;
    bool needSeparator = false;

    /// !!! fix performance (smarter update, on change only)
    for (int layer = 0 ; layer < WinLayerCount; layer++) {
        layerCount = 0;
        if (top(layer) == nullptr)
            continue;
        for (level = 0; level < 4; level++) {
            levelCount = 0;
            for (frame = top(layer); frame; frame = frame->next()) {
                if (!frame->client()->adopted())
                    continue;
                if (!frame->visibleOn(workspace))
                    continue;
                if (frame->frameOption(YFrameWindow::foIgnoreWinList))
                    continue;
                if (workspace != activeWorkspace() && frame->visibleNow())
                    continue;

                windowLevel = 0;
                if (frame->isHidden())
                    windowLevel = 3;
                else if (frame->isMinimized())
                    windowLevel = 2;
                else if (frame->isRollup())
                    windowLevel = 1;

                if (level != windowLevel)
                    continue;

                if ((levelCount == 0 && level > 0) ||
                    ((layerCount == 0 && layer > 0) && needSeparator))
                    menu->addSeparator();

                menu->add(new ActivateWindowMenuItem(frame));
                levelCount++;
                layerCount++;
                needSeparator = true;
            }
        }
    }
    return menu;
}

WindowListMenu::WindowListMenu(YActionListener *app, YWindow *parent):
    YMenu(parent)
{
    setActionListener(app);
}

void WindowListMenu::updatePopup() {
    removeAll();

    manager->createWindowMenu(this, manager->activeWorkspace()); // !!! fix

    bool first = true;

    for (int d = 0; d < workspaceCount; d++) {
        if (d == manager->activeWorkspace())
            continue;
        if (first) {
            addSeparator();
            first = false;
        }
        char s[128];
        snprintf(s, sizeof s,
                _("%lu. Workspace %-.32s"), (unsigned long)(d + 1),
                workspaceNames[d]);

        YMenu *sub = nullptr;
        if (manager->windowCount(d) > 0) // !!! do lazy create menu instead
            sub = manager->createWindowMenu(nullptr, d);
        addItem(s, (d < 10) ? 0 : -1, workspaceActionActivate[d], sub);
    }
    addSeparator();
    addItem(_("_Window list"), -2, KEY_NAME(gKeySysWindowList), actionWindowList);
}

void WindowListMenu::activatePopup(int flags) {
    super::activatePopup(flags);
    repaint();
}

// vim: set sw=4 ts=4 et:
