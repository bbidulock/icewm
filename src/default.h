#ifdef FONTS_ADOBE
#define FONT(pt) "-b&h-lucida-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDFONT(pt) "-b&h-lucida-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define TTFONT(pt) "-b&h-lucidatypewriter-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDTTFONT(pt) "-b&h-lucidatypewriter-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#else
#define FONT(pt) "-adobe-helvetica-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDFONT(pt) "-adobe-helvetica-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define TTFONT(pt) "-adobe-courier-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDTTFONT(pt) "-adobe-courier-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#endif

#define CONFIG_DEFAULT_LOOK lookNice
#define CONFIG_DEFAULT_THEME "Infadel2/default.theme"

#ifdef CFGDEF
#define XSV(t,a,b) t a(b);
#else
#define XSV(t,a,b) extern t a;
#endif

#ifndef NO_CONFIGURE
#ifdef CFGDEF
#define XIV(t,a,b) t a(b);
#else
#define XIV(t,a,b) extern t a;
#endif
#else
#ifdef CFGDEF
#define XIV(t,a,b)
#else
#define XIV(t,a,b) static const t a(b);  // I hope this can be optimized away
#endif
#endif

/************************************************************************************************************************************************************/

// !!! make these go away (make individual specific options)
#define CONFIG_LOOK_NICE
#define CONFIG_LOOK_WARP3
#define CONFIG_LOOK_WIN95
#define CONFIG_LOOK_WARP4
#define CONFIG_LOOK_MOTIF
#define CONFIG_LOOK_PIXMAP
#define CONFIG_LOOK_METAL
#define CONFIG_LOOK_GTK

#ifndef CFGDEF
typedef enum {
#ifdef CONFIG_LOOK_WIN95
    lookWin95,
#endif
#ifdef CONFIG_LOOK_MOTIF
    lookMotif,
#endif
#ifdef CONFIG_LOOK_WARP3
    lookWarp3,
#endif
#ifdef CONFIG_LOOK_WARP4
    lookWarp4,
#endif
#ifdef CONFIG_LOOK_NICE
    lookNice,
#endif
#ifdef CONFIG_LOOK_PIXMAP
    lookPixmap,
#endif
#ifdef CONFIG_LOOK_METAL
    lookMetal,
#endif
#ifdef CONFIG_LOOK_GTK
    lookGtk,
#endif
    lookMAX
} WMLook;
#endif

#ifdef CONFIG_LOOK_WIN95
#define LOOK_IS_WIN95		(wmLook == lookWin95)
#define CASE_LOOK_WIN95		case lookWin95
#else
#define LOOK_IS_WIN95 false
#define CASE_LOOK_WIN95
#endif

#ifdef CONFIG_LOOK_MOTIF
#define LOOK_IS_MOTIF		(wmLook == lookMotif)
#define CASE_LOOK_MOTIF		case lookMotif
#else
#define LOOK_IS_MOTIF		false
#define CASE_LOOK_MOTIF
#endif

#ifdef CONFIG_LOOK_WARP3
#define LOOK_IS_WARP3		(wmLook == lookWarp3)
#define CASE_LOOK_WARP3		case lookWarp3
#else
#define LOOK_IS_WARP3 false
#define CASE_LOOK_WARP3
#endif

#ifdef CONFIG_LOOK_WARP4
#define LOOK_IS_WARP4		(wmLook == lookWarp4)
#define CASE_LOOK_WARP4		case lookWarp4
#else
#define LOOK_IS_WARP4 false
#define CASE_LOOK_WARP4
#endif

#ifdef CONFIG_LOOK_NICE
#define LOOK_IS_NICE		(wmLook == lookNice)
#define CASE_LOOK_NICE		case lookNice
#else
#define LOOK_IS_NICE false
#define CASE_LOOK_NICE
#endif

#ifdef CONFIG_LOOK_PIXMAP
#define LOOK_IS_PIXMAP		(wmLook == lookPixmap)
#define CASE_LOOK_PIXMAP	case lookPixmap
#else
#define LOOK_IS_PIXMAP false
#define CASE_LOOK_PIXMAP
#endif

#ifdef CONFIG_LOOK_METAL
#define LOOK_IS_METAL		(wmLook == lookMetal)
#define CASE_LOOK_METAL		case lookMetal
#else
#define LOOK_IS_METAL false
#define CASE_LOOK_METAL
#endif

#ifdef CONFIG_LOOK_GTK
#define LOOK_IS_GTK		(wmLook == lookGtk)
#define CASE_LOOK_GTK		case lookGtk
#else
#define LOOK_IS_GTK false
#define CASE_LOOK_GTK
#endif

/************************************************************************************************************************************************************/

XIV(bool, clickFocus,				true)
XIV(bool, raiseOnFocus,				true)
XIV(bool, focusOnClickClient,			true)
XIV(bool, raiseOnClickClient,			true)
XIV(bool, raiseOnClickButton,			true)
XIV(bool, raiseOnClickFrame,			true)
XIV(bool, raiseOnClickTitleBar,			true)
XIV(bool, lowerOnClickWhenRaised,	        false)
XIV(bool, passFirstClickToClient,		true)
XIV(bool, focusOnMap,				true)
XIV(bool, focusOnMapTransient,			true)
XIV(bool, focusOnMapTransientActive,		true)
XIV(bool, focusRootWindow,			false)
XIV(bool, pointerColormap,			true)
XIV(bool, dontRotateMenuPointer,		false)
XIV(bool, sizeMaximized,			false)
XIV(bool, showMoveSizeStatus,			true)
XIV(bool, workspaceSwitchStatus,		true)
XIV(bool, beepOnNewMail,			false)
XIV(bool, warpPointer,				false)
XIV(bool, opaqueMove,				true)
XIV(bool, opaqueResize,				true)
#ifdef CONFIG_TASKBAR
XIV(bool, showTaskBar,				true)
XIV(bool, taskBarAtTop,				false)
XIV(bool, taskBarKeepBelow,			true)
XIV(bool, taskBarShowClock,			true)
XIV(bool, taskBarShowApm,			false)
XIV(bool, taskBarShowApmTime,			true) // mschy
XIV(bool, taskBarShowMailboxStatus,		true)
XIV(bool, taskBarShowStartMenu,			true)
XIV(bool, taskBarShowWindowListMenu,		true)
XIV(bool, taskBarShowWorkspaces,		true)
XIV(bool, taskBarShowWindows,			true)
#ifdef CONFIG_TRAY
XIV(bool, taskBarShowTray,			true)
XIV(bool, trayDrawBevel,			false)
XIV(bool, trayShowAllWindows,			true)
#endif
XIV(bool, taskBarShowAllWindows,		false)
XIV(bool, taskBarShowWindowIcons,		true)
XIV(bool, taskBarAutoHide,			false)
XIV(bool, taskBarDoubleHeight,			false)
XIV(bool, taskBarShowCPUStatus,			true)
XIV(bool, taskBarShowNetStatus,			true)
XIV(bool, taskBarLaunchOnSingleClick,		true)
#endif
XIV(bool, minimizeToDesktop,			false)
XIV(bool, prettyClock,				true)
XIV(bool, manualPlacement,			false)
XIV(bool, smartPlacement,			true)
XIV(bool, centerTransientsOnOwner,		true)
XIV(bool, menuMouseTracking,			false)
XIV(bool, autoRaise,				false)
XIV(bool, delayPointerFocus,			false)
XIV(bool, useMouseWheel,			false)
XIV(bool, quickSwitch,				true)
XIV(bool, quickSwitchToMinimized,		true)
XIV(bool, quickSwitchToHidden,			false)
XIV(bool, quickSwitchToAllWorkspaces,		false)
XIV(bool, quickSwitchAllIcons,			true)
XIV(bool, quickSwitchTextFirst,			false)
XIV(bool, quickSwitchSmallWindow,		false)
XIV(bool, quickSwitchHugeIcon,			true)
XIV(bool, quickSwitchFillSelection,		false)
XIV(bool, countMailMessages,			false)
XIV(bool, strongPointerFocus,			false)
XIV(bool, grabRootWindow,			true)
XIV(bool, snapMove,				true)
XIV(bool, centerBackground,			false)
XIV(bool, supportSemitransparency,		true)
XIV(bool, edgeHorzWorkspaceSwitching,		false)
XIV(bool, edgeVertWorkspaceSwitching,		false)
XIV(bool, edgeContWorkspaceSwitching,		true)
XIV(bool, showPopupsAbovePointer,		false)
XIV(bool, replayMenuCancelClick,		false)
XIV(bool, limitSize,				true)
XIV(bool, limitPosition,			true)
XIV(bool, limitByDockLayer,			false)
XIV(bool, considerHorizBorder,			false)
XIV(bool, considerVertBorder,			false)
XIV(bool, centerMaximizedWindows,		false)
XIV(bool, win95keys,				false)
XIV(bool, modMetaIsCtrlAlt,			true)
XIV(bool, autoReloadMenus,			true)
XIV(bool, showFrameIcon,			true)
XIV(bool, clientMouseActions,			true)
XIV(bool, titleBarCentered,			false)
XIV(bool, titleBarJoinLeft,			false)
XIV(bool, titleBarJoinRight,			false)
XIV(bool, showThemesMenu,			true)
XIV(bool, showLogoutMenu,			true)
XIV(bool, showHelp,				true)
XIV(bool, autoDetectGnome,			false)
#ifdef CONFIG_GNOME_MENUS
XIV(bool, gnomeAppsMenuAtToplevel,		false)
XIV(bool, gnomeUserMenuAtToplevel,		false)
XIV(bool, kdeMenuAtToplevel,			false)
XIV(bool, showGnomeAppsMenu,			true)
XIV(bool, showGnomeUserMenu,			true)
XIV(bool, showKDEMenu,				true)
#ifdef CONFIG_IMLIB
XIV(bool, gnomeFolderIcon,			true)
#endif
#endif
#ifdef CONFIG_IMLIB
XIV(bool, disableImlibCaches,			true)
#endif
XIV(bool, showAddressBar,			true)
XIV(bool, addressBarHistory,                    true)
XIV(bool, confirmLogout,			true)
#ifdef CONFIG_I18N
XIV(bool, multiByte,				true)
#endif
#ifdef CONFIG_XFREETYPE
XIV(bool, haveXft,				true)
#endif
#ifdef CONFIG_SHAPED_DECORATION
XIV(bool, protectClientWindow, 			true)
#endif
XIV(bool, inputDrawBorder,                      true)
XIV(WMLook, wmLook,				CONFIG_DEFAULT_LOOK)
XIV(int, wsBorderX,				6)
XIV(int, wsBorderY,				6)
XIV(int, wsDlgBorderX,				2)
XIV(int, wsDlgBorderY,				2)
XIV(int, wsCornerX,				24)
XIV(int, wsCornerY,				24)
XIV(int, wsTitleBar,				20)
XIV(int, titleBarJustify,			0)
XIV(int, titleBarHorzOffset,			0)
XIV(int, titleBarVertOffset,			0)
XIV(int, scrollBarWidth,			0)
XIV(int, scrollBarHeight,			0)
XIV(int, ClickMotionDistance,			4)
XIV(int, ClickMotionDelay,			200)
XIV(int, MultiClickTime,			400)
#ifdef CONFIG_TOOLTIP
XIV(int, ToolTipDelay,				1000)
XIV(int, ToolTipTime,				5000)
#endif
XIV(int, MenuActivateDelay,			10)
XIV(int, SubmenuActivateDelay,			300)
XIV(int, MenuMaximalWidth,			0)
XIV(int, EdgeResistance,			32)
XIV(int, snapDistance,				8)
XIV(int, pointerFocusDelay,			200)
XIV(int, autoRaiseDelay,			400)
XIV(int, autoHideDelay,				300)
XIV(int, edgeSwitchDelay,			600)
XIV(int, scrollBarStartDelay,			500)
XIV(int, scrollBarDelay,			30)
XIV(int, autoScrollStartDelay,			500)
XIV(int, autoScrollDelay,			60)
XIV(int, workspaceStatusTime,			2500)
XIV(int, useRootButtons,			255)	// bitmask=all
XIV(int, buttonRaiseMask,			1)
XIV(int, rootWinMenuButton,			1)
XIV(int, rootWinListButton,			2)
XIV(int, rootMenuButton,			3)
XIV(int, titleMaximizeButton,			1)
XIV(int, titleRollupButton,			2)
XIV(int, msgBoxDefaultAction,			0)
XIV(int, mailCheckDelay,			30)
XIV(int, taskBarCPUSamples,			20)
XIV(int, quickSwitchHMargin,			3)	// !!!
XIV(int, quickSwitchVMargin,			3)	// !!!
XIV(int, quickSwitchIMargin,			4)	// !!!
XIV(int, quickSwitchIBorder,			2)	// !!!
XIV(int, quickSwitchSepSize,			6)	// !!!
#ifdef CONFIG_MOVESIZE_FX
XIV(int, moveSizeInterior,			0)
XIV(int, moveSizeDimLines,			0)
XIV(int, moveSizeGaugeLines,			0)
XIV(int, moveSizeDimLabels,			0)
XIV(int, moveSizeGeomLabels,			0)
#endif

XSV(const char *, titleButtonsLeft,		"s")
XSV(const char *, titleButtonsRight,		"xmir")
XSV(const char *, titleButtonsSupported,	"xmis");
XSV(const char *, themeName,			CONFIG_DEFAULT_THEME)
XSV(const char *, themeAuthor,			0)
XSV(const char *, themeDescription,		0)
XSV(const char *, titleFontName,		BOLDFONT(120))
XSV(const char *, menuFontName,			BOLDFONT(120))
XSV(const char *, statusFontName,		BOLDTTFONT(120))
XSV(const char *, fxFontName,			BOLDTTFONT(120))
XSV(const char *, switchFontName,		BOLDTTFONT(120))
XSV(const char *, normalButtonFontName,		FONT(120))
XSV(const char *, activeButtonFontName,		BOLDFONT(120))
#ifdef CONFIG_TASKBAR
XSV(const char *, normalTaskBarFontName,	FONT(120))
XSV(const char *, activeTaskBarFontName,	BOLDFONT(120))
XSV(const char *, toolButtonFontName,		"")
XSV(const char *, normalWorkspaceFontName,	"")
XSV(const char *, activeWorkspaceFontName,	"")
#endif
XSV(const char *, minimizedWindowFontName,	FONT(120))
XSV(const char *, listBoxFontName,		FONT(120))
XSV(const char *, toolTipFontName,		FONT(120))
XSV(const char *, labelFontName,		FONT(140))
XSV(const char *, clockFontName,		TTFONT(140))
XSV(const char *, apmFontName,			TTFONT(140))
XSV(const char *, inputFontName,		TTFONT(140))
#ifdef CONFIG_MOVESIZE_FX
XSV(const char *, moveSizeFontName,		BOLDFONT(100))
#endif

XSV(const char *, iconPath,			0)
XSV(const char *, libDir,			LIBDIR)
XSV(const char *, configDir,			CFGDIR)
XSV(const char *, kdeDataDir,			KDEDIR)
XSV(const char *, mailBoxPath,			0)
XSV(const char *, mailCommand,			"xterm -name pine -title PINE -e pine")
XSV(const char *, mailClassHint,		"pine.XTerm")
XSV(const char *, newMailCommand,		0)
XSV(const char *, lockCommand,			"xlock")
XSV(const char *, clockCommand,			"xclock -name icewm -title Clock")
XSV(const char *, clockClassHint,		"icewm.XClock")
XSV(const char *, runDlgCommand,		0)
XSV(const char *, openCommand,			0)
XSV(const char *, terminalCommand,		"xterm")
XSV(const char *, logoutCommand,		0)
XSV(const char *, logoutCancelCommand,		0)
XSV(const char *, shutdownCommand,		"shutdown -h now")
XSV(const char *, rebootCommand,		"shutdown -r now")
XSV(const char *, cpuCommand,			"xterm -name top -title Process\\ Status -e top")
XSV(const char *, cpuClassHint,			"top.XTerm")
XSV(const char *, netCommand,			"xterm -name netstat -title Network\\ Status -e netstat -c")
XSV(const char *, netClassHint,			"netstat.XTerm")
XSV(const char *, netDevice,			"ppp0 eth0")
XSV(const char *, addressBarCommand,		0)
#ifdef CONFIG_I18N
XSV(const char *, fmtTime,			"%X")
XSV(const char *, fmtTimeAlt,			NULL)
XSV(const char *, fmtDate,			"%c")
#else
XSV(const char *, fmtTime,			"%H:%M:%S")
XSV(const char *, fmtTimeAlt,			NULL)
XSV(const char *, fmtDate,			"%B %A %Y-%m-%d %H:%M:%S %Z")
#endif
XSV(const char *, clrDialog,			"rgb:C0/C0/C0")
XSV(const char *, clrActiveBorder,		"rgb:C0/C0/C0")
XSV(const char *, clrInactiveBorder,		"rgb:C0/C0/C0")
XSV(const char *, clrNormalTitleButton,		"rgb:C0/C0/C0")
XSV(const char *, clrNormalTitleButtonText,	"rgb:00/00/00")
XSV(const char *, clrActiveTitleBar,		"rgb:00/00/A0")
XSV(const char *, clrInactiveTitleBar,		"rgb:80/80/80")
XSV(const char *, clrActiveTitleBarText,	"rgb:FF/FF/FF")
XSV(const char *, clrInactiveTitleBarText,	"rgb:00/00/00")
XSV(const char *, clrActiveTitleBarShadow,	"")
XSV(const char *, clrInactiveTitleBarShadow,	"")
XSV(const char *, clrNormalMinimizedWindow,	"rgb:C0/C0/C0")
XSV(const char *, clrNormalMinimizedWindowText,	"rgb:00/00/00")
XSV(const char *, clrActiveMinimizedWindow,	"rgb:E0/E0/E0")
XSV(const char *, clrActiveMinimizedWindowText,	"rgb:00/00/00")
XSV(const char *, clrNormalMenu,		"rgb:C0/C0/C0")
XSV(const char *, clrActiveMenuItem,		"rgb:A0/A0/A0")
XSV(const char *, clrActiveMenuItemText,	"rgb:00/00/00")
XSV(const char *, clrNormalMenuItemText,	"rgb:00/00/00")
XSV(const char *, clrDisabledMenuItemText,	"rgb:80/80/80")
XSV(const char *, clrDisabledMenuItemShadow,	"")
XSV(const char *, clrMoveSizeStatus,		"rgb:C0/C0/C0")
XSV(const char *, clrMoveSizeStatusText,	"rgb:00/00/00")
XSV(const char *, clrQuickSwitch,		"rgb:C0/C0/C0")
XSV(const char *, clrQuickSwitchText,		"rgb:00/00/00")
XSV(const char *, clrQuickSwitchActive,		0)
#ifdef CONFIG_TASKBAR
XSV(const char *, clrDefaultTaskBar,		"rgb:C0/C0/C0")
#endif
XSV(const char *, clrNormalButton,		"rgb:C0/C0/C0")
XSV(const char *, clrNormalButtonText,		"rgb:00/00/00")
XSV(const char *, clrActiveButton,		"rgb:E0/E0/E0")
XSV(const char *, clrActiveButtonText,		"rgb:00/00/00")
XSV(const char *, clrToolButton,		"")
XSV(const char *, clrToolButtonText,		"")
XSV(const char *, clrWorkspaceActiveButton,	"")
XSV(const char *, clrWorkspaceActiveButtonText,	"")
XSV(const char *, clrWorkspaceNormalButton,	"")
XSV(const char *, clrWorkspaceNormalButtonText,	"")
#ifdef CONFIG_TASKBAR
XSV(const char *, clrNormalTaskBarApp,		"rgb:C0/C0/C0")
XSV(const char *, clrNormalTaskBarAppText,	"rgb:00/00/00")
XSV(const char *, clrActiveTaskBarApp,		"rgb:E0/E0/E0")
XSV(const char *, clrActiveTaskBarAppText,	"rgb:00/00/00")
XSV(const char *, clrMinimizedTaskBarApp,	"rgb:A0/A0/A0")
XSV(const char *, clrMinimizedTaskBarAppText,	"rgb:00/00/00")
XSV(const char *, clrInvisibleTaskBarApp,	"rgb:80/80/80")
XSV(const char *, clrInvisibleTaskBarAppText,	"rgb:00/00/00")
#endif
XSV(const char *, clrScrollBar,			"rgb:A0/A0/A0")
XSV(const char *, clrScrollBarArrow,		"rgb:00/00/00")
XSV(const char *, clrScrollBarInactive,         "rgb:80/80/80")
XSV(const char *, clrScrollBarSlider,		"rgb:C0/C0/C0")
XSV(const char *, clrScrollBarButton,		"rgb:C0/C0/C0")
XSV(const char *, clrListBox,			"rgb:C0/C0/C0")
XSV(const char *, clrListBoxText,		"rgb:00/00/00")
XSV(const char *, clrListBoxSelected,		"rgb:80/80/80")
XSV(const char *, clrListBoxSelectedText,	"rgb:00/00/00")
#ifdef CONFIG_TOOLTIP
XSV(const char *, clrToolTip,			"rgb:E0/E0/00")
XSV(const char *, clrToolTipText,		"rgb:00/00/00")
#endif
XSV(const char *, clrClock,			"rgb:00/00/00")
XSV(const char *, clrClockText,			"rgb:00/FF/00")
XSV(const char *, clrApm,			"rgb:00/00/00")
XSV(const char *, clrApmText,			"rgb:00/FF/00")
XSV(const char *, clrInput,			"rgb:FF/FF/FF")
XSV(const char *, clrInputText,			"rgb:00/00/00")
XSV(const char *, clrInputSelection,		"rgb:80/80/80")
XSV(const char *, clrInputSelectionText,	"rgb:00/00/00")
XSV(const char *, clrLabel,			"rgb:C0/C0/C0")
XSV(const char *, clrLabelText,			"rgb:00/00/00")
XSV(const char *, clrCpuUser,			"rgb:00/FF/00")
XSV(const char *, clrCpuSys,			"rgb:FF/00/00")
XSV(const char *, clrCpuNice,			"rgb:00/00/FF")
XSV(const char *, clrCpuIdle,			"rgb:00/00/00")
XSV(const char *, clrNetSend,			"rgb:FF/FF/00")
XSV(const char *, clrNetReceive,		"rgb:FF/00/FF")
XSV(const char *, clrNetIdle,			"rgb:00/00/00")
#ifdef CONFIG_GRADIENTS
XSV(const char *, gradients,			0)
#endif
XSV(const char *, DesktopBackgroundColor,	"rgb:00/20/40")
XSV(const char *, DesktopBackgroundPixmap,	0)
XSV(const char *, DesktopTransparencyColor,	0)
XSV(const char *, DesktopTransparencyPixmap,	0)

#if defined(CFGDEF) && !defined(NO_CONFIGURE)

#ifdef CFGDESC
#define OBV(n,v,d) { n, v, d }
#define OIV(n,v,m,M,d) { n, v, m, M, d }
#define OSV(n,v,d) { n, v, true, d }
#define OKV(n,v,d) { n, &v, d }
#else
#define OBV(n,v,d) { n, v }
#define OIV(n,v,m,M,d) { n, v, m, M }
#define OSV(n,v,d) { n, v, true }
#define OKV(n,v,d) { n, &v }
#endif

static struct {
    const char *option;
    bool *value;
#ifdef CFGDESC
    const char *description;
#endif
} bool_options[] = {
    OBV("ClickToFocus",				&clickFocus,			"Focus windows by clicking"),
    OBV("RaiseOnFocus",				&raiseOnFocus,			"Raise windows when focused"),
    OBV("FocusOnClickClient",			&focusOnClickClient,		"Focus window when client area clicked"),
    OBV("RaiseOnClickClient",			&raiseOnClickClient,		"Raise window when client area clicked"),
    OBV("RaiseOnClickTitleBar",			&raiseOnClickTitleBar,		"Raise window when title bar is clicked"),
    OBV("RaiseOnClickButton",			&raiseOnClickButton,		"Raise window when frame button is clicked"),
    OBV("RaiseOnClickFrame",			&raiseOnClickFrame,		"Raise window when frame border is clicked"),
    OBV("LowerOnClickWhenRaised",               &lowerOnClickWhenRaised,	"Lower the active window when clicked again"),
    OBV("PassFirstClickToClient",		&passFirstClickToClient,	"Pass focusing click on client area to client"),
    OBV("FocusOnMap",				&focusOnMap,			"Focus normal window when initially mapped"),
    OBV("FocusOnMapTransient",			&focusOnMapTransient,		"Focus dialog window when initially mapped"),
    OBV("FocusOnMapTransientActive",		&focusOnMapTransientActive,	"Focus dialog window when initially mapped only if parent frame focused"),
    OBV("PointerColormap",			&pointerColormap,		"Colormap focus follows pointer"),
    OBV("DontRotateMenuPointer",		&dontRotateMenuPointer,		"Don't rotate the cursor for popup menus"),
    OBV("LimitSize",				&limitSize,			"Limit initial size of windows to screen"),
    OBV("LimitPosition",			&limitPosition,			"Limit initial position of windows to screen"),
    OBV("LimitByDockLayer",			&limitByDockLayer,		"Let the Dock layer limit the workspace (incompatible with GNOME Panel)"),
    OBV("ConsiderHBorder",			&considerHorizBorder,		"Consider border frames when maximizing horizontally"),
    OBV("ConsiderVBorder",			&considerVertBorder,		"Consider border frames when maximizing vertically"),
    OBV("CenterMaximizedWindows",		&centerMaximizedWindows,	"Center maximized windows which can't fit the screen (like terminals)"),
    OBV("SizeMaximized",			&sizeMaximized,			"Maximized windows can be resized"),
    OBV("ShowMoveSizeStatus",			&showMoveSizeStatus,		"Show position status window during move/resize"),
    OBV("ShowWorkspaceStatus",			&workspaceSwitchStatus,		"Show name of current workspace while switching"),
    OBV("MinimizeToDesktop",			&minimizeToDesktop,		"Display mini-icons on desktop for minimized windows"),
    OBV("StrongPointerFocus",			&strongPointerFocus,		"Always maintain focus under mouse window (makes some keyboard support non-functional or unreliable"),
    OBV("OpaqueMove",				&opaqueMove,			"Opaque window move"),
    OBV("OpaqueResize",				&opaqueResize,			"Opaque window resize"),
    OBV("ManualPlacement",			&manualPlacement,		"Windows initially placed manually by user"),
    OBV("SmartPlacement",			&smartPlacement,		"Smart window placement (minimal overlap)"),
    OBV("CenterTransientsOnOwner",		&centerTransientsOnOwner,	"Center dialogs on owner window"),
    OBV("MenuMouseTracking",			&menuMouseTracking,		"Menus track mouse even with no mouse buttons held"),
    OBV("AutoRaise",				&autoRaise,			"Auto raise windows after delay"),
    OBV("DelayPointerFocus",			&delayPointerFocus,		"Delay pointer focusing when mouse moves"),
    OBV("Win95Keys",				&win95keys,			"Support win95 keyboard keys (Penguin/Meta/Win_L,R shows menu)"),
    OBV("ModMetaIsCtrlAlt",			&modMetaIsCtrlAlt,		"Treat Penguin/Meta/Win modifier as Ctrl+Alt"),
    OBV("UseMouseWheel",			&useMouseWheel,			"Support mouse wheel"),
    OBV("ShowPopupsAbovePointer",		&showPopupsAbovePointer,	"Show popup menus above mouse pointer"),
    OBV("ReplayMenuCancelClick",		&replayMenuCancelClick,		"Send the clicks outside menus to target window"),
    OBV("QuickSwitch",				&quickSwitch,			"Alt+Tab window switching"),
    OBV("QuickSwitchToMinimized",		&quickSwitchToMinimized,	"Alt+Tab to minimized windows"),
    OBV("QuickSwitchToHidden",			&quickSwitchToHidden,		"Alt+Tab to hidden windows"),
    OBV("QuickSwitchToAllWorkspaces",		&quickSwitchToAllWorkspaces,	"Alt+Tab to windows on other workspaces"),
    OBV("QuickSwitchAllIcons",			&quickSwitchAllIcons,		"Show all reachable icons when quick switching"),
    OBV("QuickSwitchTextFirst",			&quickSwitchTextFirst,		"Show the window title above (all reachable) icons"),
    OBV("QuickSwitchSmallWindow",		&quickSwitchSmallWindow,	"Attempt to create a small QuickSwitch window (1/3 instead of 3/5 of screen width)"),
    OBV("QuickSwitchHugeIcon",			&quickSwitchHugeIcon,		"Show the huge (48x48) of the window icon for the active window"),
    OBV("QuickSwitchFillSelection",		&quickSwitchFillSelection,	"Fill the rectangle highlighting the current icon"),
    OBV("GrabRootWindow",			&grabRootWindow,		"Manage root window (EXPERIMENTAL - normally enabled!)"),
    OBV("SnapMove",				&snapMove,			"Snap to nearest screen edge/window when moving windows"),
    OBV("EdgeSwitch",				&edgeHorzWorkspaceSwitching,	"Workspace switches by moving mouse to left/right screen edge"),
    OBV("HorizontalEdgeSwitch",		        &edgeHorzWorkspaceSwitching,	"Workspace switches by moving mouse to left/right screen edge"),
    OBV("VerticalEdgeSwitch",			&edgeVertWorkspaceSwitching,	"Workspace switches by moving mouse to top/bottom screen edge"),
    OBV("ContinuousEdgeSwitch",                 &edgeContWorkspaceSwitching,    "Workspace switches continuously when moving mouse to screen edge"),
    OBV("DesktopBackgroundCenter",		&centerBackground,		"Display desktop background centered and not tiled"),
    OBV("SupportSemitransparency",		&supportSemitransparency,	"Support for semitransparent terminals like Eterm or gnome-terminal"),
    OBV("AutoReloadMenus",			&autoReloadMenus,		"Reload menu files automatically"),
    OBV("ShowMenuButtonIcon",			&showFrameIcon,			"Show application icon over menu button"),
#ifdef CONFIG_TASKBAR
    OBV("ShowTaskBar",				&showTaskBar,			"Show task bar"),
    OBV("TaskBarAtTop",				&taskBarAtTop,			"Task bar at top of the screen"),
    OBV("TaskBarKeepBelow",			&taskBarKeepBelow,		"Keep the task bar below regular windows"),
    OBV("TaskBarAutoHide",			&taskBarAutoHide,		"Auto hide task bar after delay"),
    OBV("TaskBarShowClock",			&taskBarShowClock,		"Show clock on task bar"),
    OBV("TaskBarShowAPMStatus",			&taskBarShowApm,		"Show APM status on task bar"),
    OBV("TaskBarShowAPMTime",			&taskBarShowApmTime,		"Show APM status on task bar in time-format"),	// mschy
    OBV("TaskBarClockLeds",			&prettyClock,			"Task bar clock uses nice pixmapped LCD display"),
    OBV("TaskBarShowMailboxStatus",		&taskBarShowMailboxStatus,	"Show mailbox status on task bar"),
    OBV("TaskBarMailboxStatusBeepOnNewMail",	&beepOnNewMail,			"Beep when new mail arrives"),
    OBV("TaskBarMailboxStatusCountMessages",	&countMailMessages,		"Count messages in mailbox"),
    OBV("TaskBarShowWorkspaces",		&taskBarShowWorkspaces,		"Show workspace switching buttons on task bar"),
    OBV("TaskBarShowWindows",			&taskBarShowWindows,		"Show windows on the taskbar"),
#ifdef CONFIG_TRAY
    OBV("TaskBarShowTray",			&taskBarShowTray,		"Show windows in the tray"),
    OBV("TrayDrawBevel",			&trayDrawBevel,			"Surround the tray with plastic border"),
    OBV("TrayShowAllWindows",			&trayShowAllWindows,		"Show windows from all workspaces on tray"),
#endif
    OBV("TaskBarShowAllWindows",		&taskBarShowAllWindows,		"Show windows from all workspaces on task bar"),
    OBV("TaskBarShowWindowIcons",		&taskBarShowWindowIcons,	"Show icons of windows on the task bar"),
    OBV("TaskBarShowStartMenu",			&taskBarShowStartMenu,		"Show 'Start' menu on task bar"),
    OBV("TaskBarShowWindowListMenu",		&taskBarShowWindowListMenu,	"Show 'window list' menu on task bar"),
    OBV("TaskBarShowCPUStatus",			&taskBarShowCPUStatus,		"Show CPU status on task bar (Linux 		& Solaris)"),
    OBV("TaskBarShowNetStatus",			&taskBarShowNetStatus,		"Show network status on task bar (Linux only)"),
    OBV("TaskBarDoubleHeight",			&taskBarDoubleHeight,		"Use double-height task bar"),
    OBV("TaskBarLaunchOnSingleClick",		&taskBarLaunchOnSingleClick,	"Execute taskbar applet commands (like MailCommand,	ClockCommand,	...) on single click"),
#endif
    OBV("WarpPointer",				&warpPointer,			"Move mouse when doing focusing in pointer focus mode"),
    OBV("ClientWindowMouseActions",		&clientMouseActions,		"Allow mouse actions on client windows (buggy with some programs)"),
    OBV("TitleBarCentered",			&titleBarCentered,		"Draw window title centered (obsoleted by TitleBarJustify)"),
    OBV("TitleBarJoinLeft",			&titleBarJoinLeft,		"Join title*S and title*T"),
    OBV("TitleBarJoinRight",			&titleBarJoinRight,		"Join title*T and title*B"),
    OBV("ShowThemesMenu",			&showThemesMenu,		"Show themes submenu"),
    OBV("ShowLogoutMenu",			&showLogoutMenu,		"Show logout submenu"),
    OBV("ShowHelp",				&showHelp,			"Show the help menu item"),
    OBV("AutoDetectGNOME",			&autoDetectGnome,		"Automatically disable some functionality when running under GNOME."),
#ifdef CONFIG_GNOME_MENUS
    OBV("GNOMEAppsMenuAtToplevel",		&gnomeAppsMenuAtToplevel,	"Create GNOME application menu at toplevel"),
    OBV("GNOMEUserMenuAtToplevel",		&gnomeUserMenuAtToplevel,	"Create GNOME user menu at toplevel"),
    OBV("KDEMenuAtToplevel",			&kdeMenuAtToplevel,		"Create KDE menu at toplevel"),
    OBV("ShowGnomeAppsMenu",			&showGnomeAppsMenu,		"Show GNOME application menu when possible"),
    OBV("ShowGnomeUserMenu",			&showGnomeUserMenu,		"Show GNOME user menu when possible"),
    OBV("ShowKDEMenu",				&showKDEMenu,			"Show KDE menu when possible"),
#ifdef CONFIG_IMLIB
    OBV("GNOMEFolderIcon",			&gnomeFolderIcon,		"Show GNOME's folder icon in GNOME menus"),
    OBV("DisableImlibCaches",			&disableImlibCaches,		"Disable Imlib's image/pixmap caches"),
#endif
#endif
    OBV("ShowAddressBar",			&showAddressBar,		"Show address bar in task bar"),
    OBV("AddressBarHistory",                    &addressBarHistory,             "Store the address bar's history"),
#ifdef CONFIG_I18N
    OBV("MultiByte",				&multiByte,			"Overrides automatic multiple byte detection"),
#endif
#ifdef CONFIG_XFREETYPE
    OBV("XFreeType",				&haveXft,			"Overrides automatic Xrender detection"),
#endif
    OBV("ConfirmLogout",			&confirmLogout,			"Confirm logout"),
#ifdef CONFIG_SHAPED_DECORATION
    OBV("ShapesProtectClientWindow",		&protectClientWindow,		"Don't cut client windows by shapes set trough frame corner pixmap"),
#endif
    OBV("InputDrawBorder",                      &inputDrawBorder,               "Draw a border arround input fields"),
};

static struct {
    const char *option;
    int *value;
    int min,	max;
#ifdef CFGDESC
    const char *description;
#endif
} uint_options[] = {
    OIV("BorderSizeX",				&wsBorderX, 0, 128,		"Horizontal window border"),
    OIV("BorderSizeY",				&wsBorderY, 0, 128,		"Vertical window border"),
    OIV("DlgBorderSizeX",			&wsDlgBorderX, 0, 128,		"Horizontal dialog window border"),
    OIV("DlgBorderSizeY",			&wsDlgBorderY, 0, 128,		"Vertical dialog window border"),
    OIV("CornerSizeX",				&wsCornerX, 0, 64,		"Resize corner width"),
    OIV("CornerSizeY",				&wsCornerY, 0, 64,		"Resize corner height"),
    OIV("TitleBarHeight",			&wsTitleBar, 0, 128,		"Title bar height"),
    OIV("TitleBarJustify",			&titleBarJustify, 0, 100,	"Justification of the window title"),
    OIV("TitleBarHorzOffset",			&titleBarHorzOffset, -128, 128,	"Horizontal offset for the window title text"),
    OIV("TitleBarVertOffset",			&titleBarVertOffset, -128, 128,	"Vertical offset for the window title text"),
    OIV("ScrollBarX",				&scrollBarWidth, 0, 64,		"Scrollbar width"),
    OIV("ScrollBarY",				&scrollBarHeight, 0, 64,	"Scrollbar (button) height"),
    OIV("ClickMotionDistance",			&ClickMotionDistance, 0, 32,	"Pointer motion distance before click gets interpreted as drag"),
    OIV("ClickMotionDelay",			&ClickMotionDelay, 0, 2000,	"Delay before click gets interpreted as drag"),
    OIV("MultiClickTime",			&MultiClickTime, 0, 5000,	"Multiple click time"),
    OIV("MenuActivateDelay",			&MenuActivateDelay, 0, 5000,	"Delay before activating menu items"),
    OIV("SubmenuMenuActivateDelay",		&SubmenuActivateDelay, 0, 5000,	"Delay before activating menu submenus"),
    OIV("MenuMaximalWidth",			&MenuMaximalWidth, 0, 16384,	"Maximal width of popup menus,	2/3 of the screen's width if set to zero"),
#ifdef CONFIG_TOOLTIP
    OIV("ToolTipDelay",				&ToolTipDelay, 0, 5000,		"Delay before tooltip window is displayed"),
    OIV("ToolTipTime",				&ToolTipTime, 0, 60000,		"Time before tooltip window is hidden (0 means never"),
#endif
    OIV("AutoHideDelay",			&autoHideDelay, 0, 5000,	"Delay before task bar is automatically hidden"),
    OIV("AutoRaiseDelay",			&autoRaiseDelay, 0, 5000,	"Delay before windows are auto raised"),
    OIV("EdgeResistance",			&EdgeResistance, 0, 10000,	"Resistance in pixels when trying to move windows off the screen (10000 = infinite)"),
    OIV("PointerFocusDelay",			&pointerFocusDelay, 0, 1000,	"Delay for pointer focus switching"),
    OIV("SnapDistance",				&snapDistance, 0, 64,		"Distance in pixels before windows snap together"),
    OIV("EdgeSwitchDelay",			&edgeSwitchDelay, 0, 5000,	"Screen edge workspace switching delay"),
    OIV("ScrollBarStartDelay",			&scrollBarStartDelay, 0, 5000,	"Inital scroll bar autoscroll delay"),
    OIV("ScrollBarDelay",			&scrollBarDelay, 0, 5000,	"Scroll bar autoscroll delay"),
    OIV("AutoScrollStartDelay",			&autoScrollStartDelay, 0, 5000,	"Auto scroll start delay"),
    OIV("AutoScrollDelay",			&autoScrollDelay, 0, 5000,	"Auto scroll delay"),
    OIV("WorkspaceStatusTime",			&workspaceStatusTime, 0, 2500,	"Time before workspace status window is hidden"),
    OIV("UseRootButtons",			&useRootButtons, 0, 255,	"Bitmask of root window button click to use in window manager"),
    OIV("ButtonRaiseMask",			&buttonRaiseMask, 0, 255,	"Bitmask of buttons that raise the window when pressed"),
    OIV("DesktopWinMenuButton",			&rootWinMenuButton, 0, 20,	"Desktop mouse-button click to show the menu"),
    OIV("DesktopWinListButton",			&rootWinListButton, 0, 5,	"Desktop mouse-button click to show the window list"),
    OIV("DesktopMenuButton",			&rootMenuButton, 0, 20,		"Desktop mouse-button click to show the window list menu"),
    OIV("TitleBarMaximizeButton",		&titleMaximizeButton, 0, 5,	"TitleBar mouse-button double click to maximize the window"),
    OIV("TitleBarRollupButton",			&titleRollupButton, 0, 5,	"TitleBar mouse-button double clock to rollup the window"),
    OIV("MsgBoxDefaultAction",			&msgBoxDefaultAction, 0, 1,	"Preselect to Cancel (0) or the OK (1) button in message boxes"),
    OIV("MailCheckDelay",			&mailCheckDelay, 0, (3600*24),	"Delay between new-mail checks. (seconds)"),
    OIV("QuickSwitchHorzMargin",		&quickSwitchHMargin, 0, 64,	"Horizontal margin of the quickswitch window"),
    OIV("QuickSwitchVertMargin",		&quickSwitchVMargin, 0, 64,	"Vertical margin of the quickswitch window"),
    OIV("QuickSwitchIconMargin",		&quickSwitchIMargin, 0, 64,	"Vertical margin in the quickswitch window"),
    OIV("QuickSwitchIconBorder",		&quickSwitchIBorder, 0, 64,	"Distance between the active icon and it�s border"),
    OIV("QuickSwitchSeparatorSize",		&quickSwitchSepSize, 0, 64,	"Height of the separator between (all reachable) icons and text, 0 to avoid it"),
#ifdef CONFIG_TASKBAR
    OIV("TaskBarCPUSamples",			&taskBarCPUSamples, 2, 1000,	"Width of CPU Monitor"),
#endif

#ifdef CONFIG_MOVESIZE_FX
    OIV("MoveSizeInterior",			&moveSizeInterior, 0, 31,	"Bitmask for inner decorations (1: border style, 2: titlebar, 4/8/16: grid)"),
    OIV("MoveSizeDimensionLines",		&moveSizeDimLines, 0, 4095,	"Bitmask for dimension lines (1/2/4: top left/center/right, 8/16/32: left top/middle/bottom, ...)"),
    OIV("MoveSizeGaugeLines",			&moveSizeGaugeLines, 0, 15,	"Bitmask for gauge lines (1/2/4/8: top/left/right/bottom)"),
    OIV("MoveSizeDimensionLabels",		&moveSizeDimLabels, 0, 4095,	"Bitmask for dimension labels (1/2/4: top left/center/right, 8/16/32: left top/middle/bottom, ...)"),
    OIV("MoveSizeGeometryLabels",		&moveSizeGeomLabels, 0, 127,	"Bitmask for geometry labels (1/2/4: top left/center/right, 8: center, ...)"),
#endif
};

static struct {
    const char *option;
    const char **value;
    bool initial;
#ifdef CFGDESC
    const char *description;
#endif
} string_options[] = {
    OSV("Theme",				&themeName,			"Theme name"),
    OSV("ThemeAuthor",				&themeAuthor,			"Theme author, e-mail address, credits"),
    OSV("ThemeDescription",			&themeDescription,		"Description of the theme, credits"),

    OSV("TitleButtonsLeft",			&titleButtonsLeft,		"Titlebar buttons from left to right (x=close,	m=max,	i=min,	h=hide,	r=rollup,	s=sysmenu,	d=depth)"),
    OSV("TitleButtonsRight",			&titleButtonsRight,		"Titlebar buttons from right to left (x=close,	m=max,	i=min,	h=hide,	r=rollup,	s=sysmenu,	d=depth)"),
    OSV("TitleButtonsSupported",		&titleButtonsSupported,		"Titlebar buttons supported by theme (x,m,i,r,h,s,d)"),
#ifdef CONFIG_GRADIENTS
    OSV("Gradients",				&gradients,			"List of gradient pixmaps in the current theme"),
#endif
    OSV("IconPath",				&iconPath,			"Icon search path (colon separated)"),
    OSV("KDEDataDir",				&kdeDataDir,			"Root directory for KDE data"),
    OSV("MailBoxPath",				&mailBoxPath,			"Mailbox path (use $MAIL instead)"),
    OSV("MailCommand",				&mailCommand,			"Command to run on mailbox"),
    OSV("MailClassHint",			&mailClassHint,			"WM_CLASS to allow runonce for MailCommand"),
    OSV("NewMailCommand",			&newMailCommand,		"Command to run when new mail arrives"),
    OSV("LockCommand",				&lockCommand,			"Command to lock display/screensaver"),
    OSV("ClockCommand",				&clockCommand,			"Command to run on clock"),
    OSV("ClockClassHint",			&clockClassHint,		"WM_CLASS to allow runonce for ClockCommand"),
    OSV("RunCommand",				&runDlgCommand,			"Command to select and run a program"),
    OSV("OpenCommand",				&openCommand,			""),
    OSV("TerminalCommand",			&terminalCommand,		"Terminal emulator must accept -e option."),
    OSV("LogoutCommand",			&logoutCommand,			"Command to start logout"),
    OSV("LogoutCancelCommand",			&logoutCancelCommand,		"Command to cancel logout"),
    OSV("ShutdownCommand",			&shutdownCommand,		"Command to shutdown the system"),
    OSV("RebootCommand",			&rebootCommand,			"Command to reboot the system"),
    OSV("CPUStatusCommand",			&cpuCommand,			"Command to run on CPU status"),
    OSV("CPUStatusClassHint",			&cpuClassHint,			"WM_CLASS to allow runonce for CPUStatusCommand"),
    OSV("NetStatusCommand",			&netCommand,			"Command to run on Net status"),
    OSV("NetStatusClassHint",			&netClassHint,			"WM_CLASS to allow runonce for NetStatusCommand"),
    OSV("AddressBarCommand",			&addressBarCommand,		"Command to run for address bar entries"),
    OSV("NetworkStatusDevice",			&netDevice,			"Network device to show status for"),
    OSV("TimeFormat",				&fmtTime,			"Clock Time format (strftime format string)"),
    OSV("TimeFormatAlt",			&fmtTimeAlt,			"Alternate Clock Time format (e.g. for blinking effects)"),
    OSV("DateFormat",				&fmtDate,			"Clock Date format for tooltip (strftime format string)"),

/************************************************************************************************************************************************************
 * Font definitions
 ************************************************************************************************************************************************************/
    OSV("TitleFontName",			&titleFontName,			""),
    OSV("MenuFontName",				&menuFontName,			""),
    OSV("StatusFontName",			&statusFontName,		""),
    OSV("FxFontName",				&fxFontName,			""),
    OSV("QuickSwitchFontName",			&switchFontName,		""),
    OSV("NormalButtonFontName",			&normalButtonFontName,		""),
    OSV("ActiveButtonFontName",			&activeButtonFontName,		""),
#ifdef CONFIG_TASKBAR
    OSV("NormalTaskBarFontName",		&normalTaskBarFontName,		""),
    OSV("ActiveTaskBarFontName",		&activeTaskBarFontName,		""),
    OSV("ToolButtonFontName",			&toolButtonFontName,		"fallback: NormalButtonFontName"),
    OSV("NormalWorkspaceFontName",		&normalWorkspaceFontName,	"fallback: NormalButtonFontName"),
    OSV("ActiveWorkspaceFontName",		&activeWorkspaceFontName,	"fallback: ActiveButtonFontName"),
#endif
    OSV("MinimizedWindowFontName",		&minimizedWindowFontName,	""),
    OSV("ListBoxFontName",			&listBoxFontName,		""),
    OSV("ToolTipFontName",			&toolTipFontName,		""),
    OSV("ClockFontName",			&clockFontName,			""),
    OSV("ApmFontName",				&apmFontName,			""),
    OSV("InputFontName",			&inputFontName,			""),
    OSV("LabelFontName",			&labelFontName,			""),
#ifdef CONFIG_MOVESIZE_FX    
    OSV("moveSizeFontName",			&moveSizeFontName,		BOLDFONT(100)),
#endif    

/************************************************************************************************************************************************************
 * Color definitions
 ************************************************************************************************************************************************************/
    OSV("ColorDialog",				&clrDialog,			"Background of dialog windows"),
    OSV("ColorNormalBorder",			&clrInactiveBorder,		"Border of inactive windows"),
    OSV("ColorActiveBorder",			&clrActiveBorder,		"Border of active windows"),

    OSV("ColorNormalButton",			&clrNormalButton,		"Background of regular buttons"),
    OSV("ColorNormalButtonText",		&clrNormalButtonText,		"Textcolor of regular buttons"),
    OSV("ColorActiveButton",			&clrActiveButton,		"Background of pressed buttons"),
    OSV("ColorActiveButtonText",		&clrActiveButtonText,		"Textcolor of pressed buttons"),
    OSV("ColorNormalTitleButton",		&clrNormalTitleButton,		"Background of titlebar buttons"),
    OSV("ColorNormalTitleButtonText",		&clrNormalTitleButtonText,	"Textcolor of titlebar buttons"),
    OSV("ColorToolButton",			&clrToolButton,			"Background of toolbar buttons, ColorNormalButton is used if empty"),
    OSV("ColorToolButtonText",			&clrToolButtonText,		"Textcolor of toolbar buttons, ColorNormalButtonText is used if empty"),
    OSV("ColorNormalWorkspaceButton",		&clrWorkspaceNormalButton,	"Background of workspace buttons, ColorNormalButton is used if empty"),
    OSV("ColorNormalWorkspaceButtonText",	&clrWorkspaceNormalButtonText,	"Textcolor of workspace buttons, ColorNormalButtonText is used if empty"),
    OSV("ColorActiveWorkspaceButton",		&clrWorkspaceActiveButton,	"Background of the active workspace button, ColorActiveButton is used if empty"),
    OSV("ColorActiveWorkspaceButtonText",	&clrWorkspaceActiveButtonText, "Textcolor of the active workspace button, ColorActiveButtonText is used if empty"),

    OSV("ColorNormalTitleBar",			&clrInactiveTitleBar,		"Background of the titlebar of regular windows"),
    OSV("ColorNormalTitleBarText",		&clrInactiveTitleBarText,	"Textcolor of the titlebar of regular windows"),
    OSV("ColorNormalTitleBarShadow",		&clrInactiveTitleBarShadow,	"Textshadow of the titlebar of regular windows"),
    OSV("ColorActiveTitleBar",			&clrActiveTitleBar,		"Background of the titlebar of active windows"),
    OSV("ColorActiveTitleBarText",		&clrActiveTitleBarText,		"Textcolor of the titlebar of active windows"),
    OSV("ColorActiveTitleBarShadow",		&clrActiveTitleBarShadow,	"Textshadow of the titlebar of active windows"),

    OSV("ColorNormalMinimizedWindow",		&clrNormalMinimizedWindow,	"Background for mini icons of regular windows"),
    OSV("ColorNormalMinimizedWindowText",	&clrNormalMinimizedWindowText,	"Textcolor for mini icons of regular windows"),
    OSV("ColorActiveMinimizedWindow",		&clrActiveMinimizedWindow,	"Background for mini icons of active windows"),
    OSV("ColorActiveMinimizedWindowText",	&clrActiveMinimizedWindowText,	"Textcolor for mini icons of active windows"),

    OSV("ColorNormalMenu",			&clrNormalMenu,			"Background of pop-up menus"),
    OSV("ColorNormalMenuItemText",		&clrNormalMenuItemText,		"Textcolor of regular menu items"),
    OSV("ColorActiveMenuItem",			&clrActiveMenuItem,		"Background of selected menu item, leave empty to force transparency"),
    OSV("ColorActiveMenuItemText",		&clrActiveMenuItemText,		"Textcolor of selected menu items"),
    OSV("ColorDisabledMenuItemText",		&clrDisabledMenuItemText,	"Textcolor of disabled menu items"),
    OSV("ColorDisabledMenuItemShadow",		&clrDisabledMenuItemShadow,	"Shadow of regular menu items"),

    OSV("ColorMoveSizeStatus",			&clrMoveSizeStatus,		"Background of move/resize status window"),
    OSV("ColorMoveSizeStatusText",		&clrMoveSizeStatusText,		"Textcolor of move/resize status window"),
    
    OSV("ColorQuickSwitch",			&clrQuickSwitch,		"Background of the quick switch window"),
    OSV("ColorQuickSwitchText",			&clrQuickSwitchText,		"Textcolor in the quick switch window"),
    OSV("ColorQuickSwitchActive",		&clrQuickSwitchActive,		"Rectangle arround the active icon in the quick switch window"),
#ifdef CONFIG_TASKBAR
    OSV("ColorDefaultTaskBar",			&clrDefaultTaskBar,		"Background of the taskbar"),
    OSV("ColorNormalTaskBarApp",		&clrNormalTaskBarApp,		"Background for task buttons of regular windows"),
    OSV("ColorNormalTaskBarAppText",		&clrNormalTaskBarAppText,	"Textcolor for task buttons of regular windows"),
    OSV("ColorActiveTaskBarApp",		&clrActiveTaskBarApp,		"Background for task buttons of the active window"),
    OSV("ColorActiveTaskBarAppText",		&clrActiveTaskBarAppText,	"Textcolor for task buttons of the active window"),
    OSV("ColorMinimizedTaskBarApp",		&clrMinimizedTaskBarApp,	"Background for task buttons of minimized windows"),
    OSV("ColorMinimizedTaskBarAppText",		&clrMinimizedTaskBarAppText,	"Textcolor for task buttons of minimized windows"),
    OSV("ColorInvisibleTaskBarApp",		&clrInvisibleTaskBarApp,	"Background for task buttons of windows on other workspaces"),
    OSV("ColorInvisibleTaskBarAppText",		&clrInvisibleTaskBarAppText,	"Textcolor for task buttons of windows on other workspaces"),
#endif
    OSV("ColorScrollBar",			&clrScrollBar,			"Scrollbar background (sliding area)"),
    OSV("ColorScrollBarSlider",			&clrScrollBarSlider,		"Background of the slider button in scrollbars"),
    OSV("ColorScrollBarButton",			&clrScrollBarButton,		"Background of the arrow buttons in scrollbars"),
    OSV("ColorScrollBarArrow",			&clrScrollBarButton,		"Background of the arrow buttons in scrollbars (obsolete)"),
    OSV("ColorScrollBarButtonArrow",		&clrScrollBarArrow,		"Color of active arrows on scrollbar buttons"),
    OSV("ColorScrollBarInactiveArrow",		&clrScrollBarInactive,          "Color of inactive arrows on scrollbar buttons"),

    OSV("ColorListBox",				&clrListBox,			"Background of listboxes"),
    OSV("ColorListBoxText",			&clrListBoxText,		"Textcolor in listboxes"),
    OSV("ColorListBoxSelection",		&clrListBoxSelected,		"Background of selected listbox items"),
    OSV("ColorListBoxSelectionText",		&clrListBoxSelectedText,	"Textcolor of selected listbox items"),
#ifdef CONFIG_TOOLTIP
    OSV("ColorToolTip",				&clrToolTip,			"Background of tooltips"),
    OSV("ColorToolTipText",			&clrToolTipText,		"Textcolor of tooltips"),
#endif
    OSV("ColorLabel",				&clrLabel,			"Background of labels, leave empty to force transparency"),
    OSV("ColorLabelText",			&clrLabelText,			"Textcolor of labels"),
    OSV("ColorInput",				&clrInput,			"Background of text entry fields (e.g. the addressbar)"),
    OSV("ColorInputText",			&clrInputText,			"Textcolor of text entry fields (e.g. the addressbar)"),
    OSV("ColorInputSelection",			&clrInputSelection,		"Background of selected text in an entry field"),
    OSV("ColorInputSelectionText",		&clrInputSelectionText,		"Selected text in an entry field"),

#ifdef CONFIG_APPLET_CLOCK
    OSV("ColorClock",				&clrClock,			"Background of non-LCD clock, leave empty to force transparency"),
    OSV("ColorClockText",			&clrClockText,			"Background of non-LCD monitor"),
#endif    
#ifdef CONFIG_APPLET_APM
    OSV("ColorApm",				&clrApm,			"Background of APM monitor, leave empty to force transparency"),
    OSV("ColorApmText",				&clrApmText,			"Textcolor of APM monitor"),
#endif    
#ifdef CONFIG_APPLET_CPU_STATUS
    OSV("ColorCPUStatusUser",			&clrCpuUser,			"User load on the CPU monitor"),
    OSV("ColorCPUStatusSystem",			&clrCpuSys,			"System load on the CPU monitor"),
    OSV("ColorCPUStatusNice",			&clrCpuNice,			"Nice load on the CPU monitor"),
    OSV("ColorCPUStatusIdle",			&clrCpuIdle,			"Idle (non) load on the CPU monitor, leave empty to force transparency"),
#endif    
#ifdef CONFIG_APPLET_NET_STATUS
    OSV("ColorNetSend",				&clrNetSend,			"Outgoing load on the network monitor"),
    OSV("ColorNetReceive",			&clrNetReceive,			"Incoming load on the network monitor"),
    OSV("ColorNetIdle",				&clrNetIdle,			"Idle (non) load on the network monitor, leave empty to force transparency"),
#endif

    OSV("DesktopBackgroundColor",		&DesktopBackgroundColor,	"Desktop background color"),
    OSV("DesktopBackgroundImage",		&DesktopBackgroundPixmap,	"Desktop background image"),
    OSV("DesktopTransparencyColor",		&DesktopTransparencyColor,	"Color to announce for semi-transparent windows"),
    OSV("DesktopTransparencyImage",		&DesktopTransparencyPixmap,	"Image to announce for semi-transparent windows"),
};

#ifndef NO_KEYBIND
static struct {
    const char *option;
    WMKey *value;
#ifdef CFGDESC
    const char *description;
#endif
} key_options[] = {
/************************************************************************************************************************************************************
 * Keybindings
 ************************************************************************************************************************************************************/
    OKV("KeyWinRaise",				gKeyWinRaise,			""),
    OKV("KeyWinOccupyAll",			gKeyWinOccupyAll,		""),
    OKV("KeyWinLower",				gKeyWinLower,			""),
    OKV("KeyWinClose",				gKeyWinClose,			""),
    OKV("KeyWinRestore",			gKeyWinRestore,			""),
    OKV("KeyWinPrev",				gKeyWinPrev,			""),
    OKV("KeyWinNext",				gKeyWinNext,			""),
    OKV("KeyWinMove",				gKeyWinMove,			""),
    OKV("KeyWinSize",				gKeyWinSize,			""),
    OKV("KeyWinMinimize",			gKeyWinMinimize,		""),
    OKV("KeyWinMaximize",			gKeyWinMaximize,		""),
    OKV("KeyWinMaximizeVert",			gKeyWinMaximizeVert,		""),
    OKV("KeyWinHide",				gKeyWinHide,			""),
    OKV("KeyWinRollup",				gKeyWinRollup,			""),
    OKV("KeyWinMenu",				gKeyWinMenu,			""),
    OKV("KeySysSwitchNext",			gKeySysSwitchNext,		""),
    OKV("KeySysSwitchLast",			gKeySysSwitchLast,		""),
    OKV("KeySysWinNext",			gKeySysWinNext,			""),
    OKV("KeySysWinPrev",			gKeySysWinPrev,			""),
    OKV("KeySysWinMenu",			gKeySysWinMenu,			""),
    OKV("KeySysDialog",				gKeySysDialog,			""),
    OKV("KeySysMenu",				gKeySysMenu,			""),
    OKV("KeySysRun",				gKeySysRun,			""),
    OKV("KeySysWindowList",			gKeySysWindowList,		""),
    OKV("KeySysAddressBar",			gKeySysAddressBar,		""),
    OKV("KeySysWorkspacePrev",			gKeySysWorkspacePrev,		""),
    OKV("KeySysWorkspaceNext",			gKeySysWorkspaceNext,		""),
    OKV("KeySysWorkspaceLast",			gKeySysWorkspaceLast,		""),
    OKV("KeySysWorkspacePrevTakeWin",		gKeySysWorkspacePrevTakeWin,	""),
    OKV("KeySysWorkspaceNextTakeWin",		gKeySysWorkspaceNextTakeWin,	""),
    OKV("KeySysWorkspaceLastTakeWin",		gKeySysWorkspaceLastTakeWin,	""),
    OKV("KeySysWorkspace1",			gKeySysWorkspace1,		""),
    OKV("KeySysWorkspace2",			gKeySysWorkspace2,		""),
    OKV("KeySysWorkspace3",			gKeySysWorkspace3,		""),
    OKV("KeySysWorkspace4",			gKeySysWorkspace4,		""),
    OKV("KeySysWorkspace5",			gKeySysWorkspace5,		""),
    OKV("KeySysWorkspace6",			gKeySysWorkspace6,		""),
    OKV("KeySysWorkspace7",			gKeySysWorkspace7,		""),
    OKV("KeySysWorkspace8",			gKeySysWorkspace8,		""),
    OKV("KeySysWorkspace9",			gKeySysWorkspace9,		""),
    OKV("KeySysWorkspace10",			gKeySysWorkspace10,		""),
    OKV("KeySysWorkspace11",			gKeySysWorkspace11,		""),
    OKV("KeySysWorkspace12",			gKeySysWorkspace12,		""),
    OKV("KeySysWorkspace1TakeWin",		gKeySysWorkspace1TakeWin,	""),
    OKV("KeySysWorkspace2TakeWin",		gKeySysWorkspace2TakeWin,	""),
    OKV("KeySysWorkspace3TakeWin",		gKeySysWorkspace3TakeWin,	""),
    OKV("KeySysWorkspace4TakeWin",		gKeySysWorkspace4TakeWin,	""),
    OKV("KeySysWorkspace5TakeWin",		gKeySysWorkspace5TakeWin,	""),
    OKV("KeySysWorkspace6TakeWin",		gKeySysWorkspace6TakeWin,	""),
    OKV("KeySysWorkspace7TakeWin",		gKeySysWorkspace7TakeWin,	""),
    OKV("KeySysWorkspace8TakeWin",		gKeySysWorkspace8TakeWin,	""),
    OKV("KeySysWorkspace9TakeWin",		gKeySysWorkspace9TakeWin,	""),
    OKV("KeySysWorkspace10TakeWin",		gKeySysWorkspace10TakeWin,	""),
    OKV("KeySysWorkspace11TakeWin",		gKeySysWorkspace11TakeWin,	""),
    OKV("KeySysWorkspace12TakeWin",		gKeySysWorkspace12TakeWin,	""),
    OKV("KeySysTileVertical",			gKeySysTileVertical,		""),
    OKV("KeySysTileHorizontal",			gKeySysTileHorizontal,		""),
    OKV("KeySysCascade",			gKeySysCascade,			""),
    OKV("KeySysArrange",			gKeySysArrange,			""),
    OKV("KeySysArrangeIcons",			gKeySysArrangeIcons,		""),
    OKV("KeySysMinimizeAll",			gKeySysMinimizeAll,		""),
    OKV("KeySysHideAll",			gKeySysHideAll,			""),
    OKV("KeySysUndoArrange",			gKeySysUndoArrange,		"")
};
#endif

#endif

#undef XIV
#undef XSV
