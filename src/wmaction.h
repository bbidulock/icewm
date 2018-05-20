#ifndef __WMACTION_H
#define __WMACTION_H

class YAction;

enum WMAction {
    ICEWM_ACTION_LOGOUT = 2,
    ICEWM_ACTION_CANCEL_LOGOUT = 3,
    ICEWM_ACTION_REBOOT = 4,
    ICEWM_ACTION_SHUTDOWN = 5,
    ICEWM_ACTION_ABOUT = 6,
    ICEWM_ACTION_WINDOWLIST = 7,
    ICEWM_ACTION_RESTARTWM = 8,
    ICEWM_ACTION_SUSPEND = 9,
};

enum RebootShutdown {
    Logout = 0,
    Reboot = 1,
    Shutdown = 2,
};

enum EAction {
    actionNull               = 0,

    actionCascade            = 101,
    actionArrange            = 103,
    actionTileVertical       = 105,
    actionTileHorizontal     = 107,
    actionUndoArrange        = 109,
    actionArrangeIcons       = 111,
    actionMinimizeAll        = 113,
    actionHideAll            = 115,
    actionShowDesktop        = 117,

    actionHide               = 119,
    actionShow               = 121,
    actionRaise              = 123,
    actionLower              = 125,
    actionDepth              = 127,
    actionMove               = 129,
    actionSize               = 131,
    actionMaximize           = 133,
    actionMaximizeVert       = 135,
    actionMinimize           = 137,
    actionRestore            = 139,
    actionRollup             = 141,
    actionClose              = 143,
    actionKill               = 145,
    actionOccupyAllOrCurrent = 147,
#if DO_NOT_COVER_OLD
    actionDoNotCover         = 149,
#endif
    actionFullscreen         = 151,
    actionToggleTray         = 153,
    actionCollapseTaskbar    = 155,

    actionWindowList         = 157,
    actionLogout             = 159,
    actionCancelLogout       = 161,
    actionLock               = 163,
    actionReboot             = 165,
    actionRestart            = 167,
    actionRestartXterm       = 169,
    actionShutdown           = 171,
    actionSuspend            = 173,
    actionRefresh            = 175,
    actionAbout              = 177,
    actionRun                = 179,
    actionExit               = 181,

    actionFocusClickToFocus  = 183,
    actionFocusExplicit      = 185,
    actionFocusMouseSloppy   = 187,
    actionFocusMouseStrict   = 189,
    actionFocusQuietSloppy   = 191,
    actionFocusCustom        = 193,
};

bool canShutdown(RebootShutdown reboot);
bool canLock();
/**
 * Basic check whether a shell command could possibly be run.
 */
bool couldRunCommand(const char *cmd);

#endif

// vim: set sw=4 ts=4 et:
