#include "config.h"

#include "ylib.h"
#include "aworkspaces.h"
#include "applet.h"
#include "prefs.h"
#include "wmmgr.h"
#include "wmapp.h"
#include "wmframe.h"
#include "yrect.h"
#include "ypaths.h"
#include "wmwinlist.h"
#include "wpixmaps.h"
#include "intl.h"
#include <math.h>

YColorName WorkspaceButton::normalButtonBg(&clrWorkspaceNormalButton);
YColorName WorkspaceButton::normalBackupBg(&clrNormalButton);
YColorName WorkspaceButton::normalButtonFg(&clrWorkspaceNormalButtonText);

YColorName WorkspaceButton::activeButtonBg(&clrWorkspaceActiveButton);
YColorName WorkspaceButton::activeBackupBg(&clrActiveButton);
YColorName WorkspaceButton::activeButtonFg(&clrWorkspaceActiveButtonText);

ref<YFont> WorkspaceButton::normalButtonFont;
ref<YFont> WorkspaceButton::activeButtonFont;

static ref<YResourcePaths> getResourcePaths() {
    return YResourcePaths::subdirs("workspace", false);
}

WorkspaceButton::WorkspaceButton(long ws, YWindow *parent):
    ObjectButton(parent, YAction())
{
    fWorkspace = ws;
    //setDND(true);
    setTitle(manager->workspaceName(ws));
}

void WorkspaceButton::handleClick(const XButtonEvent &up, int /*count*/) {
    switch (up.button) {
        case 2:
            if (windowList)
                windowList->showFocused(-1, -1);
            break;
        case 3:
            manager->popupWindowListMenu(this, up.x_root, up.y_root);
            break;
        case 4:
            if(taskBarUseMouseWheel) manager->switchToPrevWorkspace(false);
            break;
        case 5:
            if(taskBarUseMouseWheel) manager->switchToNextWorkspace(false);
            break;
    }
}

void WorkspaceButton::handleDNDEnter() {
    fRaiseTimer->setTimer(autoRaiseDelay, this, true);
    repaint();
}

void WorkspaceButton::handleDNDLeave() {
    if (fRaiseTimer)
        fRaiseTimer->disableTimerListener(this);
    repaint();
}

bool WorkspaceButton::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        manager->activateWorkspace(fWorkspace);
    }
    return false;
}

void WorkspaceButton::actionPerformed(YAction /*action*/, unsigned int modifiers) {
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

    fWorkspaceButtonCount = workspaceCount;

    if (fWorkspaceButtonCount > 0)
        fWorkspaceButton = new WorkspaceButton *[fWorkspaceButtonCount];
    else
        fWorkspaceButton = 0;

    if (fWorkspaceButton) {
        ref<YResourcePaths> paths = getResourcePaths();

        int ht = smallIconSize + 8;
        int leftX = 0;

        for (w = 0; w < fWorkspaceButtonCount; w++) {
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

                wk->updateName();
            }
            fWorkspaceButton[w] = wk;
        }

        for (w = 0; w < fWorkspaceButtonCount; w++) {
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
        for (long w = 0; w < fWorkspaceButtonCount; w++)
            delete fWorkspaceButton[w];
        delete [] fWorkspaceButton;
    }
    for (int i = workspaceCount; --i >= 0; --workspaceCount) {
        delete[] workspaceNames[i]; workspaceNames[i] = 0;
    }
}

void WorkspacesPane::repositionButtons() {
    MSG(("WorkspacesPane::repositionButtons()"));
    int ht = height();
    int leftX = 0;
    for (long w = 0; w < fWorkspaceButtonCount; w++) {
        YButton *wk = fWorkspaceButton[w];
        //leftX += 2;
        if (wk) {
            wk->setGeometry(YRect(leftX, 0, wk->width(), ht));
            wk->show(); // no effect if already shown
            leftX += wk->width();
        }
    }
    setSize(leftX, ht);
}

void WorkspacesPane::relabelButtons() {
    ref<YResourcePaths> paths = getResourcePaths();

    for (long w = 0; w < fWorkspaceButtonCount; w++) {
        WorkspaceButton *wk = fWorkspaceButton[w];
        if (wk) {
            if (false == pagerShowPreview) {
                ref<YImage> image
                    (paths->loadImage("workspace/",workspaceNames[w]));
                if (image != null)
                    wk->setImage(image);
                else
                    wk->setText(workspaceNames[w]);
            }
            wk->updateName();
        }
    }

    repositionButtons();
}

void WorkspacesPane::configure(const YRect &r) {
    YWindow::configure(r);

    int ht = height();
    int leftX = 0;
    for (int w = 0; w < fWorkspaceButtonCount; w++) {
        YButton *wk = fWorkspaceButton[w];
        //leftX += 2;
        if (wk) {
            wk->setGeometry(YRect(leftX, 0, wk->width(), ht));
            leftX += wk->width();
        }
    }
}

void WorkspacesPane::updateButtons() {
    MSG(("WorkspacesPane::udpateButtons(): updating %ld -> %ld",
         fWorkspaceButtonCount,workspaceCount));
    long fOldWorkspaceButtonCount = fWorkspaceButtonCount;
    fWorkspaceButtonCount = workspaceCount;
    if (fWorkspaceButtonCount != fOldWorkspaceButtonCount) {
        WorkspaceButton **fOldWorkspaceButton = fWorkspaceButton;
        fWorkspaceButton = new WorkspaceButton *[fWorkspaceButtonCount];
        if (fWorkspaceButtonCount > fOldWorkspaceButtonCount) {
            for (long w = 0; w < fOldWorkspaceButtonCount; w++)
                fWorkspaceButton[w] = fOldWorkspaceButton[w];
            int ht = smallIconSize + 8;
            MSG(("WorkspacesPane::updateButtons(): adding new buttons"));
            ref<YResourcePaths> paths = getResourcePaths();
            for (long w = fOldWorkspaceButtonCount; w < fWorkspaceButtonCount; w++) {
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

                    wk->updateName();
                }
                fWorkspaceButton[w] = wk;
            }
        } else
        if (fWorkspaceButtonCount < fOldWorkspaceButtonCount) {
            for (long w = 0; w < fWorkspaceButtonCount; w++)
                fWorkspaceButton[w] = fOldWorkspaceButton[w];
            MSG(("WorkspacesPane::updateButtons(): removing buttons"));
            for (long w = fWorkspaceButtonCount; w < fOldWorkspaceButtonCount; w++) {
                MSG(("WorkspacePane::updateButtons(): removing button for workspace %ld",w));
                delete fOldWorkspaceButton[w];
                MSG(("WorkspacePane::updateButtons(): removed  button for workspace %ld",w));
            }
        }
        if (fOldWorkspaceButton != 0)
            delete[] fOldWorkspaceButton;
        repositionButtons();
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

YColor WorkspaceButton::getColor() {
    return isPressed()
        ? activeButtonFg ? activeButtonFg : YButton::getColor()
        : normalButtonFg ? normalButtonFg : YButton::getColor();
}

YSurface WorkspaceButton::getSurface() {
    return (isPressed()
            ? YSurface(activeButtonBg ? activeButtonBg : activeBackupBg,
                                   workspacebuttonactivePixmap,
                                   workspacebuttonactivePixbuf)
            : YSurface(normalButtonBg ? normalButtonBg : normalBackupBg,
                       workspacebuttonPixmap,
                       workspacebuttonPixbuf));
}

mstring WorkspaceButton::baseName() {
    mstring name(my_basename(workspaceNames[fWorkspace]));
    name = name.trim();
    int dot = name.lastIndexOf('.');
    if (inrange(dot, 1, (int) name.length() - 2))
        name = name.substring(0, dot);
    return name;
}

void WorkspaceButton::updateName() {
    setToolTip(_("Workspace: ") + baseName());
}

void WorkspacesPane::repaint() {
    if (!pagerShowPreview) return;

    for (int w = 0; w < fWorkspaceButtonCount; w++) {
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

        unsigned wx, wy, ww, wh;
        double sf = (double) desktop->width() / w;

        ref<YIcon> icon;
        YColor colors[] = {
            surface.color,
            surface.color.brighter(),
            surface.color.darker(),
            getColor(),
            YColor(), // getColor().brighter(),
            getColor().darker()
        };

        for (YFrameWindow *yfw = manager->bottomLayer(WinLayerBelow);
                yfw && yfw->getActiveLayer() <= WinLayerDock;
                yfw = yfw->prevLayer()) {
            if (yfw->isHidden() ||
                    !yfw->visibleOn(fWorkspace) ||
                    hasbit(yfw->frameOptions(),
                        YFrameWindow::foIgnoreWinList |
                        YFrameWindow::foIgnorePagerPreview))
                continue;
            wx = (unsigned) round(double(yfw->x()) / sf) + x;
            wy = (unsigned) round(double(yfw->y()) / sf) + y;
            ww = (unsigned) round(double(yfw->width()) / sf);
            wh = (unsigned) round(double(yfw->height()) / sf);
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

                    if (pagerShowWindowIcons && ww > smallIconSize+1 &&
                            wh > smallIconSize+1 && (icon = yfw->clientIcon()) != null &&
                            icon->small() != null) {
                        g.drawImage(icon->small(),
                                    wx + (ww-smallIconSize)/2,
                                    wy + (wh-smallIconSize)/2);
                    }
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

        char label[12] = {};
        if (pagerShowNumbers) {
            snprintf(label, sizeof label, "%d", int(fWorkspace+1) % 100);
        } else {
            strlcpy(label, cstring(baseName()), min(5, int(sizeof label)));
        }
        if (label[0] != 0) {
            ref<YFont> font = getFont();

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

// vim: set sw=4 ts=4 et:
