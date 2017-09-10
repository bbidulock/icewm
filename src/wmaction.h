#ifndef __WMACTION_H
#define __WMACTION_H

class YAction;

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

extern YAction *actionCascade;
extern YAction *actionArrange;
extern YAction *actionTileVertical;
extern YAction *actionTileHorizontal;
extern YAction *actionUndoArrange;
extern YAction *actionArrangeIcons;
extern YAction *actionMinimizeAll;
extern YAction *actionHideAll;
extern YAction *actionShowDesktop;

#ifndef CONFIG_PDA
extern YAction *actionHide;
#endif
extern YAction *actionShow;
extern YAction *actionRaise;
extern YAction *actionLower;
extern YAction *actionDepth;
extern YAction *actionMove;
extern YAction *actionSize;
extern YAction *actionMaximize;
extern YAction *actionMaximizeVert;
extern YAction *actionMinimize;
extern YAction *actionRestore;
extern YAction *actionRollup;
extern YAction *actionClose;
extern YAction *actionKill;
extern YAction *actionOccupyAllOrCurrent;
#if DO_NOT_COVER_OLD
extern YAction *actionDoNotCover;
#endif
extern YAction *actionFullscreen;
extern YAction *actionToggleTray;
extern YAction *actionCollapseTaskbar;

extern YAction *actionWindowList;
extern YAction *actionLogout;
extern YAction *actionCancelLogout;
extern YAction *actionLock;
extern YAction *actionReboot;
extern YAction *actionRestart;
extern YAction *actionShutdown;
extern YAction *actionSuspend;
extern YAction *actionRefresh;
extern YAction *actionAbout;
extern YAction *actionHelp;
extern YAction *actionLicense;
extern YAction *actionRun;
extern YAction *actionExit;
extern YAction *actionFocusClickToFocus;
extern YAction *actionFocusExplicit;
extern YAction *actionFocusMouseSloppy;
extern YAction *actionFocusMouseStrict;
extern YAction *actionFocusQuietSloppy;
extern YAction *actionFocusCustom;

void initActions();
void freeActions();

bool canShutdown(bool reboot);
bool canLock();
/**
 * Basic check whether a shell command could possibly be run.
 */
bool couldRunCommand(const char *cmd);

#endif
