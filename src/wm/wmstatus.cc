/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * Status display for resize/move
 */
#include "config.h"

#ifdef CONFIG_MOVESIZE_STATUS
#include "yxutil.h"
#include "wmstatus.h"

#include "wmframe.h"
#include "wmclient.h"
#include "wmmgr.h"
#include "yconfig.h"
#include "deffonts.h"
#include "yrect.h"
#include "ycstring.h"
#include "ypaint.h"

#include <stdio.h>
#include <string.h>

YFontPrefProperty MoveSizeStatus::gStatusFont("icewm", "StatusFont", BOLDTTFONT(120));
YColorPrefProperty MoveSizeStatus::gStatusBg("icewm", "ColorMoveSizeStatus", "rgb:C0/C0/C0");
YColorPrefProperty MoveSizeStatus::gStatusFg("icewm", "ColorMoveSizeStatusText", "rgb:00/00/00");

YBoolPrefProperty MoveSizeStatus::gShowMoveSizeStatus("icewm", "ShowMoveSizeStatus", true);

//MoveSizeStatus *statusMoveSize = 0;

MoveSizeStatus::MoveSizeStatus(YWindowManager *root, YWindow *aParent): YWindow(aParent) {
    fRoot = root;
    YFont *font = gStatusFont.getFont();
    int sW = font->__textWidth("9999x9999+9999+9999");
    int sH = font->height();
    
    setGeometry((fRoot->width() - sW) / 2,
                (fRoot->height() - sH) - 8, // / 2,
                sW + 2, sH + 4);
    setStyle(wsOverrideRedirect);
}

MoveSizeStatus::~MoveSizeStatus() {
}

void MoveSizeStatus::paint(Graphics &g, const YRect &/*er*/) {
    CStr *str;
    //char str[50];

    //printf("%d %d %d %d\n", fW, fH, fX, fY);
    str = CStr::format("%dx%d+%d+%d", fW, fH, fX, fY);

    g.setColor(gStatusBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);
#if 0
    //if (switchbackPixmap)
    //    g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    //else
#endif
    g.fillRect(1, 1, width() - 3, height() - 3);
    g.setColor(gStatusFg);
    g.setFont(gStatusFont);

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
    
    fX = x; //// + frame->borderX();
    fY = y; //// + frame->borderY() + frame->titleY();
    fW = (width - (sh ? sh->base_width : 0)) / (sh ? sh->width_inc : 1);
    fH = (height - (sh ? sh->base_height : 0)) / (sh ? sh->height_inc : 1);
    repaint();
}

void MoveSizeStatus::setStatus(YFrameWindow *frame) {
    XSizeHints *sh = frame->client()->sizeHints();
    
    fX = frame->x(); //// + frame->borderX();
    fY = frame->y(); //// + frame->borderY() + frame->titleY();
    fW = (frame->client()->width() - (sh ? sh->base_width : 0)) / (sh ? sh->width_inc : 1);
    fH = (frame->client()->height() - (sh ? sh->base_height : 0)) / (sh ? sh->height_inc : 1);
    repaint();
}
#endif
