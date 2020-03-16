#ifndef THEAMABLE_H
#define THEAMABLE_H

// themable preferences (themes can set only these)

#if defined(CONFIG_I18N) || 1 // maybe not such a good idea
XIV(bool, prettyClock,                          false)
#else
XIV(bool, prettyClock,                          true)
#endif
XIV(bool, rolloverTitleButtons,                 false)

XIV(unsigned, taskbuttonIconOffset,             0)
XIV(unsigned, trayIconMaxWidth,                 32)
XIV(unsigned, trayIconMaxHeight,                24)
XIV(bool, trayDrawBevel,                        false)
XIV(bool, titleBarCentered,                     false)
XIV(bool, titleBarJoinLeft,                     false)
XIV(bool, titleBarJoinRight,                    false)
XIV(bool, showFrameIcon,                        true)

XIV(unsigned, wsBorderX,                        6)
XIV(unsigned, wsBorderY,                        6)
XIV(unsigned, wsDlgBorderX,                     2)
XIV(unsigned, wsDlgBorderY,                     2)
XIV(unsigned, wsCornerX,                        24)
XIV(unsigned, wsCornerY,                        24)
XIV(unsigned, wsTitleBar,                       20)
XIV(int, titleBarJustify,                       0)
XIV(int, titleBarHorzOffset,                    0)
XIV(int, titleBarVertOffset,                    0)
XIV(unsigned, scrollBarWidth,                   16)
XIV(unsigned, scrollBarHeight,                  16)

XIV(unsigned, menuIconSize,                     16)
XIV(unsigned, smallIconSize,                    16)
XIV(unsigned, largeIconSize,                    32)
XIV(unsigned, hugeIconSize,                     48)

XIV(unsigned, quickSwitchHMargin,               3)      // !!!
XIV(unsigned, quickSwitchVMargin,               3)      // !!!
XIV(unsigned, quickSwitchIMargin,               4)      // !!!
XIV(unsigned, quickSwitchIBorder,               2)      // !!!
XIV(unsigned, quickSwitchSepSize,               6)      // !!!
XSV(const char *, titleButtonsLeft,             "s")
XSV(const char *, titleButtonsRight,            "xmir")
XSV(const char *, titleButtonsSupported,        "xmis")
XSV(const char *, themeAuthor,                  0)
XSV(const char *, themeDescription,             0)

XFV(const char *, titleFontName,                FONT(120), "sans-serif:size=12")
XFV(const char *, menuFontName,                 BOLDFONT(100), "sans-serif:size=10:bold")
XFV(const char *, statusFontName,               BOLDTTFONT(120), "monospace:size=12:bold")
XFV(const char *, switchFontName,               BOLDTTFONT(120), "monospace:size=12:bold")
XFV(const char *, normalButtonFontName,         FONT(120), "sans-serif:size=12")
XFV(const char *, activeButtonFontName,         BOLDFONT(120), "sans-serif:size=12:bold")
XFV(const char *, normalTaskBarFontName,        FONT(120), "sans-serif:size=12")
XFV(const char *, activeTaskBarFontName,        BOLDFONT(120), "sans-serif:size=12:bold")
XFV(const char *, toolButtonFontName,           FONT(120), "sans-serif:size=12")
XFV(const char *, normalWorkspaceFontName,      FONT(120), "sans-serif:size=12")
XFV(const char *, activeWorkspaceFontName,      FONT(120), "sans-serif:size=12")
XFV(const char *, minimizedWindowFontName,      FONT(120), "sans-serif:size=12")
XFV(const char *, listBoxFontName,              FONT(120), "sans-serif:size=12")
XFV(const char *, labelFontName,                FONT(140), "sans-serif:size=12")
XFV(const char *, clockFontName,                TTFONT(140), "monospace:size=12")
XFV(const char *, tempFontName,                 TTFONT(140), "monospace:size=12")
XFV(const char *, apmFontName,                  TTFONT(140), "monospace:size=12")
XFV(const char *, inputFontName,                TTFONT(140), "monospace:size=12")

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
XSV(const char *, clrDefaultTaskBar,            "rgb:C0/C0/C0")
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
XSV(const char *, clrNormalTaskBarApp,          "rgb:C0/C0/C0")
XSV(const char *, clrNormalTaskBarAppText,      "rgb:00/00/00")
XSV(const char *, clrActiveTaskBarApp,          "rgb:E0/E0/E0")
XSV(const char *, clrActiveTaskBarAppText,      "rgb:00/00/00")
XSV(const char *, clrMinimizedTaskBarApp,       "rgb:A0/A0/A0")
XSV(const char *, clrMinimizedTaskBarAppText,   "rgb:00/00/00")
XSV(const char *, clrInvisibleTaskBarApp,       "rgb:80/80/80")
XSV(const char *, clrInvisibleTaskBarAppText,   "rgb:00/00/00")
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
XSV(const char *, clrApmBat,                    "rgb:FF/FF/00")
XSV(const char *, clrApmLine,                   "rgb:00/FF/00")
XSV(const char *, clrApmGraphBg,                "rgb:00/00/00")
XSV(const char *, clrInput,                     "rgb:FF/FF/FF")
XSV(const char *, clrInputText,                 "rgb:00/00/00")
XSV(const char *, clrInputSelection,            "rgb:80/80/80")
XSV(const char *, clrInputSelectionText,        "rgb:00/00/00")
XSV(const char *, clrLabel,                     "rgb:C0/C0/C0")
XSV(const char *, clrLabelText,                 "rgb:00/00/00")
XSV(const char *, clrCpuUser,                   "rgb:00/FF/00")
XSV(const char *, clrCpuSys,                    "rgb:FF/00/00")
XSV(const char *, clrCpuIntr,                   "rgb:FF/FF/00")
XSV(const char *, clrCpuIoWait,                 "rgb:60/00/60")
XSV(const char *, clrCpuSoftIrq,                "rgb:00/FF/FF")
XSV(const char *, clrCpuNice,                   "rgb:00/00/FF")
XSV(const char *, clrCpuIdle,                   "rgb:00/00/00")
XSV(const char *, clrCpuSteal,                  "rgb:FF/8A/91")
XSV(const char *, clrCpuTemp,                   "rgb:60/60/C0")
XSV(const char *, clrMemUser,                   "rgb:40/40/80")
XSV(const char *, clrMemBuffers,                "rgb:60/60/C0")
XSV(const char *, clrMemCached,                 "rgb:80/80/FF")
XSV(const char *, clrMemFree,                   "rgb:00/00/00")
XSV(const char *, clrNetSend,                   "rgb:FF/FF/00")
XSV(const char *, clrNetReceive,                "rgb:FF/00/FF")
XSV(const char *, clrNetIdle,                   "rgb:00/00/00")
XSV(const char *, gradients,                    0)

#if defined(CFGDEF)

cfoption icewm_themable_preferences[] = {
    OBV("RolloverButtonsSupported",             &rolloverTitleButtons,          "Does it support the 'O' title bar button images (for mouse rollover)"),
    OBV("TaskBarClockLeds",                     &prettyClock,                   "Task bar clock/APM uses nice pixmap LCD display (but then it doesn't display correctly in many languages anymore, e.g., for Japanese and Korean it works only when a real font is used and not the LCD pixmaps"),

    OUV("TaskbuttonIconOffset",                 &taskbuttonIconOffset, 0, 16,   "Width of taskbutton side icons"),
    OUV("TrayIconMaxWidth",                     &trayIconMaxWidth, 16, 128,     "Maximum scaled width of tray icons"),
    OUV("TrayIconMaxHeight",                    &trayIconMaxHeight, 16, 128,    "Maximum scaled height of tray icons"),
    OBV("TrayDrawBevel",                        &trayDrawBevel,                 "Surround the tray with plastic border"),

    OBV("TitleBarCentered",                     &titleBarCentered,              "Draw window title centered (obsoleted by TitleBarJustify)"),
    OBV("TitleBarJoinLeft",                     &titleBarJoinLeft,              "Join title*S and title*T"),
    OBV("TitleBarJoinRight",                    &titleBarJoinRight,             "Join title*T and title*B"),
    OBV("ShowMenuButtonIcon",                   &showFrameIcon,                 "Show application icon over menu button"),

    OUV("BorderSizeX",                          &wsBorderX, 0, 128,             "Horizontal window border"),
    OUV("BorderSizeY",                          &wsBorderY, 0, 128,             "Vertical window border"),
    OUV("DlgBorderSizeX",                       &wsDlgBorderX, 0, 128,          "Horizontal dialog window border"),
    OUV("DlgBorderSizeY",                       &wsDlgBorderY, 0, 128,          "Vertical dialog window border"),
    OUV("CornerSizeX",                          &wsCornerX, 0, 64,              "Resize corner width"),
    OUV("CornerSizeY",                          &wsCornerY, 0, 64,              "Resize corner height"),
    OUV("TitleBarHeight",                       &wsTitleBar, 0, 128,            "Title bar height"),
    OIV("TitleBarJustify",                      &titleBarJustify, 0, 100,       "Justification of the window title"),
    OIV("TitleBarHorzOffset",                   &titleBarHorzOffset, -128, 128, "Horizontal offset for the window title text"),
    OIV("TitleBarVertOffset",                   &titleBarVertOffset, -128, 128, "Vertical offset for the window title text"),
    OUV("ScrollBarX",                           &scrollBarWidth, 0, 64,         "Scrollbar width"),
    OUV("ScrollBarY",                           &scrollBarHeight, 0, 64,        "Scrollbar (button) height"),

    OUV("MenuIconSize",                         &menuIconSize, 8, 128,          "Menu icon size"),
    OUV("SmallIconSize",                        &smallIconSize, 8, 128,         "Dimension of the small icons"),
    OUV("LargeIconSize",                        &largeIconSize, 8, 128,         "Dimension of the large icons"),
    OUV("HugeIconSize",                         &hugeIconSize, 8, 128,          "Dimension of the large icons"),

    OUV("QuickSwitchHorzMargin",                &quickSwitchHMargin, 0, 64,     "Horizontal margin of the quickswitch window"),
    OUV("QuickSwitchVertMargin",                &quickSwitchVMargin, 0, 64,     "Vertical margin of the quickswitch window"),
    OUV("QuickSwitchIconMargin",                &quickSwitchIMargin, 0, 64,     "Vertical margin in the quickswitch window"),
    OUV("QuickSwitchIconBorder",                &quickSwitchIBorder, 0, 64,     "Distance between the active icon and it's border"),
    OUV("QuickSwitchSeparatorSize",             &quickSwitchSepSize, 0, 64,     "Height of the separator between (all reachable) icons and text, 0 to avoid it"),

    OSV("ThemeAuthor",                          &themeAuthor,                   "Theme author, e-mail address, credits"),
    OSV("ThemeDescription",                     &themeDescription,              "Description of the theme, credits"),

    OSV("TitleButtonsLeft",                     &titleButtonsLeft,              "Titlebar buttons from left to right (x=close, m=max, i=min, h=hide, r=rollup, s=sysmenu, d=depth)"),
    OSV("TitleButtonsRight",                    &titleButtonsRight,             "Titlebar buttons from right to left (x=close, m=max, i=min, h=hide, r=rollup, s=sysmenu, d=depth)"),
    OSV("TitleButtonsSupported",                &titleButtonsSupported,         "Titlebar buttons supported by theme (x,m,i,r,h,s,d)"),
/************************************************************************************************************************************************************
 * Font definitions
 ************************************************************************************************************************************************************/
    OFV("TitleFontName",                        &titleFontName,                 "Name of the title bar font."),
    OFV("MenuFontName",                         &menuFontName,                  "Name of the menu font."),
    OFV("StatusFontName",                       &statusFontName,                "Name of the status display font."),
    OFV("QuickSwitchFontName",                  &switchFontName,                "Name of the font for Alt+Tab switcher window."),
    OFV("NormalButtonFontName",                 &normalButtonFontName,          "Name of the normal button font."),
    OFV("ActiveButtonFontName",                 &activeButtonFontName,          "Name of the active button font."),
    OFV("NormalTaskBarFontName",                &normalTaskBarFontName,         "Name of the normal task bar item font."),
    OFV("ActiveTaskBarFontName",                &activeTaskBarFontName,         "Name of the active task bar item font."),
    OFV("ToolButtonFontName",                   &toolButtonFontName,            "Name of the tool button font (fallback: NormalButtonFontName)."),
    OFV("NormalWorkspaceFontName",              &normalWorkspaceFontName,       "Name of the normal workspace button font (fallback: NormalButtonFontName)."),
    OFV("ActiveWorkspaceFontName",              &activeWorkspaceFontName,       "Name of the active workspace button font (fallback: ActiveButtonFontName)."),
    OFV("MinimizedWindowFontName",              &minimizedWindowFontName,       "Name of the mini-window font."),
    OFV("ListBoxFontName",                      &listBoxFontName,               "Name of the window list font."),
    OFV("ToolTipFontName",                      &toolTipFontName,               "Name of the tool tip font."),
    OFV("ClockFontName",                        &clockFontName,                 "Name of the task bar clock font."),
    OFV("TempFontName",                         &tempFontName,                  "Name of the task bar temperature font."),
    OFV("ApmFontName",                          &apmFontName,                   "Name of the task bar battery font."),
    OFV("InputFontName",                        &inputFontName,                 "Name of the input field font."),
    OFV("LabelFontName",                        &labelFontName,                 "Name of the label font."),
/************************************************************************************************************************************************************
 * Color definitions
 ************************************************************************************************************************************************************/
    OSV("ColorDialog",                          &clrDialog,                     "Background of dialog windows"),
    OSV("ColorNormalBorder",                    &clrInactiveBorder,             "Border of inactive windows"),
    OSV("ColorActiveBorder",                    &clrActiveBorder,               "Border of active windows"),

    OSV("ColorNormalButton",                    &clrNormalButton,               "Background of regular buttons"),
    OSV("ColorNormalButtonText",                &clrNormalButtonText,           "Text color of regular buttons"),
    OSV("ColorActiveButton",                    &clrActiveButton,               "Background of pressed buttons"),
    OSV("ColorActiveButtonText",                &clrActiveButtonText,           "Text color of pressed buttons"),
    OSV("ColorNormalTitleButton",               &clrNormalTitleButton,          "Background of titlebar buttons"),
    OSV("ColorNormalTitleButtonText",           &clrNormalTitleButtonText,      "Text color of titlebar buttons"),
    OSV("ColorToolButton",                      &clrToolButton,                 "Background of toolbar buttons, ColorNormalButton is used if empty"),
    OSV("ColorToolButtonText",                  &clrToolButtonText,             "Text color of toolbar buttons, ColorNormalButtonText is used if empty"),
    OSV("ColorNormalWorkspaceButton",           &clrWorkspaceNormalButton,      "Background of workspace buttons, ColorNormalButton is used if empty"),
    OSV("ColorNormalWorkspaceButtonText",       &clrWorkspaceNormalButtonText,  "Text color of workspace buttons, ColorNormalButtonText is used if empty"),
    OSV("ColorActiveWorkspaceButton",           &clrWorkspaceActiveButton,      "Background of the active workspace button, ColorActiveButton is used if empty"),
    OSV("ColorActiveWorkspaceButtonText",       &clrWorkspaceActiveButtonText,  "Text color of the active workspace button, ColorActiveButtonText is used if empty"),

    OSV("ColorNormalTitleBar",                  &clrInactiveTitleBar,           "Background of the titlebar of regular windows"),
    OSV("ColorNormalTitleBarText",              &clrInactiveTitleBarText,       "Text color of the titlebar of regular windows"),
    OSV("ColorNormalTitleBarShadow",            &clrInactiveTitleBarShadow,     "Text shadow of the titlebar of regular windows"),
    OSV("ColorActiveTitleBar",                  &clrActiveTitleBar,             "Background of the titlebar of active windows"),
    OSV("ColorActiveTitleBarText",              &clrActiveTitleBarText,         "Text color of the titlebar of active windows"),
    OSV("ColorActiveTitleBarShadow",            &clrActiveTitleBarShadow,       "Text shadow of the titlebar of active windows"),

    OSV("ColorNormalMinimizedWindow",           &clrNormalMinimizedWindow,      "Background for mini icons of regular windows"),
    OSV("ColorNormalMinimizedWindowText",       &clrNormalMinimizedWindowText,  "Text color for mini icons of regular windows"),
    OSV("ColorActiveMinimizedWindow",           &clrActiveMinimizedWindow,      "Background for mini icons of active windows"),
    OSV("ColorActiveMinimizedWindowText",       &clrActiveMinimizedWindowText,  "Text color for mini icons of active windows"),

    OSV("ColorNormalMenu",                      &clrNormalMenu,                 "Background of pop-up menus"),
    OSV("ColorNormalMenuItemText",              &clrNormalMenuItemText,         "Text color of regular menu items"),
    OSV("ColorActiveMenuItem",                  &clrActiveMenuItem,             "Background of selected menu item, leave empty to force transparency"),
    OSV("ColorActiveMenuItemText",              &clrActiveMenuItemText,         "Text color of selected menu items"),
    OSV("ColorDisabledMenuItemText",            &clrDisabledMenuItemText,       "Text color of disabled menu items"),
    OSV("ColorDisabledMenuItemShadow",          &clrDisabledMenuItemShadow,     "Shadow of regular menu items"),

    OSV("ColorMoveSizeStatus",                  &clrMoveSizeStatus,             "Background of move/resize status window"),
    OSV("ColorMoveSizeStatusText",              &clrMoveSizeStatusText,         "Text color of move/resize status window"),

    OSV("ColorQuickSwitch",                     &clrQuickSwitch,                "Background of the quick switch window"),
    OSV("ColorQuickSwitchText",                 &clrQuickSwitchText,            "Text color in the quick switch window"),
    OSV("ColorQuickSwitchActive",               &clrQuickSwitchActive,          "Rectangle arround the active icon in the quick switch window"),
    OSV("ColorDefaultTaskBar",                  &clrDefaultTaskBar,             "Background of the taskbar"),
    OSV("ColorNormalTaskBarApp",                &clrNormalTaskBarApp,           "Background for task buttons of regular windows"),
    OSV("ColorNormalTaskBarAppText",            &clrNormalTaskBarAppText,       "Text color for task buttons of regular windows"),
    OSV("ColorActiveTaskBarApp",                &clrActiveTaskBarApp,           "Background for task buttons of the active window"),
    OSV("ColorActiveTaskBarAppText",            &clrActiveTaskBarAppText,       "Text color for task buttons of the active window"),
    OSV("ColorMinimizedTaskBarApp",             &clrMinimizedTaskBarApp,        "Background for task buttons of minimized windows"),
    OSV("ColorMinimizedTaskBarAppText",         &clrMinimizedTaskBarAppText,    "Text color for task buttons of minimized windows"),
    OSV("ColorInvisibleTaskBarApp",             &clrInvisibleTaskBarApp,        "Background for task buttons of windows on other workspaces"),
    OSV("ColorInvisibleTaskBarAppText",         &clrInvisibleTaskBarAppText,    "Text color for task buttons of windows on other workspaces"),
    OSV("ColorScrollBar",                       &clrScrollBar,                  "Scrollbar background (sliding area)"),
    OSV("ColorScrollBarSlider",                 &clrScrollBarSlider,            "Background of the slider button in scrollbars"),
    OSV("ColorScrollBarButton",                 &clrScrollBarButton,            "Background of the arrow buttons in scrollbars"),
    OSV("ColorScrollBarArrow",                  &clrScrollBarButton,            "Background of the arrow buttons in scrollbars (obsolete)"),
    OSV("ColorScrollBarButtonArrow",            &clrScrollBarArrow,             "Color of active arrows on scrollbar buttons"),
    OSV("ColorScrollBarInactiveArrow",          &clrScrollBarInactive,          "Color of inactive arrows on scrollbar buttons"),

    OSV("ColorListBox",                         &clrListBox,                    "Background of listboxes"),
    OSV("ColorListBoxText",                     &clrListBoxText,                "Text color in listboxes"),
    OSV("ColorListBoxSelection",                &clrListBoxSelected,            "Background of selected listbox items"),
    OSV("ColorListBoxSelectionText",            &clrListBoxSelectedText,        "Text color of selected listbox items"),
    OSV("ColorToolTip",                         &clrToolTip,                    "Background of tooltips"),
    OSV("ColorToolTipText",                     &clrToolTipText,                "Text color of tooltips"),
    OSV("ColorLabel",                           &clrLabel,                      "Background of labels, leave empty to force transparency"),
    OSV("ColorLabelText",                       &clrLabelText,                  "Text color of labels"),
    OSV("ColorInput",                           &clrInput,                      "Background of text entry fields (e.g., the addressbar)"),
    OSV("ColorInputText",                       &clrInputText,                  "Text color of text entry fields (e.g., the addressbar)"),
    OSV("ColorInputSelection",                  &clrInputSelection,             "Background of selected text in an entry field"),
    OSV("ColorInputSelectionText",              &clrInputSelectionText,         "Selected text in an entry field"),

    OSV("ColorClock",                           &clrClock,                      "Background of non-LCD clock, leave empty to force transparency"),
    OSV("ColorClockText",                       &clrClockText,                  "Background of non-LCD monitor"),

    OSV("ColorApm",                             &clrApm,                        "Background of APM monitor, leave empty to force transparency"),
    OSV("ColorApmText",                         &clrApmText,                    "Text color of APM monitor"),
    OSV("ColorApmBattary",                      &clrApmBat,                     "Legacy option; don't use, see ColorApmBattery"),
    OSV("ColorApmBattery",                      &clrApmBat,                     "Color of APM monitor in battery mode"),
    OSV("ColorApmLine",                         &clrApmLine,                    "Color of APM monitor in line mode"),
    OSV("ColorApmGraphBg",                      &clrApmGraphBg,                 "Background color for graph mode"),

    OSV("ColorCPUStatusUser",                   &clrCpuUser,                    "User load on the CPU monitor"),
    OSV("ColorCPUStatusSystem",                 &clrCpuSys,                     "System load on the CPU monitor"),
    OSV("ColorCPUStatusInterrupts",             &clrCpuIntr,                    "Interrupts on the CPU monitor"),
    OSV("ColorCPUStatusIoWait",                 &clrCpuIoWait,                  "IO Wait on the CPU monitor"),
    OSV("ColorCPUStatusSoftIrq",                &clrCpuSoftIrq,                 "Soft Interrupts on the CPU monitor"),
    OSV("ColorCPUStatusNice",                   &clrCpuNice,                    "Nice load on the CPU monitor"),
    OSV("ColorCPUStatusIdle",                   &clrCpuIdle,                    "Idle (non) load on the CPU monitor, leave empty to force transparency"),
    OSV("ColorCPUStatusSteal",                  &clrCpuSteal,                   "Involuntary Wait on the CPU monitor"),
    OSV("ColorCPUStatusTemp",                   &clrCpuTemp,                    "Temperature of the CPU"),

    OSV("ColorMEMStatusUser",                   &clrMemUser,                    "User program usage in the memory monitor"),
    OSV("ColorMEMStatusBuffers",                &clrMemBuffers,                 "OS buffers usage in the memory monitor"),
    OSV("ColorMEMStatusCached",                 &clrMemCached,                  "OS cached usage in the memory monitor"),
    OSV("ColorMEMStatusFree",                   &clrMemFree,                    "Free memory in the memory monitor"),

    OSV("ColorNetSend",                         &clrNetSend,                    "Outgoing load on the network monitor"),
    OSV("ColorNetReceive",                      &clrNetReceive,                 "Incoming load on the network monitor"),
    OSV("ColorNetIdle",                         &clrNetIdle,                    "Idle (non) load on the network monitor, leave empty to force transparency"),

    OSV("Gradients",                            &gradients,                     "List of gradient pixmaps in the current theme"),
    OKF("Look", setLook, ""),
    OK0()
};

#endif
#endif

// vim: set sw=4 ts=4 et:
