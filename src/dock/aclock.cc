/*
 * IceWM
 *
 * Copyright (C) 1997,98 Marko Macek
 *
 * Clock
 */
#include "config.h"
#include "ylib.h"
#include "aclock.h"

#include "yresource.h"
#include "yapp.h"
#include "prefs.h"
#include <string.h>
#include <stdio.h>

//YColor *YClock::clockBg = 0;
//YColor *YClock::clockFg = 0;
//YFont *YClock::clockFont = 0;

YColorPrefProperty YClock::gClockBg("clock_applet", "ColorClock", "rgb:00/00/00");
YColorPrefProperty YClock::gClockFg("clock_applet", "ColorClockText", "rgb:00/FF/00");
YFontPrefProperty YClock::gClockFont("clock_applet", "FontClockText", TTFONT(140));

const char *gDefaultTimeFmt = "%H:%M:%S";
const char *gDefaultDateFmt = "%B %A %Y-%m-%d %H:%M:%S %Z";

YClock::YClock(YWindow *aParent): YWindow(aParent),
    clockTimer(this, 1000),
    fFormatTime("clock_applet", "TimeFormat"),
    fFormatDate("clock_applet", "DateFormat")
{
    PixNum[0] = PixNum[1] = PixNum[2] = PixNum[3] = PixNum[4] = 0;
    PixNum[5] = PixNum[6] = PixNum[7] = PixNum[8] = PixNum[9] = 0;
    PixSpace = 0;
    PixColon = 0;
    PixSlash = 0;
    PixA = 0;
    PixP = 0;
    PixM = 0;
    PixDot = 0;

    YResourcePath *rp = app->getResourcePath("ledclock/");
    if (rp) {
        PixNum[0] = app->loadResourcePixmap(rp, "n0.xpm");
        PixNum[1] = app->loadResourcePixmap(rp, "n1.xpm");
        PixNum[2] = app->loadResourcePixmap(rp, "n2.xpm");
        PixNum[3] = app->loadResourcePixmap(rp, "n3.xpm");
        PixNum[4] = app->loadResourcePixmap(rp, "n4.xpm");
        PixNum[5] = app->loadResourcePixmap(rp, "n5.xpm");
        PixNum[6] = app->loadResourcePixmap(rp, "n6.xpm");
        PixNum[7] = app->loadResourcePixmap(rp, "n7.xpm");
        PixNum[8] = app->loadResourcePixmap(rp, "n8.xpm");
        PixNum[9] = app->loadResourcePixmap(rp, "n9.xpm");

        PixSpace = app->loadResourcePixmap(rp, "space.xpm");
        PixColon = app->loadResourcePixmap(rp, "colon.xpm");
        PixSlash = app->loadResourcePixmap(rp, "slash.xpm");
        PixDot = app->loadResourcePixmap(rp, "dot.xpm");
        PixA = app->loadResourcePixmap(rp, "a.xpm");
        PixP = app->loadResourcePixmap(rp, "p.xpm");
        PixM = app->loadResourcePixmap(rp, "m.xpm");
        delete rp;
    }

    clockUTC = false;
    toolTipUTC = false;

    //clockTimer = new YTimer(1000);
    //clockTimer->setTimerListener(this);
    clockTimer.startTimer();
    autoSize();
    updateToolTip();
    setDND(true);
}

YClock::~YClock() {
    //delete clockTimer; clockTimer = 0;

    delete PixSpace;
    delete PixSlash;
    delete PixDot;
    delete PixA;
    delete PixP;
    delete PixM;
    delete PixColon;
    for (int n = 0; n < 10; n++) delete PixNum[n];
}

void YClock::autoSize() {
    char s[64];
    time_t newTime = time(NULL);
    struct tm t = *localtime(&newTime);
    int maxDay = -1;
    int maxMonth = -1;
    int maxWidth = -1;
    const char *fmtTime = fFormatTime.getStr(gDefaultTimeFmt);

    t.tm_hour = 12;
    for (int m = 0; m < 12 ; m++) {
        int len, w;

        t.tm_mon = m;

        len = strftime(s, sizeof(s), fmtTime, &t);
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

        len = strftime(s, sizeof(s), fmtTime, &t);
        w = calcWidth(s, len);
        if (w > maxWidth) {
            maxDay = dw;
            maxWidth = w;
        }
    }
    YPref prefPrettyFont("clock_applet", "PrettyFont");
    bool pvPrettyFont = prefPrettyFont.getBool(false);
    if (!pvPrettyFont)
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

    len = strftime(s, sizeof(s), fFormatDate.getStr(gDefaultDateFmt), t);

    setToolTip(s);
}

void YClock::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify) {
        if (crossing.state & ControlMask)
            toolTipUTC = true;
        else
            toolTipUTC = false;
        clockTimer.startTimer();
    }
    clockTimer.runTimer();
    YWindow::handleCrossing(crossing);
}

void YClock::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if ((count % 2) == 0) {
            YPref prefCommand("clock_applet", "ClockCommand");
            const char *pvCommand = prefCommand.getStr(0);

            if (pvCommand && pvCommand[0])
                app->runCommand(pvCommand);
        }
    }
}

void YClock::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    int x = width();
    char s[64];
    time_t newTime = time(NULL);
    struct tm *t;
    int len, i;
    const char *fmtTime = fFormatTime.getStr(gDefaultTimeFmt);

    if (clockUTC)
        t = gmtime(&newTime);
    else
        t = localtime(&newTime);

    len = strftime(s, sizeof(s), fmtTime, t);

    YPref prefPrettyFont("clock_applet", "PrettyFont");
    bool pvPrettyFont = prefPrettyFont.getBool(false);
    if (pvPrettyFont) {
        i = len - 1;
        for (i = len - 1; x >= 0; i--) {
            YPixmap *p;
            if (i >= 0)
                p = getPixmap(s[i]);
            else
                p = getPixmap(' ');
            if (p) {
                x -= p->width();
                g.drawPixmap(p, x, 0);
            } else if (i < 0) {
                g.setColor(gClockBg);
                g.fillRect(0, 0, x, height());
                break;
            }
        }
    } else {
        int y =  (height() - 1 - gClockFont.getFont()->height()) / 2
            + gClockFont.getFont()->ascent();

        g.setColor(gClockBg);
        g.fillRect(0, 0, width(), height());
        g.setColor(gClockFg);
        g.setFont(gClockFont);
        g.drawChars(s, 0, len, 2, y);
    }
    clockTimer.startTimer();
}

bool YClock::handleTimer(YTimer *t) {
    if (t != &clockTimer)
        return false;
    if (toolTipVisible())
        updateToolTip();
    repaint();
    return true;
}

YPixmap *YClock::getPixmap(char c) {
    YPixmap *pix = 0;
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
    YPref prefPrettyFont("clock_applet", "PrettyFont");
    bool pvPrettyFont = prefPrettyFont.getBool(false);
    if (!pvPrettyFont)
        return gClockFont.getFont()->textWidth(s, count);
    else {
        int len = 0;

        while (count--) {
            YPixmap *p = getPixmap(*s++);
            if (p)
                len += p->width();
        }
        return len;
    }
}
