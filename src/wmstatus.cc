/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
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

#include "intl.h"

#include <stdio.h>
#include <string.h>

YColor *YWindowManagerStatus::statusFg = 0;
YColor *YWindowManagerStatus::statusBg = 0;

YFont *YWindowManagerStatus::statusFont = 0;

MoveSizeStatus *statusMoveSize = 0;
WorkspaceStatus *statusWorkspace = 0;

/******************************************************************************/
/******************************************************************************/

YWindowManagerStatus::YWindowManagerStatus(YWindow *aParent,
                                           const char *(*templFunc)())
  : YWindow(aParent) {
    if (statusBg == 0)
        statusBg = new YColor(clrMoveSizeStatus);
    if (statusFg == 0)
        statusFg = new YColor(clrMoveSizeStatusText);
    if (statusFont == 0)
        statusFont = YFont::getFont(statusFontName);

    int sW = statusFont->textWidth(templFunc());
    int sH = statusFont->height();
    
    setGeometry((manager->width() - sW) / 2,
                (manager->height() - sH) - 8, // / 2,
                sW + 2, sH + 4);
    setStyle(wsOverrideRedirect);
}

YWindowManagerStatus::~YWindowManagerStatus() {
}

void YWindowManagerStatus::paint(Graphics &g, int /*x*/, int /*y*/,
                                 unsigned int /*width*/,
                                 unsigned int /*height*/) {
    const char *status;
    
    g.setColor(statusBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);
    if (switchbackPixmap)
        g.fillPixmap(switchbackPixmap, 1, 1, width() - 3, height() - 3);
    else
        g.fillRect(1, 1, width() - 3, height() - 3);
    g.setColor(statusFg);
    g.setFont(statusFont);
    
    status = getStatus();
    g.drawChars(status, 0, strlen(status),
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

const char* MoveSizeStatus::getStatus() {
    static char status[50];
    snprintf(status, sizeof(status), "%dx%d+%d+%d", fW, fH, fX, fY);
    return status;
}

void MoveSizeStatus::begin(YFrameWindow *frame) {
    if (showMoveSizeStatus) {
        setStatus(frame);
        YWindowManagerStatus::begin();
    }
}

void MoveSizeStatus::setStatus(YFrameWindow *frame, int x, int y, int width, int height) {
    XSizeHints *sh = frame->client()->sizeHints();

    width -= frame->borderX() * 2;
    height -= frame->borderY() * 2 + frame->titleY();
    
    fX = x;//// + frame->borderX ();
    fY = y;//// + frame->borderY () + frame->titleY ();
    fW = (width - (sh ? sh->base_width : 0)) / (sh ? sh->width_inc : 1);
    fH = (height - (sh ? sh->base_height : 0)) / (sh ? sh->height_inc : 1);
    repaint ();
}

void MoveSizeStatus::setStatus(YFrameWindow *frame) {
    XSizeHints *sh = frame->client()->sizeHints();
    
    fX = frame->x ();//// + frame->borderX ();
    fY = frame->y ();//// + frame->borderY () + frame->titleY ();
    fW = (frame->client()->width() - (sh ? sh->base_width : 0)) / (sh ? sh->width_inc : 1);
    fH = (frame->client()->height() - (sh ? sh->base_height : 0)) / (sh ? sh->height_inc : 1);
    repaint ();
}

const char* MoveSizeStatus::templateFunction() {
    return "9999x9999+9999+9999";
}

/******************************************************************************/
/******************************************************************************/

class WorkspaceStatusTimeout:
public YTimer::Listener {
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
    timer->timerListener(new WorkspaceStatusTimeout());
}

WorkspaceStatus::~WorkspaceStatus() {
    delete timer;
}

const char* WorkspaceStatus::getStatus() {
    return getStatus(manager->workspaceName(workspace));
}

const char* WorkspaceStatus::getStatus(const char* name) {
    static char *namebuffer = NULL;
    if (namebuffer) delete [] namebuffer;
    namebuffer = strJoin(_("Workspace: "), name, NULL);
    return namebuffer;
}

void WorkspaceStatus::begin(long workspace) {
    setStatus(workspace);
    YWindowManagerStatus::begin();
}

void WorkspaceStatus::setStatus(long workspace) {
    this->workspace = workspace;
    repaint();
    
    if (timer->running())
        timer->stop();

    timer->start();
}

const char* WorkspaceStatus::templateFunction() {
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
