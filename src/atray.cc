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

#include "atray.h"
#include "applet.h"
#include "yprefs.h"
#include "yxapp.h"
#include "prefs.h"
#include "wmframe.h"
#include "wmwinlist.h"
#include "wpixmaps.h"
#include "yrect.h"

static YColorName normalTrayAppFg(&clrNormalTaskBarAppText);
static YColorName normalTrayAppBg(&clrNormalTaskBarApp);
static YColorName activeTrayAppFg(&clrActiveTaskBarAppText);
static YColorName activeTrayAppBg(&clrActiveTaskBarApp);
static YColorName minimizedTrayAppFg(&clrMinimizedTaskBarAppText);
static YColorName minimizedTrayAppBg(&clrNormalTaskBarApp);
static YColorName invisibleTrayAppFg(&clrInvisibleTaskBarAppText);
static YColorName invisibleTrayAppBg(&clrNormalTaskBarApp);

ref<YImage> TrayApp::taskMinimizedGradient;
ref<YImage> TrayApp::taskActiveGradient;
ref<YImage> TrayApp::taskNormalGradient;

TrayApp::TrayApp(ClientData *frame, TrayPane *trayPane, YWindow *aParent):
    YWindow(aParent)
{
    fFrame = frame;
    fTrayPane = trayPane;
    selected = 0;
    fShown = (trayShowAllWindows || frame->visibleNow());
    fRepainted = false;
    setParentRelative();
    setToolTip(frame->getTitle());
    setTitle(frame->getTitle());
    //setDND(true);
}

TrayApp::~TrayApp() {
}

void TrayApp::activate() const {
    getFrame()->activateWindow(true, false);
}

bool TrayApp::isFocusTraversable() {
    return true;
}

int TrayApp::getOrder() const {
    return fFrame->getTrayOrder();
}

void TrayApp::setShown(bool ashow) {
    if (fShown != ashow) {
        fShown = ashow;
        setVisible(ashow);
        fTrayPane->relayout();
    }
}

void TrayApp::configure(const YRect2& r) {
    if (r.resized()) {
        fRepainted = false;
        repaint();
    }
}

void TrayApp::repaint() {
    if (1 < geometry().pixels()) {
        GraphicsBuffer(this).paint();
        fRepainted = true;
    }
}

void TrayApp::handleExpose(const XExposeEvent& expose) {
    if (fRepainted == false && expose.count == 0) {
        repaint();
    }
}

void TrayApp::paint(Graphics &g, const YRect& r) {
    YColor bg;
    ref<YPixmap> bgPix;
    ref<YImage> bgGrad;

    int sx(parent() ? x() + parent()->x() : x());
    int sy(parent() ? y() + parent()->y() : y());

    unsigned sw((parent() && parent()->parent() ?
                 parent()->parent() : this)->width());
    unsigned sh((parent() && parent()->parent() ?
                 parent()->parent() : this)->height());

    if (!getFrame()->visibleNow()) {
        bg = invisibleTrayAppBg;
        bgPix = taskbackPixmap;
        bgGrad = getGradient();
    } else if (getFrame()->isMinimized()) {
        bg = minimizedTrayAppBg;
        bgPix = taskbuttonminimizedPixmap;
        if (taskMinimizedGradient == null && taskbuttonminimizedPixbuf != null)
            taskMinimizedGradient = taskbuttonminimizedPixbuf->scale(sw, sh);
        bgGrad = taskMinimizedGradient;
    } else if (getFrame()->focused()) {
        bg = activeTrayAppBg;
        bgPix = taskbuttonactivePixmap;
        if (taskActiveGradient == null && taskbuttonactivePixbuf != null)
            taskActiveGradient = taskbuttonactivePixbuf->scale(sw, sh);
        bgGrad = taskActiveGradient;
    } else {
        bg = normalTrayAppBg;
        bgPix = taskbuttonPixmap;
        if (taskNormalGradient == null && taskbuttonPixbuf != null)
            taskNormalGradient = taskbuttonPixbuf->scale(sw, sh);
        bgGrad = taskNormalGradient;
    }
    YSurface surface(bg, bgPix, bgGrad);
    g.drawSurface(surface, r.x(), r.y(), r.width(), r.height());

    if (selected == 3) {
        g.setColor(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.setColor(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        if (width() > 0 && height() > 0) {
            if (bgGrad != null)
                g.drawImage(bgGrad, sx, sy, width(), height(), 0, 0);
            else
            if (bgPix != null)
                g.fillPixmap(bgPix, 0, 0, width(), height(), 0, 0);
            else {
                g.setColor(bg);
                g.fillRect(0, 0, width(), height());
            }
        }
    }
    ref<YIcon> icon(getFrame()->getIcon());

    if (icon != null) {
        if (g.color() == 0)
            g.setColor(bg);
        int wd = int(width());
        int ht = int(height());
        int ti = min(trayIconMaxWidth, trayIconMaxHeight);
        int sz = max(int(YIcon::smallSize()), min(ti, min(wd, ht)));
        icon->draw(g, (wd - sz) / 2, (ht - sz) / 2, sz);
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
                        activate();
                    }
                } else if (button.button == 2) {
                    if (getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || (button.state & ControlMask)))
                        getFrame()->wmLower();
                    else {
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyWorkspace(manager->activeWorkspace());
                        activate();
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
    else if (up.button == Button4 && taskBarUseMouseWheel) {
        TrayApp* act = fTrayPane->getActive();
        TrayApp* app = Elvis(fTrayPane->predecessor(act), this);
        if (app != act)
            app->activate();
    }
    else if (up.button == Button5 && taskBarUseMouseWheel) {
        TrayApp* act = fTrayPane->getActive();
        TrayApp* app = Elvis(fTrayPane->successor(act), this);
        if (app != act)
            app->activate();
    }
}

void TrayApp::handleDNDEnter() {
    fRaiseTimer->setTimer(autoRaiseDelay, this, true);

    selected = 3;
    repaint();
}

void TrayApp::handleDNDLeave() {
    if (fRaiseTimer)
        fRaiseTimer = null;

    selected = 0;
    repaint();
}

bool TrayApp::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        fRaiseTimer = null;
        getFrame()->wmRaise();
    }
    return false;
}

TrayPane::TrayPane(IAppletContainer *taskBar, YWindow *parent):
    YWindow(parent),
    fTaskBar(taskBar),
    fNeedRelayout(true),
    fConfigured(false),
    fExposed(false)
{
    setParentRelative();
}

TrayPane::~TrayPane() {
}

TrayApp* TrayPane::predecessor(TrayApp *tapp) {
    const int count = fApps.getCount();
    const int found = find(fApps, tapp);
    if (found >= 0) {
        for (int i = count - 1; 1 <= i; --i) {
            int k = (found + i) % count;
            if (fApps[k]->getShown()) {
                return fApps[k];
            }
        }
    }
    return nullptr;
}

TrayApp* TrayPane::successor(TrayApp *tapp) {
    const int count = fApps.getCount();
    const int found = find(fApps, tapp);
    if (found >= 0) {
        for (int i = 1; i < count; ++i) {
            int k = (found + i) % count;
            if (fApps[k]->getShown()) {
                return fApps[k];
            }
        }
    }
    return nullptr;
}

TrayApp* TrayPane::findApp(YFrameWindow *frame) {
    IterType iter = fApps.iterator();
    while (++iter && iter->getFrame() != frame);
    return iter ? *iter : 0;
}

TrayApp* TrayPane::getActive() {
    return findApp(manager->getFocus());
}

TrayApp *TrayPane::addApp(YFrameWindow *frame) {
    TrayApp *tapp = new TrayApp(frame, this, this);

    if (tapp != nullptr) {
        IterType it = fApps.reverseIterator();
        while (++it && it->getOrder() > tapp->getOrder());
        (--it).insert(tapp);

        if (tapp->getShown()) {
            tapp->show();
            relayout();
        }
    }
    return tapp;
}

void TrayPane::remove(TrayApp* tapp) {
    tapp->setShown(false);
    findRemove(fApps, tapp);
}

int TrayPane::getRequiredWidth() {
    int tc = 0;

    for (IterType a = fApps.iterator(); ++a; )
        if (a->getShown()) tc++;

    return (tc ? 4 + tc * (height() - 4) : 1);
}

void TrayPane::relayoutNow() {
    if (!fNeedRelayout)
        return ;

    fNeedRelayout = false;

    unsigned nw = getRequiredWidth();
    if (nw != width()) {
        MSG(("tray: nw=%d x=%d w=%d", nw, x(), width()));
        setGeometry(YRect(x() + width() - nw, y(), nw, height()));
        fTaskBar->relayout();
    }

    int x, y;
    unsigned w, h;
    int tc = 0;

    for (IterType a = fApps.iterator(); ++a; )
        if (a->getShown()) tc++;

    w = h = height() - 4;
    x = int(width()) - 2 - tc * w;
    y = 2;

    for (IterType f = fApps.iterator(); ++f; ) {
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
        fTaskBar->contextMenu(up.x_root, up.y_root);
    }
    else if ((up.button == Button4 || up.button == Button5)
            && taskBarUseMouseWheel)
    {
        TrayApp* app = getActive();
        if (app)
            app->handleClick(up, count);
    }
}

void TrayPane::handleExpose(const XExposeEvent& e) {
    if (fExposed == false) {
        fExposed = true;
        if (fConfigured) {
            clearWindow();
        }
    }
    if (e.count == 0) {
        repaint();
    }
}

bool TrayPane::hasBorder() {
    return trayDrawBevel && wmLook != lookMetal && 1 < width() && 1 < height();
}

void TrayPane::configure(const YRect2& r) {
    if (fConfigured == false || r.resized()) {
        fConfigured = true;
        if (fExposed && visible()) {
            if (hasBorder() && 1 < r.width() && 1 < r.height()) {
                clearArea(1, 1, r.width() - 2, r.height() - 2);
            } else {
                clearWindow();
            }
            repaint();
            XEvent xev;
            while (XCheckWindowEvent(xapp->display(), handle(),
                                     ExposureMask, &xev)
                && ((xev.type == Expose && 0 < xev.xexpose.count) ||
                (xev.type == GraphicsExpose && 0 < xev.xgraphicsexpose.count)));
        }
    }
}

void TrayPane::repaint() {
    if (fConfigured && fExposed) {
        Graphics g(*this);
        paint(g, YRect(0, 0, width(), height()));
    }
}

void TrayPane::paint(Graphics &g, const YRect& r) {
    int const w(width());
    int const h(height());

    g.setColor(taskBarBg);

    if (getGradient() == null && taskbackPixmap == null) {
        if (hasBorder() && 1 < r.width() && 1 < r.height()) {
            g.fillRect(1, 1, r.width() - 2, r.height() - 2);
        } else {
            g.fillRect(0, 0, w, h);
        }
    }

    if (trayDrawBevel && 2 < w && 2 < h) {
        if (wmLook == lookMetal)
            g.draw3DRect(1, 1, w - 2, h - 2, false);
        else
            g.draw3DRect(0, 0, w - 1, h - 1, false);
    }
}

// vim: set sw=4 ts=4 et:
