/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Clock
 */

#define NEED_TIME_H

#include "config.h"
#include "aclock.h"
#include "sysdep.h"
#include "wmtaskbar.h"
#include "wpixmaps.h"
#include "wmapp.h"
#include "prefs.h"
#include "ymenuitem.h"
#include "intl.h"

static char const *AppletClockTimeFmt = "%T";

char const * YClock::strTimeFmt(struct tm const & t) {
    if ((ledPixColon == null) || (! prettyClock) || strcmp(fmtTime, "%X"))
        return (fmtTimeAlt && (t.tm_sec & 1) ? fmtTimeAlt : fmtTime);
    return AppletClockTimeFmt;
}

YClock::YClock(YSMListener *smActionListener, IAppletContainer* iapp, YWindow *aParent):
    YWindow(aParent),
    clockUTC(false),
    toolTipUTC(false),
    isVisible(false),
    clockTicked(true),
    transparent(-1),
    smActionListener(smActionListener),
    iapp(iapp),
    fMenu(0),
    negativePosition(INT_MAX),
    clockPixmap(None),
    clockBg(&clrClock),
    clockFg(&clrClockText),
    clockFont(YFont::getFont(XFA(clockFontName)))
{
    memset(positions, 0, sizeof positions);
    memset(previous, 0, sizeof previous);

    if (prettyClock && ledPixSpace != null && ledPixSpace->width() == 1)
        ledPixSpace = ledPixSpace->scale(5, ledPixSpace->height());

    clockTimer->setFixed();
    clockTimer->setTimer(1000, this, true);

    addEventMask(VisibilityChangeMask);
    autoSize();
    updateToolTip();
    setDND(true);
    setTitle("Clock");
    show();
}

YClock::~YClock() {
    if (clockPixmap)
        XFreePixmap(xapp->display(), clockPixmap);
}

void YClock::autoSize() {
    char s[TimeSize];
    time_t newTime = time(NULL);
    struct tm t = *localtime(&newTime);
    int maxMonth = -1;
    int maxWidth = -1;

    t.tm_sec = 59;
    t.tm_min = 48;
    t.tm_hour = 23;
    t.tm_mday = 25;
    t.tm_wday = 0;

    for (int m = 0; m < 12 ; m++) {
        int len, w;

        t.tm_mon = m;

        len = strftime(s, sizeof(s), strTimeFmt(t), &t);
        w = calcWidth(s, len);
        if (w > maxWidth) {
            maxMonth = m;
            maxWidth = w;
        }
    }
    t.tm_mon = maxMonth;

    for (int dw = 0; dw <= 6; dw++) {
        int len, w;

        t.tm_wday = dw;

        len = strftime(s, sizeof(s), strTimeFmt(t), &t);
        w = calcWidth(s, len);
        if (w > maxWidth) {
            maxWidth = w;
        }
    }

    if (!prettyClock)
        maxWidth += 4;

    setSize(maxWidth, taskBarGraphHeight);
}

void YClock::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (button.button == 1 && (button.state & ControlMask)) {
            clockUTC = true;
            repaint();
        }
    } else if (button.type == ButtonRelease) {
        if (button.button == 1 && clockUTC) {
            clockUTC = false;
            repaint();
        }
    }
    YWindow::handleButton(button);
}

void YClock::updateToolTip() {
    char s[DateSize];
    time_t newTime = time(NULL);
    struct tm *t;

    if (toolTipUTC)
        t = gmtime(&newTime);
    else
        t = localtime(&newTime);

    strftime(s, sizeof(s), fmtDate, t);

    setToolTip(s);
}

void YClock::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify) {
        if (crossing.state & ControlMask)
            toolTipUTC = true;
        else
            toolTipUTC = false;
        clockTimer->startTimer();
    }
    clockTimer->runTimer();
    YWindow::handleCrossing(crossing);
}

#ifdef DEBUG
static int countEvents = 0;
extern int xeventcount;
#endif

void YClock::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if (clockCommand && clockCommand[0] &&
            (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2)))
            smActionListener->runCommandOnce(clockClassHint, clockCommand);
#ifdef DEBUG
    } else if (up.button == 2) {
        if ((count % 2) == 0)
            countEvents = !countEvents;
#endif
    }
    else if (up.button == Button3) {
        fMenu = new YMenu();
        fMenu->setActionListener(this);
        fMenu->addItem(_("Clock"), -2, null, actionNull)->setEnabled(false);
        fMenu->addItem(_("_Disable"), -2, null, actionClose);
        fMenu->addItem(_("_UTC"), -2, null, actionDepth)->setChecked(clockUTC);
        fMenu->popup(0, 0, 0, up.x_root, up.y_root,
                     YPopupWindow::pfCanFlipVertical |
                     YPopupWindow::pfCanFlipHorizontal |
                     YPopupWindow::pfPopupMenu);
    }
}

void YClock::actionPerformed(YAction action, unsigned int modifiers) {
    if (action == actionClose) {
        hide();
        iapp->relayout();
    }
    else if (action == actionDepth) {
        clockUTC ^= true;
        repaint();
    }
}

void YClock::handleVisibility(const XVisibilityEvent& visib) {
    isVisible = inrange(visib.state, 0, 1);
}

void YClock::handleExpose(const XExposeEvent& e) {
    if (clockPixmap || picture())
        paint(getGraphics(), YRect(e.x, e.y, e.width, e.height));
}

void YClock::repaint() {
    if (isVisible && picture())
        paint(getGraphics(), YRect(0, 0, width(), height()));
}

void YClock::paint(Graphics &g, const YRect& r) {
    g.copyDrawable(clockPixmap, r.x(), r.y(), r.width(), r.height(), r.x(), r.y());
}

bool YClock::picture() {
    bool create = (clockPixmap == None);
    if (create)
        clockPixmap = XCreatePixmap(xapp->display(), handle(),
                                    width(), height(), depth());

    Graphics G(clockPixmap, width(), height(), depth());

    if (create)
        fill(G);

    return clockTicked
         ? clockTicked = false, draw(G), true
         : create;
}

bool YClock::draw(Graphics& g) {
    char s[TimeSize];
    timeval walltm = walltime();
    long nextChime = 1000L - walltm.tv_usec / 1000L;
    time_t newTime = walltm.tv_sec;
    struct tm *t;
    int len;

    clockTimer->setTimer(nextChime, this, true);

    if (clockUTC)
        t = gmtime(&newTime);
    else
        t = localtime(&newTime);

#ifdef DEBUG
    if (countEvents)
        len = sprintf(s, "%d", xeventcount);
    else
#endif
        len = strftime(s, sizeof(s), strTimeFmt(*t), t);

    return prettyClock
         ? paintPretty(g, s, len)
         : paintPlain(g, s, len);
}

void YClock::fill(Graphics& g)
{
    if (!prettyClock && clockBg) {
        g.setColor(clockBg);
        g.fillRect(0, 0, width(), height());
        return;
    }

    if (hasTransparency()) {
        ref<YImage> gradient(parent()->getGradient());

        if (gradient != null)
            g.drawImage(gradient, this->x(), this->y(),
                         width(), height(), 0, 0);
        else
        if (taskbackPixmap != null) {
            g.fillPixmap(taskbackPixmap, 0, 0,
                         width(), height(), this->x(), this->y());
        }
        else {
            g.setColor(taskBarBg);
            g.fillRect(0, 0, width(), height());
        }
    }
}

void YClock::fill(Graphics& g, int x, int y, int w, int h)
{
    if (!prettyClock && clockBg) {
        g.setColor(clockBg);
        g.fillRect(x, y, w, h);
    }
    else {
        ref<YImage> gradient(parent()->getGradient());

        if (gradient != null)
            g.drawImage(gradient, this->x() + x, this->y() + y,
                         w, h, x, y);
        else
        if (taskbackPixmap != null) {
            g.fillPixmap(taskbackPixmap, x, y,
                         w, h, this->x() + x, this->y() + y);
        }
        else if (clockBg) {
            g.setColor(clockBg);
            g.fillRect(x, y, w, h);
        }
        else {
            g.setColor(taskBarBg);
            g.fillRect(x, y, w, h);
        }
    }
}

bool YClock::paintPretty(Graphics& g, const char* s, int len) {
    bool paint = false;
    if (prettyClock) {
        bool const mustFill = hasTransparency();
        int x = width();

        for (int i = len - 1; x >= 0; i--) {
            ref<YPixmap> p;
            if (i >= 0)
                p = getPixmap(s[i]);
            else
                p = ledPixSpace;
            if (p != null)
                x -= p->width();

            if (i >= 0) {
                if (positions[i] == x && previous[i] == s[i])
                    continue;
                else positions[i] = x, previous[i] = s[i];
            }
            else if (i == -1) {
                if (negativePosition == x)
                    break;
                else negativePosition = x;
            }

            if (p != null) {
                if (mustFill)
                    fill(g, x, 0, p->width(), height());

                g.drawPixmap(p, x, 0);
            } else if (i < 0) {
                fill(g, 0, 0, x, height());
                break;
            }
            paint = true;
        }
    }
    return paint;
}

bool YClock::paintPlain(Graphics& g, const char* s, int len) {
    fill(g);
    if (!prettyClock) {
        int y =  (height() - 1 - clockFont->height()) / 2
            + clockFont->ascent();

        g.setColor(clockFg);
        g.setFont(clockFont);
        g.drawChars(s, 0, len, 2, y);
    }
    return true;
}

bool YClock::handleTimer(YTimer *t) {
    if (t != clockTimer)
        return false;
    if (toolTipVisible())
        updateToolTip();
    clockTicked = true;
    repaint();
    return true;
}

ref<YPixmap> YClock::getPixmap(char c) {
    ref<YPixmap> pix;
    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        pix = ledPixNum[c - '0'];
        break;
    case ' ':
        pix = ledPixSpace;
        break;
    case ':':
        pix = ledPixColon;
        break;
    case '/':
        pix = ledPixSlash;
        break;
    case '.':
        pix = ledPixDot;
        break;
    case 'p':
    case 'P':
        pix = ledPixP;
        break;
    case 'a':
    case 'A':
        pix = ledPixA;
        break;
    case 'm':
    case 'M':
        pix = ledPixM;
        break;
    }
    return pix;
}

int YClock::calcWidth(const char *s, int count) {
    if (!prettyClock)
        return clockFont->textWidth(s, count);
    else {
        int len = 0;

        while (count--) {
            ref<YPixmap> p = getPixmap(*s++);
            if (p != null)
                len += p->width();
        }
        return len;
    }
}

bool YClock::hasTransparency() {
    if (transparent == 0)
        return false;
    else if (transparent == 1)
        return true;
    if (!prettyClock) {
        transparent = 1;
        return true;
    }
    ref<YPixmap> p = getPixmap('0');
    if (p != null && p->mask()) {
        transparent = 1;
        return true;
    }
    transparent = 0;
    return false;
}

// vim: set sw=4 ts=4 et:
