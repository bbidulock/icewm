#include "config.h"

#include "atasks.h"
#include "wmtaskbar.h"
#include "deffonts.h"
#include "yapp.h"
#include "wmdesktop.h"
#include "ycstring.h"
#include "ypaint.h"
#include "ybuttonevent.h"
#include "ycrossingevent.h"

#include <string.h>
#include <stdio.h>

YColorPrefProperty TaskPane::gTaskBarBg("taskbar", "ColorBackground", "rgb:C0/C0/C0");

YColorPrefProperty TaskBarApp::gNormalAppBg("taskbar", "ColorNormalApp", "rgb:C0/C0/C0");
YColorPrefProperty TaskBarApp::gNormalAppFg("taskbar", "ColorNormalAppText", "rgb:00/00/00");
YColorPrefProperty TaskBarApp::gActiveAppBg("taskbar", "ColorActiveApp", "rgb:E0/E0/E0");
YColorPrefProperty TaskBarApp::gActiveAppFg("taskbar", "ColorNormalAppText", "rgb:00/00/00");
YColorPrefProperty TaskBarApp::gMinimizedAppBg("taskbar", "ColorMinimizedApp", "rgb:A0/A0/a0");
YColorPrefProperty TaskBarApp::gMinimizedAppFg("taskbar", "ColorMinimizedAppText", "rgb:00/00/00");
YColorPrefProperty TaskBarApp::gInvisibleAppBg("taskbar", "ColorInvisibleApp", "rgb:80/80/80");
YColorPrefProperty TaskBarApp::gInvisibleAppFg("taskbar", "ColorInvisibleAppText", "rgb:00/00/00");
YFontPrefProperty TaskBarApp::gNormalFont("taskbar", "FontNormalApp", FONT(120));
YFontPrefProperty TaskBarApp::gActiveFont("taskbar", "FontActiveApp", BOLDFONT(120));
YPixmapPrefProperty TaskBarApp::gPixmapTaskBarBackground("taskbar", "PixmapAppInvisible", "taskbarbg.xpm", DATADIR); // !!!?
YPixmapPrefProperty TaskBarApp::gPixmapNormalBackground("taskbar", "PixmapAppNormalBg", "taskbuttonbg.xpm", DATADIR);
YPixmapPrefProperty TaskBarApp::gPixmapActiveBackground("taskbar", "PixmapAppActiveBg", "taskbuttonactive.xpm", DATADIR);
YPixmapPrefProperty TaskBarApp::gPixmapMinimizedBackground("taskbar", "PixmapAppNormalBg", "taskbuttonminimized.xpm", DATADIR);

TaskBarApp::TaskBarApp(WindowInfo *frame, YWindow *aParent): YWindow(aParent) {
    fFrame = frame;
    fPrev = fNext = 0;
    selected = 0;
    fShown = true;
    setToolTip(frame->getTitle());
    //setDND(true);
}

TaskBarApp::~TaskBarApp() {
#if 0
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
#endif
}

bool TaskBarApp::isFocusTraversable() {
    return true;
}

void TaskBarApp::setShown(bool ashow) {
    if (ashow != fShown) {
        fShown = ashow;
    }
}

void TaskBarApp::paint(Graphics &g, const YRect &/*er*/) {
    YColor *bg;
    YColor *fg;
    YPixmap *bgPix = 0;
    int p = 0;

    if (!getFrame()->visibleNow()) {
        bg = gInvisibleAppBg.getColor();
        fg = gInvisibleAppFg.getColor();
        bgPix = gPixmapTaskBarBackground.getPixmap();
    } else if (getFrame()->isMinimized()) {
        bg = gMinimizedAppBg.getColor();
        fg = gMinimizedAppFg.getColor();
        bgPix = gPixmapMinimizedBackground.getPixmap();
    } else if (getFrame()->focused()) {
        bg = gActiveAppBg.getColor();
        fg = gActiveAppFg.getColor();
        bgPix = gPixmapActiveBackground.getPixmap();
    } else {
        bg = gNormalAppBg.getColor();
        fg = gNormalAppFg.getColor();
        bgPix = gPixmapNormalBackground.getPixmap();
    }

    if (selected == 3) {
        p = 2;
        g.setColor(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.setColor(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        if (getFrame()->focused() || selected == 2) {
            p = 2;
            g.setColor(bg);
            //if (wmLook == lookMetal) {
            //    g.drawBorderM(0, 0, width() - 1, height() - 1, false);
            //} else if (wmLook == lookGtk) {
                g.drawBorderG(0, 0, width() - 1, height() - 1, false);
            //}
            //else
                g.drawBorderW(0, 0, width() - 1, height() - 1, false);
        } else {
            p = 1;
            g.setColor(bg);
            //if (wmLook == lookMetal) {
            //    p = 2;
            //    g.drawBorderM(0, 0, width() - 1, height() - 1, true);
            //} else if (wmLook == lookGtk) {
             //   g.drawBorderG(0, 0, width() - 1, height() - 1, true);
            //}
            //else
                g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        }
        //if (wmLook == lookMetal) {
        //    g.fillRect(2, 2, width() - 4, height() - 4);
        //} else if (wmLook == lookGtk) {
        //    if (bgPix) {
        //        g.fillPixmap(bgPix, p, p, width() - 3, height() - 3);
        //        g.drawBorderG(0, 0, width() - 1, height() - 1, getFrame()->focused() ? false : true);
        //    } else
        //        g.fillRect(p, p, width() - 3, height() - 3);
        //}
        //else
            g.fillRect(p, p, width() - 3, height() - 3);
    }

    YIcon *i = getFrame()->getIcon();
    if (i) {
        YPixmap *s = i->small();
        if (i->small()) {
            int y = (height() - 3 - s->height()) / 2;
            g.drawPixmap(s, p + 1, p + 1 + y);
        }
    }

    const CStr *str = getFrame()->getIconTitle();
    if (!str || str->isWhitespace())
        str = getFrame()->getTitle();
    if (str) {
        g.setColor(fg);
        YFont *font = 0;
        if (getFrame()->focused())
            font = gActiveFont.getFont();
        else
            font = gNormalFont.getFont();
        if (font) {
            g.setFont(font);
            int ty = (height() - 1 + font->height()) / 2 - font->descent();
            if (ty < 2)
                ty = 2;
            g.drawCharsEllipsis(str->c_str(), str->length(),
                                p + 3 + 16,
                                p + ty,
                                width() - (p + 3 + 16 + 3));
        }
    }
}

bool TaskBarApp::eventButton(const YButtonEvent &button) {
    if (YWindow::eventButton(button))
        return true;

    if (button.getButton() == 1 || button.getButton() == 2) {
        if (button.type() == YEvent::etButtonPress) {
            selected = 2;
            repaint();
        } else if (button.type() == YEvent::etButtonRelease) {
            if (selected == 2) {
                if (button.getButton() == 1) {
                    if (getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || button.isCtrl()))
                        getFrame()->wmMinimize();
                    else {
#if 0
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyOnlyWorkspace(getFrame()->getRoot()->activeWorkspace());
#endif
                        getFrame()->activateWindow(true);
                    }
                } else if (button.getButton() == 2) {
                    if (getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || button.isCtrl()))
                        getFrame()->wmLower();
                    else {
#if 0
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyWorkspace(getFrame()->getRoot()->activeWorkspace());
#endif
                        getFrame()->activateWindow(true);
                    }
                }
            }
            selected = 0;
            repaint();
        }
    }
    return true;
}

bool TaskBarApp::eventCrossing(const YCrossingEvent &crossing) {
    if (selected > 0) {
        if (crossing.type() == YEvent::etPointerIn) {
            selected = 2;
            repaint();
        } else if (crossing.type() == YEvent::etPointerOut) {
            selected = 1;
            repaint();
        }
    }
    return YWindow::eventCrossing(crossing);
}

bool TaskBarApp::eventClick(const YClickEvent &up) {
    if (up.getButton() == 3) {
#if 0
        getFrame()->popupSystemMenu(up.x_root(), up.y_root(), -1, -1,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal |
                                    YPopupWindow::pfPopupMenu);
#endif
    }
    return YWindow::eventClick(up);
}

void TaskBarApp::handleDNDEnter(int /*nTypes*/, XAtomId * /*types*/) {
#if 0
    if (fRaiseTimer == 0)
        fRaiseTimer = new YTimer(autoRaiseDelay);
    if (fRaiseTimer) {
        fRaiseTimer->setTimerListener(this);
        fRaiseTimer->startTimer();
    }
    selected = 3;
    repaint();
#endif
}

void TaskBarApp::handleDNDLeave() {
#if 0
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    selected = 0;
    repaint();
#endif
}

bool TaskBarApp::handleTimer(YTimer */*t*/) {
#if 0
    if (t == fRaiseTimer) {
        getFrame()->wmRaise();
    }
#endif
    return false;
}

TaskPane::TaskPane(DesktopInfo *root, YWindow *parent):
     YWindow(parent),
     fShowAllWindows("taskbar", "ShowAllWindows")
{
    fRoot = root;
    fFirst = fLast = 0;
    fCount = 0;
    fNeedRelayout = true;
    root->setTaskPane(this);
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

TaskBarApp *TaskPane::addApp(WindowInfo *frame) {
    // !!!
#if 0
    if (frame->client() == frame->getRoot()->getWindowList())
        return 0;
#endif
#if 0
    if (frame->client() == frame->getRoot()->getTaskBar())
        return 0;
#endif

    TaskBarApp *tapp = new TaskBarApp(frame, this);

    if (tapp != 0) {
        insert(tapp);
        tapp->show();
        if (!frame->visibleNow() && //??? !!! visibleOn(fRoot->activeWorkspace()) &&
            !fShowAllWindows.getBool(false))
            tapp->setShown(0);
        relayout();
    }
    return tapp;
}

void TaskPane::removeApp(WindowInfo *frame) {
    TaskBarApp *f = fFirst, *next;

    while (f) {
        next = f->getNext();
        if (f->getFrame() == frame) {
            f->hide();
            remove(f);
            delete f;
            relayout();
            return;
        }
        f = next;
    }
}

void TaskPane::relayoutNow() {
    if (!fNeedRelayout)
        return;

    fNeedRelayout = false;
    printf("%d %d\n", width(), height());
    setSize(713, 24);

    int x, y, w, h;
    int tc = 0;

    TaskBarApp *a = fFirst;

    while (a) {
        if (a->getShown())
            tc++;
        a = a->getNext();
    }

    if (tc < 3) tc = 3;

    int leftX = 0;
    int rightX = width();

    w = (rightX - leftX - 2) / tc - 2;
    x = leftX;
    h = height();
    y = 0;

    TaskBarApp *f = fFirst;

    while (f) {
        if (f->getShown()) {
            printf("%d %d %d %d\n", x, y, w, h);
            f->setGeometry(x, y, w, h);
            f->show();
            x += w;
            x += 2;
        } else
            f->hide();
        f = f->getNext();
    }
}

//extern YColor *taskBarBg;

bool TaskPane::eventClick(const YClickEvent &up) {
    if (up.rightButton() && up.isSingleClick() /* && IS_BUTTON(up.state, Button3Mask)*/) {
        // !!!
#if 0
        fRoot->getTaskBar()->contextMenu(up.x_root, up.y_root);
#endif
    }
    return YWindow::eventClick(up);
}

void TaskPane::paint(Graphics &g, const YRect &/*er*/) {
    g.setColor(gTaskBarBg);
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);
#if 0 //!!!
    if (taskbackPixmap)
        g.fillPixmap(taskbackPixmap, 0, 0, width(), height());
    else
#endif
        g.fillRect(0, 0, width(), height());
}
