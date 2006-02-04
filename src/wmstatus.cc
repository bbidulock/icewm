/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 *
 * Status display for resize/move
 */
#include "config.h"

#ifndef LITE
#include "yfull.h"
#include "wmstatus.h"
#include "wmswitch.h" // !!! remove (for bg pixmap)

#include "wmframe.h"
#include "wmclient.h"
#include "wmmgr.h"
#include "prefs.h"
#include "yrect.h"

#include "intl.h"

#include <stdio.h>
#include <string.h>

YColor *YWindowManagerStatus::statusFg = 0;
YColor *YWindowManagerStatus::statusBg = 0;

ref<YFont> YWindowManagerStatus::statusFont;

MoveSizeStatus *statusMoveSize = 0;
WorkspaceStatus *statusWorkspace = 0;

/******************************************************************************/
/******************************************************************************/

YWindowManagerStatus::YWindowManagerStatus(YWindow *aParent,
                                           ustring (*templFunc)())
    : YWindow(aParent)
{
    if (statusBg == 0)
        statusBg = new YColor(clrMoveSizeStatus);
    if (statusFg == 0)
        statusFg = new YColor(clrMoveSizeStatusText);
    if (statusFont == null)
        statusFont = YFont::getFont(XFA(statusFontName));

    int sW = statusFont->textWidth(templFunc());
    int sH = statusFont->height();
    
    setGeometry(YRect((manager->width() - sW) / 2,
                      (manager->height() - sH) - 8, // / 2,
                      sW + 2, sH + 4));
    setStyle(wsOverrideRedirect);
}

YWindowManagerStatus::~YWindowManagerStatus() {
}

void YWindowManagerStatus::paint(Graphics &g, const YRect &/*r*/) {
    ustring status(null);
    
    g.setColor(statusBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);
    if (switchbackPixmap != null)
        g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    else
        g.fillRect(1, 1, width() - 3, height() - 3);
    g.setColor(statusFg);
    g.setFont(statusFont);
    
    status = getStatus();
    g.drawChars(status,
                width() / 2 - statusFont->textWidth(status) / 2,
                height() - statusFont->descent() - 2);
}

void YWindowManagerStatus::begin() {
    setPosition(x(),
#ifdef CONFIG_TASKBAR
                 taskBarAtTop ? 4 :
#endif
                 (manager->height() - height()) - 4);
    raise();
    show();
}

/******************************************************************************/
/******************************************************************************/

MoveSizeStatus::MoveSizeStatus(YWindow *aParent)
  : YWindowManagerStatus(aParent, templateFunction) {
}

MoveSizeStatus::~MoveSizeStatus() {
}

ustring MoveSizeStatus::getStatus() {
    static char status[50];
    snprintf(status, 50, "%dx%d%+d%+d", fW, fH, fX, fY);
    return status;
}

void MoveSizeStatus::begin(YFrameWindow *frame) {
    if (showMoveSizeStatus) {
        setStatus(frame);
        YWindowManagerStatus::begin();
    }
}

void MoveSizeStatus::setStatus(YFrameWindow *frame, const YRect &r) {
    XSizeHints *sh = frame->client()->sizeHints();

    int width = r.width() - frame->borderX() * 2;
    int height = r.height() - frame->borderY() * 2 - frame->titleY();
    
    fX = r.x();
    fY = r.y();
    fW = (width - (sh ? sh->base_width : 0)) / (sh ? sh->width_inc : 1);
    fH = (height - (sh ? sh->base_height : 0)) / (sh ? sh->height_inc : 1);
    repaintSync();
}

void MoveSizeStatus::setStatus(YFrameWindow *frame) {
    XSizeHints *sh = frame->client()->sizeHints();
    
    fX = frame->x ();//// + frame->borderX ();
    fY = frame->y ();//// + frame->borderY () + frame->titleY ();
    fW = (frame->client()->width() - (sh ? sh->base_width : 0)) / (sh ? sh->width_inc : 1);
    fH = (frame->client()->height() - (sh ? sh->base_height : 0)) / (sh ? sh->height_inc : 1);
    repaintSync();
}

ustring MoveSizeStatus::templateFunction() {
    return "9999x9999+9999+9999";
}

/******************************************************************************/
/******************************************************************************/

class WorkspaceStatus::Timeout: public YTimerListener {
public:
    virtual bool handleTimer(YTimer */*timer*/) {
        statusWorkspace->end();
        return false;
    }
};

/******************************************************************************/

WorkspaceStatus::WorkspaceStatus(YWindow *aParent)
  : YWindowManagerStatus(aParent, templateFunction) {
// !!! read timeout from preferences
    timer = new YTimer(workspaceStatusTime);
    timer->setTimerListener(timeout = new Timeout());
}

WorkspaceStatus::~WorkspaceStatus() {
    delete timer;
    delete timeout;
}

ustring WorkspaceStatus::getStatus() {
    return getStatus(manager->workspaceName(workspace));
}

ustring WorkspaceStatus::getStatus(const char* name) {
    return ustring(_("Workspace: ")).append(name);
}

void WorkspaceStatus::begin(long workspace) {
    setStatus(workspace);
    YWindowManagerStatus::begin();
}

void WorkspaceStatus::setStatus(long workspace) {
    this->workspace = workspace;
    repaintSync();
    
    if (timer->isRunning())
        timer->stopTimer();

    timer->startTimer();
}

ustring WorkspaceStatus::templateFunction() {
    const char* longestWorkspaceName = NULL;
    int maxWorkspaceNameLength = 0;

    for (long w = 0; w < manager->workspaceCount(); ++w) {
        const char* name = manager->workspaceName(w);
        int length = statusFont->textWidth(name);

        if (length > maxWorkspaceNameLength) {
            maxWorkspaceNameLength = length;
            longestWorkspaceName = name;
        }
    }

    return getStatus(longestWorkspaceName);
}

#endif
