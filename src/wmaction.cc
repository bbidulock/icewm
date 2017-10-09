#include "config.h"
#include "wmaction.h"
#include "yaction.h"

YAction actionCascade;
YAction actionArrange;
YAction actionTileVertical;
YAction actionTileHorizontal;
YAction actionUndoArrange;
YAction actionArrangeIcons;
YAction actionMinimizeAll;
YAction actionHideAll;
YAction actionShowDesktop;

#ifndef CONFIG_PDA
YAction actionHide;
#endif
YAction actionShow;
YAction actionRaise;
YAction actionLower;
YAction actionDepth;
YAction actionMove;
YAction actionSize;
YAction actionMaximize;
YAction actionMaximizeVert;
YAction actionMinimize;
YAction actionRestore;
YAction actionRollup;
YAction actionClose;
YAction actionKill;
YAction actionOccupyAllOrCurrent;
#if DO_NOT_COVER_OLD
YAction actionDoNotCover;
#endif
YAction actionFullscreen;
YAction actionToggleTray;

YAction actionWindowList;
YAction actionLogout;
YAction actionCancelLogout;
YAction actionLock;
YAction actionReboot;
YAction actionRestart;
YAction actionRestartXterm;
YAction actionShutdown;
YAction actionSuspend;
YAction actionRefresh;
YAction actionCollapseTaskbar;
#ifndef LITE
YAction actionAbout;
#endif
YAction actionRun;
YAction actionExit;

YAction actionFocusClickToFocus;
YAction actionFocusExplicit;
YAction actionFocusMouseSloppy;
YAction actionFocusMouseStrict;
YAction actionFocusQuietSloppy;
YAction actionFocusCustom;

// vim: set sw=4 ts=4 et:
