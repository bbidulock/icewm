/*
 * IceWM
 *
 * Copyright (C) 1999 Andreas Hinz
 *
 * Mostly derrived from aclock.cc and aclock.h by Marko Macek
 *
 * Apm status
 */
 
#include "config.h"
#include "yresource.h"
#include "yconfig.h"

#include "aapm.h"

#include "yapp.h"
#include "deffonts.h"
#include "sysdep.h"
#include "ypaint.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

//YColor *YApm::apmBg = 0;
//YColor *YApm::apmFg = 0;
//YFont *YApm::apmFont = 0;

YColorPrefProperty YApm::gApmBg("apm_applet", "ColorApm", "rgb:00/00/00");
YColorPrefProperty YApm::gApmFg("apm_applet", "ColorApmText", "rgb:00/FF/00");
YFontPrefProperty YApm::gApmFont("apm_applet", "FontApm", TTFONT(120));
YPixmapPrefProperty YApm::gPixNum0("apm_applet", "PixmapNum0", "n0.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum1("apm_applet", "PixmapNum1", "n1.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum2("apm_applet", "PixmapNum2", "n2.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum3("apm_applet", "PixmapNum3", "n3.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum4("apm_applet", "PixmapNum4", "n4.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum5("apm_applet", "PixmapNum5", "n5.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum6("apm_applet", "PixmapNum6", "n6.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum7("apm_applet", "PixmapNum7", "n7.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum8("apm_applet", "PixmapNum8", "n8.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixNum9("apm_applet", "PixmapNum9", "n9.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixSpace("apm_applet", "PixmapSpace", "space.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixColon("apm_applet", "PixmapColon", "colon.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixSlash("apm_applet", "PixmapSlash", "slash.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixDot("apm_applet", "PixmapDot", "dot.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixA("apm_applet", "PixmapA", "a.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixP("apm_applet", "PixmapP", "p.xpm", DATADIR);
YPixmapPrefProperty YApm::gPixM("apm_applet", "PixmapM", "m.xpm", DATADIR);

void ApmStr(char *s, bool Tool) {
    char buf[45];
    int len, i, fd = open("/proc/apm", O_RDONLY);
    char driver[16];
    char apmver[16];
    int apmflags;
    int ACstatus;
    int BATstatus;
    int BATflag;
    int BATlife;
    int BATtime;
    char units[16];

    if (fd == -1) {
        return;
    }

    len = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    buf[len] = 0;

    if ((i = sscanf(buf, "%s %s 0x%x 0x%x 0x%x 0x%x %d%% %d %s",
                    driver, apmver, &apmflags,
                    &ACstatus, &BATstatus, &BATflag, &BATlife,
                    &BATtime, units)) != 9)
    {
        static int error = 1;
        if (error) {
            error = 0;
            fprintf(stderr, "/proc/apm format error (%d)\n", i);
        }
        return;
    }
    if (BATlife == -1)
        BATlife = 0;

    if (!Tool) {
        sprintf(s, "%02d", BATlife);
#if 0
        while ((i < 3) && (buf[28 + i] != '%'))
            i++;
        for (len = 2; i; i--, len--) {
            *(s + len) = buf[28 + i - 1];
        }
#endif
    } else {
        sprintf(s, "%d%%", BATlife);
    }

    if (ACstatus == 0x1) {
        if (Tool)
            strcat(s, " - Power");
        else
            strcat(s, "P");
    }
    if ((BATflag & 8)) {
        if (Tool)
            strcat(s, " - Charging");
        else
            strcat(s, "M");
    }
}

YApm::YApm(YWindow *aParent): YWindow(aParent), apmTimer(this, 2000) {
    apmTimer.startTimer();
    autoSize();
}

YApm::~YApm() {
}

void YApm::updateToolTip() {
    char s[30] = { ' ', ' ', ' ', 0, 0, 0, 0 };
    
    ApmStr(s, 1);
    __setToolTip(s);
}

void YApm::autoSize() {
    int maxWidth = 54;

    YPref prefPrettyFont("apmstatus_applet", "PrettyFont");
    bool pvPrettyFont = prefPrettyFont.getBool(true);

    if (!pvPrettyFont) maxWidth += 4;
    setSize(maxWidth, 20);
}

void YApm::paint(Graphics &g, const YRect &/*er*/) {
    unsigned int x = 0;
    char s[8] = { ' ', ' ', ' ', 0, 0, 0, 0, 0 };
    int len, i;
    
    ApmStr(s, 0); len = strlen(s);

    // !!! optimize
    YPref prefPrettyFont("apmstatus_applet", "PrettyFont");
    bool pvPrettyFont = prefPrettyFont.getBool(true);

    if (pvPrettyFont) {
        YPixmap *p;
        for (i = 0; x < width(); i++) {
            if (i < len) {
                p = getPixmap(s[i]);
            } else
                p = gPixSpace.getPixmap();
   
            if (p) {
                g.drawPixmap(p, x, 0);
                x += p->width();
            } else if (i >= len) {
                g.setColor(gApmBg);
                g.fillRect(x, 0, width() - x, height());
                break;
            }
        }
    } else {
        int y = (height() - 1 - gApmFont.getFont()->height()) / 2 + gApmFont.getFont()->ascent();
	
        g.setColor(gApmBg);
        g.fillRect(0, 0, width(), 21);
        g.setColor(gApmFg);
        g.setFont(gApmFont);
        g.__drawChars(s, 0, len, 2, y);
    }
}

bool YApm::handleTimer(YTimer *t) {
    if (t != &apmTimer) return false;

    if (toolTipVisible())
        updateToolTip();
    repaint();
    return true;
}

YPixmap *YApm::getPixmap(char c) {
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

int YApm::__calcWidth(const char *s, int count) {
    YPref prefPrettyFont("apmstatus_applet", "PrettyFont");
    bool pvPrettyFont = prefPrettyFont.getBool(true);

    if (!pvPrettyFont)  return gApmFont.getFont()->__textWidth(s, count);
    else {
        int len = 0;

        while (count--) {
            YPixmap *p = getPixmap(*s++);
            if (p) len += p->width();
        }
        return len;
    }
}
