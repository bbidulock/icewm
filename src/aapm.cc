/*
 * IceWM
 *
 * Copyright (C) 1999 Andreas Hinz
 *
 * Mostly derrived from aclock.cc and aclock.h by Marko Macek
 *
 * Apm status
 */

#define NEED_TIME_H

#include "config.h"
#include "ylib.h"
#include "sysdep.h"

#include "aapm.h"
#include "aclock.h"

#include "wmtaskbar.h"
#include "yapp.h"
#include "prefs.h"
#include "intl.h"
#include <string.h>
#include <stdio.h>

#ifdef CONFIG_APPLET_APM

YColor *YApm::apmBg = 0;
YColor *YApm::apmFg = 0;
YFont *YApm::apmFont = 0;

extern YPixmap *taskbackPixmap;

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
        return ;
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
            warn(_("/proc/apm - unknown format (%d)"), i);
        }
        return ;
    }
    if (BATlife == -1)
        BATlife = 0;

    if (!Tool) {
        if (taskBarShowApmTime) { // mschy
            if (BATtime == -1) {
                // -1 indicates that apm-bios can't
                // calculate time for akku
                // no wonder -> we're plugged!
                sprintf(s, "%02d", BATlife);
            } else
                sprintf(s, "%d:%02d", BATtime/60,BATtime%60);
        } else
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



    if (ACstatus == 0x1)
        if (Tool)
            strcat(s,_(" - Power"));
        else
            strcat(s,_("P"));
    if ((BATflag & 8))
        if (Tool)
            strcat(s,_(" - Charging"));
        else
            strcat(s,_("M"));
}

YApm::YApm(YWindow *aParent): YWindow(aParent) {
    if (apmBg == 0 && *clrApm) apmBg = new YColor(clrApm);
    if (apmFg == 0) apmFg = new YColor(clrApmText);
    if (apmFont == 0) apmFont = YFont::getFont(apmFontName);

    apmTimer = new YTimer(2000);
    apmTimer->timerListener(this);
    apmTimer->start();
    autoSize();
 // setDND(true);
}

YApm::~YApm() {
    delete apmTimer; apmTimer = 0;
}

void YApm::updateToolTip() {
    char s[30]={' ',' ',' ', 0, 0, 0, 0};

    ApmStr(s,1);
    setToolTip(s);
}

void YApm::autoSize() {
    int maxWidth=54;

    if (!prettyClock) maxWidth += 4;
    setSize(maxWidth, 20);
}

void YApm::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    unsigned int x = 0;
    char s[30]={' ',' ',' ',0,0,0,0,0};
    int len,i;

    ApmStr(s,0); len=strlen(s);

    if (prettyClock) {
        YPixmap *p;

        for (i = 0; x < width(); i++) {
            if (i < len) {
                p = getPixmap(s[i]);
            } else p = PixSpace;

            if (p) {
                g.drawPixmap(p, x, 0);
                x += p->width();
            } else if (i >= len) {
                g.setColor(apmBg);
                g.fillRect(x, 0, width() - x, height());
                break;
            }
        }
    } else {
        int y = (height() - 1 - apmFont->height()) / 2 + apmFont->ascent();

        if (apmBg) {
	    g.setColor(apmBg);
	    g.fillRect(0, 0, width(), height());
	} else {
#ifdef CONFIG_GRADIENTS
	    class YPixbuf * gradient(parent()->getGradient());

	    if (gradient)
		g.copyPixbuf(*gradient, this->x(), this->y(),
					width(), height(), 0, 0);
	    else
#endif
	    if (taskbackPixmap)
		g.fillPixmap(taskbackPixmap, this->x(), this->y(),
					     width(), height());
	}

        g.setColor(apmFg);
        g.setFont(apmFont);
        g.drawChars(s, 0, len, 2, y);
    }
}

void YApm::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask))
        taskBar->contextMenu(up.x_root, up.y_root);
}

bool YApm::handleTimer(YTimer *t) {
    if (t != apmTimer) return false;

    if (toolTipVisible())
        updateToolTip();
    repaint();
    return true;
}

YPixmap *YApm::getPixmap(char c) {
    YPixmap *pix = 0;
    switch (c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': pix = PixNum[c - '0']; break;
    case ' ': pix = PixSpace; break;
    case ':': pix = PixColon; break;
    case '.': pix = PixDot; break;
    case 'P': pix = PixP; break;
    case 'M': pix = PixM; break;
    }

    return pix;
}

int YApm::calcWidth(const char *s, int count) {
    if (!prettyClock)  return apmFont->textWidth(s, count);
    else {
        int len = 0;

        while (count--) {
            YPixmap *p = getPixmap(*s++);
            if (p) len += p->width();
        }
        return len;
    }
}
#endif
