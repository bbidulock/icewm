#ifndef WMACTION_H_

class YAction;

extern YAction *actionCascade;
extern YAction *actionArrange;
extern YAction *actionTileVertical;
extern YAction *actionTileHorizontal;
extern YAction *actionUndoArrange;
extern YAction *actionArrangeIcons;
extern YAction *actionMinimizeAll;
extern YAction *actionHideAll;

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
extern YAction *actionDoNotCover;
extern YAction *actionFullscreen;

extern YAction *actionWindowList;
extern YAction *actionLogout;
extern YAction *actionCancelLogout;
extern YAction *actionRefresh;
extern YAction *actionAbout;
extern YAction *actionHelp;
extern YAction *actionLicense;
extern YAction *actionRun;
extern YAction *actionExit;

void initActions();

#endif
