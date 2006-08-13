#include "config.h"

#ifdef CONFIG_TASKBAR

#include "ylib.h"
#include "ypixbuf.h"
#include "atasks.h"
#include "wmtaskbar.h"
#include "yprefs.h"
#include "prefs.h"
#include "yxapp.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "wmwinlist.h"
#include "yrect.h"
#include "yicon.h"
#include "sysdep.h"
#include <string.h>

static YColor *normalTaskBarAppFg = 0;
static YColor *normalTaskBarAppBg = 0;
static YColor *activeTaskBarAppFg = 0;
static YColor *activeTaskBarAppBg = 0;
static YColor *minimizedTaskBarAppFg = 0;
static YColor *minimizedTaskBarAppBg = 0;
static YColor *invisibleTaskBarAppFg = 0;
static YColor *invisibleTaskBarAppBg = 0;
static YColor *taskBarBg = 0;
static ref<YFont> normalTaskBarFont;
static ref<YFont> activeTaskBarFont;

YTimer *TaskBarApp::fRaiseTimer = 0;

TaskBarApp::TaskBarApp(ClientData *frame, YWindow *aParent): YWindow(aParent) {
    if (normalTaskBarAppFg == 0) {
        normalTaskBarAppBg = new YColor(clrNormalTaskBarApp);
        normalTaskBarAppFg = new YColor(clrNormalTaskBarAppText);
        activeTaskBarAppBg = new YColor(clrActiveTaskBarApp);
        activeTaskBarAppFg = new YColor(clrActiveTaskBarAppText);
        minimizedTaskBarAppBg = new YColor(clrMinimizedTaskBarApp);
        minimizedTaskBarAppFg = new YColor(clrMinimizedTaskBarAppText);
        invisibleTaskBarAppBg = new YColor(clrInvisibleTaskBarApp);
        invisibleTaskBarAppFg = new YColor(clrInvisibleTaskBarAppText);
        normalTaskBarFont = YFont::getFont(XFA(normalTaskBarFontName));
        activeTaskBarFont = YFont::getFont(XFA(activeTaskBarFontName));
    }
    fFrame = frame;
    fPrev = fNext = 0;
    selected = 0;
    fShown = true;
    fFlashing = false;
    fFlashOn = false;
    fFlashTimer = 0;
    fFlashStart = 0;
    setToolTip(frame->getTitle());
    //setDND(true);
}

TaskBarApp::~TaskBarApp() {
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    delete fFlashTimer; fFlashTimer = 0;
}

bool TaskBarApp::isFocusTraversable() {
    return true;
}

void TaskBarApp::setShown(bool ashow) {
    if (ashow != fShown) {
        fShown = ashow;
    }
}

void TaskBarApp::setFlash(bool flashing) {
    if (fFlashing != flashing) {
        fFlashing = flashing;

        if (fFlashing) {
            fFlashOn = true;
            fFlashStart = time(NULL);
            if (fFlashTimer == 0)
                fFlashTimer = new YTimer(250);
            if (fFlashTimer) {
                fFlashTimer->setTimerListener(this);
                fFlashTimer->startTimer();
            }
        } else {
            //fFlashTimer->stopTimer();
        }
    }
}

void TaskBarApp::paint(Graphics &g, const YRect &/*r*/) {
    YColor *bg, *fg;
    ref<YPixmap> bgPix;
#ifdef CONFIG_GRADIENTS
    ref<YPixbuf> bgGrad;
#endif

    int p(0);
    int style = 0;

    if (selected == 3)
        style = 3;
    else if (getFrame()->focused() || selected == 2)
        style = 2;
    else
        style = 1;

    if (fFlashing) {
        if (fFlashOn) {
            bg = activeTaskBarAppBg;
            fg = activeTaskBarAppFg;
            bgPix = taskbuttonactivePixmap;
#ifdef CONFIG_GRADIENTS
            bgGrad = taskbuttonactivePixbuf;
#endif
            style = 1;
        } else {
            bg = normalTaskBarAppBg;
            fg = normalTaskBarAppFg;
            bgPix = taskbuttonPixmap;
#ifdef CONFIG_GRADIENTS
            bgGrad = taskbuttonPixbuf;
#endif
            style = 1;
        }
    } else if (!getFrame()->visibleNow()) {
        bg = invisibleTaskBarAppBg;
        fg = invisibleTaskBarAppFg;
        bgPix = taskbackPixmap;
#ifdef CONFIG_GRADIENTS
        bgGrad = taskbackPixbuf;
#endif
    } else if (getFrame()->isMinimized()) {
        bg = minimizedTaskBarAppBg;
        fg = minimizedTaskBarAppFg;
        bgPix = taskbuttonminimizedPixmap;
#ifdef CONFIG_GRADIENTS
        bgGrad = taskbuttonminimizedPixbuf;
#endif
    } else if (getFrame()->focused()) {
        bg = activeTaskBarAppBg;
        fg = activeTaskBarAppFg;
        bgPix = taskbuttonactivePixmap;
#ifdef CONFIG_GRADIENTS
        bgGrad = taskbuttonactivePixbuf;
#endif
    } else {
        bg = normalTaskBarAppBg;
        fg = normalTaskBarAppFg;
        bgPix = taskbuttonPixmap;
#ifdef CONFIG_GRADIENTS
        bgGrad = taskbuttonPixbuf;
#endif
    }

    if (style == 3) {
        p = 2;
        g.setColor(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.setColor(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        g.setColor(bg);
    
        if (style == 2) {
            p = 2;
            if (wmLook == lookMetal) {
                g.drawBorderM(0, 0, width() - 1, height() - 1, false);
            } else if (wmLook == lookGtk) {
                g.drawBorderG(0, 0, width() - 1, height() - 1, false);
            }
            else if (wmLook != lookFlat)
                g.drawBorderW(0, 0, width() - 1, height() - 1, false);
        } else {
            p = 1;
            if (wmLook == lookMetal) {
                p = 2;
                g.drawBorderM(0, 0, width() - 1, height() - 1, true);
            } else if (wmLook == lookGtk) {
                g.drawBorderG(0, 0, width() - 1, height() - 1, true);
            }
            else if (wmLook != lookFlat)
                g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        }

        int const dp(wmLook == lookFlat ? 0: wmLook == lookMetal ? 2 : p);
        int const ds(wmLook == lookFlat ? 0: wmLook == lookMetal ? 4 : 3);

        if (width() > ds && height() > ds) {
#ifdef CONFIG_GRADIENTS
            if (bgGrad != null)
                g.drawGradient(bgGrad, dp, dp, width() - ds, height() - ds);
            else
#endif
            if (bgPix != null)
                g.fillPixmap(bgPix, dp, dp, width() - ds, height() - ds);
            else
                g.fillRect(dp, dp, width() - ds, height() - ds);
        }
    }

#ifndef LITE
    YIcon *icon(getFrame()->getIcon());

    if (taskBarShowWindowIcons && icon) {
        ref<YIconImage> small = icon->small();

        if (small != null) {
            int const y((height() - 3 - small->height() - 
                         ((wmLook == lookMetal) ? 1 : 0)) / 2);
            g.drawImage(small, p + 1, p + 1 + y);
        }
    }
#endif

    const char *str = getFrame()->getIconTitle();
    if (strIsEmpty(str)) str = getFrame()->getTitle();

    if (str) {
        ref<YFont> font =
            getFrame()->focused() ? activeTaskBarFont : normalTaskBarFont;

        if (font != null) {
            g.setColor(fg);
            g.setFont(font);

            int iconSize = 0;
#ifndef LITE
            if (taskBarShowWindowIcons)
                iconSize = YIcon::smallSize();
#endif
            int const tx = 3 + iconSize;
            int const ty = max(2,
                               (height() + font->height() -
                                ((wmLook == lookMetal || wmLook == lookFlat) ? 2 : 1)) / 2 -
                               font->descent());
            int const wm = width() - p - 3 - iconSize - 3;

            g.drawStringEllipsis(p + tx, p + ty, str, wm);
        }
    }
}

void TaskBarApp::handleButton(const XButtonEvent &button) {
    YWindow::handleButton(button);
    if (button.button == 1 || button.button == 2) {
        if (button.type == ButtonPress) {
            selected = 2;
            repaint();
        } else if (button.type == ButtonRelease) {
            if (selected == 2) {
                if (button.button == 1) {
                    if (getFrame()->focused() && getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || (button.state & ControlMask)))
                        getFrame()->wmMinimize();
                    else {
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyOnlyWorkspace(manager->activeWorkspace());
                        getFrame()->activateWindow(true);
                    }
                } else if (button.button == 2) {
                    if (getFrame()->focused() && getFrame()->visibleNow() &&
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

void TaskBarApp::handleCrossing(const XCrossingEvent &crossing) {
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

void TaskBarApp::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal |
                                    YPopupWindow::pfPopupMenu);
    }
}

void TaskBarApp::handleDNDEnter() {
    if (fRaiseTimer == 0)
        fRaiseTimer = new YTimer(autoRaiseDelay);
    if (fRaiseTimer) {
        fRaiseTimer->setTimerListener(this);
        fRaiseTimer->startTimer();
    }
    selected = 3;
    repaint();
}

void TaskBarApp::handleDNDLeave() {
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    selected = 0;
    repaint();
}

bool TaskBarApp::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        getFrame()->wmRaise();
    }
    if (t == fFlashTimer) {
        if (!fFlashing) {
            fFlashOn = 0;
            fFlashStart = 0;
            return false;
        }
        fFlashOn = !fFlashOn;
        if (focusRequestFlashTime != 0)
            if (time(NULL) - fFlashStart > focusRequestFlashTime)
                fFlashing = false;
        repaint();
        return fFlashing;
    }
    return false;
}

TaskPane::TaskPane(YWindow *parent): YWindow(parent) {
    if (taskBarBg == 0)
        taskBarBg = new YColor(clrDefaultTaskBar);
    fFirst = fLast = 0;
    fCount = 0;
    fNeedRelayout = true;
}

TaskPane::~TaskPane() {
}

void TaskPane::insert(TaskBarApp *tapp) {
    fCount++;
    tapp->setNext(0);
    tapp->setPrev(fLast);
    if (fLast)
        fLast->setNext(tapp);
    else
        fFirst = tapp;
    fLast = tapp;
}

void TaskPane::remove(TaskBarApp *tapp) {
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

TaskBarApp *TaskPane::addApp(YFrameWindow *frame) {
#ifdef CONFIG_WINLIST
    if (frame->client() == windowList)
        return 0;
#endif
    if (frame->client() == taskBar)
        return 0;

    TaskBarApp *tapp = new TaskBarApp(frame, this);

    if (tapp != 0) {
        insert(tapp);
#if 0
        tapp->show();
        if (!frame->visibleOn(manager->activeWorkspace()) &&
            !taskBarShowAllWindows)
            tapp->setShown(0);
        if (frame->owner() != 0 && !taskBarShowTransientWindows)
            tapp->setShown(0);
        relayout();
#endif
    }
    return tapp;
}

void TaskPane::removeApp(YFrameWindow *frame) {
    for (TaskBarApp *task(fFirst); NULL != task; task = task->getNext()) {
        if (task->getFrame() == frame) {
            task->hide();
            remove(task);
            delete task;

            relayout();
            return;
        }
    }
}

void TaskPane::relayoutNow() {
    if (!fNeedRelayout)
        return ;

    fNeedRelayout = false;

    int x, y, w, h;
    int tc = 0;

    TaskBarApp *a = fFirst;

    while (a) {
        if (a->getShown())
            tc++;
        a = a->getNext();
    }
    if (tc < taskBarButtonWidthDivisor) tc = taskBarButtonWidthDivisor;

    int leftX = 0;
    int rightX = width();

    w = (rightX - leftX - 2) / tc;
    int rem = (rightX - leftX - 2) % tc;
    x = leftX;
    h = height();
    y = 0;

    TaskBarApp *f = fFirst;
    int lc = 0;

    while (f) {
        if (f->getShown()) {
            int w1 = w;

            if (lc < rem)
                w1++;

            f->setGeometry(YRect(x, y, w1, h));
            f->show();
            x += w1;
            x += 0;
            lc++;
        } else
            f->hide();
        f = f->getNext();
    }
}

void TaskPane::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        taskBar->contextMenu(up.x_root, up.y_root);
    }
}

void TaskPane::paint(Graphics &g, const YRect &/*r*/) {
    g.setColor(taskBarBg);
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);

#ifdef CONFIG_GRADIENTS
    ref<YPixbuf> gradient = parent()->getGradient();

    if (gradient != null)
        g.copyPixbuf(*gradient, x(), y(), width(), height(), 0, 0);
    else
#endif    
    if (taskbackPixmap != null)
        g.fillPixmap(taskbackPixmap, 0, 0, width(), height(), x(), y());
    else
        g.fillRect(0, 0, width(), height());
}

#endif
