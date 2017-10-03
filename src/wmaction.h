#ifndef __WMACTION_H
#define __WMACTION_H

#define ICEWM_ACTION_NOP 0
//#define ICEWM_ACTION_PING 1
#define ICEWM_ACTION_LOGOUT 2
#define ICEWM_ACTION_CANCEL_LOGOUT 3
#define ICEWM_ACTION_REBOOT 4
#define ICEWM_ACTION_SHUTDOWN 5
#define ICEWM_ACTION_ABOUT 6
#define ICEWM_ACTION_WINDOWLIST 7
#define ICEWM_ACTION_RESTARTWM 8
#define ICEWM_ACTION_SUSPEND 9

enum eWmActions {
    actionCascade,
    actionArrange,
    actionTileVertical,
    actionTileHorizontal,
    actionUndoArrange,
    actionArrangeIcons,
    actionMinimizeAll,
    actionHideAll,
    actionShowDesktop,

#ifndef CONFIG_PDA
    actionHide,
#endif
    actionShow,
    actionRaise,
    actionLower,
    actionDepth,
    actionMove,
    actionSize,
    actionMaximize,
    actionMaximizeVert,
    actionMinimize,
    actionRestore,
    actionRollup,
    actionClose,
    actionKill,
    actionOccupyAllOrCurrent,
#if DO_NOT_COVER_OLD
    actionDoNotCover,
#endif
    actionFullscreen,
    actionToggleTray,
    actionCollapseTaskbar,

    actionWindowList,
    actionLogout,
    actionCancelLogout,
    actionLock,
    actionReboot,
    actionRestart,
    actionRestartXterm,
    actionShutdown,
    actionSuspend,
    actionRefresh,
    actionAbout,
    actionHelp,
    actionLicense,
    actionRun,
    actionExit,
    actionFocusClickToFocus,
    actionFocusExplicit,
    actionFocusMouseSloppy,
    actionFocusMouseStrict,
    actionFocusQuietSloppy,
    actionFocusCustom
};

bool canShutdown(bool reboot);
bool canLock();
/**
 * Basic check whether a shell command could possibly be run.
 */
bool couldRunCommand(const char *cmd);

#endif
