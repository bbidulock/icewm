#ifndef __DEFAULT_H
#define __DEFAULT_H

#include "yconfig.h"

/************************************************************************************************************************************************************/
XIV(bool, clickFocus,                           true)
XIV(bool, focusOnAppRaise,                      true) 
XIV(bool, raiseOnFocus,                         true)
XIV(bool, focusOnClickClient,                   true)
XIV(bool, raiseOnClickClient,                   true)
XIV(bool, raiseOnClickButton,                   true)
XIV(bool, raiseOnClickFrame,                    true)
XIV(bool, raiseOnClickTitleBar,                 true)
XIV(bool, lowerOnClickWhenRaised,               false)
XIV(bool, passFirstClickToClient,               true)
XIV(bool, focusOnMap,                           true)
XIV(bool, focusChangesWorkspace,                true)
XIV(bool, focusOnMapTransient,                  true)
XIV(bool, focusOnMapTransientActive,            true)
XIV(bool, focusRootWindow,                      false)
XIV(bool, pointerColormap,                      true)
XIV(bool, sizeMaximized,                        false)
XIV(bool, showMoveSizeStatus,                   true)
XIV(bool, workspaceSwitchStatus,                true)
XIV(bool, beepOnNewMail,                        false)
XIV(bool, warpPointer,                          false)
XIV(bool, opaqueMove,                           true)
XIV(bool, opaqueResize,                         true)
#ifdef CONFIG_TASKBAR
XIV(bool, showTaskBar,                          true)
XIV(bool, taskBarAtTop,                         false)
XIV(bool, taskBarKeepBelow,                     false)
XIV(bool, taskBarShowClock,                     true)
XIV(bool, taskBarShowApm,                       false)
XIV(bool, taskBarShowApmTime,                   true) // mschy
XIV(bool, taskBarShowMailboxStatus,             true)
XIV(bool, taskBarShowStartMenu,                 true)
XIV(bool, taskBarShowWindowListMenu,            true)
XIV(bool, taskBarShowWorkspaces,                true)
XIV(bool, taskBarShowWindows,                   true)
XIV(bool, taskBarShowShowDesktopButton,         true)
#ifdef CONFIG_TRAY
XIV(bool, taskBarShowTray,                      true)
XIV(bool, trayShowAllWindows,                   true)
#endif
XIV(bool, taskBarShowAllWindows,                false)
XIV(bool, taskBarShowWindowIcons,               true)
XIV(bool, taskBarAutoHide,                      false)
XIV(bool, taskBarDoubleHeight,                  false)
XIV(bool, taskBarWorkspacesLeft,                true)
XIV(bool, taskBarShowCPUStatus,                 true)
XIV(bool, taskBarShowNetStatus,                 true)
XIV(bool, taskBarLaunchOnSingleClick,           true)
XIV(bool, taskBarShowCollapseButton,            true)
#endif
XIV(bool, minimizeToDesktop,                    false)
XIV(bool, manualPlacement,                      false)
XIV(bool, smartPlacement,                       true)
XIV(bool, centerTransientsOnOwner,              true)
XIV(bool, autoRaise,                            false)
XIV(bool, delayPointerFocus,                    false)
XIV(bool, useMouseWheel,                        false)
XIV(bool, quickSwitch,                          true)
XIV(bool, quickSwitchToMinimized,               true)
XIV(bool, quickSwitchToHidden,                  true)
XIV(bool, quickSwitchToAllWorkspaces,           false)
XIV(bool, quickSwitchGroupWorkspaces,           true)
XIV(bool, quickSwitchAllIcons,                  true)
XIV(bool, quickSwitchTextFirst,                 false)
XIV(bool, quickSwitchVertical,                  false)
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
XIV(bool, limitByDockLayer,                     true)
XIV(bool, considerHorizBorder,                  false)
XIV(bool, considerVertBorder,                   false)
XIV(bool, centerMaximizedWindows,               false)
XIV(bool, win95keys,                            true)
XIV(bool, autoReloadMenus,                      true)
XIV(bool, clientMouseActions,                   true)
XIV(bool, showPrograms,                         false)
XIV(bool, showThemesMenu,                       true)
XIV(bool, showLogoutMenu,                       true)
XIV(bool, showLogoutSubMenu,                    true)
XIV(bool, showAbout,                            true)
XIV(bool, showRun,                              true)
XIV(bool, showWindowList,                       true)
XIV(bool, showHelp,                             true)
XIV(bool, allowFullscreen,                      true)
XIV(bool, enableAddressBar,                     true);
XIV(bool, showAddressBar,                       true)
XIV(bool, confirmLogout,                        true)
#ifdef CONFIG_SHAPED_DECORATION
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
XIV(int, workspaceStatusTime,                   2500)
XIV(int, useRootButtons,                        255)    // bitmask=all
XIV(int, buttonRaiseMask,                       1)
XIV(int, rootWinMenuButton,                     0)
XIV(int, rootWinListButton,                     2)
XIV(int, rootMenuButton,                        3)
XIV(int, titleMaximizeButton,                   1)
XIV(int, titleRollupButton,                     2)
XIV(int, msgBoxDefaultAction,                   0)
XIV(int, mailCheckDelay,                        30)
XIV(int, taskBarCPUSamples,                     20)
XIV(int, focusRequestFlashTime,                 0)
XIV(int, nestedThemeMenuMinNumber,              15)

#ifdef CONFIG_APPLET_APM
XSV(const char *, acpiIgnoreBatteries,          0)
#endif

XSV(const char *, mailBoxPath,                  0)
XSV(const char *, mailCommand,                  "xterm -name pine -title PINE -e pine")
XSV(const char *, mailClassHint,                "pine.XTerm")
XSV(const char *, newMailCommand,               0)
XSV(const char *, lockCommand,                  0)
XSV(const char *, clockCommand,                 "xclock -name icewm -title Clock")
XSV(const char *, clockClassHint,               "icewm.XClock")
XSV(const char *, runDlgCommand,                0)
XSV(const char *, openCommand,                  0)
XSV(const char *, terminalCommand,              "xterm")
XSV(const char *, logoutCommand,                0)
XSV(const char *, logoutCancelCommand,          0)
XSV(const char *, shutdownCommand,              0)
XSV(const char *, rebootCommand,                0)
XIV(int, taskBarCPUDelay,                       500)
XIV(int, taskBarNetSamples,                     20)
XIV(int, taskBarNetDelay,                       500)
XSV(const char *, cpuCommand,                   "xterm -name top -title Process\\ Status -e top")
XSV(const char *, cpuClassHint,                 "top.XTerm")
XSV(const char *, netCommand,                   "xterm -name netstat -title 'Network Status' -e netstat -c")
XSV(const char *, netClassHint,                 "netstat.XTerm")
XSV(const char *, netDevice,                    "ppp0 eth0")
XSV(const char *, addressBarCommand,            0)
#ifdef CONFIG_I18N
XSV(const char *, fmtTime,                      "%X")
XSV(const char *, fmtTimeAlt,                   NULL)
XSV(const char *, fmtDate,                      "%c")
#else
XSV(const char *, fmtTime,                      "%H:%M:%S")
XSV(const char *, fmtTimeAlt,                   NULL)
XSV(const char *, fmtDate,                      "%Y-%m-%d %H:%M:%S %z %B %A")
#endif

#if defined(CFGDEF) && !defined(NO_CONFIGURE)

cfoption icewm_preferences[] = {
    OBV("ClickToFocus",                         &clickFocus,                    "Focus windows by clicking"),
    OBV("FocusOnAppRaise",                      &focusOnAppRaise,               "Focus windows when application requests to raise"),
    OBV("RaiseOnFocus",                         &raiseOnFocus,                  "Raise windows when focused"),
    OBV("FocusOnClickClient",                   &focusOnClickClient,            "Focus window when client area clicked"),
    OBV("RaiseOnClickClient",                   &raiseOnClickClient,            "Raise window when client area clicked"),
    OBV("RaiseOnClickTitleBar",                 &raiseOnClickTitleBar,          "Raise window when title bar is clicked"),
    OBV("RaiseOnClickButton",                   &raiseOnClickButton,            "Raise window when frame button is clicked"),
    OBV("RaiseOnClickFrame",                    &raiseOnClickFrame,             "Raise window when frame border is clicked"),
    OBV("LowerOnClickWhenRaised",               &lowerOnClickWhenRaised,        "Lower the active window when clicked again"),
    OBV("PassFirstClickToClient",               &passFirstClickToClient,        "Pass focusing click on client area to client"),
    OBV("FocusChangesWorkspace",                &focusChangesWorkspace,         "Change to the workspace of newly focused windows"),
    OBV("FocusOnMap",                           &focusOnMap,                    "Focus normal window when initially mapped"),
    OBV("FocusOnMapTransient",                  &focusOnMapTransient,           "Focus dialog window when initially mapped"),
    OBV("FocusOnMapTransientActive",            &focusOnMapTransientActive,     "Focus dialog window when initially mapped only if parent frame focused"),
    OBV("PointerColormap",                      &pointerColormap,               "Colormap focus follows pointer"),
    OBV("DontRotateMenuPointer",                &dontRotateMenuPointer,         "Don't rotate the cursor for popup menus"),
    OBV("LimitSize",                            &limitSize,                     "Limit size of windows to screen"),
    OBV("LimitPosition",                        &limitPosition,                 "Limit position of windows to screen"),
    OBV("LimitByDockLayer",                     &limitByDockLayer,              "Let the Dock layer limit the workspace (incompatible with GNOME Panel)"),
    OBV("ConsiderHBorder",                      &considerHorizBorder,           "Consider border frames when maximizing horizontally"),
    OBV("ConsiderVBorder",                      &considerVertBorder,            "Consider border frames when maximizing vertically"),
    OBV("CenterMaximizedWindows",               &centerMaximizedWindows,        "Center maximized windows which can't fit the screen (like terminals)"),
    OBV("SizeMaximized",                        &sizeMaximized,                 "Maximized windows can be resized"),
    OBV("ShowMoveSizeStatus",                   &showMoveSizeStatus,            "Show position status window during move/resize"),
    OBV("ShowWorkspaceStatus",                  &workspaceSwitchStatus,         "Show name of current workspace while switching"),
    OBV("MinimizeToDesktop",                    &minimizeToDesktop,             "Display mini-icons on desktop for minimized windows"),
    OBV("StrongPointerFocus",                   &strongPointerFocus,            "Always maintain focus under mouse window (makes some keyboard support non-functional or unreliable"),
    OBV("OpaqueMove",                           &opaqueMove,                    "Opaque window move"),
    OBV("OpaqueResize",                         &opaqueResize,                  "Opaque window resize"),
    OBV("ManualPlacement",                      &manualPlacement,               "Windows initially placed manually by user"),
    OBV("SmartPlacement",                       &smartPlacement,                "Smart window placement (minimal overlap)"),
    OBV("CenterTransientsOnOwner",              &centerTransientsOnOwner,       "Center dialogs on owner window"),
    OBV("MenuMouseTracking",                    &menuMouseTracking,             "Menus track mouse even with no mouse buttons held"),
    OBV("AutoRaise",                            &autoRaise,                     "Auto raise windows after delay"),
    OBV("DelayPointerFocus",                    &delayPointerFocus,             "Delay pointer focusing when mouse moves"),
    OBV("Win95Keys",                            &win95keys,                     "Support win95 keyboard keys (Penguin/Meta/Win_L,R shows menu)"),
    OBV("ModSuperIsCtrlAlt",                     &modSuperIsCtrlAlt,            "Treat Super/Win modifier as Ctrl+Alt"),
    OBV("UseMouseWheel",                        &useMouseWheel,                 "Support mouse wheel"),
    OBV("ShowPopupsAbovePointer",               &showPopupsAbovePointer,        "Show popup menus above mouse pointer"),
    OBV("ReplayMenuCancelClick",                &replayMenuCancelClick,         "Send the clicks outside menus to target window"),
    OBV("QuickSwitch",                          &quickSwitch,                   "Alt+Tab window switching"),
    OBV("QuickSwitchToMinimized",               &quickSwitchToMinimized,        "Alt+Tab to minimized windows"),
    OBV("QuickSwitchToHidden",                  &quickSwitchToHidden,           "Alt+Tab to hidden windows"),
    OBV("QuickSwitchToAllWorkspaces",           &quickSwitchToAllWorkspaces,    "Alt+Tab to windows on other workspaces"),
    OBV("QuickSwitchGroupWorkspaces",           &quickSwitchGroupWorkspaces,    "Alt+Tab: group windows on current workspace"),
    OBV("QuickSwitchAllIcons",                  &quickSwitchAllIcons,           "Show all reachable icons when quick switching"),
    OBV("QuickSwitchTextFirst",                 &quickSwitchTextFirst,          "Show the window title above (all reachable) icons"),
    OBV("QuickSwitchSmallWindow",               &quickSwitchSmallWindow,        "Attempt to create a small QuickSwitch window (1/3 instead of 3/5 of screen width)"),
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
#ifdef CONFIG_TASKBAR
    OBV("ShowTaskBar",                          &showTaskBar,                   "Show task bar"),
    OBV("TaskBarAtTop",                         &taskBarAtTop,                  "Task bar at top of the screen"),
    OBV("TaskBarKeepBelow",                     &taskBarKeepBelow,              "Keep the task bar below regular windows"),
    OBV("TaskBarAutoHide",                      &taskBarAutoHide,               "Auto hide task bar after delay"),
    OBV("TaskBarShowClock",                     &taskBarShowClock,              "Show clock on task bar"),
    OBV("TaskBarShowAPMStatus",                 &taskBarShowApm,                "Show APM/ACPI/Battery/Power status monitor on task bar"),
    OBV("TaskBarShowAPMTime",                   &taskBarShowApmTime,            "Show APM status on task bar in time-format"),  // mschy
    OBV("TaskBarShowMailboxStatus",             &taskBarShowMailboxStatus,      "Show mailbox status on task bar"),
    OBV("TaskBarMailboxStatusBeepOnNewMail",    &beepOnNewMail,                 "Beep when new mail arrives"),
    OBV("TaskBarMailboxStatusCountMessages",    &countMailMessages,             "Count messages in mailbox"),
    OBV("TaskBarShowWorkspaces",                &taskBarShowWorkspaces,         "Show workspace switching buttons on task bar"),
    OBV("TaskBarShowWindows",                   &taskBarShowWindows,            "Show windows on the taskbar"),
    OBV("TaskBarShowShowDesktopButton",         &taskBarShowShowDesktopButton,  "Show 'show desktop' button on taskbar"),
#ifdef CONFIG_TRAY
    OBV("TaskBarShowTray",                      &taskBarShowTray,               "Show windows in the tray"),
    OBV("TrayShowAllWindows",                   &trayShowAllWindows,            "Show windows from all workspaces on tray"),
#endif
    OBV("TaskBarShowAllWindows",                &taskBarShowAllWindows,         "Show windows from all workspaces on task bar"),
    OBV("TaskBarShowWindowIcons",               &taskBarShowWindowIcons,        "Show icons of windows on the task bar"),
    OBV("TaskBarShowStartMenu",                 &taskBarShowStartMenu,          "Show 'Start' menu on task bar"),
    OBV("TaskBarShowWindowListMenu",            &taskBarShowWindowListMenu,     "Show 'window list' menu on task bar"),
    OBV("TaskBarShowCPUStatus",                 &taskBarShowCPUStatus,          "Show CPU status on task bar (Linux             & Solaris)"),
    OBV("TaskBarShowNetStatus",                 &taskBarShowNetStatus,          "Show network status on task bar (Linux only)"),
    OBV("TaskBarShowCollapseButton",            &taskBarShowCollapseButton,     "Show a button to collapse the taskbar"),
    OBV("TaskBarDoubleHeight",                  &taskBarDoubleHeight,           "Use double-height task bar"),
    OBV("TaskBarWorkspacesLeft",                &taskBarWorkspacesLeft,         "Place workspace pager on left, not right"),
    OBV("TaskBarLaunchOnSingleClick",           &taskBarLaunchOnSingleClick,    "Execute taskbar applet commands (like MailCommand,     ClockCommand,   ...) on single click"),
#endif
    OBV("WarpPointer",                          &warpPointer,                   "Move mouse when doing focusing in pointer focus mode"),
    OBV("ClientWindowMouseActions",             &clientMouseActions,            "Allow mouse actions on client windows (buggy with some programs)"),
    OBV("ShowProgramsMenu",                     &showPrograms,                  "Show programs submenu"),
    OBV("ShowThemesMenu",                       &showThemesMenu,                "Show themes submenu"),
    OBV("ShowLogoutMenu",                       &showLogoutMenu,                "Show logout submenu"),
    OBV("ShowHelp",                             &showHelp,                      "Show the help menu item"),
    OBV("ShowLogoutSubMenu",                    &showLogoutSubMenu,             "Show logout submenu"),
    OBV("ShowAbout",                            &showAbout,                     "Show the about menu item"),
    OBV("ShowRun",                              &showRun,                       "Show the run menu item"),
    OBV("ShowWindowList",                       &showWindowList,                "Show the window menu item"),
    OBV("AllowFullscreen",                      &allowFullscreen,               "Allow to switch a window to fullscreen"),
    OBV("EnableAddressBar",                     &enableAddressBar,              "Enable address bar functionality in taskbar"),
    OBV("ShowAddressBar",                       &showAddressBar,                "Show address bar in task bar"),
#ifdef CONFIG_I18N
    OBV("MultiByte",                            &multiByte,                     "Overrides automatic multiple byte detection"),
#endif
    OBV("ConfirmLogout",                        &confirmLogout,                 "Confirm logout"),
#ifdef CONFIG_SHAPED_DECORATION
    OBV("ShapesProtectClientWindow",            &protectClientWindow,           "Don't cut client windows by shapes set trough frame corner pixmap"),
#endif
    OBV("DoubleBuffer",                         &doubleBuffer,                  "Use double buffering when redrawing the display"),
    OIV("ClickMotionDistance",                  &ClickMotionDistance, 0, 32,    "Pointer motion distance before click gets interpreted as drag"),
    OIV("ClickMotionDelay",                     &ClickMotionDelay, 0, 2000,     "Delay before click gets interpreted as drag"),
    OIV("MultiClickTime",                       &MultiClickTime, 0, 5000,       "Multiple click time"),
    OIV("MenuActivateDelay",                    &MenuActivateDelay, 0, 5000,    "Delay before activating menu items"),
    OIV("SubmenuMenuActivateDelay",             &SubmenuActivateDelay, 0, 5000, "Delay before activating menu submenus"),
    OIV("MenuMaximalWidth",                     &MenuMaximalWidth, 0, 16384,    "Maximal width of popup menus,  2/3 of the screen's width if set to zero"),
#ifdef CONFIG_TOOLTIP
    OIV("ToolTipDelay",                         &ToolTipDelay, 0, 5000,         "Delay before tooltip window is displayed"),
    OIV("ToolTipTime",                          &ToolTipTime, 0, 60000,         "Time before tooltip window is hidden (0 means never"),
#endif
    OIV("AutoHideDelay",                        &autoHideDelay, 0, 5000,        "Delay before task bar is hidden"),
    OIV("AutoShowDelay",                        &autoShowDelay, 0, 5000,        "Delay before task bar is shown"),
    OIV("AutoRaiseDelay",                       &autoRaiseDelay, 0, 5000,       "Delay before windows are auto raised"),
    OIV("EdgeResistance",                       &EdgeResistance, 0, 10000,      "Resistance in pixels when trying to move windows off the screen (10000 = infinite)"),
    OIV("PointerFocusDelay",                    &pointerFocusDelay, 0, 1000,    "Delay for pointer focus switching"),
    OIV("SnapDistance",                         &snapDistance, 0, 64,           "Distance in pixels before windows snap together"),
    OIV("EdgeSwitchDelay",                      &edgeSwitchDelay, 0, 5000,      "Screen edge workspace switching delay"),
    OIV("ScrollBarStartDelay",                  &scrollBarStartDelay, 0, 5000,  "Inital scroll bar autoscroll delay"),
    OIV("ScrollBarDelay",                       &scrollBarDelay, 0, 5000,       "Scroll bar autoscroll delay"),
    OIV("AutoScrollStartDelay",                 &autoScrollStartDelay, 0, 5000, "Auto scroll start delay"),
    OIV("AutoScrollDelay",                      &autoScrollDelay, 0, 5000,      "Auto scroll delay"),
    OIV("WorkspaceStatusTime",                  &workspaceStatusTime, 0, 2500,  "Time before workspace status window is hidden"),
    OIV("UseRootButtons",                       &useRootButtons, 0, 255,        "Bitmask of root window button click to use in window manager"),
    OIV("ButtonRaiseMask",                      &buttonRaiseMask, 0, 255,       "Bitmask of buttons that raise the window when pressed"),
    OIV("DesktopWinMenuButton",                 &rootWinMenuButton, 0, 20,      "Desktop mouse-button click to show the window list menu"),
    OIV("DesktopWinListButton",                 &rootWinListButton, 0, 20,       "Desktop mouse-button click to show the window list"),
    OIV("DesktopMenuButton",                    &rootMenuButton, 0, 20,         "Desktop mouse-button click to show the root menu"),
    OIV("TitleBarMaximizeButton",               &titleMaximizeButton, 0, 5,     "TitleBar mouse-button double click to maximize the window"),
    OIV("TitleBarRollupButton",                 &titleRollupButton, 0, 5,       "TitleBar mouse-button double click to rollup the window"),
    OIV("MsgBoxDefaultAction",                  &msgBoxDefaultAction, 0, 1,     "Preselect to Cancel (0) or the OK (1) button in message boxes"),
    OIV("MailCheckDelay",                       &mailCheckDelay, 0, (3600*24),  "Delay between new-mail checks. (seconds)"),
#ifdef CONFIG_TASKBAR
    OIV("TaskBarCPUSamples",                    &taskBarCPUSamples, 2, 1000,    "Width of CPU Monitor"),
    OIV("TaskBarCPUDelay",                      &taskBarCPUDelay, 10, (60*60*1000),    "Delay between CPU Monitor samples in ms"),
    OIV("TaskBarNetSamples",                    &taskBarNetSamples, 2, 1000,    "Width of Net Monitor"),
    OIV("TaskBarNetDelay",                      &taskBarNetDelay, 10, (60*60*1000),    "Delay between Net Monitor samples in ms"),
#endif

    OIV("XineramaPrimaryScreen",                &xineramaPrimaryScreen, 0, 63, "Primary screen for xinerama (taskbar, ...)"),
    OIV("FocusRequestFlashTime",                &focusRequestFlashTime, 0, (3600 * 24), "Number of seconds the taskbar app will blink when requesting focus"),
    OIV("NestedThemeMenuMinNumber",             &nestedThemeMenuMinNumber,  0, 1234,  "Minimal number of themes after which the Themes menu becomes nested (0=disabled)"),
///    OSV("Theme",                                &themeName,                     "Theme name"),
    OSV("IconPath",                             &iconPath,                      "Icon search path (colon separated)"),
    OSV("MailBoxPath",                          &mailBoxPath,                   "Mailbox path (use $MAIL instead)"),
    OSV("MailCommand",                          &mailCommand,                   "Command to run on mailbox"),
    OSV("MailClassHint",                        &mailClassHint,                 "WM_CLASS to allow runonce for MailCommand"),
    OSV("NewMailCommand",                       &newMailCommand,                "Command to run when new mail arrives"),
    OSV("LockCommand",                          &lockCommand,                   "Command to lock display/screensaver"),
    OSV("ClockCommand",                         &clockCommand,                  "Command to run on clock"),
    OSV("ClockClassHint",                       &clockClassHint,                "WM_CLASS to allow runonce for ClockCommand"),
    OSV("RunCommand",                           &runDlgCommand,                 "Command to select and run a program"),
    OSV("OpenCommand",                          &openCommand,                   ""),
    OSV("TerminalCommand",                      &terminalCommand,               "Terminal emulator must accept -e option."),
    OSV("LogoutCommand",                        &logoutCommand,                 "Command to start logout"),
    OSV("LogoutCancelCommand",                  &logoutCancelCommand,           "Command to cancel logout"),
    OSV("ShutdownCommand",                      &shutdownCommand,               "Command to shutdown the system"),
    OSV("RebootCommand",                        &rebootCommand,                 "Command to reboot the system"),
    OSV("CPUStatusCommand",                     &cpuCommand,                    "Command to run on CPU status"),
    OSV("CPUStatusClassHint",                   &cpuClassHint,                  "WM_CLASS to allow runonce for CPUStatusCommand"),
    OSV("NetStatusCommand",                     &netCommand,                    "Command to run on Net status"),
    OSV("NetStatusClassHint",                   &netClassHint,                  "WM_CLASS to allow runonce for NetStatusCommand"),
    OSV("AddressBarCommand",                    &addressBarCommand,             "Command to run for address bar entries"),
    OSV("NetworkStatusDevice",                  &netDevice,                     "Network device to show status for"),
    OSV("TimeFormat",                           &fmtTime,                       "Clock Time format (strftime format string)"),
    OSV("TimeFormatAlt",                        &fmtTimeAlt,                    "Alternate Clock Time format (e.g. for blinking effects)"),
    OSV("DateFormat",                           &fmtDate,                       "Clock Date format for tooltip (strftime format string)"),
#ifdef CONFIG_APPLET_APM
    OSV("AcpiIgnoreBatteries",                  &acpiIgnoreBatteries,           "List of battery names (directories) in /proc/acpi/battery to ignore. Useful when more slots are built-in, but only one battery is used"),
#endif


#ifndef NO_KEYBIND
    OKV("MouseWinMove",                         gMouseWinMove,                  "Mouse binding for window move"),
    OKV("MouseWinSize",                         gMouseWinSize,                  "Mouse binding for window resize"),
    OKV("KeyWinRaise",                          gKeyWinRaise,                   ""),
    OKV("KeyWinOccupyAll",                      gKeyWinOccupyAll,               ""),
    OKV("KeyWinLower",                          gKeyWinLower,                   ""),
    OKV("KeyWinClose",                          gKeyWinClose,                   ""),
    OKV("KeyWinRestore",                        gKeyWinRestore,                 ""),
    OKV("KeyWinPrev",                           gKeyWinPrev,                    ""),
    OKV("KeyWinNext",                           gKeyWinNext,                    ""),
    OKV("KeyWinMove",                           gKeyWinMove,                    ""),
    OKV("KeyWinSize",                           gKeyWinSize,                    ""),
    OKV("KeyWinMinimize",                       gKeyWinMinimize,                ""),
    OKV("KeyWinMaximize",                       gKeyWinMaximize,                ""),
    OKV("KeyWinMaximizeVert",                   gKeyWinMaximizeVert,            ""),
    OKV("KeyWinMaximizeHoriz",                  gKeyWinMaximizeHoriz,           ""),
    OKV("KeyWinFullscreen",                     gKeyWinFullscreen,              ""),
    OKV("KeyWinHide",                           gKeyWinHide,                    ""),
    OKV("KeyWinRollup",                         gKeyWinRollup,                  ""),
    OKV("KeyWinMenu",                           gKeyWinMenu,                    ""),
    OKV("KeyWinArrangeN",                       gKeyWinArrangeN,                ""),
    OKV("KeyWinArrangeNE",                      gKeyWinArrangeNE,               ""),
    OKV("KeyWinArrangeE",                       gKeyWinArrangeE,                ""),
    OKV("KeyWinArrangeSE",                      gKeyWinArrangeSE,               ""),
    OKV("KeyWinArrangeS",                       gKeyWinArrangeS,                ""),
    OKV("KeyWinArrangeSW",                      gKeyWinArrangeSW,               ""),
    OKV("KeyWinArrangeW",                       gKeyWinArrangeW,                ""),
    OKV("KeyWinArrangeNW",                      gKeyWinArrangeNW,               ""),
    OKV("KeyWinArrangeC",                       gKeyWinArrangeC,                ""),
    OKV("KeySysSwitchNext",                     gKeySysSwitchNext,              ""),
    OKV("KeySysSwitchLast",                     gKeySysSwitchLast,              ""),
    OKV("KeySysWinNext",                        gKeySysWinNext,                 ""),
    OKV("KeySysWinPrev",                        gKeySysWinPrev,                 ""),
    OKV("KeySysWinMenu",                        gKeySysWinMenu,                 ""),
    OKV("KeySysDialog",                         gKeySysDialog,                  ""),
    OKV("KeySysMenu",                           gKeySysMenu,                    ""),
///    OKV("KeySysRun",                            gKeySysRun,                     ""),
    OKV("KeySysWindowList",                     gKeySysWindowList,              ""),
    OKV("KeySysWinListMenu",                    gKeySysWinListMenu,             ""),
    OKV("KeySysAddressBar",                     gKeySysAddressBar,              ""),
    OKV("KeySysWorkspacePrev",                  gKeySysWorkspacePrev,           ""),
    OKV("KeySysWorkspaceNext",                  gKeySysWorkspaceNext,           ""),
    OKV("KeySysWorkspaceLast",                  gKeySysWorkspaceLast,           ""),
    OKV("KeySysWorkspacePrevTakeWin",           gKeySysWorkspacePrevTakeWin,    ""),
    OKV("KeySysWorkspaceNextTakeWin",           gKeySysWorkspaceNextTakeWin,    ""),
    OKV("KeySysWorkspaceLastTakeWin",           gKeySysWorkspaceLastTakeWin,    ""),
    OKV("KeySysWorkspace1",                     gKeySysWorkspace1,              ""),
    OKV("KeySysWorkspace2",                     gKeySysWorkspace2,              ""),
    OKV("KeySysWorkspace3",                     gKeySysWorkspace3,              ""),
    OKV("KeySysWorkspace4",                     gKeySysWorkspace4,              ""),
    OKV("KeySysWorkspace5",                     gKeySysWorkspace5,              ""),
    OKV("KeySysWorkspace6",                     gKeySysWorkspace6,              ""),
    OKV("KeySysWorkspace7",                     gKeySysWorkspace7,              ""),
    OKV("KeySysWorkspace8",                     gKeySysWorkspace8,              ""),
    OKV("KeySysWorkspace9",                     gKeySysWorkspace9,              ""),
    OKV("KeySysWorkspace10",                    gKeySysWorkspace10,             ""),
    OKV("KeySysWorkspace11",                    gKeySysWorkspace11,             ""),
    OKV("KeySysWorkspace12",                    gKeySysWorkspace12,             ""),
    OKV("KeySysWorkspace1TakeWin",              gKeySysWorkspace1TakeWin,       ""),
    OKV("KeySysWorkspace2TakeWin",              gKeySysWorkspace2TakeWin,       ""),
    OKV("KeySysWorkspace3TakeWin",              gKeySysWorkspace3TakeWin,       ""),
    OKV("KeySysWorkspace4TakeWin",              gKeySysWorkspace4TakeWin,       ""),
    OKV("KeySysWorkspace5TakeWin",              gKeySysWorkspace5TakeWin,       ""),
    OKV("KeySysWorkspace6TakeWin",              gKeySysWorkspace6TakeWin,       ""),
    OKV("KeySysWorkspace7TakeWin",              gKeySysWorkspace7TakeWin,       ""),
    OKV("KeySysWorkspace8TakeWin",              gKeySysWorkspace8TakeWin,       ""),
    OKV("KeySysWorkspace9TakeWin",              gKeySysWorkspace9TakeWin,       ""),
    OKV("KeySysWorkspace10TakeWin",             gKeySysWorkspace10TakeWin,      ""),
    OKV("KeySysWorkspace11TakeWin",             gKeySysWorkspace11TakeWin,      ""),
    OKV("KeySysWorkspace12TakeWin",             gKeySysWorkspace12TakeWin,      ""),
    OKV("KeySysTileVertical",                   gKeySysTileVertical,            ""),
    OKV("KeySysTileHorizontal",                 gKeySysTileHorizontal,          ""),
    OKV("KeySysCascade",                        gKeySysCascade,                 ""),
    OKV("KeySysArrange",                        gKeySysArrange,                 ""),
    OKV("KeySysArrangeIcons",                   gKeySysArrangeIcons,            ""),
    OKV("KeySysMinimizeAll",                    gKeySysMinimizeAll,             ""),
    OKV("KeySysHideAll",                        gKeySysHideAll,                 ""),
    OKV("KeySysUndoArrange",                    gKeySysUndoArrange,             ""),
    OKV("KeySysShowDesktop",                    gKeySysShowDesktop,             ""),
#endif
    OKF("WorkspaceNames", addWorkspace, ""),
    OK0()
};

#endif

#include "themable.h"
#endif /* __DEFAULT_H */
