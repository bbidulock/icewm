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

YColorPrefProperty YClock::gClockBg("clock_applet", "ColorClock", "rgb:00/00/00");
YColorPrefProperty YClock::gClockFg("clock_applet", "ColorClockText", "rgb:00/FF/00");
YFontPrefProperty YClock::gClockFont("clock_applet", "FontClockText", TTFONT(140));
YPixmapPrefProperty YClock::gPixNum0("clock_applet", "PixmapNum0", "n0.xpm");
YPixmapPrefProperty YClock::gPixNum1("clock_applet", "PixmapNum1", "n1.xpm");
YPixmapPrefProperty YClock::gPixNum2("clock_applet", "PixmapNum2", "n2.xpm");
YPixmapPrefProperty YClock::gPixNum3("clock_applet", "PixmapNum3", "n3.xpm");
YPixmapPrefProperty YClock::gPixNum4("clock_applet", "PixmapNum4", "n4.xpm");
YPixmapPrefProperty YClock::gPixNum5("clock_applet", "PixmapNum5", "n5.xpm");
YPixmapPrefProperty YClock::gPixNum6("clock_applet", "PixmapNum6", "n6.xpm");
YPixmapPrefProperty YClock::gPixNum7("clock_applet", "PixmapNum7", "n7.xpm");
YPixmapPrefProperty YClock::gPixNum8("clock_applet", "PixmapNum8", "n8.xpm");
YPixmapPrefProperty YClock::gPixNum9("clock_applet", "PixmapNum9", "n9.xpm");
YPixmapPrefProperty YClock::gPixSpace("clock_applet", "PixmapSpace", "space.xpm");
YPixmapPrefProperty YClock::gPixColon("clock_applet", "PixmapColon", "colon.xpm");
YPixmapPrefProperty YClock::gPixSlash("clock_applet", "PixmapSlash", "slash.xpm");
YPixmapPrefProperty YClock::gPixDot("clock_applet", "PixmapDot", "dot.xpm");
YPixmapPrefProperty YClock::gPixA("clock_applet", "PixmapA", "a.xpm");
YPixmapPrefProperty YClock::gPixP("clock_applet", "PixmapP", "p.xpm");
YPixmapPrefProperty YClock::gPixM("clock_applet", "PixmapM", "m.xpm");

const char *gDefaultTimeFmt = "%H:%M:%S";
const char *gDefaultDateFmt = "%B %A %Y-%m-%d %H:%M:%S %Z";

YClock::YClock(YWindow *aParent): YWindow(aParent),
    clockTimer(this, 1000),
    fFormatTime("clock_applet", "TimeFormat"),
    fFormatDate("clock_applet", "DateFormat")
{
    clockUTC = false;
    toolTipUTC = false;

    clockTimer.startTimer();
    autoSize();
    updateToolTip();
    setDND(true);
}

YClock::~YClock() {
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

    _setToolTip(s);
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
        //g.drawText(YRect(0, 0, width(), height()),
        //           CStaticStr(s),
        //           DrawText_VCenter);
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
    case '0': pix = gPixNum0.getPixmap(); break;
    case '1': pix = gPixNum1.getPixmap(); break;
    case '2': pix = gPixNum2.getPixmap(); break;
    case '3': pix = gPixNum3.getPixmap(); break;
    case '4': pix = gPixNum4.getPixmap(); break;
    case '5': pix = gPixNum5.getPixmap(); break;
    case '6': pix = gPixNum6.getPixmap(); break;
    case '7': pix = gPixNum7.getPixmap(); break;
    case '8': pix = gPixNum8.getPixmap(); break;
    case '9': pix = gPixNum9.getPixmap(); break;
    case ' ': pix = gPixSpace.getPixmap(); break;
    case ':': pix = gPixColon.getPixmap(); break;
    case '/': pix = gPixSlash.getPixmap(); break;
    case '.': pix = gPixDot.getPixmap(); break;
    case 'p':
    case 'P': pix = gPixP.getPixmap(); break;
    case 'a':
    case 'A': pix = gPixA.getPixmap(); break;
    case 'm':
    case 'M': pix = gPixM.getPixmap(); break;
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
