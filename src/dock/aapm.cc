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
#include "ylib.h"
#include "yresource.h"

#include "aapm.h"

#include "yapp.h"
#include "prefs.h"
#include "sysdep.h"

YColor *YApm::apmBg = 0;
YColor *YApm::apmFg = 0;
YFont *YApm::apmFont = 0;

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
            fprintf(stderr, "/proc/apm format error (%d)\n", i);
        }
        return ;
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

    if (ACstatus == 0x1)
        if (Tool)
            strcat(s," - Power");
        else
            strcat(s,"P");
    if ((BATflag & 8))
        if (Tool)
            strcat(s," - Charging");
        else
            strcat(s,"M");
}

YApm::YApm(YWindow *aParent): YWindow(aParent), apmTimer(this, 2000) {
    if (apmBg == 0) apmBg = new YColor(clrApm);
    if (apmFg == 0) apmFg = new YColor(clrApmText);
    if (apmFont == 0) apmFont = YFont::getFont(apmFontName);

    // !!! combine this with Clock (make LEDPainter class or sth)
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

    /*apmTimer = new YTimer(2000);
    apmTimer->setTimerListener(this);*/
    apmTimer.startTimer();
    autoSize();
 // setDND(true);
}

YApm::~YApm() {
    //delete apmTimer; apmTimer = 0;
    delete PixSpace;
    delete PixSlash;
    delete PixDot;
    delete PixA;
    delete PixP;
    delete PixM;
    delete PixColon;
    for (int n = 0; n < 10; n++) delete PixNum[n];
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
    char s[8]={' ',' ',' ',0,0,0,0,0};
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
	
	g.setColor(apmBg);
	g.fillRect(0, 0, width(), 21);
	g.setColor(apmFg);
	g.setFont(apmFont);
	g.drawChars(s, 0, len, 2, y);
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
