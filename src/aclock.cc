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
#include "wpixmaps.h"
#include "wmapp.h"
#include "prefs.h"


#ifdef CONFIG_APPLET_CLOCK

static char const *AppletClockTimeFmt = "%T";

inline char const * strTimeFmt(struct tm const & t) {
    if ((ledPixColon == null) || (! prettyClock))
        return (fmtTimeAlt && (t.tm_sec & 1) ? fmtTimeAlt : fmtTime);
    return AppletClockTimeFmt;
}

YClock::YClock(YSMListener *smActionListener, YWindow *aParent): YWindow(aParent) {
    this->smActionListener = smActionListener;
    clockBg = *clrClock ? new YColor(clrClock) : 0;
    clockFg = new YColor(clrClockText);
    clockFont = YFont::getFont(XFA(clockFontName));

    clockUTC = false;
    toolTipUTC = false;
    transparent = -1;

    clockTimer = new YTimer(1000);
    clockTimer->setFixed();
    clockTimer->setTimerListener(this);
    clockTimer->startTimer();
    autoSize();
    updateToolTip();
    setDND(true);
}

YClock::~YClock() {
    delete clockTimer; clockTimer = 0;
    delete clockBg; clockBg = 0;
    delete clockFg; clockFg = 0;
    clockFont = null;
}

void YClock::autoSize() {
    char s[64];
    time_t newTime = time(NULL);
    struct tm t = *localtime(&newTime);
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
        ref<YImage> gradient(parent()->getGradient());
    
        if (gradient != null)
            g.drawImage(gradient, this->x(), this->y(),
                         width(), height(), 0, 0);
        else 
#endif
        if (taskbackPixmap != null) {
            g.fillPixmap(taskbackPixmap, 0, 0,
                         width(), height(), this->x(), this->y());
        }
        else {
            g.setColor(getTaskBarBg());
            g.fillRect(0, 0, width(), height());
        }
    }

    if (prettyClock) {
        for (i = len - 1; x >= 0; i--) {
            ref<YPixmap> p;
            if (i >= 0)
                p = getPixmap(s[i]);
            else
                p = ledPixSpace;
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
#endif
