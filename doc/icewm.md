---
title: IceWM Manual
---

Introduction
============

This document is the manual for the **IceWM** X11 window manager.
The [IceWM manpages](https://ice-wm.org/man/) provide additional documentation.

The goal of IceWM is to provide a small, fast and familiar window manager for the X11 window system, which is compatible with the EWMH and ICCCM window manager protocols. Generally, it tries to make all functions available both by keyboard and by mouse (this is not currently possible when using mouse focus).

IceWM originally was designed to emulate the look of Motif, OS/2 Warp 4, OS/2 Warp 3 and Windows 95. Since it has a theming engine others styles are possible. A large number of original themes have been created and [published by users](https://themes.ice-wm.org/).

Further information can be found at [the IceWM website](https://ice-wm.org/),
at the [current site of development](https://github.com/bbidulock/icewm/),
which includes the [support requests](https://github.com/bbidulock/icewm/issues/).

Copying
=======

IceWM and its documentation are covered by a GNU LGPL license,
which is included in the distribution.

First steps
===========

IceWM components
----------------

The IceWM suite consists of the following core applications provided by the main package:

- **[icewm](https://ice-wm.org/man/icewm)** - The actual window manager binary. It handles window placement and draws the window decorations.

- **[icewmbg](https://ice-wm.org/man/icewmbg)** - The background setting application. It can assign plain background color or images in different formats to the X background. Either shared or separate for different workspaces. This program should be started before `icewm`.

- **[icewm-session](https://ice-wm.org/man/icewm-session)** - The IceWM session manager runs all of the above. This is the preferred program to start IceWM.

- **[icewm-menu-fdo](https://ice-wm.org/man/icewm-menu-fdo)** - This generates IceWM program menus from FreeDesktop `.desktop` files.

- **[icewmhint](https://ice-wm.org/man/icewmhint)** - Is a simple utility for passing IceWM hints to IceWM by window class and instance. Icewmhint uses a special property, '"\_ICEWM\_WINOPHINT"', on the root window to pass special hints to IceWM.

- **[icehelp](https://ice-wm.org/man/icehelp)** - Is used by icewm to display the 'IceWM manual' and the manpages. See the output of `icehelp --help` for details.

- **[icesh](https://ice-wm.org/man/icesh)** - Could be used to manage IceWM internals from the command line.

Starting icewm
--------------

The `icewm` program alone is suitable for use with desktop environments like GNOME. If you wish to run the whole IceWM suite (WM, background changer, Docklet support, and startup/shutdown script handling), use the `icewm-session` binary instead of pure `icewm`. Note that this is not a complete Session Manager but it helps to automate the startup.

First make sure that you choose the correct [X startup](https://www.tldp.org/HOWTO/XWindow-User-HOWTO/runningx.html) script in your home directory. For most distributions either the file `$HOME/.xsession` or `$HOME/.xinitrc` is honored by startx and X display managers like KDM. On RedHat, the `$HOME/.Xclients` may be used instead. In all cases, choose the one recommended by your distribution and make sure that there is no concurrency between the X startup scripts. Ensure that the script is executable. Mine looks something like this:

    #!/bin/bash
    # run profile to set $PATH and other env vars correctly
    . $HOME/.bash_profile
    # setup touchpad and the external mouse
    xset m 7 2
    xinput set-ptr-feedback 0 7 1.9 1

    # start icewm-session
    exec icewm-session
    xterm

The xterm on the last line is there simply to make sure that your X session doesn't crash if IceWM does (should never happen). You can restart IceWM from there or start some other window manager. The session will close if you close the xterm.

Startup and shutdown scripts
----------------------------

After initialization `icewm-session` will search the resource path for a `startup` script. If this file is found and if it is executable `icewm-session` will run this script. During termination of `icewm-session` the `shutdown` script is executed.

Example `~/.icewm/startup` script:

    #!/bin/bash

    [ -x ~/.icewm/restart ] && source ~/.icewm/restart

    gnome-terminal --geometry 80x25+217+235 &
    xscreensaver &

On termination the `shutdown` script will be run first, then `icewm-session` will terminate icewm and icewmbg.

Hint: `icewm-session` is meant for easy desktop initialization and it is part of IceWM due to popular demand. For more sophisticated session management one could use a real session manager. IceWM supports the XSESSION protocol.

Extra session environment
-------------------------

Please note that if icewm-session is used as the only startup mechanism (without having .xsession involved), one can write additional environment settings into the file `$HOME/.icewm/env`. Expansion of simple shell style variables should be supported on most platforms. Shell command expansion is supported if `wordexp` was configured. This extra environment is only effective in applications started by icewm-session and their subprocesses.

Example `env`:

    PATH=~/bin:$PATH
    LANG=de_DE.UTF-8

Focus models
============

IceWM implements four general focus models:

- **clickToRaise**

  Exactly like Win95, OS/2 Warp. When window is clicked with a mouse, it is raised and activated.

- **clickToFocus**

  Window is raised and focused when titlebar or frame border is clicked. Window is focused but not raised when window interior is clicked.

- **pointerFocus**

  When the mouse is moved, focus is set to window under a mouse. It should be possible to change focus with the keyboard when mouse is not moved.

- **explicitFocus**

  When a window is clicked, it is activated, but not raised. New windows do not automatically get the focus unless they are transient windows for the active window.

Detailed configuration is possible using the configuration file options.

Mouse Commands
==============

Frame Commands
--------------

- **Left Button**

  Select and raise the window. On the window frame, resize the window.

- **Right Button**

  When dragged, moves the window. When clicked, displays the context menu.

Title Bar Commands
------------------

- **Any Button Drag**

  Move the window.

- **Alt + Left Button**

  Lower the window.

- **Left Button Double Click**

  Maximize/Restore the window.

- **Middle Button Double Click**

  Rollup/Unroll the window.

The Ctrl key can be used together with a mouse button to prevent a window from being raised to the top of the stack.

Taskbar commands
----------------

- **Left Button Click**

  Activate the workspace with the window and raise the window. Toggles the minimized/active state of the window.

- **Shift + Left Button Click**

  Move window to current workspace. This only works when windows from all workspaces are shown on the taskbar all the time.

- **Control + Left Button Click**

  Minimize/restore the window.

- **Middle Button Click**

  Toggle raised/lowered state of the window.

- **Shift + Middle Button Click**

  Add the window to the current workspace.

- **Control + Middle Button Click**

  Lower the window.

- **Right Button Click**

  Display a context menu.

Keyboard Commands
=================

The Alt key is assumed to be the key defined as the Mod1 modifier.

- `Alt+F1`

  Raise the window.

- `Alt+F2`

  Make a window occupy all desktops.

- `Alt+F3`

  Lower the window to the bottom of the stack.

- `Alt+F4`

  Close the window.

- `Alt+F5`

  Restore the window state if maximized or minimized/hidden.

- `Alt+F6`

  Focus to next window.

- `Alt+Shift+F6`

  Focus to previous window.

- `Alt+F7`

  Starts movement of the active window. Move the window either by the mouse or by the arrow keys. The arrow keys can be accelerated four times by the Shift key or sixteen times by the Control key. Press the left button or the Enter key when done. To cancel press Escape.

- `Alt+F8`

  Starts resizing of the active window. Resize the window either by the mouse or by the arrow keys. The arrow keys can be accelerated four times by the Shift key or sixteen times by the Control key. Press the left button or the Enter key when done. To cancel press Escape.

- `Alt+F9`

  Minimize the window to taskbar.

- `Alt+F10`

  Maximize the window.

- `Alt+Shift+F10`

  Maximize the window vertically (toggle).

- `Alt+F11`

  Hide the window (appears in window list, but not on taskbar).

- `Alt+F12`

  Rollup the window.

- `Ctrl+Escape`

  Show the start menu.

- `Ctrl+Alt+Escape`

  Show the window list.

- `Shift+Escape`

  Show the system-menu of the window.

- `Alt+Escape`

  Focus to next window (down in zorder)

- `Alt+Shift+Escape`

  Focus to previous window (up in zorder)

- `Alt+Tab`

  Switch between windows (top -> bottom).

- `Alt+Shift+Tab`

  Switch between windows (bottom <- top).

- `Ctrl+Alt+LeftArrow`

  Switch to the previous workspace (cycle).

- `Ctrl+Alt+RightArrow`

  Switch to the next workspace (cycle).

- `Ctrl+Alt+DownArrow`

  Switch to the previously active workspace.

- `Ctrl+Alt+Shift+LeftArrow`

  Move the focused window to the previous workspace and activate it.

- `Ctrl+Alt+Shift+RightArrow`

  Move the focused window to the next workspace and activate it.

- `Ctrl+Alt+Shift+DownArrow`

  Move the focused window to the previously active workspace and activate it.

- `Ctrl+Alt+Delete`

  displays the session dialog.

- `Ctrl+Alt+Space`

  Activate the AddressBar. This is a command line in the taskbar where a shell command can be typed. Pressing the Enter key will execute the command. If **AddressBarCommand** was configured it will be used to execute the command otherwise `/bin/sh` is used. If the **Control** key was also pressed then the command is executed in a terminal as given by **TerminalCommand**. The address bar maintains a history which is navigable by the Up and Down keys. A rich set of editing operations is supported, including cut-/copy-/paste-operations and file completion using **Tab** or **Ctrl-I**.

- `Ctrl+Alt+d`

  Show the desktop.

- `Ctrl+Alt+h`

  Toggle taskbar visibility.

The Resource Path
=================

IceWM looks in several locations for configuration information, themes and customization; together these locations are called the resource path. The resource path contains the following directories:

$ICEWM\_PRIVCFG; $XDG\_CONFIG\_HOME/icewm; $HOME/.icewm
The first of these which is defined and existent is used to search for the user's personal customization.

/etc/icewm
the system-wide defaults directory

/usr/share/icewm OR /usr/local/share/icewm
the compiled-in default directory with default files

The directories are searched in the above order, so any file located in the system/install directory can be overridden by the user by creating the same directory hierarchy under `$HOME/.icewm`.

To customize icewm yourself, you could create the `$HOME/.icewm` directory and copy the files that you wish to modify, like `preferences`, `keys` or `winoptions`, from `/etc/icewm`, `/usr/share/icewm` or `/usr/local/share/icewm` and then modify as you like.

To customize the default themes, create the `$HOME/.icewm/themes` directory and copy all the theme files there and then modify as necessary. Each theme has its own subdirectory. Themes can be downloaded from <https://themes.ice-wm.org>.

Configuration Files
===================

IceWM uses the following configuration files:

- **[theme](https://ice-wm.org/man/icewm-theme)**

  Currently selected theme

- **[preferences](https://ice-wm.org/man/icewm-preferences)**

  General settings - paths, colors, fonts...

- **[prefoverride](https://ice-wm.org/man/icewm-prefoverride)**

  Settings that should override the themes.

- **[menu](https://ice-wm.org/man/icewm-menu)**

  Menu of startable applications. Usually customized by the user.

- **[programs](https://ice-wm.org/man/icewm-programs)**

  Automatically generated menu of startable applications (this should be used for **wmconfig**, **menu** or similar packages, perhaps as a part of the login or X startup sequence).

- **[winoptions](https://ice-wm.org/man/icewm-winoptions)**

  Application window options

- **[keys](https://ice-wm.org/man/icewm-keys)**

  Global keybindings to launch applications (not window manager related)

- **[toolbar](https://ice-wm.org/man/icewm-toolbar)**

  Quick launch application icons on the taskbar.

Theme
=====

The `theme` file contains the currently selected theme. It will be overwritten automatically when a theme is selected from the Themes menu.

Preferences
===========

This section shows preferences that can be set in `preferences`. Themes will not be able to override these settings. Default values are shown following the equal sign.

Focus and Behavior
------------------

The following settings can be set to value 1 (enabled) or value 0 (disabled).

- `ClickToFocus = 1`

  Enables click to focus mode.

- `RaiseOnFocus = 1`

  Window is raised when focused.

- `FocusOnClickClient = 1`

  Window is focused when client area is clicked.

- `RaiseOnClickClient = 1`

  Window is raised when client area is clicked.

- `RaiseOnClickTitleBar = 1`

  Window is raised when titlebar is clicked.

- `RaiseOnClickButton = 1`

  Window is raised when title bar button is clicked.

- `RaiseOnClickFrame = 1`

  Window is raised when frame is clicked.

- `LowerOnClickWhenRaised = 0`

  Lower the active window when clicked again.

- `PassFirstClickToClient = 1`

  The click which raises the window is also passed to the client.

- `FocusChangesWorkspace = 0`

  Change to the workspace of newly focused windows.

- `AutoRaise = 0`

  Windows will raise automatically after AutoRaiseDelay when focused.

- `StrongPointerFocus = 0`

  When focus follows mouse always give the focus to the window under mouse pointer - Even when no mouse motion has occurred. Using this is not recommended. Please prefer to use just ClickToFocus=0.

- `FocusOnMap = 1`

  Window is focused after being mapped.

- `FocusOnMapTransient = 1`

  Transient window is focused after being mapped.

- `FocusOnMapTransientActive = 1`

  Focus dialog window when initially mapped only if parent frame focused.

- `FocusOnAppRaise = 1`

  The window is focused when application raises it.

- `RequestFocusOnAppRaise = 1`

  Request focus (flashing in taskbar) when application requests raise.

- `MapInactiveOnTop = 1`

  Put new windows on top even if not focusing them.

- `PointerColormap = 0`

  Colormap focus follows pointer.

- `DontRotateMenuPointer = 1`

  Don't rotate the cursor for popup menus.

- `LimitSize = 1`

  Limit size of windows to screen.

- `LimitPosition = 1`

  Limit position of windows to screen.

- `LimitByDockLayer = 0`

  Let the Dock layer limit the workspace (incompatible with GNOME Panel).

- `ConsiderHBorder = 0`

  Consider border frames when maximizing horizontally.

- `ConsiderVBorder = 0`

  Consider border frames when maximizing vertically.

- `ConsiderSizeHintsMaximized = 1`

  Consider XSizeHints if frame is maximized.

- `CenterMaximizedWindows = 0`

  Center maximized windows which can't fit the screen (like terminals).

- `HideBordersMaximized = 0`

  Hide window borders if window is maximized.

- `HideTitleBarWhenMaximized = 0`

  Hide title bar when maximized.

- `CenterLarge = 0`

  Center large windows.

- `CenterTransientsOnOwner = 1`

  Center dialogs on owner window.

- `SizeMaximized = 0`

  Window can be resized when maximized.

- `MinimizeToDesktop = 0`

  Window is minimized to desktop area (in addition to the taskbar).

- `MiniIconsPlaceHorizontal = 0`

  Place the mini-icons horizontal instead of vertical.

- `MiniIconsRightToLeft = 0`

  Place new mini-icons from right to left.

- `MiniIconsBottomToTop = 0`

  Place new mini-icons from bottom to top.

- `GrabRootWindow = 1`

  Manage root window (EXPERIMENTAL - normally enabled!).

- `ShowMoveSizeStatus = 1`

  Move/resize status window is visible when moving/resizing the window.

- `ShowWorkspaceStatus = 1`

  Show name of current workspace while switching.

- `ShowWorkspaceStatusAfterSwitch = 1`

  Show name of current workspace while switching workspaces.

- `ShowWorkspaceStatusAfterActivation = 1`

  Show name of current workspace after explicit activation.

- `WarpPointer = 0`

  Pointer is moved in pointer focus move when focus is moved using the keyboard.

- `OpaqueMove = 1`

  Window is immediately moved when dragged, no outline is shown.

- `OpaqueResize = 0`

  Window is immediately resized when dragged, no outline is shown.

- `DelayPointerFocus = 1`

  Similar to delayed auto raise.

- `Win95Keys = 0`

  Makes 3 additional keys perform sensible functions. The keys must be mapped to MetaL, MetaR and Menu. The left one will activate the start menu and the right one will display the window list.

- `ModSuperIsCtrlAlt = 1`

  Treat Super/Win modifier as Ctrl+Alt.

- `UseMouseWheel = 0`

  Mouse wheel support.

- `TaskBarTaskGrouping = 0`

  Group applications with the same class name under a single task button.

- `ShowPopupsAbovePointer = 0`

  Show popup menus above mouse pointer.

- `ReplayMenuCancelClick = 0`

  Send the clicks outside menus to target window.

- `ManualPlacement = 0`

  Windows must be placed manually by the user.

- `SmartPlacement = 1`

  Smart window placement (minimal overlap).

- `IgnoreNoFocusHint = 0`

  Ignore no-accept-focus hint set by some windows.

- `MenuMouseTracking = 0`

  If enabled, menus will track the mouse even when no mouse button is pressed.

- `ClientWindowMouseActions = 1`

  Allow mouse actions on client windows.

- `SnapMove = 1`

  Snap to nearest screen edge/window when moving windows.

- `SnapDistance = 8`

  Distance in pixels before windows snap together

- `ArrangeWindowsOnScreenSizeChange = 1`

  Automatically arrange windows when screen size changes.

- `MsgBoxDefaultAction = 0`

  Preselect to Cancel (0) or the OK (1) button in message boxes

- `EdgeResistance = 32`

  Resistance to move window with mouse outside screen limits. Setting it to 10000 makes the resistance infinite.

- `AllowFullscreen = 1`

  Allow to switch a window to fullscreen.

- `FullscreenUseAllMonitors = 0`

  Span over all available screens if window goes into fullscreen.

- `NetWorkAreaBehaviour = 0`

  NET\_WORKAREA behaviour: 0 (single/multimonitor with STRUT information, like metacity), 1 (always full desktop), 2 (singlemonitor with STRUT, multimonitor without STRUT).

- `ConfirmLogout = 1`

  Confirm Logout.

- `MultiByte = 1`

  Overrides automatic multiple byte detection.

- `ShapesProtectClientWindow = 1`

  Don't cut client windows by shapes set trough frame corner pixmap.

- `DoubleBuffer = 1`

  Use double buffering when redrawing the display.

- `XRRDisable = 0`

  Disable use of new XRANDR API for dual head (nvidia workaround)

- `PreferFreetypeFonts = 1`

  Favor \*Xft fonts over core X11 fonts where possible.

- `IconPath = /usr/share/icons/hicolor:/usr/share/icons:/usr/share/pixmaps`

  Icon search path (colon separated)

- `MailCommand = xterm -name mutt -e mutt`

  Command to run on mailbox.

- `MailClassHint = mutt.XTerm`

  WM\_CLASS to allow runonce for MailCommand.

- `LockCommand =`
  Command to lock display/screensaver.

- `ClockCommand = xclock -name icewm -title Clock`

  Command to run on clock.

- `ClockClassHint = icewm.XClock`

  WM\_CLASS to allow runonce for ClockCommand.

- `RunCommand =`
  Command to select and run a program.

- `OpenCommand =`
  Command to run to open a file.

- `TerminalCommand = xterm`

Terminal emulator must accept -e option.

- `LogoutCommand =`
  Command to start logout.

- `LogoutCancelCommand =`
  Command to cancel logout.

- `ShutdownCommand =`
  Command to shutdown the system.

- `RebootCommand =`
  Command to reboot the system.

- `SuspendCommand =`
  Command to send the system to standby mode.

- `CPUStatusCommand =`
  Command to run on CPU status.

- `CPUStatusClassHint = top.XTerm`

  WM\_CLASS to allow runonce for CPUStatusCommand.

- `CPUStatusCombine = 1`

  Combine all CPUs to one.

- `NetStatusCommand =`
  Command to run on Net status.

- `NetStatusClassHint = netstat.XTerm`

  WM\_CLASS to allow runonce for NetStatusCommand.

- `AddressBarCommand =`
  Command to run for address bar entries.

- `XRRPrimaryScreenName =`
  screen/output name of the primary screen.

Quick Switch List
-----------------

- `QuickSwitch = 1`

  enable Alt+Tab window switcher.

- `QuickSwitchToMinimized = 1`

  Alt+Tab switches to minimized windows too.

- `QuickSwitchToHidden = 1`

  Alt+Tab to hidden windows.

- `QuickSwitchToUrgent = 1`

  Prioritize Alt+Tab to urgent windows.

- `QuickSwitchToAllWorkspaces = 1`

  Alt+Tab switches to windows on any workspace.

- `QuickSwitchGroupWorkspaces = 1`

  Alt+Tab: group windows on current workspace.

- `QuickSwitchAllIcons = 1`

  Show all reachable icons when quick switching.

- `QuickSwitchTextFirst = 0`

  Show the window title above (all reachable) icons.

- `QuickSwitchSmallWindow = 0`

  Attempt to create a small QuickSwitch window (1/3 instead of 3/5 of

- `QuickSwitchMaxWidth = 0`

  Go through all window titles and choose width of the longest one.

- `QuickSwitchVertical = 1`

  Place the icons and titles vertical instead of horizontal.

- `QuickSwitchHugeIcon = 0`

  Show the huge (48x48) of the window icon for the active window.

- `QuickSwitchFillSelection = 0`

  Fill the rectangle highlighting the current icon.

Edge Workspace Switching
------------------------

- `EdgeSwitch = 0`

  Workspace switches by moving mouse to left/right screen edge.

- `HorizontalEdgeSwitch = 0`

  Workspace switches by moving mouse to left/right screen edge.

- `VerticalEdgeSwitch = 0`

  Workspace switches by moving mouse to top/bottom screen edge.

- `ContinuousEdgeSwitch = 1`

  Workspace switches continuously when moving mouse to screen edge.

Task Bar
--------

The following settings can be set to value 1 (enabled) or value 0 (disabled).

- `ShowTaskBar = 1`

  Task bar is visible.

- `TaskBarAtTop = 0`

  Task bar is located at top of screen.

- `TaskBarKeepBelow = 1`

  Keep the task bar below regular windows

- `TaskBarAutoHide = 0`

  Task bar will auto hide when mouse leaves it.

- `TaskBarFullscreenAutoShow = 1`

  Auto show task bar when fullscreen window active.

- `TaskBarShowClock = 1`

  Task bar clock is visible.

- `TaskBarShowAPMStatus = 0`

  Show battery status monitor on task bar.

- `TaskBarShowAPMAuto = 1`

  Enable TaskBarShowAPMStatus if a battery is present.

- `TaskBarShowAPMTime = 1`

  Show battery status on task bar in time-format.

- `TaskBarShowAPMGraph = 1`

  Show battery status in graph mode.

- `TaskBarShowMailboxStatus = 1`

  Display status of mailbox (see 'Mailbox' below).

- `TaskBarMailboxStatusBeepOnNewMail = 0`

  Beep when new mail arrives.

- `TaskBarMailboxStatusCountMessages = 0`

  Display mail message count as tooltip.

- `TaskBarShowWorkspaces = 1`

  Show workspace switching buttons on task bar.

- `TaskBarShowWindows = 1`

  Show windows on the taskbar.

- `TaskBarShowShowDesktopButton = 1`

  Show 'show desktop' button on taskbar.
  If set to 2, it will move the icon to the right side, after the clock.

- `ShowEllipsis = 0`

  Show Ellipsis in taskbar items and some other locations. This is a visual indicator like "..." to show that a string didn't fit into the visible area.

- `TaskBarShowTray = 1`

  Show windows in the tray.

- `TrayShowAllWindows = 1`

  Show windows from all workspaces on tray.

- `TaskBarEnableSystemTray = 1`

  Enable the system tray in the taskbar.

- `TaskBarShowTransientWindows = 1`

  Show transient (dialogs, ...) windows on task bar.

- `TaskBarShowAllWindows = 0`

  Show windows from all workspaces on task bar.

- `TaskBarShowWindowIcons = 1`

  Show icons of windows on the task bar.

- `TaskBarShowStartMenu = 1`

  Show button for the start menu on the task bar.

- `TaskBarShowWindowListMenu = 1`

  Show button for window list menu on taskbar.

- `TaskBarShowCPUStatus = 1`

  Show CPU status on task bar (Linux & Solaris).

- `CPUStatusShowRamUsage = 1`

  Show RAM usage in CPU status tool tip.

- `CPUStatusShowSwapUsage = 1`

  Show swap usage in CPU status tool tip.

- `CPUStatusShowAcpiTemp = 1`

  Show ACPI temperature in CPU status tool tip.

- `CPUStatusShowAcpiTempInGraph = 0`

  Show ACPI temperature in CPU graph.

- `AcpiIgnoreBatteries =`
  List of battery names ignore.

- `CPUStatusShowCpuFreq = 1`

  Show CPU frequency in CPU status tool tip.

- `NetStatusShowOnlyRunning = 0`

  Show network status only for connected devices, such as an active ethernet link or associated wireless interface. If false, any network interface that has been brought up will be displayed.

- `TaskBarShowMEMStatus = 1`

  Show memory usage status on task bar (Linux only).

- `TaskBarShowNetStatus = 1`

  Show network status on task bar (Linux only).

- `NetworkStatusDevice = "[ew]*"`
  List of network devices to be displayed in tray, space separated. Shell wildcard patterns can also be used.

- `TaskBarShowCollapseButton = 0`

  Show a button to collapse the taskbar.

- `TaskBarDoubleHeight = 0`

  Double height task bar

- `TaskBarWorkspacesLeft = 1`

  Place workspace pager on left, not right.

- `TaskBarWorkspacesTop = 0`

  Place workspace pager on top row when using dual-height taskbar.

- `PagerShowPreview = 1`

  Show a mini desktop preview on each workspace button. By pressing the middle mouse button the 'window list' is shown. The right button activates the 'window list menu'. By using the scroll wheel over the 'workspace list' one can quickly cycle over all workspaces.

- `PagerShowWindowIcons = 1`

  Draw window icons inside large enough preview windows on pager (if PagerShowPreview=1).

- `PagerShowMinimized = 1`

  Draw even minimized windows as unfilled rectangles (if PagerShowPreview=1).

- `PagerShowBorders = 1`

  Draw border around workspace buttons (if PagerShowPreview=1).

- `PagerShowNumbers = 1`

  Show number of workspace on workspace button (if PagerShowPreview=1).

- `TaskBarLaunchOnSingleClick = 1`

  Execute taskbar applet commands (like MailCommand, ClockCommand, ...) on single click.

- `EnableAddressBar = 1`

  Enable address bar functionality in taskbar.

- `ShowAddressBar = 1`

  Show address bar in task bar.

- `TimeFormat = "%X"`

  format for the taskbar clock (time) (see strftime(3) manpage).

- `TimeFormatAlt = ""`

  Alternate Clock Time format (e.g. for blinking effects).

- `DateFormat = "%c"`

  format for the taskbar clock tooltip (date+time) (see strftime(3) manpage).

- `TaskBarCPUSamples = 20`

  Width of CPU Monitor.

- `TaskBarMEMSamples = 20`

  Width of Memory Monitor.

- `TaskBarNetSamples = 20`

  Width of Net Monitor.

- `TaskbarButtonWidthDivisor = 3`

  Default number of tasks in taskbar.

- `TaskBarWidthPercentage = 100`

  Task bar width as percentage of the screen width.

- `TaskBarJustify = "left"`

  Taskbar justify left, right or center.

- `TaskBarApmGraphWidth = 10`

  Width of battery Monitor.

- `TaskBarGraphHeight = 20`

  Height of taskbar monitoring applets.

- `XineramaPrimaryScreen = 0`

  Primary screen for xinerama (taskbar, ...).

Mailbox Monitoring (updated 2018-03-04)
---------------------------------------

- `MailCheckDelay = 30`

  Delay between new-mail checks. (seconds).

- `MailBoxPath = ""`

This may contain a list of mailbox specifications. Mailboxes are separated by a space. If `TaskBarShowMailboxStatus` is enabled then IceWM will monitor each mailbox for status changes each `MailCheckDelay` seconds. For each mailbox IceWM will show an icon on the taskbar. The icon shows if there is mail, if there is unread mail, or if there is new mail. Hovering the mouse pointer over an icon shows the number of messages in this mailbox and also the number of unread mails. A mailbox can be the path to a file in conventional *mbox* format. If the path points to a directory then *Maildir* format is assumed. Remote mail boxes are specified by an URL using the Common Internet Scheme Syntax (RFC 1738):

    scheme://user:password@server[:port][/path]

Supported schemes are `pop3`, `pop3s`, `imap`, `imaps` and `file`. The `pop3s` and `imaps` schemes depend on the presence of the `openssl` command for `TLS/SSL` encryption. This is also the case if `port` is either `993` for imap or `995` for pop3. When the scheme is omitted then `file` is assumed. IMAP subfolders can be given by the path component. Reserved characters like *slash*, *at* and *colon* can be specified using escape sequences with a hexadecimal encoding like `%2f` for the slash or `%40` for the at sign. For example:

    file:///var/spool/mail/captnmark
    file:///home/captnmark/Maildir/
    pop3://markus:%2f%40%3a@maol.ch/
    pop3s://markus:password@pop.gmail.com/
    imap://mathias@localhost/INBOX.Maillisten.icewm-user
    imaps://mathias:password@imap.gmail.com/INBOX

To help solve network protocol errors for pop3 and imap set the environment variable `ICEWM_MAILCHECK_TRACE`. IceWM will then log communication details for POP3 and IMAP mailboxes. Just set `export ICEWM_MAILCHECK_TRACE=1` before executing icewm, or set this in the `env` configuration file.

Note that for IceWM to access Gmail you must first configure your Gmail account to enable POP3 or IMAP access. To allow non-Gmail applications like IceWM to use it see the Gmail help site for "Let less secure apps use your account". Also set secure file permissions on your IceWM preferences file and the directory which contains it. It is unwise to store a password on file ever. Consider a wallet extension for IceWM. The following Perl snippet demonstrates how to hex encode a password like `!p@a%s&s~`:

    perl -e 'foreach(split("", $ARGV[0])) { printf "%%%02x", ord($_); }; print "\n";' '!p@a%s&s~'
    %21%40%23%24%25%5e%26%2a%7e

- `NewMailCommand =`

  The command to be run when new mail arrives. It is executed by `/bin/sh -c`.
  The following environment variables will be set:

  - `ICEWM_MAILBOX` mailbox index number of `MailBoxPath` starting from 1.
  - `ICEWM_COUNT` gives the total number of messages in this mailbox.
  - `ICEWM_UNREAD` gives the number of unread messages in this mailbox.

Menus
-----

- `AutoReloadMenus = 1`

  Reload menu files automatically if set to 1.

- `ShowProgramsMenu = 0`

  Show programs submenu.

- `ShowSettingsMenu = 1`

  Show settings submenu.

- `ShowFocusModeMenu = 1`

  Show focus mode submenu.

- `ShowThemesMenu = 1`

  Show themes submenu.

- `ShowLogoutMenu = 1`

  Show logout menu.

- `ShowHelp = 1`

  Show the help menu item.

- `ShowLogoutSubMenu = 1`

  Show logout submenu.

- `ShowAbout = 1`

  Show the about menu item.

- `ShowRun = 1`

  Show the run menu item.

- `ShowWindowList = 1`

  Show the window menu item.

- `MenuMaximalWidth = 0`

  Maximal width of popup menus, 2/3 of the screen's width if set to zero.

- `NestedThemeMenuMinNumber = 25`

  Minimal number of themes after which the Themes menu becomes nested (0=disabled).

Timings
-------

- `DelayFuzziness = 10`

  Percentage of delay/timeout fuzziness to allow for merging of timer timeouts.

- `ClickMotionDistance = 5`

  Movement before click is interpreted as drag.

- `ClickMotionDelay = 200`

  Delay before click gets interpreted as drag.

- `MultiClickTime = 400`

  Time (ms) to recognize for double click.

- `MenuActivateDelay = 40`

  Delay before activating menu items.

- `SubmenuMenuActivateDelay = 300`

  Delay before activating menu submenus.

- `ToolTipDelay = 5000`

  Time before showing the tooltip.

- `ToolTipTime = 60000`

  Time before tooltip window is hidden (0 means never).

- `AutoHideDelay = 300`

  Time to auto hide taskbar (must enable first with TaskBarAutoHide).

- `AutoShowDelay = 500`

  Delay before task bar is shown.

- `AutoRaiseDelay = 400`

  Time to auto raise (must enable first with AutoRaise).

- `PointerFocusDelay = 200`

  Delay for pointer focus switching.

- `EdgeSwitchDelay = 600`

  Screen edge workspace switching delay.

- `ScrollBarStartDelay = 500`

  Initial scroll bar autoscroll delay

- `ScrollBarDelay = 30`

  Scroll bar autoscroll delay

- `AutoScrollStartDelay = 500`

  Auto scroll start delay

- `AutoScrollDelay = 60`

  Auto scroll delay

- `WorkspaceStatusTime = 2500`

  Time before workspace status window is hidden.

- `TaskBarCPUDelay = 500`

  Delay between CPU Monitor samples in ms.

- `TaskBarMEMDelay = 500`

  Delay between Memory Monitor samples in ms.

- `TaskBarNetDelay = 500`

  Delay between Net Monitor samples in ms.

- `FocusRequestFlashTime = 0`

  Number of seconds the taskbar app will blink when requesting focus (0 = forever).

- `FocusRequestFlashInterval = 250`

  Taskbar blink interval (ms) when requesting focus (0 = blinking disabled).

- `BatteryPollingPeriod = 10`

  Delay between power status updates (seconds).

Buttons and Keys
----------------

- `UseRootButtons = 255`

  Bitmask of root window button click to use in window manager.

- `ButtonRaiseMask = 1`

  Bitmask of buttons that raise the window when pressed.

- `DesktopWinMenuButton = 0`

  Desktop mouse-button click to show the window list menu.

- `DesktopWinListButton = 2`

  Desktop mouse-button click to show the window list.

- `DesktopMenuButton = 3`

  Desktop mouse-button click to show the root menu.

- `TitleBarMaximizeButton = 1`

  TitleBar mouse-button double click to maximize the window.

- `TitleBarRollupButton = 2`

  TitleBar mouse-button double click to rollup the window.

- `RolloverButtonsSupported = 0`

  Does it support the 'O' title bar button images (for mouse rollover)

Workspaces
----------

- WorkspaceNames

  List of workspace names, for example:

    WorkspaceNames=" 1 ", " 2 ", " 3 ", " 4 "

Paths
-----

- LibPath

  Path to the icewm/lib directory.

- IconPath

  Path to the icon directory. Multiple paths can be given using the colon as a separator.

Programs
--------

- ClockCommand

  program to run when the clock is double clicked.

- MailCommand

  program to run when mailbox icon is double clicked.

- LockCommand

  program to run to lock the screen.

- RunCommand

  program to run when **Run** is selected from the start menu.

Window Menus
------------

WinMenuItems
Items to show in the window menus, possible values are:

- `a=rAise`

- `c=Close`

- `f=Fullscreen`

- `h=Hide`

- `i=trayIcon`

- `k=Kill`

- `l=Lower`

- `m=Move`

- `n=miNimize`

- `r=Restore`

- `s=Size`

- `t=moveTo`

- `u=rollUp`

- `w=WindowsList`

- `x=maXimize`

- `y=laYer`

  Examples:

    WinMenuItems=rmsnxfhualyticw   #Default menu
    WinMenuItems=rmsnxfhualytickw  #Menu with all possible options
    WinMenuItems=rmsnxc            #MS-Windows menu

Theme Settings
==============

This section shows settings that can be set in theme files. They can also be set in `preferences` file but themes will override the values set there. To override the theme values the settings should be set in `prefoverride` file. Default values are shown following the equal sign.

- `ThemeAuthor =`

  Theme author, e-mail address, credits.

- `ThemeDescription =`

  Description of the theme, credits.

- `Gradients =`

  List of gradient pixmaps in the current theme.

Borders
-------

The following settings can be set to a numeric value.

- `BorderSizeX = 6`

  Left/right border width.

- `BorderSizeY = 6`

  Top/bottom border height.

- `DlgBorderSizeX = 2`

  Left/right border width of non-resizable windows.

- `DlgBorderSizeY = 2`

  Top/bottom border height of non-resizable windows.

- `CornerSizeX = 24`

  Width of the window corner.

- `CornerSizeY = 24`

  Height of the window corner.

- `TitleBarHeight = 20`

  Height of the title bar.

- `TitleBarJustify = 0`

  Justification of the window title.

- `TitleBarHorzOffset = 0`

  Horizontal offset for the window title text.

- `TitleBarVertOffset = 0`

  Vertical offset for the window title text.

- `TitleBarCentered = 0`

  Draw window title centered (obsoleted by TitleBarJustify)

- `TitleBarJoinLeft = 0`

  Join title\*S and title\*T

- `TitleBarJoinRight = 0`

  Join title\*T and title\*B

- `ScrollBarX = 16`

  Scrollbar width.

- `ScrollBarY = 16`

  Scrollbar (button) height.

- `MenuIconSize = 16`

  Menu icon size.

- `SmallIconSize = 16`

  Dimension of the small icons.

- `LargeIconSize = 32`

  Dimension of the large icons.

- `HugeIconSize = 48`

  Dimension of the large icons.

- `QuickSwitchHorzMargin = 3`

  Horizontal margin of the quickswitch window.

- `QuickSwitchVertMargin = 3`

  Vertical margin of the quickswitch window.

- `QuickSwitchIconMargin = 4`

  Vertical margin in the quickswitch window.

- `QuickSwitchIconBorder = 2`

  Distance between the active icon and it's border.

- `QuickSwitchSeparatorSize = 6`

  Height of the separator between (all reachable) icons and text, 0 to avoid it.

- `ShowMenuButtonIcon = 1`

  Show application icon over menu button.

- `TitleButtonsLeft = "s"`

  Titlebar buttons from left to right (x=close, m=max, i=min, h=hide, r=rollup, s=sysmenu, d=depth).

- `TitleButtonsRight = "xmir"`

  Titlebar buttons from right to left (x=close, m=max, i=min, h=hide, r=rollup, s=sysmenu, d=depth).

- `TitleButtonsSupported = "xmis"`

  Titlebar buttons supported by theme (x,m,i,r,h,s,d).

Fonts
-----

The following settings can be set to a string value.

- `TitleFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the title bar font.

- `MenuFontName = "-*-sans-bold-r-*-*-*-100-*-*-*-*-*-*"`

  Name of the menu font.

- `StatusFontName = "-*-monospace-bold-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the status display font.

- `QuickSwitchFontName = "-*-monospace-bold-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the font for Alt+Tab switcher window.

- `NormalButtonFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the normal button font.

- `ActiveButtonFontName = "-*-sans-bold-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the active button font.

- `NormalTaskBarFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the normal task bar item font.

- `ActiveTaskBarFontName = "-*-sans-bold-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the active task bar item font.

- `ToolButtonFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the tool button font (fallback: NormalButtonFontName).

- `NormalWorkspaceFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the normal workspace button font (fallback: NormalButtonFontName).

- `ActiveWorkspaceFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the active workspace button font (fallback: ActiveButtonFontName).

- `MinimizedWindowFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the mini-window font.

- `ListBoxFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the window list font.

- `ToolTipFontName = "-*-sans-medium-r-*-*-*-120-*-*-*-*-*-*"`

  Name of the tool tip font.

- `ClockFontName = "-*-monospace-medium-r-*-*-*-140-*-*-*-*-*-*"`

  Name of the task bar clock font.

- `TempFontName = "-*-monospace-medium-r-*-*-*-140-*-*-*-*-*-*"`

  Name of the task bar temperature font.

- `ApmFontName = "-*-monospace-medium-r-*-*-*-140-*-*-*-*-*-*"`

  Name of the task bar battery font.

- `InputFontName = "-*-monospace-medium-r-*-*-*-140-*-*-*-*-*-*"`

  Name of the input field font.

- `LabelFontName = "-*-sans-medium-r-*-*-*-140-*-*-*-*-*-*"`

  Name of the label font.

When IceWM is configured with `--enable-xfreetype`, only the settings with "Xft" suffix will be used. They specifiy the font name in fontconfig format:

    MenuFontNameXft="sans-serif:size=12:bold"

- `TitleFontNameXft = "sans-serif:size=12"`

  Name of the title bar font.

- `MenuFontNameXft = "sans-serif:size=10:bold"`

  Name of the menu font.

- `StatusFontNameXft = "monospace:size=12:bold"`

  Name of the status display font.

- `QuickSwitchFontNameXft = "monospace:size=12:bold"`

  Name of the font for Alt+Tab switcher window.

- `NormalButtonFontNameXft = "sans-serif:size=12"`

  Name of the normal button font.

- `ActiveButtonFontNameXft = "sans-serif:size=12:bold"`

  Name of the active button font.

- `NormalTaskBarFontNameXft = "sans-serif:size=12"`

  Name of the normal task bar item font.

- `ActiveTaskBarFontNameXft = "sans-serif:size=12:bold"`

  Name of the active task bar item font.

- `ToolButtonFontNameXft = "sans-serif:size=12"`

  Name of the tool button font (fallback: NormalButtonFontNameXft).

- `NormalWorkspaceFontNameXft = "sans-serif:size=12"`

  Name of the normal workspace button font (fallback: NormalButtonFontNameXft).

- `ActiveWorkspaceFontNameXft = "sans-serif:size=12"`

  Name of the active workspace button font (fallback: ActiveButtonFontNameXft).

- `MinimizedWindowFontNameXft = "sans-serif:size=12"`

  Name of the mini-window font.

- `ListBoxFontNameXft = "sans-serif:size=12"`

  Name of the window list font.

- `ToolTipFontNameXft = "sans-serif:size=12"`

  Name of the tool tip font.

- `ClockFontNameXft = "monospace:size=12"`

  Name of the task bar clock font.

- `TempFontNameXft = "monospace:size=12"`

  Name of the task bar temperature font.

- `ApmFontNameXft = "monospace:size=12"`

  Name of the task bar battery font.

- `InputFontNameXft = "monospace:size=12"`

  Name of the input field font.

- `LabelFontNameXft = "sans-serif:size=12"`

  Name of the label font.

Colors
------

- `ColorDialog = "rgb:C0/C0/C0"`

  Background of dialog windows.

- `ColorNormalBorder = "rgb:C0/C0/C0"`

  Border of inactive windows.

- `ColorActiveBorder = "rgb:C0/C0/C0"`

  Border of active windows.

- `ColorNormalButton = "rgb:C0/C0/C0"`

  Background of regular buttons.

- `ColorNormalButtonText = "rgb:00/00/00"`

  Textcolor of regular buttons.

- `ColorActiveButton = "rgb:E0/E0/E0"`

  Background of pressed buttons.

- `ColorActiveButtonText = "rgb:00/00/00"`

  Textcolor of pressed buttons.

- `ColorNormalTitleButton = "rgb:C0/C0/C0"`

  Background of titlebar buttons.

- `ColorNormalTitleButtonText = "rgb:00/00/00"`

  Textcolor of titlebar buttons.

- `ColorToolButton = ""`

  Background of toolbar buttons, ColorNormalButton is used if empty.

- `ColorToolButtonText = ""`

  Textcolor of toolbar buttons, ColorNormalButtonText is used if empty.

- `ColorNormalWorkspaceButton = ""`

  Background of workspace buttons, ColorNormalButton is used if empty.

- `ColorNormalWorkspaceButtonText = ""`

  Textcolor of workspace buttons, ColorNormalButtonText is used if empty.

- `ColorActiveWorkspaceButton = ""`

  Background of the active workspace button, ColorActiveButton is used if empty.

- `ColorActiveWorkspaceButtonText = ""`

  Textcolor of the active workspace button, ColorActiveButtonText is used if empty.

- `ColorNormalTitleBar = "rgb:80/80/80"`

  Background of the titlebar of regular windows.

- `ColorNormalTitleBarText = "rgb:00/00/00"`

  Textcolor of the titlebar of regular windows.

- `ColorNormalTitleBarShadow = ""`

  Textshadow of the titlebar of regular windows.

- `ColorActiveTitleBar = "rgb:00/00/A0"`

  Background of the titlebar of active windows.

- `ColorActiveTitleBarText = "rgb:FF/FF/FF"`

  Textcolor of the titlebar of active windows.

- `ColorActiveTitleBarShadow = ""`

  Textshadow of the titlebar of active windows.

- `ColorNormalMinimizedWindow = "rgb:C0/C0/C0"`

  Background for mini icons of regular windows.

- `ColorNormalMinimizedWindowText = "rgb:00/00/00"`

  Textcolor for mini icons of regular windows.

- `ColorActiveMinimizedWindow = "rgb:E0/E0/E0"`

  Background for mini icons of active windows.

- `ColorActiveMinimizedWindowText = "rgb:00/00/00"`

  Textcolor for mini icons of active windows.

- `ColorNormalMenu = "rgb:C0/C0/C0"`

  Background of pop-up menus.

- `ColorNormalMenuItemText = "rgb:00/00/00"`

  Textcolor of regular menu items.

- `ColorActiveMenuItem = "rgb:A0/A0/A0"`

  Background of selected menu item, leave empty to force transparency.

- `ColorActiveMenuItemText = "rgb:00/00/00"`

  Textcolor of selected menu items.

- `ColorDisabledMenuItemText = "rgb:80/80/80"`

  Textcolor of disabled menu items.

- `ColorDisabledMenuItemShadow = ""`

  Shadow of regular menu items.

- `ColorMoveSizeStatus = "rgb:C0/C0/C0"`

  Background of move/resize status window.

- `ColorMoveSizeStatusText = "rgb:00/00/00"`

  Textcolor of move/resize status window.

- `ColorQuickSwitch = "rgb:C0/C0/C0"`

  Background of the quick switch window.

- `ColorQuickSwitchText = "rgb:00/00/00"`

  Textcolor in the quick switch window.

- `ColorQuickSwitchActive = ""`

  Rectangle arround the active icon in the quick switch window.

- `ColorDefaultTaskBar = "rgb:C0/C0/C0"`

  Background of the taskbar.

- `ColorNormalTaskBarApp = "rgb:C0/C0/C0"`

  Background for task buttons of regular windows.

- `ColorNormalTaskBarAppText = "rgb:00/00/00"`

  Textcolor for task buttons of regular windows.

- `ColorActiveTaskBarApp = "rgb:E0/E0/E0"`

  Background for task buttons of the active window.

- `ColorActiveTaskBarAppText = "rgb:00/00/00"`

  Textcolor for task buttons of the active window.

- `ColorMinimizedTaskBarApp = "rgb:A0/A0/A0"`

  Background for task buttons of minimized windows.

- `ColorMinimizedTaskBarAppText = "rgb:00/00/00"`

  Textcolor for task buttons of minimized windows.

- `ColorInvisibleTaskBarApp = "rgb:80/80/80"`

  Background for task buttons of windows on other workspaces.

- `ColorInvisibleTaskBarAppText = "rgb:00/00/00"`

  Textcolor for task buttons of windows on other workspaces.

- `ColorScrollBar = "rgb:A0/A0/A0"`

  Scrollbar background (sliding area).

- `ColorScrollBarSlider = "rgb:C0/C0/C0"`

  Background of the slider button in scrollbars.

- `ColorScrollBarButton = "rgb:C0/C0/C0"`

  Background of the arrow buttons in scrollbars.

- `ColorScrollBarArrow = "rgb:C0/C0/C0"`

  Background of the arrow buttons in scrollbars (obsolete).

- `ColorScrollBarButtonArrow = "rgb:00/00/00"`

  Color of active arrows on scrollbar buttons.

- `ColorScrollBarInactiveArrow = "rgb:80/80/80"`

  Color of inactive arrows on scrollbar buttons.

- `ColorListBox = "rgb:C0/C0/C0"`

  Background of listboxes.

- `ColorListBoxText = "rgb:00/00/00"`

  Textcolor in listboxes.

- `ColorListBoxSelection = "rgb:80/80/80"`

  Background of selected listbox items.

- `ColorListBoxSelectionText = "rgb:00/00/00"`

  Textcolor of selected listbox items.

- `ColorToolTip = "rgb:E0/E0/00"`

  Background of tooltips.

- `ColorToolTipText = "rgb:00/00/00"`

  Textcolor of tooltips.

- `ColorLabel = "rgb:C0/C0/C0"`

  Background of labels, leave empty to force transparency.

- `ColorLabelText = "rgb:00/00/00"`

  Textcolor of labels.

- `ColorInput = "rgb:FF/FF/FF"`

  Background of text entry fields (e.g. the addressbar).

- `ColorInputText = "rgb:00/00/00"`

  Textcolor of text entry fields (e.g. the addressbar).

- `ColorInputSelection = "rgb:80/80/80"`

  Background of selected text in an entry field.

- `ColorInputSelectionText = "rgb:00/00/00"`

  Selected text in an entry field.

- `ColorClock = "rgb:00/00/00"`

  Background of non-LCD clock, leave empty to force transparency.

- `ColorClockText = "rgb:00/FF/00"`

  Background of non-LCD monitor.

- `ColorApm = "rgb:00/00/00"`

  Background of battery monitor, leave empty to force transparency.

- `ColorApmText = "rgb:00/FF/00"`

  Textcolor of battery monitor.

- `ColorApmBattary = "rgb:FF/FF/00"`

  Color of battery monitor when discharging.

- `ColorApmLine = "rgb:00/FF/00"`

  Color of battery monitor when charging.

- `ColorApmGraphBg = "rgb:00/00/00"`

  Background color for graph mode.

- `ColorCPUStatusUser = "rgb:00/FF/00"`

  User load on the CPU monitor.

- `ColorCPUStatusSystem = "rgb:FF/00/00"`

  System load on the CPU monitor.

- `ColorCPUStatusInterrupts = "rgb:FF/FF/00"`

  Interrupts on the CPU monitor.

- `ColorCPUStatusIoWait = "rgb:60/00/60"`

  IO Wait on the CPU monitor.

- `ColorCPUStatusSoftIrq = "rgb:00/FF/FF"`

  Soft Interrupts on the CPU monitor.

- `ColorCPUStatusNice = "rgb:00/00/FF"`

  Nice load on the CPU monitor.

- `ColorCPUStatusIdle = "rgb:00/00/00"`

  Idle (non) load on the CPU monitor, leave empty to force transparency.

- `ColorCPUStatusSteal = "rgb:FF/8A/91"`

  Involuntary Wait on the CPU monitor.

- `ColorCPUStatusTemp = "rgb:60/60/C0"`

  Temperature of the CPU.

- `ColorMEMStatusUser = "rgb:40/40/80"`

  User program usage in the memory monitor.

- `ColorMEMStatusBuffers = "rgb:60/60/C0"`

  OS buffers usage in the memory monitor.

- `ColorMEMStatusCached = "rgb:80/80/FF"`

  OS cached usage in the memory monitor.

- `ColorMEMStatusFree = "rgb:00/00/00"`

  Free memory in the memory monitor.

- `ColorNetSend = "rgb:FF/FF/00"`

  Outgoing load on the network monitor.

- `ColorNetReceive = "rgb:FF/00/FF"`

  Incoming load on the network monitor.

- `ColorNetIdle = "rgb:00/00/00"`

  Idle (non) load on the network monitor, leave empty to force transparency.

- `ColorApmBattery = rgb:FF/FF/00`

  Color of battery monitor in battery mode.

Desktop Background
------------------

The following options are used by `icewmbg`:

- `DesktopBackgroundCenter = 0`

  Display desktop background centered and not tiled. (set to 0 or 1).

- `DesktopBackgroundScaled = 0`

  Resize desktop background to full screen.

- `DesktopBackgroundColor = ""`

  Color(s) of the desktop background.

- `DesktopBackgroundImage = ""`

  Image(s) for desktop background. If you want IceWM to ignore the desktop background image / color set both DesktopBackgroundColor and DesktopBackgroundImage to an empty value ("").

- `SupportSemitransparency = 1`

  Support for semitransparent terminals like Eterm or gnome-terminal.

- `DesktopTransparencyColor = ""`

  Color(s) to announce for semitransparent windows.

- `DesktopTransparencyImage = ""`

  Image(s) to announce for semitransparent windows.

- `DesktopBackgroundMultihead = 0`

  Paint the background image over all multihead monitors combined.

Task Bar Style
--------------

- `TaskBarClockLeds = 1`

  Display clock using LCD style pixmaps.

Menus and Toolbar (updated 2018-05-06)
======================================

menu
----

Within the `menu` configuration file you can configure which programs are to appear in the root/start menu.

Submenus can be created with:

    menu "title" icon_name {
    # contained items
    }
    separator
    menufile "title" icon_name filename
    menuprog "title" icon_name program arguments
    menuprogreload "title" icon_name timeout program arguments
    include filename
    includeprog program arguments

Menus can be populated with submenus and with program entries as explained below for the `program` configuration file. Comments start with a \#-sign.

The `menufile` directive creates a submenu with a title and an icon, while `menuprog` and `menuprogreload` create a submenu with entries from the output of `program arguments` where the timeout of `menuprogreload` says to reload the submenu after the timeout expires. The `include` statement loads more configuration from the given filename, while `includeprog` runs `program arguments` to parse the output.

toolbar
-------

The `toolbar` configuration file is used to put programs as buttons on the taskbar. It uses the same syntax as the `menu` file.

programs
--------

Usually automatically generated menu configuration file of installed programs. The `programs` file should be automatically generated by `wmconfig` (Redhat), `menu` (Debian), or `icewm-menu-fdo`.

Programs can be added using the following syntax:

    prog "title" icon_name program_executable options

Restarting another window manager can be done using the restart program:

    restart "title" icon_name program_executable options

icon\_name can be `-` if icon is not wanted, or `!` if the icon name shall be guessed by checking the command (i.e. the real executable file, following symlinks as needed).

The "runonce" keyword allows to launch an application only when no window has the WM\_CLASS hint specified. Otherwise the first window having this class hint is mapped and raised. Syntax:

    runonce "title" icon_name "res_name.res_class" program_executable options
    runonce "title" icon_name "res_name" program_executable options
    runonce "title" icon_name ".res_class" program_executable options

The class hint of an application window can be figured out by running

    xprop WM_CLASS

Submenus can be added using the same keywords as the `menu` configuration file.

Only double quotes are interpreted by IceWM. IceWM doesn't run the shell automatically, so you may have to do that.

Hot Keys
========

IceWM allows launching of arbitrary programs with any key combination. This is configured in the `keys` file. The syntax of this file is like:

**key** "key combination" program options...

For example:

    key  "Alt+Ctrl+t"  xterm -rv
    key "Ctrl+Shift+r" icewm --restart
    runonce "Alt+F12"  "res_name.res_class" program_executable options

Window Options (updated 2018-03-04)
===================================

The **winoptions** file is used to configure settings for individual application windows. Each line in this file must have one of the following formats:

- **window\_name.window\_class.option: argument**

- **window\_name.window\_role.option: argument**

- **window\_class.option: argument**

- **window\_name.option: argument**

- **window\_role.option: argument**

- **.option: argument**

The last format sets a default option value for all windows. Each window on the desktop should have **name** and **class** resources associated with it. Some applications also have a **window role** resource. They can be determined using the `xprop` utility. When used on a toplevel window, `xprop | grep -e CLASS -e ROLE` should output a line like this:

    WM_CLASS = "name", "Class"

and may also display a line like this:

    WM_WINDOW_ROLE = "window role"

It's possible that an application's **name** and/or **class** contains a dot character (**.**), which is also used by IceWM to separate **name**, **class** and **role** values. In this case precede the dot with the backslash character. In the following example, we suppose an application's window has `the.name` as its **name** value and `The.Class` as its **class** value and for this combination we set **option** to **argument**.

    the\.name.The\.Class.option: argument

Options that can be set per window are as follows:

- **icon**

  The name of the icon.

- **workspace**

  Default workspace for window (number, counting from 0)

- **layer**

  The default stacking layer for the window. Layer can be one of the following seven strings:

  - *Desktop*
    Desktop window. There should be only one window in this layer.

  - *Below*
    Below default layer.

  - *Normal*
    Default layer for the windows.

  - *OnTop*
    Above the default.

  - *Dock*
    Layer for windows docked to the edge of the screen.

  - *AboveDock*
    Layer for the windows above the dock.

  - *Menu*
    Layer for the windows above the dock.

  You can also use a number from 0 to 15.

- **geometry**

  The default geometry for the window. This geometry should be specified in the usual X11-geometry-syntax, formal notation:

        [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]

- **tray**

  The default tray option for the window. This affects both the tray and the task pane. Tray can be one of the following strings:

  - *Ignore*
    Don't add an icon to the tray pane.

  - *Minimized*
    Add an icon the the tray. Remove the task pane button when minimized.

  - *Exclusive*
    Add an icon the the tray. Never create a task pane button.

- **order: 0**

  The sorting order of task buttons and tray icons. The default value is zero. Increasing positive values go farther right, while decreasing negative values go farther left. The order option applies to the task pane, the tray pane and the system tray.

- **allWorkspaces: 0**

  If set to 1, window will be visible on all workspaces.

- **appTakesFocus: 0**

  if set to 1, IceWM will assume the window supports the WM\_TAKE\_FOCUS protocol even if the window did not advertise that it does.

- **dBorder: 1**

  If set to 0, window will not have a border.

- **dClose: 1**

  If set to 0, window will not have a close button.

- **dDepth: 1**

  If set to 0, window will not have a depth button.

- **dHide: 1**

  If set to 0, window will not have a hide button.

- **dMaximize: 1**

  If set to 0, window will not have a maximize button.

- **dMinimize: 1**

  If set to 0, window will not have a minimize button.

- **dResize: 1**

  If set to 0, window will not have a resize border.

- **dRollup: 1**

  If set to 0, window will not have a shade button.

- **dSysMenu: 1**

  If set to 0, window will not have a system menu.

- **dTitleBar: 1**

  If set to 0, window will not have a title bar.

- **doNotCover: 0**

  if set to 1, this window will limit the workspace available for regular applications. At the moment the window has to be sticky to make it work.

- **doNotFocus: 0**

  if set to 1, IceWM will never give focus to the window.

- **fClose: 1**

  If set to 0, window will not be closable.

- **fHide: 1**

  If set to 0, window will not be hidable.

- **fMaximize: 1**

  If set to 0, window will not be maximizable.

- **fMinimize: 1**

  If set to 0, window will not be minimizable.

- **fMove: 1**

  If set to 0, window will not be movable.

- **fResize: 1**

  If set to 0, window will not be resizable.

- **fRollup: 1**

  If set to 0, window will not be shadable.

- **forcedClose: 0**

  if set to 1 and the application had not registered WM\_DELETE\_WINDOW, a close confirmation dialog won't be offered upon closing the window.

- **fullKeys: 0**

  If set to 1, the window manager leave more keys (Alt+F?) to the application.

- **ignoreNoFocusHint: 0**

  if set to 1, IceWM will focus even if the window does not handle input.

- **ignorePagerPreview: 0**

  If set to 1, window will not appear in pager preview.

- **ignorePositionHint: 0**

  if set to 1, IceWM will ignore the position hint.

- **ignoreQuickSwitch: 0**

  If set to 1, window will not be accessible using QuickSwitch feature (Alt+Tab).

- **ignoreTaskBar: 0**

  If set to 1, window will not appear on the task bar.

- **ignoreUrgentHint: 0**

  if set to 1, IceWM will ignore it if the window sets the urgent hint.

- **ignoreWinList: 0**

  If set to 1, window will not appear in the window list.

- **noFocusOnAppRaise: 0**

  if set to 1, window will not automatically get focus as application raises it.

- **noFocusOnMap: 0**

  if set to 1, IceWM will not assign focus when the window is mapped for the first time.

- **noIgnoreTaskBar: 0**

  if set to 1, will show the window on the taskbar.

- **startFullscreen: 0**

  if set to 1, window will cover the entire screen.

- **startMaximized: 0**

  if set to 1, window starts maximized.

- **startMaximizedHorz: 0**

  if set to 1, window starts maximized horizontally.

- **startMaximizedVert: 0**

  if set to 1, window starts maximized vertically.

- **startMinimized: 0**

  if set to 1, window starts minimized.

Icon handling
=============

Generic
-------

The window manager expects to find two XPM files for each icon specified in the configuration files as *ICON*. They should be named like this:

- **ICON\_16x16.xpm**

  A small 16x16 pixmap.

- **ICON\_32x32.xpm**

  A normal 32x32 pixmap.

- **ICON\_48x48.xpm**

  A large 48x48 pixmap.

Other pixmap sizes like 20x20, 24x24, 40x40, 48x48, 64x64 might be used in the future. Perhaps we need a file format that can contain more than one image (with different sizes and color depths) like Windows'95 and OS/2 .ICO files.

It would be nice to have a feature from OS/2 that varies the icon size with screen resolution (16x16 and 32x32 icons are quite small on 4000x4000 screens ;-)

GDK-Pixbuf
----------

When icewm was configured with the `--enable-gdk-pixbuf` option all of GdkPixbuf's image formats are supported. Use them by specifying the full filename or an absolute path:

- **ICON.bmp**

  A PPM icon in your IconPath.

- **/usr/share/pixmap/ICON.png**

  An PNG image with absolute location.

Mouse cursors
=============

IceWM scans the theme and configuration directories for a subdirectory called *cursors* containing monochrome but transparent XPM files. To change the mouse cursor you have to use this filenames:

- **left.xbm**

  Default cursor (usually pointer to the left).

- **right.xbm**

  Menu cursor (usually pointer to the right).

- **move.xbm**

  Window movement cursor.

- **sizeTL.xbm**

  Cursor when you resize the window by top left.

- **sizeT.xbm**

  Cursor when you resize the window by top.

- **sizeTR.xbm**

  Cursor when you resize the window by top right.

- **sizeL.xbm**

  Cursor when you resize the window by left.

- **sizeR.xbm**

  Cursor when you resize the window by right.

- **sizeBL.xbm**

  Cursor when you resize the window by bottom left.

- **sizeB.xbm**

  Cursor when you resize the window by bottom.

- **sizeBR.xbm**

  Cursor when you resize the window by bottom right.

Themes
======

Themes are used to configure the way the window manager looks. Things like fonts, colors, border sizes, button pixmaps can be configured. Put together they form a theme.

Theme files are searched in the `themes` subdirectories.

These directories contain other directories that contain related theme files and their .xpm files. Each theme file specifies fonts, colors, border sizes, ...

The theme to use is specified in `~/.icewm/theme` file:

- `Theme = "nice/default.theme"`

  Name of the theme to use. Both the directory and theme file name must be specified.

If the theme directory contains a file named *fonts.dir* created by mkfontdir the theme directory is inserted into the X servers font search path.

[www.box-look.org](https://themes.ice-wm.org) has a very large number of themes which were created by users of IceWM.

Command Line Options
====================

`icewm` supports the following options:

    -d, --display=NAME
    NAME of the X server to use.

    --client-id=ID
    Client id to use when contacting session manager.

    --sync
    Synchronize X11 commands.

    -c, --config=FILE
    Load preferences from FILE.

    -t, --theme=FILE
    Load theme from FILE.

    --postpreferences
    Print preferences after all processing.

    -V, --version
    Prints version information and exits.

    -h, --help
    Prints this usage screen and exits.

    --replace
    Replace an existing window manager.

    -r, --restart
    Tell the running icewm to restart itself.

    --configured
    Print the compile time configuration.

    --directories
    Print the configuration directories.

    -l, --list-themes
    Print a list of all available themes.

The restart option can be used to reload the IceWM configuration after modifications. It is the preferred way to restart IceWM from the command line or in scripts. To load a different theme from the command line, combine this with the `--theme=NAME` option like:

    icewm -r -t CrystalBlue

The theme name will then be saved to the 'theme' configuration file, before restarting IceWM.

Glossary
========

Resource Path
A set of directories used by IceWM to locate resources like configuration files, themes, icons. See section **The Resource Path**.

Authors
=======

Authors having contributed to this document include Gallium, Macek, Hasselmann, Gijsbers, Bidulock and Bloch.

See Also
========

[icehelp(1)](https://ice-wm.org/man/icehelp),
[icesh(1)](https://ice-wm.org/man/icesh),
[icesound(1)](https://ice-wm.org/man/icesound),
[icewm-env(5)](https://ice-wm.org/man/icewm-env),
[icewm-focus_mode(5)](https://ice-wm.org/man/icewm-focus_mode.html),
[icewm-keys(5)](https://ice-wm.org/man/icewm-keys),
[icewm-menu-fdo(1)](https://ice-wm.org/man/icewm-menu-fdo),
[icewm-menu(5)](https://ice-wm.org/man/icewm-menu),
[icewm-preferences(5)](https://ice-wm.org/man/icewm-preferences),
[icewm-prefoverride(5)](https://ice-wm.org/man/icewm-prefoverride),
[icewm-programs(5)](https://ice-wm.org/man/icewm-programs),
[icewm-session(1)](https://ice-wm.org/man/icewm-session),
[icewm-set-gnomewm(1)](https://ice-wm.org/man/icewm-set-gnomewm),
[icewm-shutdown(5)](https://ice-wm.org/man/icewm-shutdown),
[icewm-startup(5)](https://ice-wm.org/man/icewm-startup),
[icewm-theme(5)](https://ice-wm.org/man/icewm-theme),
[icewm-toolbar(5)](https://ice-wm.org/man/icewm-toolbar),
[icewm-winoptions(5)](https://ice-wm.org/man/icewm-winoptions),
[icewm(1)](https://ice-wm.org/man/icewm),
[icewmbg(1)](https://ice-wm.org/man/icewmbg),
[icewmhint(1)](https://ice-wm.org/man/icewmhint),
[icewmtray(1)](https://ice-wm.org/man/icewmtray).

