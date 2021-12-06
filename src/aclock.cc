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
#include "applet.h"
#include "wpixmaps.h"
#include "wmapp.h"
#include "prefs.h"
#include "ymenuitem.h"
#include "intl.h"

static const char AppletClockTimeFmt[] = "%T";

const char* YClock::strTimeFmt(const struct tm& t) {
    if (fTimeFormat && isEmpty(fAltFormat))
        return fTimeFormat;
    if (ledPixColon == null || ! prettyClock || strcmp(fmtTime, "%X"))
        return (fAltFormat && (t.tm_sec & 1) ? fAltFormat : fTimeFormat);
    return AppletClockTimeFmt;
}

YClock::YClock(YSMListener* sml, IAppletContainer* iapp, YWindow* parent,
               const char* envTZ, const char* myTZ, const char* format):
    IApplet(this, parent),
    clockUTC(false),
    toolTipUTC(false),
    clockTicked(true),
    paintCount(0),
    transparent(-1),
    smActionListener(sml),
    iapp(iapp),
    fMenu(nullptr),
    fTimeFormat(format),
    fAltFormat(fmtTimeAlt),
    fPid(0),
    negativePosition(INT_MAX),
    clockBg(&clrClock),
    clockFg(&clrClockText),
    defaultTimezone(envTZ),
    displayTimezone(myTZ),
    currentTimezone(envTZ)
{
    memset(positions, 0, sizeof positions);
    memset(previous, 0, sizeof previous);
    memset(lastTime, 0, sizeof lastTime);

    if (prettyClock && ledPixSpace != null && ledPixSpace->width() == 1)
        ledPixSpace = ledPixSpace->scale(5, ledPixSpace->height());

    clockTimer->setFixed();
    clockTimer->setTimer(1000, this, true);

    autoSize();
    setTitle("Clock");
    show();
}

YClock::~YClock() {
}

bool YClock::timezone(bool restore) {
    if (displayTimezone) {
        const char* want = restore ? defaultTimezone : displayTimezone;
        const char* have = currentTimezone;
        if (want != have && (!want || !have || strcmp(want, have))) {
            if (want) {
                setenv("TZ", want, True);
            } else {
                unsetenv("TZ");
            }
            currentTimezone = want;
            tzset();
            return true;
        }
    }
    return false;
}

void YClock::autoSize() {
    bool restore = timezone();
    char str[TimeSize];
    time_t newTime = seconds();
    struct tm t = *localtime(&newTime);
    int maxMonth = -1;
    int maxWidth = -1;

    t.tm_sec = 58;
    t.tm_min = 48;
    t.tm_hour = 23;
    t.tm_mday = 25;
    t.tm_wday = 0;

    for (int m = 0; m < 12 ; m++) {
        t.tm_mon = m;
        int len = strftime(str, TimeSize, strTimeFmt(t), &t);
        int w = calcWidth(str, len);
        if (w > maxWidth) {
            maxMonth = m;
            maxWidth = w;
        }
    }
    t.tm_mon = maxMonth;

    for (int dw = 0; dw <= 6; dw++) {
        t.tm_wday = dw;
        int len = strftime(str, TimeSize, strTimeFmt(t), &t);
        int w = calcWidth(str, len);
        if (w > maxWidth) {
            maxWidth = w;
        }
    }

    if (!prettyClock)
        maxWidth += 4;

    setSize(maxWidth, unsigned(taskBarGraphHeight));
    timezone(restore);
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
    bool restore = timezone();
    char str[DateSize];
    time_t newTime = seconds();
    struct tm *t = toolTipUTC ? gmtime(&newTime) : localtime(&newTime);
    strftime(str, DateSize, fmtDate, t);
    setToolTip(str);
    timezone(restore);
}

void YClock::handleCrossing(const XCrossingEvent &crossing) {
    if (crossing.type == EnterNotify) {
        toolTipUTC = hasbit(crossing.state, ControlMask);
        clockTimer->startTimer();
    }
    clockTimer->runTimer();
    YWindow::handleCrossing(crossing);
}

#ifdef DEBUG
static int countEvents = 0;
extern int xeventcount;
#endif

const YAction actionClockHM;
const YAction actionClockHMS;
const YAction actionClockDHM;
const YAction actionClockDate;
const YAction actionClockDefault;
const YAction actionClockUTC;

void YClock::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if (clockCommand && clockCommand[0] &&
            (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2))) {
            bool restore = timezone();
            smActionListener->runCommandOnce(clockClassHint, clockCommand, &fPid);
            timezone(restore);
        }
#ifdef DEBUG
    } else if (up.button == 2) {
        if ((count % 2) == 0)
            countEvents = !countEvents;
#endif
    }
    else if (up.button == Button3) {
        fMenu = new YMenu();
        fMenu->setActionListener(this);
        fMenu->addItem(_("CLOCK"), -2, null, actionNull)->setEnabled(false);
        fMenu->addSeparator();
        fMenu->addItem("%H:%M", -2, null, actionClockHM);
        fMenu->addItem("%H:%M:%S", -2, null, actionClockHMS);
        fMenu->addItem("%d %H:%M", -2, null, actionClockDHM);
        if (!prettyClock)
            fMenu->addItem(_("Date"), -2, null, actionClockDate);
        fMenu->addItem(_("Default"), -2, null, actionClockDefault);
        fMenu->addItem(_("_Disable"), -2, null, actionClose);
        fMenu->addItem(_("_UTC"), -2, null, actionClockUTC)->setChecked(clockUTC);
        fMenu->popup(nullptr, nullptr, nullptr, up.x_root, up.y_root,
                     YPopupWindow::pfCanFlipVertical |
                     YPopupWindow::pfCanFlipHorizontal |
                     YPopupWindow::pfPopupMenu);
    }
}

void YClock::changeTimeFormat(const char* format) {
    bool restore = timezone();
    fTimeFormat = format;
    fAltFormat = nullptr;
    autoSize();
    freePixmap();
    memset(positions, 0, sizeof positions);
    memset(previous, 0, sizeof previous);
    memset(lastTime, 0, sizeof lastTime);
    negativePosition = INT_MAX;
    clockTicked = true;
    repaint();
    iapp->relayout();
    timezone(restore);
}

void YClock::actionPerformed(YAction action, unsigned int modifiers) {
    if (action == actionClose) {
        hide();
        iapp->relayout();
    }
    else if (action == actionClockUTC) {
        clockUTC ^= true;
        clockTicked = true;
        bool restore = timezone();
        repaint();
        timezone(restore);
    }
    else if (action == actionClockHM) {
        changeTimeFormat(" %H:%M ");
    }
    else if (action == actionClockHMS) {
        changeTimeFormat(" %H:%M:%S ");
    }
    else if (action == actionClockDHM) {
        changeTimeFormat(" %d %H:%M ");
    }
    else if (action == actionClockDate) {
        changeTimeFormat(" %c ");
    }
    else if (action == actionClockDefault) {
        changeTimeFormat(nullptr);
    }
}

bool YClock::picture() {
    bool create = (hasPixmap() == false);

    Graphics G(getPixmap(), width(), height(), depth());

    if (create) {
        memset(positions, 0, sizeof positions);
        memset(previous, 0, sizeof previous);
        memset(lastTime, 0, sizeof lastTime);
        negativePosition = INT_MAX;
        fill(G);
    }

    return clockTicked
         ? clockTicked = false, draw(G), true
         : create;
}

bool YClock::draw(Graphics& g) {
    bool restore = timezone();
    timeval walltm = walltime();
    long nextChime = 1000L - walltm.tv_usec / 1000L;
    time_t newTime = walltm.tv_sec;

    clockTimer->setTimer(nextChime, this, true);

    auto t = clockUTC ? gmtime(&newTime) : localtime(&newTime);

    char str[TimeSize];
    int len =
#ifdef DEBUG
        countEvents ? snprintf(str, TimeSize, "%d", xeventcount) :
#endif
        strftime(str, TimeSize, strTimeFmt(*t), t);

    bool drawn = true;
    if (toolTipVisible() || strcmp(str, lastTime)) {
        memcpy(lastTime, str, TimeSize);
        drawn = prettyClock
             ? paintPretty(g, str, len)
             : paintPlain(g, str, len);
    }

    timezone(restore);
    return drawn;
}

void YClock::fill(Graphics& g)
{
    if (!prettyClock && clockBg) {
        g.setColor(clockBg);
        g.fillRect(0, 0, width(), height());
        return;
    }

    if (hasTransparency()) {
        ref<YImage> gradient(getGradient());

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
        ref<YImage> gradient(getGradient());
        if (gradient != null) {
            g.drawImage(gradient, this->x() + x, this->y() + y,
                         w, h, x, y);
        }
        else if (taskbackPixmap != null) {
            XRectangle clip = YRect(x, y, w, h);
            g.setClipRectangles(&clip, 1);
            g.fillPixmap(taskbackPixmap, x, y,
                         w, h, this->x() + x, this->y() + y);
            g.resetClip();
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

bool YClock::paintPretty(Graphics& g, const char* str, int len) {
    bool paint = false;
    if (prettyClock) {
        bool const mustFill = hasTransparency();
        int x = int(width());
        int y = 0;

        ++paintCount;
        for (int i = len - 1; x >= 0; i--) {
            ref<YPixmap> pix(i >= 0 ? getPixmap(str[i]) : ledPixSpace);
            if (pix != null)
                x -= pix->width();

            if (paintCount <= 1) {
                // evade bug
            }
            else if (i >= 0) {
                if (positions[i] == x && previous[i] == str[i])
                    continue;
                else positions[i] = x, previous[i] = str[i];
            }
            else if (i == -1) {
                if (negativePosition == x)
                    break;
                else negativePosition = x;
            }

            if (pix != null) {
                if (mustFill)
                    fill(g, x, 0, pix->width(), height());

                g.drawPixmap(pix, x, y);
            }
            else if (i < 0 && 0 < x) {
                fill(g, 0, 0, x, height());
                break;
            }
            paint = true;
        }
    }
    return paint;
}

bool YClock::paintPlain(Graphics& g, const char* str, int len) {
    fill(g);
    if (!prettyClock && (clockFont || (clockFont = clockFontName) != null)) {
        int y = clockFont->ascent() + (height() - 1 - clockFont->height()) / 2;
        g.setColor(clockFg);
        g.setFont(clockFont);
        g.drawChars(str, 0, len, 2, y);
    }
    return true;
}

bool YClock::handleTimer(YTimer *t) {
    if (t == clockTimer) {
        if (toolTipVisible())
            updateToolTip();
        clockTicked = true;
        repaint();
        return true;
    }
    return false;
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

int YClock::calcWidth(const char* str, int count) {
    int len = 0;
    if (prettyClock) {
        for (char c : YRange<const char>(str, count)) {
            ref<YPixmap> pix = getPixmap(c);
            if (pix != null)
                len += pix->width();
        }
    }
    else if (clockFont || (clockFont = clockFontName) != null) {
        len = clockFont->textWidth(str, count);
    }
    return len;
}

bool YClock::hasTransparency() {
    if (transparent < 0) {
        if (prettyClock == false) {
            transparent = 1;
        }
        else if ((ledPixColon != null && ledPixColon->mask())
             || (ledPixNum[0] != null && ledPixNum[0]->mask())) {
            transparent = 1;
        }
        else {
            transparent = 0;
        }
    }
    return bool(transparent & 1);
}

ClockSet::ClockSet(YSMListener* sml, IAppletContainer* iapp, YWindow* parent) {
    const char* tz = strstr(fmtTime, "TZ=");
    if (tz) {
        if (tz - fmtTime > 1) {
            specs += newstr(fmtTime, tz - fmtTime);
        }
        while (tz) {
            const char* next = strstr(tz + 3, "TZ=");
            int len = int(next ? next - tz : strlen(tz));
            specs += newstr(tz, len);
            tz = next;
        }
        char* envtz = getenv("TZ");
        for (char* spec : specs) {
            if (strncmp(spec, "TZ=", 3) == 0) {
                char* space = strchr(spec, ' ');
                if (space && strlen(space) > 2) {
                    *space = '\0';
                    clocks += new YClock(sml, iapp, parent,
                                         envtz, spec + 3, space + 1);
                }
            } else {
                clocks += new YClock(sml, iapp, parent,
                                     nullptr, nullptr, spec);
            }
        }
    }
    else if (1 < strlen(fmtTime)) {
        clocks += new YClock(sml, iapp, parent,
                             nullptr, nullptr, fmtTime);
    }
}

ClockSet::~ClockSet() {
    clocks.clear();
    for (char* spec : specs) {
        delete[] spec;
    }
}

// vim: set sw=4 ts=4 et:
