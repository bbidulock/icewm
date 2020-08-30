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
#include "workspaces.h"
#include "intl.h"

#include <stdio.h>
#include <string.h>

YColorName YWindowManagerStatus::statusFg(&clrMoveSizeStatusText);
YColorName YWindowManagerStatus::statusBg(&clrMoveSizeStatus);

ref<YFont> YWindowManagerStatus::statusFont;

lazy<MoveSizeStatus> statusMoveSize;
lazy<WorkspaceStatus> statusWorkspace;

/******************************************************************************/
/******************************************************************************/

YWindowManagerStatus::YWindowManagerStatus()
    : YWindow()
    , fConfigured(false)
{
    setStyle(wsOverrideRedirect | wsSaveUnder | wsNoExpose);
    setTitle("IceStatus");
    setClassHint("status", "IceWM");
    setNetWindowType(_XA_NET_WM_WINDOW_TYPE_NOTIFICATION);
}

YWindowManagerStatus::~YWindowManagerStatus() {
    if (statusMoveSize == nullptr || statusWorkspace == nullptr)
        statusFont = null;
}

void YWindowManagerStatus::configure(const YRect2& r) {
    if (fConfigured == false || r.resized()) {
        configureStatus();
    }
}

void YWindowManagerStatus::configureStatus() {
    fConfigured = true;

    if (statusFont == null)
        statusFont = YFont::getFont(XFA(statusFontName));

    int sW = statusFont->textWidth(longestStatus());
    int sH = statusFont->height();

    int scn = desktop->getScreenForRect(x(), y(), width(), height());
    YRect geo(desktop->getScreenGeometry(scn));
    setGeometry(YRect(geo.x() + (geo.width() - sW) / 2,
                      geo.y() + (geo.height() - sH) - 8, // / 2,
                      sW + 2, sH + 4));
}

void YWindowManagerStatus::repaintSync() {
    if (fConfigured == false) {
        configureStatus();
    }
    GraphicsBuffer(this).paint();
}

void YWindowManagerStatus::paint(Graphics &g, const YRect &/*r*/) {
    g.setColor(statusBg);
    g.drawBorderW(0, 0, width() - 1, height() - 1, true);
    int x = 1, y = 1, w = int(width()) - 3, h = int(height()) - 3;
    if (0 < w && 0 < h) {
        if (switchbackPixbuf != null)
            g.drawGradient(switchbackPixbuf, x, y, w, h, 0, 0, w, h);
        else if (switchbackPixmap != null)
            g.fillPixmap(switchbackPixmap, x, y, w, h);
        else
            g.fillRect(x, y, w, h);
    }
    g.setColor(statusFg);
    g.setFont(statusFont);

    mstring status(getStatus());
    g.drawChars(status,
                width() / 2 - statusFont->textWidth(status) / 2,
                height() - statusFont->descent() - 2);
}

void YWindowManagerStatus::begin() {
    setPosition(x(),
                 taskBarAtTop ? 4 :
                 (desktop->height() - height()) - 4);
    raise();
    show();
}

/******************************************************************************/
/******************************************************************************/

MoveSizeStatus::MoveSizeStatus()
  : YWindowManagerStatus(),
        fX(0), fY(0), fW(0), fH(0) {
}

MoveSizeStatus::~MoveSizeStatus() {
}

void MoveSizeStatus::end() {
    super::end();
    statusMoveSize = null;
}

mstring MoveSizeStatus::longestStatus() {
   return "9999x9999+9999+9999";
}

mstring MoveSizeStatus::getStatus() {
    char status[50];
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
    if (t == &timer) {
        end();
    }
    return false;
};

WorkspaceStatus::WorkspaceStatus() :
    YWindowManagerStatus(),
    workspace(0),
    timer(workspaceStatusTime, this, false)
{
}

WorkspaceStatus::~WorkspaceStatus() {
}

void WorkspaceStatus::end() {
    super::end();
    statusWorkspace = null;
}

mstring WorkspaceStatus::getStatus() {
    return getStatus(workspaceNames[workspace]);
}

mstring WorkspaceStatus::getStatus(const char* name) {
    return mstring(_("Workspace: ")).append(name);
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

mstring WorkspaceStatus::longestStatus() {
    const char* longestWorkspaceName = nullptr;
    int maxWorkspaceNameLength = 0;

    for (int w = 0; w < workspaceCount; ++w) {
        const char* name = workspaceNames[w];
        int length = statusFont->textWidth(name);

        if (length > maxWorkspaceNameLength) {
            maxWorkspaceNameLength = length;
            longestWorkspaceName = name;
        }
    }

    return getStatus(longestWorkspaceName);
}

// vim: set sw=4 ts=4 et:
