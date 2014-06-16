#ifndef __GUIEVENT_H
#define __GUIEVENT_H

/* events don't get queued... is that a problem ? */

#define XA_GUI_EVENT_NAME "ICEWM_GUI_EVENT"

enum GUIEvent {
    geStartup,             //
    geShutdown,            //
    geRestart,             //
    geCloseAll,
    geLaunchApp,           //
    geWorkspaceChange,     //
    geWindowOpened,        //
    geWindowClosed,        //
    geDialogOpened,
    geDialogClosed,
    geWindowMax,           //
    geWindowRestore,       //
    geWindowMin,           //
    geWindowHide,          //
    geWindowRollup,        //
    geWindowMoved,
    geWindowSized,
    geWindowLower
};

#ifdef GUI_EVENT_NAMES
static struct {
    GUIEvent type;
    const char *name;
} gui_events[] =
{
    { geStartup, "startup" },
    { geShutdown, "shutdown" },
    { geRestart, "restart" },
    { geCloseAll, "closeAll" },
    { geLaunchApp, "launchApp" },
    { geWorkspaceChange, "workspaceChange" },
    { geWindowOpened, "windowOpen" },
    { geWindowClosed, "windowClose" },
    { geDialogOpened, "dialogOpen" },
    { geDialogClosed, "dialogClose" },
    { geWindowMin, "windowMin" },
    { geWindowMax, "windowMax" },
    { geWindowRestore, "windowRestore" },
    { geWindowHide, "windowHide" },
    { geWindowRollup, "windowRollup" },
    { geWindowLower, "windowLower" },
    { geWindowSized, "windowSized" },
    { geWindowMoved, "windowMoved" }
};

#endif

#endif
