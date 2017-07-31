#include "config.h"
#include "wmaction.h"
#include "yaction.h"

YAction *actionCascade(0);
YAction *actionArrange(0);
YAction *actionTileVertical(0);
YAction *actionTileHorizontal(0);
YAction *actionUndoArrange(0);
YAction *actionArrangeIcons(0);
YAction *actionMinimizeAll(0);
YAction *actionHideAll(0);
YAction *actionShowDesktop = 0;

#ifndef CONFIG_PDA
YAction *actionHide(0);
#endif
YAction *actionShow(0);
YAction *actionRaise(0);
YAction *actionLower(0);
YAction *actionDepth(0);
YAction *actionMove(0);
YAction *actionSize(0);
YAction *actionMaximize(0);
YAction *actionMaximizeVert(0);
YAction *actionMinimize(0);
YAction *actionRestore(0);
YAction *actionRollup(0);
YAction *actionClose(0);
YAction *actionKill(0);
YAction *actionOccupyAllOrCurrent(0);
#if DO_NOT_COVER_OLD
YAction *actionDoNotCover(0);
#endif
YAction *actionFullscreen(0);
YAction *actionToggleTray(0);

YAction *actionWindowList(0);
YAction *actionLogout(0);
YAction *actionCancelLogout(0);
YAction *actionLock(0);
YAction *actionReboot(0);
YAction *actionRestart(0);
YAction *actionShutdown(0);
YAction *actionRefresh(0);
YAction *actionCollapseTaskbar(0);
#ifndef LITE
YAction *actionAbout(0);
YAction *actionHelp(0);
YAction *actionLicense(0);
#endif
YAction *actionRun(0);
YAction *actionExit(0);

YAction *actionFocusClickToFocus(0);
YAction *actionFocusExplicit(0);
YAction *actionFocusMouseSloppy(0);
YAction *actionFocusMouseStrict(0);
YAction *actionFocusQuietSloppy(0);
YAction *actionFocusCustom(0);

void initActions() {
    actionCascade = new YAction();
    actionArrange = new YAction();
    actionTileVertical = new YAction();
    actionTileHorizontal = new YAction();
    actionUndoArrange = new YAction();
    actionArrangeIcons = new YAction();
    actionMinimizeAll = new YAction();
    actionHideAll = new YAction();
    actionShowDesktop = new YAction();
#ifndef CONFIG_PDA
    actionHide = new YAction();
#endif
    actionShow = new YAction();
    actionRaise = new YAction();
    actionLower = new YAction();
    actionDepth = new YAction();
    actionMove = new YAction();
    actionSize = new YAction();
    actionMaximize = new YAction();
    actionMaximizeVert = new YAction();
    actionMinimize = new YAction();
    actionRestore = new YAction();
    actionRollup = new YAction();
    actionClose = new YAction();
    actionKill = new YAction();
    actionOccupyAllOrCurrent = new YAction();
#if DO_NOT_COVER_OLD
    actionDoNotCover = new YAction();
#endif
    actionFullscreen = new YAction();
    actionToggleTray = new YAction();
    actionWindowList = new YAction();
    actionLogout = new YAction();
    actionCancelLogout = new YAction();
    actionLock = new YAction();
    actionReboot = new YAction();
    actionRestart = new YAction();
    actionShutdown = new YAction();
    actionRefresh = new YAction();
    actionCollapseTaskbar = new YAction();
#ifndef LITE
    actionAbout = new YAction();
    actionHelp = new YAction();
    actionLicense = new YAction();
#endif
    actionRun = new YAction();
    actionExit = new YAction();

    actionFocusClickToFocus = new YAction();
    actionFocusExplicit = new YAction();
    actionFocusMouseSloppy = new YAction();
    actionFocusMouseStrict = new YAction();
    actionFocusQuietSloppy = new YAction();
    actionFocusCustom = new YAction();
}

void freeActions() {
    delete actionCascade; actionCascade = 0;
    delete actionArrange; actionArrange = 0;
    delete actionTileVertical; actionTileVertical = 0;
    delete actionTileHorizontal; actionTileHorizontal = 0;
    delete actionUndoArrange; actionUndoArrange = 0;
    delete actionArrangeIcons; actionArrangeIcons = 0;
    delete actionMinimizeAll; actionMinimizeAll = 0;
    delete actionHideAll; actionHideAll = 0;
    delete actionShowDesktop; actionShowDesktop = 0;
#ifndef CONFIG_PDA
    delete actionHide; actionHide = 0;
#endif
    delete actionShow; actionShow = 0;
    delete actionRaise; actionRaise = 0;
    delete actionLower; actionLower = 0;
    delete actionDepth; actionDepth = 0;
    delete actionMove; actionMove = 0;
    delete actionSize; actionSize = 0;
    delete actionMaximize; actionMaximize = 0;
    delete actionMaximizeVert; actionMaximizeVert = 0;
    delete actionMinimize; actionMinimize = 0;
    delete actionRestore; actionRestore = 0;
    delete actionRollup; actionRollup = 0;
    delete actionClose; actionClose = 0;
    delete actionKill; actionKill = 0;
    delete actionOccupyAllOrCurrent; actionOccupyAllOrCurrent = 0;
#if DO_NOT_COVER_OLD
    delete actionDoNotCover; actionDoNotCover = 0;
#endif
    delete actionFullscreen; actionFullscreen = 0;
    delete actionToggleTray; actionToggleTray = 0;
    delete actionWindowList; actionWindowList = 0;
    delete actionLogout; actionLogout = 0;
    delete actionCancelLogout; actionCancelLogout = 0;
    delete actionLock; actionLock = 0;
    delete actionReboot; actionReboot = 0;
    delete actionRestart; actionRestart = 0;
    delete actionShutdown; actionShutdown = 0;
    delete actionRefresh; actionRefresh = 0;
    delete actionCollapseTaskbar; actionCollapseTaskbar = 0;
#ifndef LITE
    delete actionAbout; actionAbout = 0;
    delete actionHelp; actionHelp = 0;
    delete actionLicense; actionLicense = 0;
#endif
    delete actionRun; actionRun = 0;
    delete actionExit; actionExit = 0;

    delete actionFocusClickToFocus; actionFocusClickToFocus = 0;
    delete actionFocusExplicit; actionFocusExplicit = 0;
    delete actionFocusMouseSloppy; actionFocusMouseSloppy = 0;
    delete actionFocusMouseStrict; actionFocusMouseStrict = 0;
    delete actionFocusQuietSloppy; actionFocusQuietSloppy = 0;
    delete actionFocusCustom; actionFocusCustom = 0;
}
