/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * Status display for resize/move
 */
#include "config.h"

#ifndef LITE
#include "yfull.h"
#include "wmstatus.h"
//#include "wmswitch.h" // !!! remove (for bg pixmap)

#include "wmframe.h"
#include "wmclient.h"
#include "wmmgr.h"
#include "yconfig.h"
#include "prefs.h"
#include "yrect.h"
#include "ycstring.h"

#include <stdio.h>
#include <string.h>

YColor *MoveSizeStatus::statusFg = 0;
YColor *MoveSizeStatus::statusBg = 0;

YFont *MoveSizeStatus::statusFont = 0;

YBoolPrefProperty MoveSizeStatus::gShowMoveSizeStatus("icewm", "ShowMoveSizeStatus", true);

//MoveSizeStatus *statusMoveSize = 0;

MoveSizeStatus::MoveSizeStatus(YWindowManager *root, YWindow *aParent): YWindow(aParent) {
    if (statusBg == 0) {
        YPref prefColorStatus("icewm", "ColorMoveSizeStatus");
        const char *pvColorStatus = prefColorStatus.getStr("rgb:C0/C0/C0");
        statusBg = new YColor(pvColorStatus);
    }
    if (statusFg == 0) {
        YPref prefColorStatusText("icewm", "ColorMoveSizeStatusText");
        const char *pvColorStatusText = prefColorStatusText.getStr("rgb:00/00/00");
        statusFg = new YColor(pvColorStatusText);
    }
    if (statusFont == 0) {
        YPref prefFontStatus("icewm", "StatusFont");
        const char *pvFontStatus = prefFontStatus.getStr(BOLDTTFONT(120));
        statusFont = YFont::getFont(pvFontStatus);
    }

    fRoot = root;
    int sW = statusFont->textWidth("9999x9999+9999+9999");
    int sH = statusFont->height();
    
    setGeometry((fRoot->width() - sW) / 2,
                (fRoot->height() - sH) - 8, // / 2,
                sW + 2, sH + 4);
    setStyle(wsOverrideRedirect);
}

MoveSizeStatus::~MoveSizeStatus() {
}

void MoveSizeStatus::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    CStr *str;
    //char str[50];

    printf("%d %d %d %d\n", fW, fH, fX, fY);
    str = CStr::format("%dx%d+%d+%d", fW, fH, fX, fY);

    g.setColor(statusBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);
#if 0
    //if (switchbackPixmap)
    //    g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    //else
#endif
    g.fillRect(1, 1, width() - 3, height() - 3);
    g.setColor(statusFg);
    g.setFont(statusFont);

    g.drawText(YRect(0, 0, width(), height()),
               str, DrawText_VCenter | DrawText_HCenter);

    delete str;
}

void MoveSizeStatus::begin(YFrameWindow *frame) {
    if (gShowMoveSizeStatus.getBool()) {
        setPosition(x(),
#ifdef CONFIG_TASKBAR
                    taskBarAtTop ? 4 :
#endif
                    (fRoot->height() - height()) - 4);
        raise();
        show();
    }
    setStatus(frame);
}

void MoveSizeStatus::setStatus(YFrameWindow *frame, int x, int y, int width, int height) {
    XSizeHints *sh = frame->client()->sizeHints();

    width -= frame->borderLeft() + frame->borderRight();
    height -= frame->borderTop() + frame->borderBottom() + frame->titleY();
    
    fX = x;//// + frame->borderX();
    fY = y;//// + frame->borderY() + frame->titleY();
    fW = (width - (sh ? sh->base_width : 0)) / (sh ? sh->width_inc : 1);
    fH = (height - (sh ? sh->base_height : 0)) / (sh ? sh->height_inc : 1);
    repaint();
}

void MoveSizeStatus::setStatus(YFrameWindow *frame) {
    XSizeHints *sh = frame->client()->sizeHints();
    
    fX = frame->x();//// + frame->borderX();
    fY = frame->y();//// + frame->borderY() + frame->titleY();
    fW = (frame->client()->width() - (sh ? sh->base_width : 0)) / (sh ? sh->width_inc : 1);
    fH = (frame->client()->height() - (sh ? sh->base_height : 0)) / (sh ? sh->height_inc : 1);
    repaint();
}
#endif
