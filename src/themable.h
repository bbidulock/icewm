
// themable preferences (themes can set only these)

#if defined(CONFIG_I18N) || 1 // maybe not such a good idea
XIV(bool, prettyClock,                          false)
#else 
XIV(bool, prettyClock,                          true)
#endif
XIV(bool, rolloverTitleButtons,                 false)

XIV(bool, trayDrawBevel,                        false)
XIV(bool, titleBarCentered,                     false)
XIV(bool, titleBarJoinLeft,                     false)
XIV(bool, titleBarJoinRight,                    false)

XIV(int, wsBorderX,                             6)
XIV(int, wsBorderY,                             6)
XIV(int, wsDlgBorderX,                          2)
XIV(int, wsDlgBorderY,                          2)
XIV(int, wsCornerX,                             24)
XIV(int, wsCornerY,                             24)
XIV(int, wsTitleBar,                            20)
XIV(int, titleBarJustify,                       0)
XIV(int, titleBarHorzOffset,                    0)
XIV(int, titleBarVertOffset,                    0)
XIV(int, scrollBarWidth,                        16)
XIV(int, scrollBarHeight,                       16)

XIV(int, smallIconSize,                         16)
XIV(int, largeIconSize,                         32)
XIV(int, hugeIconSize,                          48)

XIV(int, quickSwitchHMargin,                    3)      // !!!
XIV(int, quickSwitchVMargin,                    3)      // !!!
XIV(int, quickSwitchIMargin,                    4)      // !!!
XIV(int, quickSwitchIBorder,                    2)      // !!!
XIV(int, quickSwitchSepSize,                    6)      // !!!
#ifdef CONFIG_MOVESIZE_FX
XIV(int, moveSizeInterior,                      0)
XIV(int, moveSizeDimLines,                      0)
XIV(int, moveSizeGaugeLines,                    0)
XIV(int, moveSizeDimLabels,                     0)
XIV(int, moveSizeGeomLabels,                    0)
#endif
XSV(const char *, titleButtonsLeft,             "s")
XSV(const char *, titleButtonsRight,            "xmir")
XSV(const char *, titleButtonsSupported,        "xmis");
XSV(const char *, themeAuthor,                  0)
XSV(const char *, themeDescription,             0)
XSV(const char *, titleFontName,                BOLDFONT(120))
XSV(const char *, menuFontName,                 BOLDFONT(120))
XSV(const char *, statusFontName,               BOLDTTFONT(120))
XSV(const char *, fxFontName,                   BOLDTTFONT(120))
XSV(const char *, switchFontName,               BOLDTTFONT(120))
XSV(const char *, normalButtonFontName,         FONT(120))
XSV(const char *, activeButtonFontName,         BOLDFONT(120))
#ifdef CONFIG_TASKBAR
XSV(const char *, normalTaskBarFontName,        FONT(120))
XSV(const char *, activeTaskBarFontName,        BOLDFONT(120))
XSV(const char *, toolButtonFontName,           "")
XSV(const char *, normalWorkspaceFontName,      "")
XSV(const char *, activeWorkspaceFontName,      "")
#endif
XSV(const char *, minimizedWindowFontName,      FONT(120))
XSV(const char *, listBoxFontName,              FONT(120))
XSV(const char *, labelFontName,                FONT(140))
XSV(const char *, clockFontName,                TTFONT(140))
XSV(const char *, apmFontName,                  TTFONT(140))
XSV(const char *, inputFontName,                TTFONT(140))
#ifdef CONFIG_MOVESIZE_FX
XSV(const char *, moveSizeFontName,             BOLDFONT(100))
#endif
XSV(const char *, clrDialog,                    "rgb:C0/C0/C0")
XSV(const char *, clrActiveBorder,              "rgb:C0/C0/C0")
XSV(const char *, clrInactiveBorder,            "rgb:C0/C0/C0")
XSV(const char *, clrNormalTitleButton,         "rgb:C0/C0/C0")
XSV(const char *, clrNormalTitleButtonText,     "rgb:00/00/00")
XSV(const char *, clrActiveTitleBar,            "rgb:00/00/A0")
XSV(const char *, clrInactiveTitleBar,          "rgb:80/80/80")
XSV(const char *, clrActiveTitleBarText,        "rgb:FF/FF/FF")
XSV(const char *, clrInactiveTitleBarText,      "rgb:00/00/00")
XSV(const char *, clrActiveTitleBarShadow,      "")
XSV(const char *, clrInactiveTitleBarShadow,    "")
XSV(const char *, clrNormalMinimizedWindow,     "rgb:C0/C0/C0")
XSV(const char *, clrNormalMinimizedWindowText, "rgb:00/00/00")
XSV(const char *, clrActiveMinimizedWindow,     "rgb:E0/E0/E0")
XSV(const char *, clrActiveMinimizedWindowText, "rgb:00/00/00")
XSV(const char *, clrNormalMenu,                "rgb:C0/C0/C0")
XSV(const char *, clrActiveMenuItem,            "rgb:A0/A0/A0")
XSV(const char *, clrActiveMenuItemText,        "rgb:00/00/00")
XSV(const char *, clrNormalMenuItemText,        "rgb:00/00/00")
XSV(const char *, clrDisabledMenuItemText,      "rgb:80/80/80")
XSV(const char *, clrDisabledMenuItemShadow,    "")
XSV(const char *, clrMoveSizeStatus,            "rgb:C0/C0/C0")
XSV(const char *, clrMoveSizeStatusText,        "rgb:00/00/00")
XSV(const char *, clrQuickSwitch,               "rgb:C0/C0/C0")
XSV(const char *, clrQuickSwitchText,           "rgb:00/00/00")
XSV(const char *, clrQuickSwitchActive,         0)
#ifdef CONFIG_TASKBAR
XSV(const char *, clrDefaultTaskBar,            "rgb:C0/C0/C0")
#endif
XSV(const char *, clrNormalButton,              "rgb:C0/C0/C0")
XSV(const char *, clrNormalButtonText,          "rgb:00/00/00")
XSV(const char *, clrActiveButton,              "rgb:E0/E0/E0")
XSV(const char *, clrActiveButtonText,          "rgb:00/00/00")
XSV(const char *, clrToolButton,                "")
XSV(const char *, clrToolButtonText,            "")
XSV(const char *, clrWorkspaceActiveButton,     "")
XSV(const char *, clrWorkspaceActiveButtonText, "")
XSV(const char *, clrWorkspaceNormalButton,     "")
XSV(const char *, clrWorkspaceNormalButtonText, "")
#ifdef CONFIG_TASKBAR
XSV(const char *, clrNormalTaskBarApp,          "rgb:C0/C0/C0")
XSV(const char *, clrNormalTaskBarAppText,      "rgb:00/00/00")
XSV(const char *, clrActiveTaskBarApp,          "rgb:E0/E0/E0")
XSV(const char *, clrActiveTaskBarAppText,      "rgb:00/00/00")
XSV(const char *, clrMinimizedTaskBarApp,       "rgb:A0/A0/A0")
XSV(const char *, clrMinimizedTaskBarAppText,   "rgb:00/00/00")
XSV(const char *, clrInvisibleTaskBarApp,       "rgb:80/80/80")
XSV(const char *, clrInvisibleTaskBarAppText,   "rgb:00/00/00")
#endif
XSV(const char *, clrScrollBar,                 "rgb:A0/A0/A0")
XSV(const char *, clrScrollBarArrow,            "rgb:00/00/00")
XSV(const char *, clrScrollBarInactive,         "rgb:80/80/80")
XSV(const char *, clrScrollBarSlider,           "rgb:C0/C0/C0")
XSV(const char *, clrScrollBarButton,           "rgb:C0/C0/C0")
XSV(const char *, clrListBox,                   "rgb:C0/C0/C0")
XSV(const char *, clrListBoxText,               "rgb:00/00/00")
XSV(const char *, clrListBoxSelected,           "rgb:80/80/80")
XSV(const char *, clrListBoxSelectedText,       "rgb:00/00/00")
XSV(const char *, clrClock,                     "rgb:00/00/00")
XSV(const char *, clrClockText,                 "rgb:00/FF/00")
XSV(const char *, clrApm,                       "rgb:00/00/00")
XSV(const char *, clrApmText,                   "rgb:00/FF/00")
XSV(const char *, clrInput,                     "rgb:FF/FF/FF")
XSV(const char *, clrInputText,                 "rgb:00/00/00")
XSV(const char *, clrInputSelection,            "rgb:80/80/80")
XSV(const char *, clrInputSelectionText,        "rgb:00/00/00")
XSV(const char *, clrLabel,                     "rgb:C0/C0/C0")
XSV(const char *, clrLabelText,                 "rgb:00/00/00")
XSV(const char *, clrCpuUser,                   "rgb:00/FF/00")
XSV(const char *, clrCpuSys,                    "rgb:FF/00/00")
XSV(const char *, clrCpuNice,                   "rgb:00/00/FF")
XSV(const char *, clrCpuIdle,                   "rgb:00/00/00")
XSV(const char *, clrNetSend,                   "rgb:FF/FF/00")
XSV(const char *, clrNetReceive,                "rgb:FF/00/FF")
XSV(const char *, clrNetIdle,                   "rgb:00/00/00")

#ifdef CONFIG_GRADIENTS
XSV(const char *, gradients,                    0)
#endif

#if defined(CFGDEF) && !defined(NO_CONFIGURE)

cfoption icewm_themable_preferences[] = {
    OBV("RolloverButtonsSupported",                             &rolloverTitleButtons,                      "Does it support the 'O' title bar button images (for mouse rollover)"),
    OBV("TaskBarClockLeds",                     &prettyClock,                   "Task bar clock/APM uses nice pixmapped LCD display (but then it doesn't display correctly in many languages anymore, e.g. for Japanese and Korean it works only when a real font is used and not the LEDs"),
    OBV("TrayDrawBevel",                        &trayDrawBevel,                 "Surround the tray with plastic border"),

    OBV("TitleBarCentered",                     &titleBarCentered,              "Draw window title centered (obsoleted by TitleBarJustify)"),
    OBV("TitleBarJoinLeft",                     &titleBarJoinLeft,              "Join title*S and title*T"),
    OBV("TitleBarJoinRight",                    &titleBarJoinRight,             "Join title*T and title*B"),

    OIV("BorderSizeX",                          &wsBorderX, 0, 128,             "Horizontal window border"),
    OIV("BorderSizeY",                          &wsBorderY, 0, 128,             "Vertical window border"),
    OIV("DlgBorderSizeX",                       &wsDlgBorderX, 0, 128,          "Horizontal dialog window border"),
    OIV("DlgBorderSizeY",                       &wsDlgBorderY, 0, 128,          "Vertical dialog window border"),
    OIV("CornerSizeX",                          &wsCornerX, 0, 64,              "Resize corner width"),
    OIV("CornerSizeY",                          &wsCornerY, 0, 64,              "Resize corner height"),
    OIV("TitleBarHeight",                       &wsTitleBar, 0, 128,            "Title bar height"),
    OIV("TitleBarJustify",                      &titleBarJustify, 0, 100,       "Justification of the window title"),
    OIV("TitleBarHorzOffset",                   &titleBarHorzOffset, -128, 128, "Horizontal offset for the window title text"),
    OIV("TitleBarVertOffset",                   &titleBarVertOffset, -128, 128, "Vertical offset for the window title text"),
    OIV("ScrollBarX",                           &scrollBarWidth, 0, 64,         "Scrollbar width"),
    OIV("ScrollBarY",                           &scrollBarHeight, 0, 64,        "Scrollbar (button) height"),

    OIV("SmallIconSize",                        &smallIconSize, 8, 128,         "Dimension of the small icons"),
    OIV("LargeIconSize",                        &largeIconSize, 8, 128,         "Dimension of the large icons"),
    OIV("HugeIconSize",                         &hugeIconSize, 8, 128,         "Dimension of the large icons"),

    OIV("QuickSwitchHorzMargin",                &quickSwitchHMargin, 0, 64,     "Horizontal margin of the quickswitch window"),
    OIV("QuickSwitchVertMargin",                &quickSwitchVMargin, 0, 64,     "Vertical margin of the quickswitch window"),
    OIV("QuickSwitchIconMargin",                &quickSwitchIMargin, 0, 64,     "Vertical margin in the quickswitch window"),
    OIV("QuickSwitchIconBorder",                &quickSwitchIBorder, 0, 64,     "Distance between the active icon and it's border"),
    OIV("QuickSwitchSeparatorSize",             &quickSwitchSepSize, 0, 64,     "Height of the separator between (all reachable) icons and text, 0 to avoid it"),

#ifdef CONFIG_MOVESIZE_FX
    OIV("MoveSizeInterior",                     &moveSizeInterior, 0, 31,       "Bitmask for inner decorations (1: border style, 2: titlebar, 4/8/16: grid)"),
    OIV("MoveSizeDimensionLines",               &moveSizeDimLines, 0, 4095,     "Bitmask for dimension lines (1/2/4: top left/center/right, 8/16/32: left top/middle/bottom, ...)"),
    OIV("MoveSizeGaugeLines",                   &moveSizeGaugeLines, 0, 15,     "Bitmask for gauge lines (1/2/4/8: top/left/right/bottom)"),
    OIV("MoveSizeDimensionLabels",              &moveSizeDimLabels, 0, 4095,    "Bitmask for dimension labels (1/2/4: top left/center/right, 8/16/32: left top/middle/bottom, ...)"),
    OIV("MoveSizeGeometryLabels",               &moveSizeGeomLabels, 0, 127,    "Bitmask for geometry labels (1/2/4: top left/center/right, 8: center, ...)"),
#endif

    OSV("ThemeAuthor",                          &themeAuthor,                   "Theme author, e-mail address, credits"),
    OSV("ThemeDescription",                     &themeDescription,              "Description of the theme, credits"),

    OSV("TitleButtonsLeft",                     &titleButtonsLeft,              "Titlebar buttons from left to right (x=close,  m=max,  i=min,  h=hide, r=rollup,       s=sysmenu,      d=depth)"),
    OSV("TitleButtonsRight",                    &titleButtonsRight,             "Titlebar buttons from right to left (x=close,  m=max,  i=min,  h=hide, r=rollup,       s=sysmenu,      d=depth)"),
    OSV("TitleButtonsSupported",                &titleButtonsSupported,         "Titlebar buttons supported by theme (x,m,i,r,h,s,d)"),
/************************************************************************************************************************************************************
 * Font definitions
 ************************************************************************************************************************************************************/
    OSV("TitleFontName",                        &titleFontName,                 ""),
    OSV("MenuFontName",                         &menuFontName,                  ""),
    OSV("StatusFontName",                       &statusFontName,                ""),
    OSV("FxFontName",                           &fxFontName,                    ""),
    OSV("QuickSwitchFontName",                  &switchFontName,                ""),
    OSV("NormalButtonFontName",                 &normalButtonFontName,          ""),
    OSV("ActiveButtonFontName",                 &activeButtonFontName,          ""),
#ifdef CONFIG_TASKBAR
    OSV("NormalTaskBarFontName",                &normalTaskBarFontName,         ""),
    OSV("ActiveTaskBarFontName",                &activeTaskBarFontName,         ""),
    OSV("ToolButtonFontName",                   &toolButtonFontName,            "fallback: NormalButtonFontName"),
    OSV("NormalWorkspaceFontName",              &normalWorkspaceFontName,       "fallback: NormalButtonFontName"),
    OSV("ActiveWorkspaceFontName",              &activeWorkspaceFontName,       "fallback: ActiveButtonFontName"),
#endif
    OSV("MinimizedWindowFontName",              &minimizedWindowFontName,       ""),
    OSV("ListBoxFontName",                      &listBoxFontName,               ""),
    OSV("ToolTipFontName",                      &toolTipFontName,               ""),
    OSV("ClockFontName",                        &clockFontName,                 ""),
    OSV("ApmFontName",                          &apmFontName,                   ""),
    OSV("InputFontName",                        &inputFontName,                 ""),
    OSV("LabelFontName",                        &labelFontName,                 ""),
#ifdef CONFIG_MOVESIZE_FX    
    OSV("moveSizeFontName",                     &moveSizeFontName,              BOLDFONT(100)),
#endif    
/************************************************************************************************************************************************************
 * Color definitions
 ************************************************************************************************************************************************************/
    OSV("ColorDialog",                          &clrDialog,                     "Background of dialog windows"),
    OSV("ColorNormalBorder",                    &clrInactiveBorder,             "Border of inactive windows"),
    OSV("ColorActiveBorder",                    &clrActiveBorder,               "Border of active windows"),

    OSV("ColorNormalButton",                    &clrNormalButton,               "Background of regular buttons"),
    OSV("ColorNormalButtonText",                &clrNormalButtonText,           "Textcolor of regular buttons"),
    OSV("ColorActiveButton",                    &clrActiveButton,               "Background of pressed buttons"),
    OSV("ColorActiveButtonText",                &clrActiveButtonText,           "Textcolor of pressed buttons"),
    OSV("ColorNormalTitleButton",               &clrNormalTitleButton,          "Background of titlebar buttons"),
    OSV("ColorNormalTitleButtonText",           &clrNormalTitleButtonText,      "Textcolor of titlebar buttons"),
    OSV("ColorToolButton",                      &clrToolButton,                 "Background of toolbar buttons, ColorNormalButton is used if empty"),
    OSV("ColorToolButtonText",                  &clrToolButtonText,             "Textcolor of toolbar buttons, ColorNormalButtonText is used if empty"),
    OSV("ColorNormalWorkspaceButton",           &clrWorkspaceNormalButton,      "Background of workspace buttons, ColorNormalButton is used if empty"),
    OSV("ColorNormalWorkspaceButtonText",       &clrWorkspaceNormalButtonText,  "Textcolor of workspace buttons, ColorNormalButtonText is used if empty"),
    OSV("ColorActiveWorkspaceButton",           &clrWorkspaceActiveButton,      "Background of the active workspace button, ColorActiveButton is used if empty"),
    OSV("ColorActiveWorkspaceButtonText",       &clrWorkspaceActiveButtonText, "Textcolor of the active workspace button, ColorActiveButtonText is used if empty"),

    OSV("ColorNormalTitleBar",                  &clrInactiveTitleBar,           "Background of the titlebar of regular windows"),
    OSV("ColorNormalTitleBarText",              &clrInactiveTitleBarText,       "Textcolor of the titlebar of regular windows"),
    OSV("ColorNormalTitleBarShadow",            &clrInactiveTitleBarShadow,     "Textshadow of the titlebar of regular windows"),
    OSV("ColorActiveTitleBar",                  &clrActiveTitleBar,             "Background of the titlebar of active windows"),
    OSV("ColorActiveTitleBarText",              &clrActiveTitleBarText,         "Textcolor of the titlebar of active windows"),
    OSV("ColorActiveTitleBarShadow",            &clrActiveTitleBarShadow,       "Textshadow of the titlebar of active windows"),

    OSV("ColorNormalMinimizedWindow",           &clrNormalMinimizedWindow,      "Background for mini icons of regular windows"),
    OSV("ColorNormalMinimizedWindowText",       &clrNormalMinimizedWindowText,  "Textcolor for mini icons of regular windows"),
    OSV("ColorActiveMinimizedWindow",           &clrActiveMinimizedWindow,      "Background for mini icons of active windows"),
    OSV("ColorActiveMinimizedWindowText",       &clrActiveMinimizedWindowText,  "Textcolor for mini icons of active windows"),

    OSV("ColorNormalMenu",                      &clrNormalMenu,                 "Background of pop-up menus"),
    OSV("ColorNormalMenuItemText",              &clrNormalMenuItemText,         "Textcolor of regular menu items"),
    OSV("ColorActiveMenuItem",                  &clrActiveMenuItem,             "Background of selected menu item, leave empty to force transparency"),
    OSV("ColorActiveMenuItemText",              &clrActiveMenuItemText,         "Textcolor of selected menu items"),
    OSV("ColorDisabledMenuItemText",            &clrDisabledMenuItemText,       "Textcolor of disabled menu items"),
    OSV("ColorDisabledMenuItemShadow",          &clrDisabledMenuItemShadow,     "Shadow of regular menu items"),

    OSV("ColorMoveSizeStatus",                  &clrMoveSizeStatus,             "Background of move/resize status window"),
    OSV("ColorMoveSizeStatusText",              &clrMoveSizeStatusText,         "Textcolor of move/resize status window"),
    
    OSV("ColorQuickSwitch",                     &clrQuickSwitch,                "Background of the quick switch window"),
    OSV("ColorQuickSwitchText",                 &clrQuickSwitchText,            "Textcolor in the quick switch window"),
    OSV("ColorQuickSwitchActive",               &clrQuickSwitchActive,          "Rectangle arround the active icon in the quick switch window"),
#ifdef CONFIG_TASKBAR
    OSV("ColorDefaultTaskBar",                  &clrDefaultTaskBar,             "Background of the taskbar"),
    OSV("ColorNormalTaskBarApp",                &clrNormalTaskBarApp,           "Background for task buttons of regular windows"),
    OSV("ColorNormalTaskBarAppText",            &clrNormalTaskBarAppText,       "Textcolor for task buttons of regular windows"),
    OSV("ColorActiveTaskBarApp",                &clrActiveTaskBarApp,           "Background for task buttons of the active window"),
    OSV("ColorActiveTaskBarAppText",            &clrActiveTaskBarAppText,       "Textcolor for task buttons of the active window"),
    OSV("ColorMinimizedTaskBarApp",             &clrMinimizedTaskBarApp,        "Background for task buttons of minimized windows"),
    OSV("ColorMinimizedTaskBarAppText",         &clrMinimizedTaskBarAppText,    "Textcolor for task buttons of minimized windows"),
    OSV("ColorInvisibleTaskBarApp",             &clrInvisibleTaskBarApp,        "Background for task buttons of windows on other workspaces"),
    OSV("ColorInvisibleTaskBarAppText",         &clrInvisibleTaskBarAppText,    "Textcolor for task buttons of windows on other workspaces"),
#endif
    OSV("ColorScrollBar",                       &clrScrollBar,                  "Scrollbar background (sliding area)"),
    OSV("ColorScrollBarSlider",                 &clrScrollBarSlider,            "Background of the slider button in scrollbars"),
    OSV("ColorScrollBarButton",                 &clrScrollBarButton,            "Background of the arrow buttons in scrollbars"),
    OSV("ColorScrollBarArrow",                  &clrScrollBarButton,            "Background of the arrow buttons in scrollbars (obsolete)"),
    OSV("ColorScrollBarButtonArrow",            &clrScrollBarArrow,             "Color of active arrows on scrollbar buttons"),
    OSV("ColorScrollBarInactiveArrow",          &clrScrollBarInactive,          "Color of inactive arrows on scrollbar buttons"),

    OSV("ColorListBox",                         &clrListBox,                    "Background of listboxes"),
    OSV("ColorListBoxText",                     &clrListBoxText,                "Textcolor in listboxes"),
    OSV("ColorListBoxSelection",                &clrListBoxSelected,            "Background of selected listbox items"),
    OSV("ColorListBoxSelectionText",            &clrListBoxSelectedText,        "Textcolor of selected listbox items"),
#ifdef CONFIG_TOOLTIP
    OSV("ColorToolTip",                         &clrToolTip,                    "Background of tooltips"),
    OSV("ColorToolTipText",                     &clrToolTipText,                "Textcolor of tooltips"),
#endif
    OSV("ColorLabel",                           &clrLabel,                      "Background of labels, leave empty to force transparency"),
    OSV("ColorLabelText",                       &clrLabelText,                  "Textcolor of labels"),
    OSV("ColorInput",                           &clrInput,                      "Background of text entry fields (e.g. the addressbar)"),
    OSV("ColorInputText",                       &clrInputText,                  "Textcolor of text entry fields (e.g. the addressbar)"),
    OSV("ColorInputSelection",                  &clrInputSelection,             "Background of selected text in an entry field"),
    OSV("ColorInputSelectionText",              &clrInputSelectionText,         "Selected text in an entry field"),

#ifdef CONFIG_APPLET_CLOCK
    OSV("ColorClock",                           &clrClock,                      "Background of non-LCD clock, leave empty to force transparency"),
    OSV("ColorClockText",                       &clrClockText,                  "Background of non-LCD monitor"),
#endif    
#ifdef CONFIG_APPLET_APM
    OSV("ColorApm",                             &clrApm,                        "Background of APM monitor, leave empty to force transparency"),
    OSV("ColorApmText",                         &clrApmText,                    "Textcolor of APM monitor"),
#endif    
#ifdef CONFIG_APPLET_CPU_STATUS
    OSV("ColorCPUStatusUser",                   &clrCpuUser,                    "User load on the CPU monitor"),
    OSV("ColorCPUStatusSystem",                 &clrCpuSys,                     "System load on the CPU monitor"),
    OSV("ColorCPUStatusNice",                   &clrCpuNice,                    "Nice load on the CPU monitor"),
    OSV("ColorCPUStatusIdle",                   &clrCpuIdle,                    "Idle (non) load on the CPU monitor, leave empty to force transparency"),
#endif    
#ifdef CONFIG_APPLET_NET_STATUS
    OSV("ColorNetSend",                         &clrNetSend,                    "Outgoing load on the network monitor"),
    OSV("ColorNetReceive",                      &clrNetReceive,                 "Incoming load on the network monitor"),
    OSV("ColorNetIdle",                         &clrNetIdle,                    "Idle (non) load on the network monitor, leave empty to force transparency"),
#endif
#ifdef CONFIG_GRADIENTS
    OSV("Gradients",                            &gradients,                     "List of gradient pixmaps in the current theme"),
#endif
    OKF("Look", setLook, ""),
    OK0()
};

#endif
