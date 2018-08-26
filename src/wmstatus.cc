/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 *
 * Status display for resize/move
 */
#include "config.h"

#include "yfull.h"
#include "wmstatus.h"
#include "wpixmaps.h"
#include "wmframe.h"
#include "wmclient.h"
#include "wmmgr.h"
#include "prefs.h"
#include "yrect.h"

#include "intl.h"

#include <stdio.h>
#include <string.h>

YColorName YWindowManagerStatus::statusFg(&clrMoveSizeStatusText);
YColorName YWindowManagerStatus::statusBg(&clrMoveSizeStatus);

ref<YFont> YWindowManagerStatus::statusFont;

MoveSizeStatus *statusMoveSize = 0;
WorkspaceStatus *statusWorkspace = 0;

/******************************************************************************/
/******************************************************************************/

YWindowManagerStatus::YWindowManagerStatus(YWindow *aParent,
                const ustring &sampleString)
    : YWindow(aParent)
{
    if (statusFont == null)
        statusFont = YFont::getFont(XFA(statusFontName));

    int sW = statusFont->textWidth(sampleString);
    int sH = statusFont->height();

    setGeometry(YRect((manager->width() - sW) / 2,
                      (manager->height() - sH) - 8, // / 2,
                      sW + 2, sH + 4));
    setStyle(wsOverrideRedirect | wsSaveUnder);
    setTitle("IceStatus");
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
                 taskBarAtTop ? 4 :
                 (manager->height() - height()) - 4);
    raise();
    show();
}

/******************************************************************************/
/******************************************************************************/

#define statusTemplate "9999x9999+9999+9999"

MoveSizeStatus::MoveSizeStatus(YWindow *aParent)
  : YWindowManagerStatus(aParent, mstring(statusTemplate, sizeof(statusTemplate)-1)),
        fX(0), fY(0), fW(0), fH(0) {
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
    if (sh && (sh->flags & PResizeInc)) {
        fW = (width - sh->base_width) / non_zero(sh->width_inc);
        fH = (height - sh->base_height) / non_zero(sh->height_inc);
    } else {
        fW = width;
        fH = height;
    }
    repaintSync();
}

void MoveSizeStatus::setStatus(YFrameWindow *frame) {
    XSizeHints *sh = frame->client()->sizeHints();

    fX = frame->x ();//// + frame->borderX ();
    fY = frame->y ();//// + frame->borderY () + frame->titleY ();
    if (sh && (sh->flags & PResizeInc)) {
        fW = (frame->client()->width() - sh->base_width) / non_zero(sh->width_inc);
        fH = (frame->client()->height() - sh->base_height) / non_zero(sh->height_inc);
    } else {
        fW = frame->client()->width();
        fH = frame->client()->height();
    }
    repaintSync();
}

/******************************************************************************/
/******************************************************************************/

bool WorkspaceStatus::handleTimer(YTimer *t) {
    statusWorkspace->end();
    return false;
};

WorkspaceStatus::WorkspaceStatus(YWindow *aParent, ustring templateString) :
    YWindowManagerStatus(aParent, templateString), workspace(0),
    timer(workspaceStatusTime, this, false)
{
// !!! read timeout from preferences
}

WorkspaceStatus::~WorkspaceStatus() {
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

    timer.stopTimer();
    timer.startTimer();
}

WorkspaceStatus * WorkspaceStatus::createInstance(YWindow *aParent) {
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

    return new WorkspaceStatus(aParent, getStatus(longestWorkspaceName));
}

// vim: set sw=4 ts=4 et:
