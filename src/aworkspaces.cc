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
#include "wmwinlist.h"

#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

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
ref<YImage> workspacebuttonPixbuf;
ref<YImage> workspacebuttonactivePixbuf;
#endif

WorkspaceButton::WorkspaceButton(long ws, YWindow *parent): ObjectButton(parent, (YAction *)0)
{
    fWorkspace = ws;
    //setDND(true);
}

void WorkspaceButton::handleClick(const XButtonEvent &up, int /*count*/) {
    switch (up.button) {
#ifdef CONFIG_WINLIST
        case 2:
            if (windowList)
                windowList->showFocused(-1, -1);
            break;
#endif
#ifdef CONFIG_WINMENU
        case 3:
            manager->popupWindowListMenu(this, up.x_root, up.y_root);
            break;
#endif
        case 4:
            manager->switchToPrevWorkspace(false);
            break;
        case 5:
            manager->switchToNextWorkspace(false);
            break;
    }
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
        ref<YResourcePaths> paths = YResourcePaths::subdirs(null, false);

        int ht = smallIconSize + 8;
        int leftX = 0;

        for (w = 0; w < workspaceCount; w++) {
            WorkspaceButton *wk = new WorkspaceButton(w, this);
            if (wk) {
                if (pagerShowPreview) {
                    wk->setSize((int) round((double)
                                (ht * desktop->width() / desktop->height())), ht);
                } else {
                    ref<YImage> image
                        (paths->loadImage("workspace/", workspaceNames[w]));
                    if (image != null)
                        wk->setImage(image);
                    else
                        wk->setText(workspaceNames[w]);
                }
#if 0
                ref<YImage> image
                    (paths->loadImage("workspace/", workspaceNames[w]));

                if (image != null)
                    wk->setImage(image);
                else
                    wk->setText(workspaceNames[w]);
#endif

/// TODO "why my_basename here?"
                char * wn(newstr(my_basename(workspaceNames[w])));
                char * ext(strrchr(wn, '.'));
                if (ext) *ext = '\0';

                wk->setToolTip(ustring(_("Workspace: ")).append(wn));

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

void WorkspacesPane::configure(const YRect &r) {
    YWindow::configure(r);

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

void WorkspacesPane::repaint() {
    if (!pagerShowPreview) return;

    for (int w = 0; w < workspaceCount; w++) {
        fWorkspaceButton[w]->repaint();
    }
}

void WorkspaceButton::paint(Graphics &g, const YRect &/*r*/) {
    if (!pagerShowPreview) {
        YButton::paint(g, YRect(0,0,0,0));
        return;
    }

    int x(0), y(0), w(width()), h(height());

    if (w > 1 && h > 1) {
        YSurface surface(getSurface());
        g.setColor(surface.color);
        g.drawSurface(surface, x, y, w, h);

        if (pagerShowBorders) {
            x += 1; y += 1; w -= 2; h -= 2;
        }

        int wx, wy, ww, wh;
        double sf = (double) desktop->width() / w;

        ref<YIcon> icon;
        YColor *colors[] = {
            surface.color,
            surface.color->brighter(),
            surface.color->darker(),
            getColor(),
            NULL, // getColor()->brighter(),
            getColor()->darker()
        };

        for (YFrameWindow *yfw = manager->bottomLayer(WinLayerBelow);
                yfw && yfw->getActiveLayer() <= WinLayerDock;
                yfw = yfw->prevLayer()) {
            if (yfw->isHidden() ||
                    !yfw->visibleOn(fWorkspace) ||
                    (yfw->frameOptions() & YFrameWindow::foIgnoreWinList))
                continue;
            wx = (int) round(yfw->x() / sf) + x;
            wy = (int) round(yfw->y() / sf) + y;
            ww = (int) round(yfw->width() / sf);
            wh = (int) round(yfw->height()  / sf);
            if (ww < 1 || wh < 1)
                continue;
            if (yfw->isMaximizedVert()) { // !!! hack 
                wy = y; wh = h;
            }
            if (yfw->isMinimized()) {
                if (!pagerShowMinimized)
                    continue;
                g.setColor(colors[2]);
            } else {
                if (ww > 2 && wh > 2) {
                    if (yfw->focused())
                        g.setColor(colors[1]);
                    else
                        g.setColor(colors[2]);
                    g.fillRect(wx+1, wy+1, ww-2, wh-2);

#ifndef LITE
                    if (pagerShowWindowIcons && ww > smallIconSize+1 &&
                            wh > smallIconSize+1 && (icon = yfw->clientIcon()) != null &&
                            icon->small() != null) {
                        g.drawImage(icon->small(),
                                    wx + (ww-smallIconSize)/2,
                                    wy + (wh-smallIconSize)/2);
                    }
#endif
                }
                g.setColor(colors[5]);
            }
            if (ww == 1 && wh == 1)
                g.drawPoint(wx, wy);
            else
                g.drawRect(wx, wy, ww-1, wh-1);
        }

        if (pagerShowBorders) {
            g.setColor(surface.color);
            g.draw3DRect(x-1, y-1, w+1, h+1, !isPressed());
        }

        if (pagerShowNumbers) {
            ref<YFont> font = getFont();

            char label[3];
            sprintf(label, "%ld", (fWorkspace+1) % 100);

            wx = (w - font->textWidth(label)) / 2 + x;
            wy = (h - font->height()) / 2 + font->ascent() + y;

            g.setFont(font);
            g.setColor(colors[0]);
            g.drawChars(label, 0, strlen(label), wx+1, wy+1);
            g.setColor(colors[3]);
            g.drawChars(label, 0, strlen(label), wx, wy);
        }
    }
}

#endif
