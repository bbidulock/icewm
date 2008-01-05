#ifndef WMACTION_H_

class YAction;

#define ICEWM_ACTION_NOP 0
//#define ICEWM_ACTION_PING 1
#define ICEWM_ACTION_LOGOUT 2
#define ICEWM_ACTION_CANCEL_LOGOUT 3
#define ICEWM_ACTION_REBOOT 4
#define ICEWM_ACTION_SHUTDOWN 5
#define ICEWM_ACTION_ABOUT 6
#define ICEWM_ACTION_WINDOWLIST 7

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
extern YAction *actionRefresh;
extern YAction *actionAbout;
extern YAction *actionHelp;
extern YAction *actionLicense;
extern YAction *actionRun;
extern YAction *actionExit;
extern YAction *actionFocusClickToFocus;
extern YAction *actionFocusMouseSloppy;
extern YAction *actionFocusCustom;

void initActions();

bool canShutdown(bool reboot);
bool canLock();

#endif
