/*
 *  IceWM - Window tray
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/05/15: Jan Krupa <j.krupa@netweb.cz>
 *  - initial version
 *  2001/05/27: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - ported to IceWM 1.0.9 (gradients, ...)
 *
 *  TODO: share code with atask.cc
 */

#include "config.h"

#ifdef CONFIG_TRAY

#include "ylib.h"
#include "ypixbuf.h"
#include "atray.h"
#include "wmtaskbar.h"
#include "yprefs.h"
#include "prefs.h"
#include "yxapp.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "wmwinlist.h"
#include "yrect.h"
#include "yicon.h"

#include <string.h>

static YColor *taskBarBg = 0;
static YColor *normalTrayAppFg = 0;
static YColor *normalTrayAppBg = 0;
static YColor *activeTrayAppFg = 0;
static YColor *activeTrayAppBg = 0;
static YColor *minimizedTrayAppFg = 0;
static YColor *minimizedTrayAppBg = 0;
static YColor *invisibleTrayAppFg = 0;
static YColor *invisibleTrayAppBg = 0;
static ref<YFont> normalTrayFont;
static ref<YFont> activeTrayFont;

#ifdef CONFIG_GRADIENTS
ref<YPixbuf> TrayApp::taskMinimizedGradient;
ref<YPixbuf> TrayApp::taskActiveGradient;
ref<YPixbuf> TrayApp::taskNormalGradient;
#endif

TrayApp::TrayApp(ClientData *frame, YWindow *aParent): YWindow(aParent) {
    if (normalTrayAppFg == 0) {
        normalTrayAppBg = new YColor(clrNormalTaskBarApp);
        normalTrayAppFg = new YColor(clrNormalTaskBarAppText);
        activeTrayAppBg = new YColor(clrActiveTaskBarApp);
        activeTrayAppFg = new YColor(clrActiveTaskBarAppText);
        minimizedTrayAppBg = new YColor(clrNormalTaskBarApp);
        minimizedTrayAppFg = new YColor(clrMinimizedTaskBarAppText);
        invisibleTrayAppBg = new YColor(clrNormalTaskBarApp);
        invisibleTrayAppFg = new YColor(clrInvisibleTaskBarAppText);
        normalTrayFont = YFont::getFont(XFA(normalTaskBarFontName));
        activeTrayFont = YFont::getFont(XFA(activeTaskBarFontName));
    }
    fFrame = frame;
    fPrev = fNext = 0;
    selected = 0;
    fShown = true;
    setToolTip(frame->getTitle());
    //setDND(true);
}

TrayApp::~TrayApp() {
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
}

bool TrayApp::isFocusTraversable() {
    return true;
}

void TrayApp::setShown(bool ashow) {
    if (ashow != fShown) {
        fShown = ashow;
    }
}

void TrayApp::paint(Graphics &g, const YRect &/*r*/) {
    YColor *bg, *fg;
    ref<YPixmap> bgPix;
#ifdef CONFIG_GRADIENTS
    ref<YPixbuf> bgGrad;
#endif

    int p(0);
    
#ifdef CONFIG_GRADIENTS
    int sx(parent() ? x() + parent()->x() : x());
    int sy(parent() ? y() + parent()->y() : y());

    unsigned sw((parent() && parent()->parent() ? 
                 parent()->parent() : this)->width());
    unsigned sh((parent() && parent()->parent() ? 
                 parent()->parent() : this)->height());
#endif

    if (!getFrame()->visibleNow()) {
        bg = invisibleTrayAppBg;
        fg = invisibleTrayAppFg;
        bgPix = taskbackPixmap;
#ifdef CONFIG_GRADIENTS
        bgGrad = getGradient();
#endif
    } else if (getFrame()->isMinimized()) {
        bg = minimizedTrayAppBg;
        fg = minimizedTrayAppFg;
        bgPix = taskbuttonminimizedPixmap;
#ifdef CONFIG_GRADIENTS
        if (taskMinimizedGradient == null && taskbuttonminimizedPixbuf != null)
            taskMinimizedGradient = YPixbuf::scale(taskbuttonminimizedPixbuf, sw, sh);
        bgGrad = taskMinimizedGradient;
#endif
    } else if (getFrame()->focused()) {
        bg = activeTrayAppBg;
        fg = activeTrayAppFg;
        bgPix = taskbuttonactivePixmap;
#ifdef CONFIG_GRADIENTS
        if (taskActiveGradient == null && taskbuttonactivePixbuf != null)
            taskActiveGradient = YPixbuf::scale(taskbuttonactivePixbuf, sw, sh);
        bgGrad = taskActiveGradient;
#endif
    } else {
        bg = normalTrayAppBg;
        fg = normalTrayAppFg;
        bgPix = taskbuttonPixmap;
#ifdef CONFIG_GRADIENTS
        if (taskNormalGradient == null && taskbuttonPixbuf != null)
            taskNormalGradient = YPixbuf::scale(taskbuttonPixbuf, sw, sh);
        bgGrad = taskNormalGradient;
#endif
    }

    if (selected == 3) {
        p = 2;
        g.setColor(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.setColor(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        if (width() > 0 && height() > 0) {
#ifdef CONFIG_GRADIENTS
            if (bgGrad != null)
                g.copyPixbuf(*bgGrad, sx, sy, width(), height(), 0, 0);
            else
#endif
            if (bgPix != null)
                g.fillPixmap(bgPix, 0, 0, width(), height(), 0, 0);
            else {
                g.setColor(bg);
                g.fillRect(0, 0, width(), height());
            }
        }
    }

    YIcon *icon(getFrame()->getIcon());

    if (icon) {
        ref<YIconImage> small = icon->small();
        if (small != null) g.drawImage(small, 2, 2);
    }
}

void TrayApp::handleButton(const XButtonEvent &button) {
    YWindow::handleButton(button);
    if (button.button == 1 || button.button == 2) {
        if (button.type == ButtonPress) {
            selected = 2;
            repaint();
        } else if (button.type == ButtonRelease) {
            if (selected == 2) {
                if (button.button == 1) {
                    if (getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || (button.state & ControlMask)))
                        getFrame()->wmMinimize();
                    else {
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyOnlyWorkspace(manager->activeWorkspace());
                        getFrame()->activateWindow(true);
                    }
                } else if (button.button == 2) {
                    if (getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || (button.state & ControlMask)))
                        getFrame()->wmLower();
                    else {
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyWorkspace(manager->activeWorkspace());
                        getFrame()->activateWindow(true);
                    }
                }
            }
            selected = 0;
            repaint();
        }
    }
}

void TrayApp::handleCrossing(const XCrossingEvent &crossing) {
    if (selected > 0) {
        if (crossing.type == EnterNotify) {
            selected = 2;
            repaint();
        } else if (crossing.type == LeaveNotify) {
            selected = 1;
            repaint();
        }
    }
    YWindow::handleCrossing(crossing);
}

void TrayApp::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal |
                                    YPopupWindow::pfPopupMenu);
    }
}

void TrayApp::handleDNDEnter() {
    if (fRaiseTimer == 0)
        fRaiseTimer = new YTimer(autoRaiseDelay);
    if (fRaiseTimer) {
        fRaiseTimer->setTimerListener(this);
        fRaiseTimer->startTimer();
    }
    selected = 3;
    repaint();
}

void TrayApp::handleDNDLeave() {
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    selected = 0;
    repaint();
}

bool TrayApp::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        getFrame()->wmRaise();
    }
    return false;
}

TrayPane::TrayPane(YWindow *parent): YWindow(parent) {
    if (taskBarBg == 0) 
        taskBarBg = new YColor(clrDefaultTaskBar);
    fFirst = fLast = 0;
    fCount = 0;
    fNeedRelayout = true;
}

TrayPane::~TrayPane() {
}

void TrayPane::insert(TrayApp *tapp) {
    fCount++;
    tapp->setNext(0);
    tapp->setPrev(fLast);
    if (fLast)
        fLast->setNext(tapp);
    else
        fFirst = tapp;
    fLast = tapp;
}

void TrayPane::remove(TrayApp *tapp) {
    fCount--;

    if (tapp->getPrev())
        tapp->getPrev()->setNext(tapp->getNext());
    else
        fFirst = tapp->getNext();

    if (tapp->getNext())
        tapp->getNext()->setPrev(tapp->getPrev());
    else
        fLast = tapp->getPrev();
}

TrayApp *TrayPane::addApp(YFrameWindow *frame) {
#ifdef CONFIG_WINLIST
    if (frame->client() == windowList)
        return 0;
#endif
    if (frame->client() == taskBar)
        return 0;

    TrayApp *tapp = new TrayApp(frame, this);

    if (tapp != 0) {
        insert(tapp);
        tapp->show();

        if (!(frame->visibleOn(manager->activeWorkspace()) ||
              trayShowAllWindows))
            tapp->setShown(0);

        relayout();
    }
    return tapp;
}

void TrayPane::removeApp(YFrameWindow *frame) {
    for (TrayApp *icon(fFirst); NULL != icon; icon = icon->getNext()) {
        if (icon->getFrame() == frame) {
            icon->hide();
            remove(icon);
            delete icon;

            relayout();
            return;
        }
    }
}

int TrayPane::getRequiredWidth() {
    int tc = 0;

    for (TrayApp *a(fFirst); a != NULL; a = a->getNext())
        if (a->getShown()) tc++;

    return (tc ? 4 + tc * (height() - 4) : 1);
}

void TrayPane::relayoutNow() {
    if (!fNeedRelayout)
        return ;

    fNeedRelayout = false;

    int nw = getRequiredWidth();
    if (nw != width()) {
        MSG(("tray: nw=%d x=%d w=%d", nw, x(), width()));
        setGeometry(YRect(x() + width() - nw, y(), nw, height()));
        taskBar->relayout();
    }

    int x, y, w, h;
    int tc = 0;

    for (TrayApp *a(fFirst); a != NULL; a = a->getNext())
        if (a->getShown()) tc++;

    w = h = height() - 4;
    x = width() - 2 - tc * w;
    y = 2;

    for (TrayApp *f(fFirst); f != NULL; f = f->getNext()) {
        if (f->getShown()) {
            f->setGeometry(YRect(x, y, w, h));
            f->show();
            x += w;
        } else
            f->hide();
    }
}

void TrayPane::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        taskBar->contextMenu(up.x_root, up.y_root);
    }
}

void TrayPane::paint(Graphics &g, const YRect &/*r*/) {
    int const w(width());
    int const h(height());

    g.setColor(taskBarBg);
    
#ifdef CONFIG_GRADIENTS
    ref<YPixbuf> gradient(parent() ? parent()->getGradient() : null);

    if (gradient != null)
        g.copyPixbuf(*gradient, x(), y(), w, h, 0, 0);
    else 
#endif    
    if (taskbackPixmap != null)
        g.fillPixmap(taskbackPixmap, 0, 0, w, h, x(), y());
    else
        g.fillRect(0, 0, w, h);
    
    if (trayDrawBevel && w > 1)
        if (wmLook == lookMetal)
            g.draw3DRect(1, 1, w - 2, h - 2, false);
        else
            g.draw3DRect(0, 0, w - 1, h - 1, false);
}

#endif
