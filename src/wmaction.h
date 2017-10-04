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

/*
 * Reserved IDs for most predefined actions.
 * The static IDs shall have odd number values which is a basic safety measure
 * to avoid clashes with raw pointers used as action ID.
 *
 * NOTE: please add notes for reserved areas if they are defined with static
 * values in other sources.
 */
enum EWmActions {
	actionInvalid = 0,
    actionCascade = 1,
    actionArrange = 3,
    actionTileVertical = 5,
    actionTileHorizontal = 7,
    actionUndoArrange = 9,
    actionArrangeIcons = 11,
    actionMinimizeAll = 13,
    actionHideAll = 15,
    actionShowDesktop = 17,

#ifndef CONFIG_PDA
    actionHide = 23,
#endif
    actionShow = 27,
    actionRaise = 29,
    actionLower = 31,
    actionDepth = 33,
    actionMove = 35,
    actionSize = 37,
    actionMaximize = 39,
    actionMaximizeVert = 41,
    actionMinimize = 43,
    actionRestore = 45,
    actionRollup = 47,
    actionClose = 49,
    actionKill = 51,
    actionOccupyAllOrCurrent = 53,
#if DO_NOT_COVER_OLD
    actionDoNotCover = 57,
#endif
    actionFullscreen = 61,
    actionToggleTray = 63,
    actionCollapseTaskbar = 65,

    actionWindowList = 69,
    actionLogout = 71,
    actionCancelLogout = 73,
    actionLock = 75,
    actionReboot = 77,
    actionRestart = 79,
    actionRestartXterm = 81,
    actionShutdown = 83,
    actionSuspend = 85,
    actionRefresh = 87,
    actionAbout = 89,
    actionHelp = 91,
    actionLicense = 93,
    actionRun = 95,
    actionExit = 97,
    actionFocusClickToFocus = 99,
    actionFocusExplicit = 101,
    actionFocusMouseSloppy = 103,
    actionFocusMouseStrict = 105,
    actionFocusQuietSloppy = 107,
    actionFocusCustom = 109

    /* NOTE:
     * 201-233 reserved for yinput.*
     */

    /*
     * NOTE: dynamically assigned values start from 511, see misc.cc for details.
     */
};

bool canShutdown(bool reboot);
bool canLock();
/**
 * Basic check whether a shell command could possibly be run.
 */
bool couldRunCommand(const char *cmd);

#endif

// vim: set sw=4 ts=4 et:
