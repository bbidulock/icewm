/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Clock
 */

#define NEED_TIME_H

#include "config.h"
#include "ylib.h"
#include "sysdep.h"

#include "aclock.h"
#include "wmtaskbar.h"

#include "wmapp.h"
#include "prefs.h"


#ifdef CONFIG_APPLET_CLOCK

ref<YPixmap> PixNum[10];
ref<YPixmap> PixSpace;
ref<YPixmap> PixColon;
ref<YPixmap> PixSlash;
ref<YPixmap> PixA;
ref<YPixmap> PixP;
ref<YPixmap> PixM;
ref<YPixmap> PixDot;
ref<YPixmap> PixPercent;

YColor *YClock::clockBg = 0;
YColor *YClock::clockFg = 0;
ref<YFont> YClock::clockFont;

extern ref<YPixmap> taskbackPixmap;
static YColor *taskBarBg = 0;

inline char const * strTimeFmt(struct tm const & t) {
    return (fmtTimeAlt && (t.tm_sec & 1) ? fmtTimeAlt : fmtTime);
}

YClock::YClock(YWindow *aParent): YWindow(aParent) {
    if (clockBg == 0 && *clrClock)
        clockBg = new YColor(clrClock);
    if (clockFg == 0)
        clockFg = new YColor(clrClockText);
    if (clockFont == null)
        clockFont = YFont::getFont(XFA(clockFontName));
    if (taskBarBg == 0) {
        taskBarBg = new YColor(clrDefaultTaskBar);
    }

    clockUTC = false;
    toolTipUTC = false;
    transparent = -1;

    clockTimer = new YTimer(1000);
    clockTimer->setTimerListener(this);
    clockTimer->startTimer();
    autoSize();
    updateToolTip();
    setDND(true);
}

YClock::~YClock() {
    delete clockTimer; clockTimer = 0;
}

void YClock::autoSize() {
    char s[64];
    time_t newTime = time(NULL);
    struct tm t = *localtime(&newTime);
    int maxDay = -1;
    int maxMonth = -1;
    int maxWidth = -1;

    t.tm_hour = 12;
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
            maxDay = dw;
            maxWidth = w;
        }
    }
    if (!prettyClock)
        maxWidth += 4;
    setSize(maxWidth, 20);
}

void YClock::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (button.button == 1 && (button.state & ControlMask)) {
            clockUTC = true;
            repaint();
        }
    } else if (button.type == ButtonRelease) {
        if (button.button == 1) {
            clockUTC = false;
            repaint();
        }
    }
    YWindow::handleButton(button);
}

void YClock::updateToolTip() {
    char s[128];
    time_t newTime = time(NULL);
    struct tm *t;
    int len;

    if (toolTipUTC)
        t = gmtime(&newTime);
    else
        t = localtime(&newTime);

    len = strftime(s, sizeof(s), fmtDate, t);

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
            wmapp->runCommandOnce(clockClassHint, clockCommand);
#ifdef DEBUG
    } else if (up.button == 2) {
        if ((count % 2) == 0)
            countEvents = !countEvents;
#endif
    }
}

void YClock::paint(Graphics &g, const YRect &/*r*/) {
    int x = width();
    char s[64];
    time_t newTime = time(NULL);
    struct tm *t;
    int len, i;

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

    
    //clean backgroung first, so that it is possible
    //to use transparent lcd pixmaps
    if (hasTransparency()) {
#ifdef CONFIG_GRADIENTS
        ref<YPixbuf> gradient(parent()->getGradient());
    
        if (gradient != null)
            g.copyPixbuf(*gradient, this->x(), this->y(),
                         width(), height(), 0, 0);
        else 
#endif
        if (taskbackPixmap != null) {
            g.fillPixmap(taskbackPixmap, 0, 0,
                         width(), height(), this->x(), this->y());
        }
        else {
            g.setColor(taskBarBg);
            g.fillRect(0, 0, width(), height());
        }
    }

    if (prettyClock) {
        i = len - 1;
        for (i = len - 1; x >= 0; i--) {
            ref<YPixmap> p;
            if (i >= 0)
                p = getPixmap(s[i]);
            else
                p = PixSpace;
            if (p != null) {
                x -= p->width();
                g.drawPixmap(p, x, 0);
            } else if (i < 0) {
                g.setColor(clockBg);
                g.fillRect(0, 0, x, height());
                break;
            }
        }
    } else {
        int y =  (height() - 1 - clockFont->height()) / 2
            + clockFont->ascent();

        if (clockBg) {
            g.setColor(clockBg);
            g.fillRect(0, 0, width(), height());
        }

        g.setColor(clockFg);
        g.setFont(clockFont);
        g.drawChars(s, 0, len, 2, y);
    }
    clockTimer->startTimer();
}

bool YClock::handleTimer(YTimer *t) {
    if (t != clockTimer)
        return false;
    if (toolTipVisible())
        updateToolTip();
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
        pix = PixNum[c - '0'];
        break;
    case ' ':
        pix = PixSpace;
        break;
    case ':':
        pix = PixColon;
        break;
    case '/':
        pix = PixSlash;
        break;
    case '.':
        pix = PixDot;
        break;
    case 'p':
    case 'P':
        pix = PixP;
        break;
    case 'a':
    case 'A':
        pix = PixA;
        break;
    case 'm':
    case 'M':
        pix = PixM;
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
#endif
