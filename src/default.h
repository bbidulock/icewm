#ifndef DEFAULT_H
#define DEFAULT_H

#include "yconfig.h"

/************************************************************************************************************************************************************/
XIV(bool, clickFocus,                           true)
XIV(bool, focusOnAppRaise,                      false)
XIV(bool, requestFocusOnAppRaise,               true)
XIV(bool, raiseOnFocus,                         true)
XIV(bool, focusOnClickClient,                   true)
XIV(bool, raiseOnClickClient,                   true)
XIV(bool, raiseOnClickButton,                   true)
XIV(bool, raiseOnClickFrame,                    true)
XIV(bool, raiseOnClickTitleBar,                 true)
XIV(bool, lowerOnClickWhenRaised,               false)
XIV(bool, passFirstClickToClient,               true)
XIV(bool, focusOnMap,                           true)
XIV(bool, mapInactiveOnTop,                     true)
XIV(bool, focusChangesWorkspace,                false)
XIV(bool, focusCurrentWorkspace,                false)
XIV(bool, focusOnMapTransient,                  false)
XIV(bool, focusOnMapTransientActive,            true)
XIV(bool, focusRootWindow,                      false)
XIV(bool, pointerColormap,                      true)
XIV(bool, sizeMaximized,                        false)
XIV(bool, showMoveSizeStatus,                   true)
XIV(bool, workspaceSwitchStatus,                true)
XIV(bool, beepOnNewMail,                        false)
XIV(bool, warpPointer,                          false)
XIV(bool, warpPointerOnEdgeSwitch,              false)
XIV(bool, opaqueMove,                           true)
XIV(bool, opaqueResize,                         true)
XIV(bool, hideTitleBarWhenMaximized,            false)
XSV(const char *, winMenuItems,                 "rmsnxfhualytiecw")
XIV(bool, showTaskBar,                          true)
XIV(bool, taskBarAtTop,                         false)
XIV(bool, taskBarKeepBelow,                     false)
XIV(bool, taskBarShowClock,                     true)
XIV(bool, taskBarShowApm,                       false)
XIV(bool, taskBarShowApmAuto,                   true)
XIV(bool, taskBarShowApmTime,                   true)
XIV(bool, taskBarShowApmGraph,                  true)
XIV(bool, taskBarShowMailboxStatus,             true)
XIV(bool, taskBarShowStartMenu,                 true)
XIV(bool, taskBarShowWindowListMenu,            true)
XIV(bool, taskBarShowWorkspaces,                true)
XIV(bool, taskBarShowWindows,                   true)
XIV(int, taskBarShowShowDesktopButton,          1)

XIV(int, taskBarButtonWidthDivisor,             3)
XIV(int, taskBarWidthPercentage,                100)
XSV(const char *, taskBarJustify,               "left")
XIV(bool, taskBarEnableSystemTray,              true)
XIV(bool, taskBarShowTray,                      true)
XIV(bool, trayShowAllWindows,                   true)
XIV(bool, taskBarShowTransientWindows,          true)
XIV(bool, taskBarShowAllWindows,                false)
XIV(bool, taskBarShowWindowIcons,               true)
XIV(bool, taskBarShowWindowTitles,              true)
XIV(bool, taskBarAutoHide,                      false)
XIV(bool, taskBarFullscreenAutoShow,            true)
XIV(bool, taskBarDoubleHeight,                  false)
XIV(bool, taskBarWorkspacesLeft,                true)
XIV(bool, taskBarWorkspacesTop,                 false)
XSV(const char *, taskBarWorkspacesLimit,       0)
XIV(bool, taskBarUseMouseWheel,                 true)
XIV(int, taskBarTaskGrouping,                   0)
XIV(bool, pagerShowPreview,                     true)
XIV(bool, pagerShowWindowIcons,                 true)
XIV(bool, pagerShowMinimized,                   true)
XIV(bool, pagerShowBorders,                     true)
XIV(bool, pagerShowLabels,                      true)
XIV(bool, pagerShowNumbers,                     false)
XIV(bool, taskBarShowCPUStatus,                 true)
XIV(bool, cpustatusShowRamUsage,                true)
XIV(bool, cpustatusShowSwapUsage,               true)
XIV(bool, cpustatusShowAcpiTemp,                true)
XIV(bool, cpustatusShowAcpiTempInGraph,         false)
XIV(bool, cpustatusShowCpuFreq,                 true)
XIV(bool, netstatusShowOnlyRunning,             false)
XIV(bool, taskBarShowMEMStatus,                 true)
XIV(bool, taskBarShowNetStatus,                 true)
XIV(bool, taskBarLaunchOnSingleClick,           true)
XIV(bool, taskBarShowCollapseButton,            false)
XIV(bool, minimizeToDesktop,                    false)
XIV(bool, miniIconsPlaceHorizontal,             false)
XIV(bool, miniIconsRightToLeft,                 false)
XIV(bool, miniIconsBottomToTop,                 false)
XIV(bool, manualPlacement,                      false)
XIV(bool, smartPlacement,                       true)
XIV(bool, centerLarge,                          false)
XIV(bool, centerTransientsOnOwner,              true)
XIV(bool, autoRaise,                            false)
XIV(bool, delayPointerFocus,                    true)
XIV(bool, useMouseWheel,                        false)
XIV(bool, quickSwitch,                          true)
XIV(bool, quickSwitchToMinimized,               true)
XIV(bool, quickSwitchToHidden,                  true)
XIV(bool, quickSwitchToUrgent,                  true)
XIV(bool, quickSwitchToAllWorkspaces,           false)
XIV(bool, quickSwitchGroupWorkspaces,           true)
XIV(int,  quickSwitchPersistence,               0)
XIV(bool, quickSwitchRaiseCandidate,            false)
XIV(bool, quickSwitchAllIcons,                  true)
XIV(bool, quickSwitchTextFirst,                 false)
XIV(bool, quickSwitchVertical,                  true)
XIV(bool, quickSwitchSmallWindow,               false)
XIV(bool, quickSwitchMaxWidth,                  false)
XIV(bool, quickSwitchHugeIcon,                  false)
XIV(bool, quickSwitchFillSelection,             false)
XIV(bool, countMailMessages,                    false)
XIV(bool, strongPointerFocus,                   false)
XIV(bool, snapMove,                             true)
XIV(bool, edgeHorzWorkspaceSwitching,           false)
XIV(bool, edgeVertWorkspaceSwitching,           false)
XIV(bool, edgeContWorkspaceSwitching,           true)
XIV(bool, limitSize,                            true)
XIV(bool, limitPosition,                        true)
XIV(bool, limitByDockLayer,                     false)
XIV(bool, considerHorizBorder,                  false)
XIV(bool, considerVertBorder,                   false)
XIV(bool, considerSizeHintsMaximized,           true)
XIV(bool, centerMaximizedWindows,               false)
XIV(bool, hideBordersMaximized,                 false)
XIV(bool, win95keys,                            true)
XIV(bool, autoReloadMenus,                      true)
XIV(bool, arrangeWindowsOnScreenSizeChange,     true)
XIV(bool, clientMouseActions,                   true)
XIV(bool, showPrograms,                         false)
XIV(bool, showSettingsMenu,                     true)
XIV(bool, showFocusModeMenu,                    true)
XIV(bool, showThemesMenu,                       true)
XIV(bool, showLogoutMenu,                       true)
XIV(bool, showLogoutSubMenu,                    true)
XIV(bool, showAbout,                            true)
XIV(bool, showRun,                              true)
XIV(bool, showWindowList,                       true)
XIV(bool, showHelp,                             true)
XIV(bool, allowFullscreen,                      true)
XIV(bool, fullscreenUseAllMonitors,             false)
XIV(bool, enableAddressBar,                     true)
XIV(bool, showAddressBar,                       true)
XIV(bool, confirmLogout,                        true)
#ifdef CONFIG_SHAPE
XIV(bool, protectClientWindow,                  true)
#endif
XIV(int, MenuMaximalWidth,                      0)
XIV(int, EdgeResistance,                        32)
XIV(int, snapDistance,                          8)
XIV(int, pointerFocusDelay,                     200)
XIV(int, autoRaiseDelay,                        400)
XIV(int, autoHideDelay,                         300)
XIV(int, autoShowDelay,                         500)
XIV(int, edgeSwitchDelay,                       600)
XIV(int, scrollBarStartDelay,                   500)
XIV(int, scrollBarDelay,                        30)
XIV(int, workspaceStatusTime,                   700)
XIV(int, useRootButtons,                        255)    // bitmask=all
XIV(int, buttonRaiseMask,                       1)
XIV(unsigned, rootWinMenuButton,                0)
XIV(unsigned, rootWinListButton,                2)
XIV(unsigned, rootMenuButton,                   3)
XIV(unsigned, titleMaximizeButton,              1)
XIV(unsigned, titleRollupButton,                2)
XIV(int, msgBoxDefaultAction,                   0)
XIV(int, pingTimeout,                           3)
XIV(int, mailCheckDelay,                        30)
XIV(int, taskBarCPUSamples,                     20)
XIV(int, taskBarApmGraphWidth,                  10)
XIV(int, taskBarMEMSamples,                     20)
XIV(int, focusRequestFlashTime,                 0)
XIV(int, focusRequestFlashInterval,             250)
XIV(int, nestedThemeMenuMinNumber,              21)
XIV(int, batteryPollingPeriod,                  10)
XIV(int, netWorkAreaBehaviour,                  0)

XSV(const char *, acpiIgnoreBatteries,          0)
XSV(const char *, mailBoxPath,                  0)
XSV(const char *, mailCommand,                  TERM " -name mutt -e mutt")
XSV(const char *, mailClassHint,                "mutt.XTerm")
XSV(const char *, newMailCommand,               0)
XSV(const char *, lockCommand,                  0)
XSV(const char *, clockCommand,                 "xclock -name icewm -title Clock")
XSV(const char *, clockClassHint,               "icewm.XClock")
XSV(const char *, runDlgCommand,                0)
XSV(const char *, openCommand,                  0)
XSV(const char *, terminalCommand,              TERM " -hold")
XSV(const char *, logoutCommand,                0)
XSV(const char *, logoutCancelCommand,          0)
#if __linux__
XSV(const char *, shutdownCommand,              "test -e /run/systemd/system && systemctl poweroff || loginctl poweroff")
XSV(const char *, rebootCommand,                "test -e /run/systemd/system && systemctl reboot || loginctl reboot")
XSV(const char *, suspendCommand,               "test -e /run/systemd/system && systemctl suspend || loginctl suspend")
XSV(const char *, hibernateCommand,             "test -e /run/systemd/system && systemctl hibernate || loginctl hibernate")
#else
XSV(const char *, shutdownCommand,              0)
XSV(const char *, rebootCommand,                0)
XSV(const char *, suspendCommand,               0)
XSV(const char *, hibernateCommand,             0)
#endif
XIV(int, taskBarCPUDelay,                       500)
XIV(int, taskBarMEMDelay,                       500)
XIV(int, taskBarNetSamples,                     20)
XIV(int, taskBarNetDelay,                       500)
XSV(const char *, cpuCommand,                   TERM " -name top -title Process\\ Status -e top")
XSV(const char *, cpuClassHint,                 "top.XTerm")
XIV(bool, cpuCombine,                           true)

#if __linux__
XSV(const char *, netCommand,                   TERM " -name 'ss' -title 'Socket Statistics' -hold -e sh -c 'which ss > /dev/null && watch -t ss -putswl || netstat -c'")
XSV(const char *, netClassHint,                 "ss.XTerm")
#else
XSV(const char *, netCommand,                   TERM " -name netstat -title 'Network Status' -hold -e netstat -c")
XSV(const char *, netClassHint,                 "netstat.XTerm")
#endif

#if __OpenBSD__
#define NET_DEVICES                             "[ew]* vio*"
#else
#define NET_DEVICES                             "[ew]*"
#endif
XSV(const char *, netDevice,                    NET_DEVICES)
#undef NET_DEVICES

XSV(const char *, addressBarCommand,            0)
XSV(const char *, dockApps,                     "right high desktop")
#ifdef CONFIG_I18N
XSV(const char *, fmtTime,                      "%X")
XSV(const char *, fmtTimeAlt,                   NULL)
XSV(const char *, fmtDate,                      "%c")
#else
XSV(const char *, fmtTime,                      "%H:%M:%S")
XSV(const char *, fmtTimeAlt,                   NULL)
XSV(const char *, fmtDate,                      "%Y-%m-%d %H:%M:%S %z %B %A")
#endif

#ifdef CFGDEF

cfoption icewm_preferences[] = {
    OBV("ClickToFocus",                         &clickFocus,                    "Focus windows by clicking"),
    OBV("FocusOnAppRaise",                      &focusOnAppRaise,               "Focus windows when application requests to raise"),
    OBV("RequestFocusOnAppRaise",               &requestFocusOnAppRaise,        "Request focus (flashing in taskbar) when application requests raise"),
    OBV("RaiseOnFocus",                         &raiseOnFocus,                  "Raise window when it receives focus"),
    OBV("FocusOnClickClient",                   &focusOnClickClient,            "Focus window when client area clicked"),
    OBV("RaiseOnClickClient",                   &raiseOnClickClient,            "Raise window when client area clicked"),
    OBV("RaiseOnClickTitleBar",                 &raiseOnClickTitleBar,          "Raise window when title bar is clicked"),
    OBV("RaiseOnClickButton",                   &raiseOnClickButton,            "Raise window when frame button is clicked"),
    OBV("RaiseOnClickFrame",                    &raiseOnClickFrame,             "Raise window when frame border is clicked"),
    OBV("LowerOnClickWhenRaised",               &lowerOnClickWhenRaised,        "Lower the active window when clicked again"),
    OBV("PassFirstClickToClient",               &passFirstClickToClient,        "Pass focusing click on client area to client"),
    OBV("FocusChangesWorkspace",                &focusChangesWorkspace,         "Change to the workspace of newly focused windows"),
    OBV("FocusCurrentWorkspace",                &focusCurrentWorkspace,         "Move newly focused windows to current workspace"),
    OBV("FocusOnMap",                           &focusOnMap,                    "Focus normal window when initially mapped"),
    OBV("FocusOnMapTransient",                  &focusOnMapTransient,           "Focus dialog window when initially mapped"),
    OBV("FocusOnMapTransientActive",            &focusOnMapTransientActive,     "Focus dialog window when initially mapped only if parent frame focused"),
    OBV("MapInactiveOnTop",                     &mapInactiveOnTop,              "Put new windows on top even if not focusing them"),
    OBV("PointerColormap",                      &pointerColormap,               "Colormap focus follows pointer"),
    OBV("DontRotateMenuPointer",                &dontRotateMenuPointer,         "Don't rotate the cursor for popup menus"),
    OBV("LimitSize",                            &limitSize,                     "Limit size of windows to screen"),
    OBV("LimitPosition",                        &limitPosition,                 "Limit position of windows to screen"),
    OBV("LimitByDockLayer",                     &limitByDockLayer,              "Let the Dock layer limit the workspace (incompatible with GNOME Panel)"),
    OBV("ConsiderHBorder",                      &considerHorizBorder,           "Consider border frames when maximizing horizontally"),
    OBV("ConsiderVBorder",                      &considerVertBorder,            "Consider border frames when maximizing vertically"),
    OBV("ConsiderSizeHintsMaximized",           &considerSizeHintsMaximized,    "Consider XSizeHints if frame is maximized"),
    OBV("CenterMaximizedWindows",               &centerMaximizedWindows,        "Center maximized windows that can't fit the screen (like terminals)"),
    OBV("HideBordersMaximized",                 &hideBordersMaximized,          "Hide window borders if window is maximized"),
    OBV("SizeMaximized",                        &sizeMaximized,                 "Maximized windows can be resized"),
    OBV("ShowMoveSizeStatus",                   &showMoveSizeStatus,            "Show position status window during move/resize"),
    OBV("ShowWorkspaceStatus",                  &workspaceSwitchStatus,         "Show name of current workspace while switching"),
    OBV("MinimizeToDesktop",                    &minimizeToDesktop,             "Display mini-icons on desktop for minimized windows"),
    OBV("MiniIconsPlaceHorizontal",             &miniIconsPlaceHorizontal,      "Place the mini-icons horizontal instead of vertical"),
    OBV("MiniIconsRightToLeft",                 &miniIconsRightToLeft,          "Place new mini-icons from right to left"),
    OBV("MiniIconsBottomToTop",                 &miniIconsBottomToTop,          "Place new mini-icons from bottom to top"),
    OBV("StrongPointerFocus",                   &strongPointerFocus,            "Always maintain focus under mouse window (makes some keyboard support non-functional or unreliable)"),
    OBV("OpaqueMove",                           &opaqueMove,                    "Opaque window move"),
    OBV("OpaqueResize",                         &opaqueResize,                  "Opaque window resize"),
    OBV("ManualPlacement",                      &manualPlacement,               "Windows initially placed manually by user"),
    OBV("SmartPlacement",                       &smartPlacement,                "Smart window placement with minimal overlap"),
    OBV("HideTitleBarWhenMaximized",            &hideTitleBarWhenMaximized,     "Hide title bar when maximized"),
    OBV("CenterLarge",                          &centerLarge,                   "Center large windows"),
    OBV("CenterTransientsOnOwner",              &centerTransientsOnOwner,       "Center dialogs on owner window"),
    OBV("MenuMouseTracking",                    &menuMouseTracking,             "Menus track mouse even with no mouse buttons held"),
    OBV("AutoRaise",                            &autoRaise,                     "Auto raise windows after delay"),
    OBV("DelayPointerFocus",                    &delayPointerFocus,             "Delay pointer focusing when mouse moves"),
    OBV("Win95Keys",                            &win95keys,                     "Support the Windows/Super key modifier to activate special functions.  The left Super key toggles the Start menu, while the right Super key toggles the Window list window."),
    OBV("ModSuperIsCtrlAlt",                    &modSuperIsCtrlAlt,             "Treat the Super/Win key modifier as a synonym for the Ctrl+Alt modifier combination. The default key bindings have many occurrences of Ctrl+Alt.  If you enable this, then the Super modifier is an alternative way to activate them."),
    OBV("UseMouseWheel",                        &useMouseWheel,                 "Support mouse wheel. When pressing Ctrl+Alt rotating the mouse wheel on the root window will cycle the focus over the windows."),
    OBV("ShowPopupsAbovePointer",               &showPopupsAbovePointer,        "Show popup menus above mouse pointer"),
    OBV("ReplayMenuCancelClick",                &replayMenuCancelClick,         "Send the clicks outside menus to target window"),
    OBV("QuickSwitch",                          &quickSwitch,                   "Enable Alt+Tab window switching"),
    OBV("QuickSwitchToMinimized",               &quickSwitchToMinimized,        "Enable Alt+Tab to minimized windows"),
    OBV("QuickSwitchToHidden",                  &quickSwitchToHidden,           "Enable Alt+Tab to hidden windows"),
    OBV("QuickSwitchToUrgent",                  &quickSwitchToUrgent,           "Prioritize Alt+Tab to urgent windows"),
    OBV("QuickSwitchToAllWorkspaces",           &quickSwitchToAllWorkspaces,    "Include windows from all workspaces in Alt+Tab"),
    OBV("QuickSwitchGroupWorkspaces",           &quickSwitchGroupWorkspaces,    "Group windows by workspace together in Alt+Tab"),
    OIV("QuickSwitchPersistence",               &quickSwitchPersistence, 0, 86400, "Time in seconds to remember the state of Alt+Tab"),
    OBV("QuickSwitchRaiseCandidate",            &quickSwitchRaiseCandidate,     "Raise a selected window while Alt+Tabbing in the QuickSwitch"),
    OBV("QuickSwitchAllIcons",                  &quickSwitchAllIcons,           "Show all reachable icons when quick switching"),
    OBV("QuickSwitchTextFirst",                 &quickSwitchTextFirst,          "Show the window title above (all reachable) icons"),
    OBV("QuickSwitchSmallWindow",               &quickSwitchSmallWindow,        "Create a smaller QuickSwitch window of 1/3 screen width"),
    OBV("QuickSwitchMaxWidth",                  &quickSwitchMaxWidth,           "Go trough all window titles and choose width of the longest one"),
    OBV("QuickSwitchVertical",                  &quickSwitchVertical,           "Place the icons and titles vertical instead of horizontal"),
    OBV("QuickSwitchHugeIcon",                  &quickSwitchHugeIcon,           "Show the huge (48x48) of the window icon for the active window"),
    OBV("QuickSwitchFillSelection",             &quickSwitchFillSelection,      "Fill the rectangle highlighting the current icon"),
    OBV("GrabRootWindow",                       &grabRootWindow,                "Manage root window (EXPERIMENTAL - normally enabled!)"),
    OBV("SnapMove",                             &snapMove,                      "Snap to nearest screen edge/window when moving windows"),
    OBV("EdgeSwitch",                           &edgeHorzWorkspaceSwitching,    "Workspace switches by moving mouse to left/right screen edge"),
    OBV("HorizontalEdgeSwitch",                 &edgeHorzWorkspaceSwitching,    "Workspace switches by moving mouse to left/right screen edge"),
    OBV("VerticalEdgeSwitch",                   &edgeVertWorkspaceSwitching,    "Workspace switches by moving mouse to top/bottom screen edge"),
    OBV("ContinuousEdgeSwitch",                 &edgeContWorkspaceSwitching,    "Workspace switches continuously when moving mouse to screen edge"),
    OBV("AutoReloadMenus",                      &autoReloadMenus,               "Reload menu files automatically"),
    OBV("ArrangeWindowsOnScreenSizeChange",     &arrangeWindowsOnScreenSizeChange, "Automatically arrange windows when screen size changes"),
    OBV("ShowTaskBar",                          &showTaskBar,                   "Show task bar"),
    OBV("TaskBarAtTop",                         &taskBarAtTop,                  "Task bar at top of the screen"),
    OBV("TaskBarKeepBelow",                     &taskBarKeepBelow,              "Keep the task bar below regular windows"),
    OBV("TaskBarAutoHide",                      &taskBarAutoHide,               "Auto hide task bar after delay"),
    OBV("TaskBarFullscreenAutoShow",            &taskBarFullscreenAutoShow,     "Auto show task bar when fullscreen window active"),
    OBV("TaskBarShowClock",                     &taskBarShowClock,              "Show clock on task bar"),
    OBV("TaskBarShowAPMStatus",                 &taskBarShowApm,                "Show battery status monitor on task bar"),
    OBV("TaskBarShowAPMAuto",                   &taskBarShowApmAuto,            "Enable TaskBarShowAPMStatus if a battery is present"),
    OBV("TaskBarShowAPMTime",                   &taskBarShowApmTime,            "Show battery status on task bar in time-format"),
    OBV("TaskBarShowAPMGraph",                  &taskBarShowApmGraph,           "Show battery status in graph mode"),
    OBV("TaskBarShowMailboxStatus",             &taskBarShowMailboxStatus,      "Show mailbox status on task bar"),
    OBV("TaskBarMailboxStatusBeepOnNewMail",    &beepOnNewMail,                 "Beep when new mail arrives"),
    OBV("TaskBarMailboxStatusCountMessages",    &countMailMessages,             "Count messages in mailbox"),
    OBV("TaskBarShowWorkspaces",                &taskBarShowWorkspaces,         "Show workspace switching buttons on task bar"),
    OBV("TaskBarShowWindows",                   &taskBarShowWindows,            "Show windows on the taskbar"),
    OIV("TaskBarShowShowDesktopButton",         &taskBarShowShowDesktopButton, 0, 2, "Show 'show desktop' button on taskbar (value of 2 to put after the clock)"),
    OBV("ShowEllipsis",                         &showEllipsis,                  "Show Ellipsis in taskbar items as indicator of further collapsed content."),
    OBV("TaskBarShowTray",                      &taskBarShowTray,               "Show application icons in the tray panel"),
    OBV("TaskBarEnableSystemTray",              &taskBarEnableSystemTray,       "Enable the system tray in the taskbar"),
    OBV("TrayShowAllWindows",                   &trayShowAllWindows,            "Show windows from all workspaces on tray"),
    OBV("TaskBarShowTransientWindows",          &taskBarShowTransientWindows,   "Show transient (dialogs, ...) windows on task bar"),
    OBV("TaskBarShowAllWindows",                &taskBarShowAllWindows,         "Show windows from all workspaces on task bar"),
    OBV("TaskBarShowWindowIcons",               &taskBarShowWindowIcons,        "Show icons of windows on task buttons of the task bar"),
    OBV("TaskBarShowWindowTitles",              &taskBarShowWindowTitles,       "Show titles of windows on task buttons of the task bar"),
    OBV("TaskBarShowStartMenu",                 &taskBarShowStartMenu,          "Show 'Start' menu on task bar"),
    OBV("TaskBarShowWindowListMenu",            &taskBarShowWindowListMenu,     "Show 'window list' menu on task bar"),
    OBV("TaskBarShowCPUStatus",                 &taskBarShowCPUStatus,          "Show CPU status on task bar"),
    OBV("CPUStatusShowRamUsage",                &cpustatusShowRamUsage,         "Show RAM usage in CPU status tool tip"),
    OBV("CPUStatusShowSwapUsage",               &cpustatusShowSwapUsage,        "Show swap usage in CPU status tool tip"),
    OBV("CPUStatusShowAcpiTemp",                &cpustatusShowAcpiTemp,         "Show ACPI temperature in CPU status tool tip"),
    OBV("CPUStatusShowAcpiTempInGraph",         &cpustatusShowAcpiTempInGraph,  "Show ACPI temperature in CPU status bar"),
    OBV("CPUStatusShowCpuFreq",                 &cpustatusShowCpuFreq,          "Show CPU frequency in CPU status tool tip"),
    OBV("NetStatusShowOnlyRunning",             &netstatusShowOnlyRunning,      "Show network status only for connected devices."),
    OBV("TaskBarShowMEMStatus",                 &taskBarShowMEMStatus,          "Show memory usage status on task bar (Linux only)"),
    OBV("TaskBarShowNetStatus",                 &taskBarShowNetStatus,          "Show network status on task bar"),
    OBV("TaskBarShowCollapseButton",            &taskBarShowCollapseButton,     "Show a button to collapse the taskbar"),
    OBV("TaskBarDoubleHeight",                  &taskBarDoubleHeight,           "Use double-height task bar"),
    OBV("TaskBarWorkspacesLeft",                &taskBarWorkspacesLeft,         "Place workspace pager on left, not right"),
    OBV("TaskBarWorkspacesTop",                 &taskBarWorkspacesTop,          "Place workspace pager on top row when using dual-height taskbar"),
    OSV("TaskBarWorkspacesLimit",               &taskBarWorkspacesLimit,        "Limit number of taskbar workspaces"),
    OBV("TaskBarUseMouseWheel",                 &taskBarUseMouseWheel,          "Enable mouse wheel cycling over workspaces and task buttons in taskbar"),
    OIV("TaskBarTaskGrouping",                  &taskBarTaskGrouping, 0, 3,     "Group applications with the same class name under a single task button: 0=off, 1=digits, 2=dots, 3=both."),
    OBV("PagerShowPreview",                     &pagerShowPreview,              "Show a mini desktop preview on each workspace button"),
    OBV("PagerShowWindowIcons",                 &pagerShowWindowIcons,          "Draw window icons inside large enough preview windows on pager (if PagerShowPreview=1)"),
    OBV("PagerShowMinimized",                   &pagerShowMinimized,            "Draw even minimized windows as unfilled rectangles (if PagerShowPreview=1)"),
    OBV("PagerShowBorders",                     &pagerShowBorders,              "Draw border around workspace buttons (if PagerShowPreview=1)"),
    OBV("PagerShowLabels",                      &pagerShowLabels,               "Show workspace name label on workspace button (if PagerShowPreview=1)"),
    OBV("PagerShowNumbers",                     &pagerShowNumbers,              "Show number of workspace on workspace button (if PagerShowPreview=1)"),
    OBV("TaskBarLaunchOnSingleClick",           &taskBarLaunchOnSingleClick,    "Execute taskbar applet commands (like MailCommand, ClockCommand, ...) on single click"),
//    OBV("WarpPointer",                          &warpPointer,                   "Move mouse when doing focusing in pointer focus mode"),
    OBV("ClientWindowMouseActions",             &clientMouseActions,            "Allow mouse actions on client windows (buggy with some programs)"),
    OBV("ShowProgramsMenu",                     &showPrograms,                  "Show programs submenu in the program menu"),
    OBV("ShowSettingsMenu",                     &showSettingsMenu,              "Show settings submenu in the program menu"),
    OBV("ShowFocusModeMenu",                    &showFocusModeMenu,             "Show focus mode submenu in the program menu"),
    OBV("ShowThemesMenu",                       &showThemesMenu,                "Show themes submenu in the program menu"),
    OBV("ShowLogoutMenu",                       &showLogoutMenu,                "Show logout menu in the program menu"),
    OBV("ShowHelp",                             &showHelp,                      "Show the help menu item in the program menu"),
    OBV("ShowLogoutSubMenu",                    &showLogoutSubMenu,             "Show logout submenu in the program menu"),
    OBV("ShowAbout",                            &showAbout,                     "Show the about menu item in the program menu"),
    OBV("ShowRun",                              &showRun,                       "Show the run menu item in the program menu"),
    OBV("ShowWindowList",                       &showWindowList,                "Show the window menu item in the program menu"),
    OBV("AllowFullscreen",                      &allowFullscreen,               "Allow to switch a window to fullscreen"),
    OBV("FullscreenUseAllMonitors",             &fullscreenUseAllMonitors,      "Span over all available screens if window goes into fullscreen"),
    OBV("EnableAddressBar",                     &enableAddressBar,              "Enable address bar functionality in taskbar"),
    OBV("ShowAddressBar",                       &showAddressBar,                "Show address bar in task bar"),
#ifdef CONFIG_I18N
    OBV("MultiByte",                            &multiByte,                     "Overrides automatic multiple byte detection"),
#endif
    OBV("ConfirmLogout",                        &confirmLogout,                 "Confirm logout"),
#ifdef CONFIG_SHAPE
    OBV("ShapesProtectClientWindow",            &protectClientWindow,           "Don't cut client windows by shapes set trough frame corner pixmap"),
#endif
    OBV("DoubleBuffer",                         &doubleBuffer,                  "Use double buffering when redrawing the display"),
    OBV("XRRDisable",                           &xrrDisable,                    "Disable use of new XRANDR API for dual head (nvidia workaround)"),
    OBV("PreferFreetypeFonts",                  &fontPreferFreetype,            "Favour Xft fonts over core X11 fonts where possible"),
    OIV("DelayFuzziness",                       &DelayFuzziness, 0, 100,        "Delay fuzziness in ms, to allow merging of multiple timer timeouts into one for notebook power saving"),
    OIV("ClickMotionDistance",                  &ClickMotionDistance, 0, 32,    "Pointer motion distance before click gets interpreted as drag"),
    OIV("ClickMotionDelay",                     &ClickMotionDelay, 0, 2000,     "Delay in ms before click gets interpreted as drag"),
    OIV("MultiClickTime",                       &MultiClickTime, 0, 5000,       "Multiple click time in ms"),
    OIV("MenuActivateDelay",                    &MenuActivateDelay, 0, 5000,    "Delay in ms before activating menu items"),
    OIV("SubmenuMenuActivateDelay",             &SubmenuActivateDelay, 0, 5000, "Delay in ms before activating menu submenus"),
    OIV("MenuMaximalWidth",                     &MenuMaximalWidth, 0, 16384,    "Maximal width of popup menus,  2/3 of the screen's width if set to zero"),
    OBV("ToolTipIcon",                          &ToolTipIcon,                   "Show an application icon in toolbar and tray tooltips"),
    OIV("ToolTipDelay",                         &ToolTipDelay, 0, 5000,         "Delay in ms before tooltip window is displayed"),
    OIV("ToolTipTime",                          &ToolTipTime, 0, 60000,         "Time in ms before tooltip window is hidden (0 means never)"),
    OIV("AutoHideDelay",                        &autoHideDelay, 0, 5000,        "Delay in ms before task bar is hidden"),
    OIV("AutoShowDelay",                        &autoShowDelay, 0, 5000,        "Delay in ms before task bar is shown"),
    OIV("AutoRaiseDelay",                       &autoRaiseDelay, 0, 5000,       "Delay in ms before windows are auto raised"),
    OIV("EdgeResistance",                       &EdgeResistance, 0, 10000,      "Resistance in pixels when trying to move windows off the screen (10000 = infinite)"),
    OIV("PointerFocusDelay",                    &pointerFocusDelay, 0, 1000,    "Delay in ms for pointer focus switching"),
    OIV("SnapDistance",                         &snapDistance, 0, 64,           "Distance in pixels before windows snap together"),
    OIV("EdgeSwitchDelay",                      &edgeSwitchDelay, 0, 5000,      "Screen edge workspace switching delay in ms"),
    OIV("ScrollBarStartDelay",                  &scrollBarStartDelay, 0, 5000,  "Inital scroll bar autoscroll delay in ms"),
    OIV("ScrollBarDelay",                       &scrollBarDelay, 0, 5000,       "Scroll bar autoscroll delay in ms"),
    OIV("AutoScrollStartDelay",                 &autoScrollStartDelay, 0, 5000, "Auto scroll start delay in ms"),
    OIV("AutoScrollDelay",                      &autoScrollDelay, 0, 5000,      "Auto scroll delay in ms"),
    OIV("WorkspaceStatusTime",                  &workspaceStatusTime, 0, 2500,  "Time before workspace status window is hidden in ms"),
    OIV("UseRootButtons",                       &useRootButtons, 0, 255,        "Bitmask of root window button click to use in window manager"),
    OIV("ButtonRaiseMask",                      &buttonRaiseMask, 0, 255,       "Bitmask of buttons that raise the window when pressed"),
    OIV("DesktopWinMenuButton",                 &rootWinMenuButton, 0, 20,      "Desktop mouse-button click to show the window list menu"),
    OIV("DesktopWinListButton",                 &rootWinListButton, 0, 20,       "Desktop mouse-button click to show the window list"),
    OIV("DesktopMenuButton",                    &rootMenuButton, 0, 20,         "Desktop mouse-button click to show the root menu"),
    OIV("TitleBarMaximizeButton",               &titleMaximizeButton, 0, 5,     "TitleBar mouse-button double click to maximize the window"),
    OIV("TitleBarRollupButton",                 &titleRollupButton, 0, 5,       "TitleBar mouse-button double click to rollup the window"),
    OIV("MsgBoxDefaultAction",                  &msgBoxDefaultAction, 0, 1,     "Preselect to Cancel (0) or the OK (1) button in message boxes"),
    OIV("PingTimeout",                          &pingTimeout, 0, (3600*24),     "Timeout in seconds for applications to respond to _NET_WM_PING protocol"),
    OIV("MailCheckDelay",                       &mailCheckDelay, 0, (3600*24),  "Delay between new-mail checks in seconds"),
    OIV("TaskBarCPUDelay",                      &taskBarCPUDelay, 10, (60*60*1000),    "Delay between CPU Monitor samples in ms"),
    OIV("TaskBarCPUSamples",                    &taskBarCPUSamples, 2, 1000,    "The width of the CPU Monitor applet in pixels"),
    OIV("TaskBarMEMSamples",                    &taskBarMEMSamples, 2, 1000,    "The width of the Memory Monitor applet in pixels"),
    OIV("TaskBarMEMDelay",                      &taskBarMEMDelay, 10, (60*60*1000),    "Delay between Memory Monitor samples in ms"),
    OIV("TaskBarNetSamples",                    &taskBarNetSamples, 2, 1000,    "The width of the Net Monitor applet in pixels"),
    OIV("TaskBarNetDelay",                      &taskBarNetDelay, 10, (60*60*1000),    "Delay between Net Monitor samples in ms"),
    OIV("TaskbarButtonWidthDivisor",            &taskBarButtonWidthDivisor, 1, 50, "default number of tasks in taskbar"),
    OIV("TaskBarWidthPercentage",               &taskBarWidthPercentage, 0, 100, "Task bar width as percentage of the screen width"),
    OSV("TaskBarJustify",                       &taskBarJustify, "Taskbar justify left, right or center"),
    OIV("TaskBarApmGraphWidth",                 &taskBarApmGraphWidth, 1, 1000,  "Width of battery Monitor"),

    OIV("XineramaPrimaryScreen",                &xineramaPrimaryScreen, 0, 63, "Primary screen for xinerama where taskbar is shown"),
    OIV("FocusRequestFlashTime",                &focusRequestFlashTime, 0, (3600 * 24), "Number of seconds the taskbar app will blink when requesting focus (0 = forever)"),
    OIV("FocusRequestFlashInterval",            &focusRequestFlashInterval, 0, 30000, "Taskbar blink interval (ms) when requesting focus (0 = blinking disabled)"),
    OIV("NestedThemeMenuMinNumber",             &nestedThemeMenuMinNumber,  0, 1234,  "Minimal number of themes after which the Themes menu becomes nested (0=disabled)"),
    OIV("BatteryPollingPeriod",                 &batteryPollingPeriod, 2, 3600, "Delay between power status updates in seconds"),
    OIV("NetWorkAreaBehaviour",                 &netWorkAreaBehaviour, 0, 2,    "NET_WORKAREA behaviour: 0 (single/multimonitor with STRUT information, like metacity), 1 (always full desktop), 2 (singlemonitor with STRUT, multimonitor without STRUT)"),
///    OSV("Theme",                                &themeName,                     "Theme name"),
    OSV("IconPath",                             &iconPath,                      "Icon search path (colon separated)"),
    OSV("IconThemes",                           &iconThemes,                    "Colon separated icon theme list with wildcard support. Minus prefix - can be used to exclude themes."),
    OSV("MailBoxPath",                          &mailBoxPath,                   "Colon separated paths of your mailboxes, otherwise $MAILPATH or $MAIL is used"),
    OSV("MailCommand",                          &mailCommand,                   "Command to run on mailbox"),
    OSV("MailClassHint",                        &mailClassHint,                 "WM_CLASS to allow runonce for MailCommand"),
    OSV("NewMailCommand",                       &newMailCommand,                "Command to run when new mail arrives"),
    OSV("LockCommand",                          &lockCommand,                   "Command to lock display/screensaver"),
    OSV("ClockCommand",                         &clockCommand,                  "Command to run on clock"),
    OSV("ClockClassHint",                       &clockClassHint,                "WM_CLASS to allow runonce for ClockCommand"),
    OSV("RunCommand",                           &runDlgCommand,                 "Command to select and run a program"),
    OSV("OpenCommand",                          &openCommand,                   "Command to select and run a program."),
    OSV("TerminalCommand",                      &terminalCommand,               "Terminal emulator must accept -e option."),
    OSV("LogoutCommand",                        &logoutCommand,                 "Command to start logout"),
    OSV("LogoutCancelCommand",                  &logoutCancelCommand,           "Command to cancel logout"),
    OSV("ShutdownCommand",                      &shutdownCommand,               "Command to shutdown the system"),
    OSV("RebootCommand",                        &rebootCommand,                 "Command to reboot the system"),
    OSV("SuspendCommand",                       &suspendCommand,                "Command to send the system to standby mode"),
    OSV("HibernateCommand",                     &hibernateCommand,              "Command to hibernate the system"),
    OSV("CPUStatusCommand",                     &cpuCommand,                    "Command to run on CPU status"),
    OSV("CPUStatusClassHint",                   &cpuClassHint,                  "WM_CLASS to allow runonce for CPUStatusCommand"),
    OBV("CPUStatusCombine",                     &cpuCombine,                    "Combine all CPUs to one"),
    OSV("NetStatusCommand",                     &netCommand,                    "Command to run on Net status"),
    OSV("NetStatusClassHint",                   &netClassHint,                  "WM_CLASS to allow runonce for NetStatusCommand"),
    OSV("AddressBarCommand",                    &addressBarCommand,             "Command to run for address bar entries"),
    OSV("NetworkStatusDevice",                  &netDevice,                     "Network device to show status for"),
    OSV("TimeFormat",                           &fmtTime,                       "Clock Time format (strftime format string)"),
    OSV("TimeFormatAlt",                        &fmtTimeAlt,                    "Alternate Clock Time format for blinking effects"),
    OSV("DateFormat",                           &fmtDate,                       "Clock Date format for tooltip (strftime format string)"),
    OSV("DockApps",                             &dockApps,                       "Support DockApps (right, left, center, down, high, above, below, desktop, or empty to disable). Control with Ctrl+Mouse."),
    OSV("XRRPrimaryScreenName",                 &xineramaPrimaryScreenName,     "screen/output name of the primary screen"),
    OSV("AcpiIgnoreBatteries",                  &acpiIgnoreBatteries,           "List of battery names (directories) in /proc/acpi/battery to ignore. Useful when more slots are built-in, but only one battery is used"),

    OKV("MouseWinMove",                         gMouseWinMove,                  "Mouse binding for window move"),
    OKV("MouseWinSize",                         gMouseWinSize,                  "Mouse binding for window resize"),
    OKV("MouseWinRaise",                        gMouseWinRaise,                 "Mouse binding to raise window"),
    OKV("MouseWinLower",                        gMouseWinLower,                 "Mouse binding to lower window"),
    OKV("MouseWinTabbing",                      gMouseWinTabbing,               "Mouse binding to create tabs"),
    OKV("KeyWinRaise",                          gKeyWinRaise,                   "Raises the window that currently has input focus."),
    OKV("KeyWinOccupyAll",                      gKeyWinOccupyAll,               "Makes the active window occupy all work spaces."),
    OKV("KeyWinLower",                          gKeyWinLower,                   "Lowers the window that currently has input focus."),
    OKV("KeyWinClose",                          gKeyWinClose,                   "Closes the active window."),
    OKV("KeyWinRestore",                        gKeyWinRestore,                 "Restores the active window to its visible state."),
    OKV("KeyWinPrev",                           gKeyWinPrev,                    "Switches focus to the previous window."),
    OKV("KeyWinNext",                           gKeyWinNext,                    "Switches focus to the next window."),
    OKV("KeyWinMove",                           gKeyWinMove,                    "Starts movement of the active window."),
    OKV("KeyWinSize",                           gKeyWinSize,                    "Starts resizing of the active window."),
    OKV("KeyWinMinimize",                       gKeyWinMinimize,                "Iconifies the active window."),
    OKV("KeyWinMaximize",                       gKeyWinMaximize,                "Maximizes the active window with borders."),
    OKV("KeyWinMaximizeVert",                   gKeyWinMaximizeVert,            "Maximizes the active window vertically."),
    OKV("KeyWinMaximizeHoriz",                  gKeyWinMaximizeHoriz,           "Maximizes the active window horizontally."),
    OKV("KeyWinFullscreen",                     gKeyWinFullscreen,              "Maximizes the active window without borders."),
    OKV("KeyWinHide",                           gKeyWinHide,                    "Hides the active window."),
    OKV("KeyWinRollup",                         gKeyWinRollup,                  "Rolls up the active window."),
    OKV("KeyWinMenu",                           gKeyWinMenu,                    "Posts the window menu."),
    OKV("KeyWinArrangeN",                       gKeyWinArrangeN,                "Moves the active window to the top middle of the screen."),
    OKV("KeyWinArrangeNE",                      gKeyWinArrangeNE,               "Moves the active window to the top right of the screen."),
    OKV("KeyWinArrangeE",                       gKeyWinArrangeE,                "Moves the active window to the middle right of the screen."),
    OKV("KeyWinArrangeSE",                      gKeyWinArrangeSE,               "Moves the active window to the bottom right of the screen."),
    OKV("KeyWinArrangeS",                       gKeyWinArrangeS,                "Moves the active window to the bottom middle of the screen."),
    OKV("KeyWinArrangeSW",                      gKeyWinArrangeSW,               "Moves the active window to the bottom left of the screen."),
    OKV("KeyWinArrangeW",                       gKeyWinArrangeW,                "Moves the active window to the middle left of the screen."),
    OKV("KeyWinArrangeNW",                      gKeyWinArrangeNW,               "Moves the active window to the top left corner of the screen."),
    OKV("KeyWinArrangeC",                       gKeyWinArrangeC,                "Moves the active window to the top middle of the screen."),
    OKV("KeyWinTileLeft",                       gKeyWinTileLeft,                "Let the active window occupy the left half of the screen"),
    OKV("KeyWinTileRight",                      gKeyWinTileRight,               "Let the active window occupy the right half of the screen"),
    OKV("KeyWinTileTop",                        gKeyWinTileTop,                 "Let the active window occupy the top half of the screen"),
    OKV("KeyWinTileBottom",                     gKeyWinTileBottom,              "Let the active window occupy the bottom half of the screen"),
    OKV("KeyWinTileTopLeft",                    gKeyWinTileTopLeft,             "Let the active window occupy the top left quarter of the screen"),
    OKV("KeyWinTileTopRight",                   gKeyWinTileTopRight,            "Let the active window occupy the top right quarter of the screen"),
    OKV("KeyWinTileBottomLeft",                 gKeyWinTileBottomLeft,          "Let the active window occupy the bottom left quarter of the screen"),
    OKV("KeyWinTileBottomRight",                gKeyWinTileBottomRight,         "Let the active window occupy the bottom right quarter of the screen"),
    OKV("KeyWinTileCenter",                     gKeyWinTileCenter,              "Let the active window occupy the center quarter of the screen"),
    OKV("KeyWinSmartPlace",                     gKeyWinSmartPlace,              "Smart place the active window."),
    OKV("KeySysSwitchNext",                     gKeySysSwitchNext,              "Opens the QuickSwitch popup and/or moves the selector in the QuickSwitch popup"),
    OKV("KeySysSwitchLast",                     gKeySysSwitchLast,              "Works like KeySysSwitchNext but moving in the opposite direction."),
    OKV("KeySysSwitchClass",                    gKeySysSwitchClass,             "Is like KeySysSwitchNext but only for windows with the same WM_CLASS property as the currently focused window."),
    OKV("KeySysWinNext",                        gKeySysWinNext,                 "Give focus to the next window and raise it."),
    OKV("KeySysWinPrev",                        gKeySysWinPrev,                 "Give focus to the previous window and raise it."),
    OKV("KeyTaskBarSwitchNext",                 gKeyTaskBarSwitchNext,          "Switch to the next window in the Task Bar"),
    OKV("KeyTaskBarSwitchPrev",                 gKeyTaskBarSwitchPrev,          "Switch to the previous window in the Task Bar"),
    OKV("KeyTaskBarMoveNext",                   gKeyTaskBarMoveNext,            "Move the Task Bar button of the current window right"),
    OKV("KeyTaskBarMovePrev",                   gKeyTaskBarMovePrev,            "Move the Task Bar button of the current window left"),
    OKV("KeySysWinMenu",                        gKeySysWinMenu,                 "Posts the system window menu."),
    OKV("KeySysDialog",                         gKeySysDialog,                  "Opens the IceWM system dialog in the center of the screen."),
    OKV("KeySysMenu",                           gKeySysMenu,                    "Activates the IceWM root menu in the lower left corner."),
    OKV("KeySysWindowList",                     gKeySysWindowList,              "Opens the IceWM system window list in the center of the screen."),
    OKV("KeySysWinListMenu",                    gKeySysWinListMenu,             "Shows the window list menu."),
    OKV("KeySysKeyboardNext",                   gKeySysKeyboardNext,            "Switch to the next keyboard layout in the KeyboardLayouts list."),
    OKV("KeySysAddressBar",                     gKeySysAddressBar,              "Opens the address bar in the task bar where a command can be typed."),
    OKV("KeySysWorkspacePrev",                  gKeySysWorkspacePrev,           "Goes one workspace to the left."),
    OKV("KeySysWorkspaceNext",                  gKeySysWorkspaceNext,           "Goes one workspace to the right."),
    OKV("KeySysWorkspaceLast",                  gKeySysWorkspaceLast,           "Goes to the previous workspace."),
    OKV("KeySysWorkspacePrevTakeWin",           gKeySysWorkspacePrevTakeWin,    "Takes the active window one workspace to the left."),
    OKV("KeySysWorkspaceNextTakeWin",           gKeySysWorkspaceNextTakeWin,    "Takes the active window one workspace to the right."),
    OKV("KeySysWorkspaceLastTakeWin",           gKeySysWorkspaceLastTakeWin,    "Takes the active window to the previous workspace."),
    OKV("KeySysWorkspace1",                     gKeySysWorkspace1,              "Goes to workspace 1."),
    OKV("KeySysWorkspace2",                     gKeySysWorkspace2,              "Goes to workspace 2."),
    OKV("KeySysWorkspace3",                     gKeySysWorkspace3,              "Goes to workspace 3."),
    OKV("KeySysWorkspace4",                     gKeySysWorkspace4,              "Goes to workspace 4."),
    OKV("KeySysWorkspace5",                     gKeySysWorkspace5,              "Goes to workspace 5."),
    OKV("KeySysWorkspace6",                     gKeySysWorkspace6,              "Goes to workspace 6."),
    OKV("KeySysWorkspace7",                     gKeySysWorkspace7,              "Goes to workspace 7."),
    OKV("KeySysWorkspace8",                     gKeySysWorkspace8,              "Goes to workspace 8."),
    OKV("KeySysWorkspace9",                     gKeySysWorkspace9,              "Goes to workspace 9."),
    OKV("KeySysWorkspace10",                    gKeySysWorkspace10,             "Goes to workspace 10."),
    OKV("KeySysWorkspace11",                    gKeySysWorkspace11,             "Goes to workspace 11."),
    OKV("KeySysWorkspace12",                    gKeySysWorkspace12,             "Goes to workspace 12."),
    OKV("KeySysWorkspace1TakeWin",              gKeySysWorkspace1TakeWin,       "Takes the active window to workspace 1."),
    OKV("KeySysWorkspace2TakeWin",              gKeySysWorkspace2TakeWin,       "Takes the active window to workspace 2."),
    OKV("KeySysWorkspace3TakeWin",              gKeySysWorkspace3TakeWin,       "Takes the active window to workspace 3."),
    OKV("KeySysWorkspace4TakeWin",              gKeySysWorkspace4TakeWin,       "Takes the active window to workspace 4."),
    OKV("KeySysWorkspace5TakeWin",              gKeySysWorkspace5TakeWin,       "Takes the active window to workspace 5."),
    OKV("KeySysWorkspace6TakeWin",              gKeySysWorkspace6TakeWin,       "Takes the active window to workspace 6."),
    OKV("KeySysWorkspace7TakeWin",              gKeySysWorkspace7TakeWin,       "Takes the active window to workspace 7."),
    OKV("KeySysWorkspace8TakeWin",              gKeySysWorkspace8TakeWin,       "Takes the active window to workspace 8."),
    OKV("KeySysWorkspace9TakeWin",              gKeySysWorkspace9TakeWin,       "Takes the active window to workspace 9."),
    OKV("KeySysWorkspace10TakeWin",             gKeySysWorkspace10TakeWin,      "Takes the active window to workspace 10."),
    OKV("KeySysWorkspace11TakeWin",             gKeySysWorkspace11TakeWin,      "Takes the active window to workspace 11."),
    OKV("KeySysWorkspace12TakeWin",             gKeySysWorkspace12TakeWin,      "Takes the active window to workspace 12."),
    OKV("KeySysTileVertical",                   gKeySysTileVertical,            "Tiles all windows from left to right maximized vertically."),
    OKV("KeySysTileHorizontal",                 gKeySysTileHorizontal,          "Tiles all windows from top to bottom maximized horizontally."),
    OKV("KeySysCascade",                        gKeySysCascade,                 "Makes a horizontal cascade of all windows which are maximized vertically."),
    OKV("KeySysArrange",                        gKeySysArrange,                 "Rearranges the windows."),
    OKV("KeySysArrangeIcons",                   gKeySysArrangeIcons,            "Rearranges the icons on the desktop, if MinimizeToDesktop is true."),
    OKV("KeySysMinimizeAll",                    gKeySysMinimizeAll,             "Minimizes all windows."),
    OKV("KeySysHideAll",                        gKeySysHideAll,                 "Hides all windows."),
    OKV("KeySysUndoArrange",                    gKeySysUndoArrange,             "Undoes arrangement."),
    OKV("KeySysShowDesktop",                    gKeySysShowDesktop,             "Minimizes all windows to show the desktop."),
    OKV("KeySysCollapseTaskBar",                gKeySysCollapseTaskBar,         "Toggles displaying the task bar."),

    OKF("WorkspaceNames",                       addWorkspace, "Add a workspace"),
    OKF("KeyboardLayouts",                      addKeyboard, "Add a keyboard layout"),
    OSV("WinMenuItems",                         &winMenuItems,                  "The list of items to be supported in the menu window (rmsnxfhualytieckw)"),
    OK0()
};

#endif

#if defined(GENPREF) || defined(WMAPP)

static bool alphaBlending;
static bool synchronizeX11;
static const char* outputFile;
static const char* splashFile(ICESPLASH);
static const char* tracingModules;
extern bool loggingEvents;

cfoption wmapp_preferences[] = {
    OBV("Alpha",        &alphaBlending,  "Use a 32-bit visual for alpha blending"),
    OBV("Synchronize",  &synchronizeX11, "Synchronize X11 for debugging (slow)"),
    OBV("LogEvents",    &loggingEvents,  "Enable event logging for debugging"),
    OSV("OutputFile",   &outputFile,     "Redirect all icewm output to FILE"),
    OSV("Splash",       &splashFile,     "Splash image on startup (IceWM.jpg)"),
    OSV("Trace",        &tracingModules, "Enable tracing for the given modules"),
    OSV("Theme",        &themeName,      "The name of the theme"),
    OK0()
};
#endif

#include "themable.h"
#endif

// vim: set sw=4 ts=4 et:
