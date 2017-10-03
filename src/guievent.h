#ifndef __GUIEVENT_H
#define __GUIEVENT_H

/* events don't get queued... is that a problem ? */

#define XA_GUI_EVENT_NAME "ICEWM_GUI_EVENT"

enum GUIEvent {
    geStartup,          // 00 implemented
    geShutdown,         // 01 implemented
    geRestart,          // 02 implemented
    geCloseAll,         // 03 is not used
    geLaunchApp,        // 04 implemented
    geWorkspaceChange,  // 05 implemented

    geWindowOpened,     // 06 implemented
    geWindowClosed,     // 07 implemented
    geDialogOpened,     // 08 implemented
    geDialogClosed,     // 09 implemented
    geWindowMax,        // 10 implemented
    geWindowRestore,    // 11 implemented

    geWindowMin,        // 12 implemented
    geWindowHide,       // 13 implemented
    geWindowRollup,     // 14 implemented
    geWindowMoved,      // 15 implemented
    geWindowSized,      // 16 implemented
    geWindowLower,      // 17 implemented
};

#ifdef GUI_EVENT_NAMES

#define NUM_GUI_EVENTS  18

static const char* gui_event_names[NUM_GUI_EVENTS] =
{
    "startup",          // 00
    "shutdown",         // 01
    "restart",          // 02
    "closeAll",         // 03
    "launchApp",        // 04
    "workspaceChange",  // 05

    "windowOpen",       // 06
    "windowClose",      // 07
    "dialogOpen",       // 08
    "dialogClose",      // 09
    "windowMax",        // 10
    "windowRestore",    // 11

    "windowMin",        // 12
    "windowHide",       // 13
    "windowRollup",     // 14
    "windowMoved",      // 15
    "windowSized",      // 16
    "windowLower",      // 17
};

#endif

#endif

// vim: set sw=4 ts=4 et:
