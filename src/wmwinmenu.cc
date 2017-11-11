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

#include "intl.h"

class ActivateWindowMenuItem: public YMenuItem {
public:
    ActivateWindowMenuItem(YFrameWindow *frame):
        YMenuItem(frame->getTitle(), -1, null, YAction(), 0),
        fFrame(frame)
    {
        if (fFrame->clientIcon() != null)
            setIcon(fFrame->clientIcon());
    }

    virtual void actionPerformed(YActionListener * /*listener*/, YAction /*action*/, unsigned int modifiers) {
        YFrameWindow *f = manager->topLayer();

        while (f) {
            if ((void *)f == fFrame) {
                if (modifiers & ShiftMask)
                    f->wmOccupyOnlyWorkspace(manager->activeWorkspace());
                manager->activate(f, true);
                return ;
            }
            f = f->nextLayer();
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
        if (top(layer) == 0)
            continue;
        for (level = 0; level < 4; level++) {
            levelCount = 0;
            for (frame = top(layer); frame; frame = frame->next()) {
                if (!frame->client()->adopted())
                    continue;
                if (!frame->visibleOn(workspace))
                    continue;
                if (frame->frameOptions() & YFrameWindow::foIgnoreWinList)
                    continue;
                if (workspace != activeWorkspace() &&
                    frame->visibleOn(activeWorkspace()))
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

WindowListMenu::WindowListMenu(IApp *app, YWindow *parent): YMenu(parent) {
    this->app = app;
}

void WindowListMenu::updatePopup() {
    removeAll();

    manager->createWindowMenu(this, manager->activeWorkspace()); // !!! fix

    bool first = true;

    for (long d = 0; d < manager->workspaceCount(); d++) {
        if (d == manager->activeWorkspace())
            continue;
        if (first) {
            addSeparator();
            first = false;
        }
        char s[128];
        snprintf(s, sizeof s,
                _("%lu. Workspace %-.32s"), (unsigned long)(d + 1),
                manager->workspaceName(d));

        YMenu *sub = 0;
        if (manager->windowCount(d) > 0) // !!! do lazy create menu instead
            sub = manager->createWindowMenu(0, d);
        addItem(s, (d < 10) ? 0 : -1, workspaceActionActivate[d], sub);
    }
    addSeparator();
    addItem(_("_Window list"), -2, KEY_NAME(gKeySysWindowList), actionWindowList);
}

// vim: set sw=4 ts=4 et:
