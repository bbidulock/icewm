#include "config.h"
#include "wmaction.h"
#include "yaction.h"

YAction *actionHide = 0;
YAction *actionShow = 0;
YAction *actionRaise = 0;
YAction *actionLower = 0;
YAction *actionDepth = 0;
YAction *actionMove = 0;
YAction *actionSize = 0;
YAction *actionMaximize = 0;
YAction *actionMaximizeVert = 0;
YAction *actionMinimize = 0;
YAction *actionRestore = 0;
YAction *actionRollup = 0;
YAction *actionClose = 0;
YAction *actionKill = 0;
YAction *actionOccupyAllOrCurrent = 0;

YAction *actionExit = 0;

void initActions() {
#if 0
    actionCascade = new YAction();
    actionArrange = new YAction();
    actionTileVertical = new YAction();
    actionTileHorizontal = new YAction();
    actionUndoArrange = new YAction();
    actionArrangeIcons = new YAction();
    actionMinimizeAll = new YAction();
    actionHideAll = new YAction();
    actionLogout = new YAction();
    actionCancelLogout = new YAction();
    actionWindowList = new YAction();
    actionRefresh = new YAction();
    actionAbout = new YAction();
    actionHelp = new YAction();
    actionLicense = new YAction();
    actionRun = new YAction();
#endif

    actionHide = new YAction();
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
    actionExit = new YAction();
}
