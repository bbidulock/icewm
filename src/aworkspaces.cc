#include "config.h"

#ifdef CONFIG_TASKBAR

#include "ylib.h"
#include "aworkspaces.h"
#include "wmtaskbar.h"
#include "prefs.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "wmframe.h"
#include "yrect.h"
#include "yicon.h"

#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "base.h"

YColor * WorkspaceButton::normalButtonBg(NULL);
YColor * WorkspaceButton::normalButtonFg(NULL);

YColor * WorkspaceButton::activeButtonBg(NULL);
YColor * WorkspaceButton::activeButtonFg(NULL);

ref<YFont> WorkspaceButton::normalButtonFont;
ref<YFont> WorkspaceButton::activeButtonFont;

ref<YPixmap> workspacebuttonPixmap;
ref<YPixmap> workspacebuttonactivePixmap;

#ifdef CONFIG_GRADIENTS
ref<YPixbuf> workspacebuttonPixbuf;
ref<YPixbuf> workspacebuttonactivePixbuf;
#endif

WorkspaceButton::WorkspaceButton(long ws, YWindow *parent): ObjectButton(parent, (YAction *)0)
{
    fWorkspace = ws;
    //setDND(true);
}

void WorkspaceButton::handleClick(const XButtonEvent &/*up*/, int /*count*/) {
}

void WorkspaceButton::handleDNDEnter() {
    if (fRaiseTimer == 0)
        fRaiseTimer = new YTimer(autoRaiseDelay);
    if (fRaiseTimer) {
        fRaiseTimer->setTimerListener(this);
        fRaiseTimer->startTimer();
    }
    repaint();
}

void WorkspaceButton::handleDNDLeave() {
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    repaint();
}

bool WorkspaceButton::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        manager->activateWorkspace(fWorkspace);
    }
    return false;
}

void WorkspaceButton::actionPerformed(YAction */*action*/, unsigned int modifiers) {
    if (modifiers & ShiftMask) {
        manager->switchToWorkspace(fWorkspace, true);
    } else if (modifiers & xapp->AltMask) {
        if (manager->getFocus())
            manager->getFocus()->wmOccupyWorkspace(fWorkspace);
    } else {
        manager->activateWorkspace(fWorkspace);
        return;
    }
}

WorkspacesPane::WorkspacesPane(YWindow *parent): YWindow(parent) {
    long w;

    if (workspaceCount > 0)
        fWorkspaceButton = new WorkspaceButton *[workspaceCount];
    else
        fWorkspaceButton = 0;

    if (fWorkspaceButton) {
        YResourcePaths paths("", false);

        int ht = 24;
        int leftX = 0;

        for (w = 0; w < workspaceCount; w++) {
            WorkspaceButton *wk = new WorkspaceButton(w, this);
            if (wk) {
                ref<YIconImage> image
                    (paths.loadImage("workspace/", workspaceNames[w]));

                if (image != null)
                    wk->setImage(image);
                else
                    wk->setText(workspaceNames[w]);

                char * wn(newstr(my_basename(workspaceNames[w])));
                char * ext(strrchr(wn, '.'));
                if (ext) *ext = '\0';

                char * tt(strJoin(_("Workspace: "), wn, NULL));
                delete[] wn;

                wk->setToolTip(tt);
                delete[] tt;

                //if ((int)wk->height() + 1 > ht) ht = wk->height() + 1;
            }
            fWorkspaceButton[w] = wk;
        }

        for (w = 0; w < workspaceCount; w++) {
            YButton *wk = fWorkspaceButton[w];
            //leftX += 2;
            if (wk) {
                wk->setGeometry(YRect(leftX, 0, wk->width(), ht));
                wk->show();
                leftX += wk->width();
            }
        }
        setSize(leftX, ht);
    }
}

WorkspacesPane::~WorkspacesPane() {
    if (fWorkspaceButton) {
        for (long w = 0; w < workspaceCount; w++)
            delete fWorkspaceButton[w];
        delete [] fWorkspaceButton;
    }
}

void WorkspacesPane::configure(const YRect &r, const bool resized) {
    YWindow::configure(r, resized);

    int ht = height();
    int leftX = 0;
    for (int w = 0; w < workspaceCount; w++) {
        YButton *wk = fWorkspaceButton[w];
        //leftX += 2;
        if (wk) {
            wk->setGeometry(YRect(leftX, 0, wk->width(), ht));
            leftX += wk->width();
        }
    }
}

WorkspaceButton *WorkspacesPane::workspaceButton(long n) {
    return (fWorkspaceButton ? fWorkspaceButton[n] : NULL);
}

ref<YFont> WorkspaceButton::getFont() {
    return isPressed()
        ? (*activeWorkspaceFontName || *activeWorkspaceFontNameXft)
        ? activeButtonFont != null
        ? activeButtonFont
        : activeButtonFont = YFont::getFont(XFA(activeWorkspaceFontName))
        : YButton::getFont()
        : (*normalWorkspaceFontName || *normalWorkspaceFontNameXft)
        ? normalButtonFont != null
        ? normalButtonFont
        : normalButtonFont = YFont::getFont(XFA(normalWorkspaceFontName))
        : YButton::getFont();
}

YColor * WorkspaceButton::getColor() {
    return isPressed()
        ? *clrWorkspaceActiveButtonText
        ? activeButtonFg
        ? activeButtonFg
        : activeButtonFg = new YColor(clrWorkspaceActiveButtonText)
        : YButton::getColor()
        : *clrWorkspaceNormalButtonText
        ? normalButtonFg
        ? normalButtonFg
        : normalButtonFg = new YColor(clrWorkspaceNormalButtonText)
        : YButton::getColor();
}

YSurface WorkspaceButton::getSurface() {
    if (activeButtonBg == 0)
        activeButtonBg =
            new YColor(*clrWorkspaceActiveButton
                       ? clrWorkspaceActiveButton : clrActiveButton);
    if (normalButtonBg == 0)
        normalButtonBg =
            new YColor(*clrWorkspaceNormalButton
                       ? clrWorkspaceNormalButton : clrNormalButton);

#ifdef CONFIG_GRADIENTS    
    return (isPressed() ? YSurface(activeButtonBg, 
                                   workspacebuttonactivePixmap,
                                   workspacebuttonactivePixbuf)
            : YSurface(normalButtonBg,
                       workspacebuttonPixmap,
                       workspacebuttonPixbuf));
#else
    return (isPressed() ? YSurface(activeButtonBg, workspacebuttonactivePixmap)
            : YSurface(normalButtonBg, workspacebuttonPixmap));
#endif
}

#endif
