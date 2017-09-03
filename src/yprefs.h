#ifndef __YPREFS_H
#define __YPREFS_H

#include "ylib.h"
#include "yconfig.h"

XIV(bool, dontRotateMenuPointer,                true)
XIV(bool, menuMouseTracking,                    false)
XIV(bool, replayMenuCancelClick,                false)
XIV(bool, showPopupsAbovePointer,               false)
XIV(bool, showEllipsis,                         true)
#ifdef CONFIG_I18N
XIV(bool, multiByte,                            true)
#endif
XIV(bool, modSuperIsCtrlAlt,                    true)
XIV(bool, doubleBuffer,                         true)
XIV(bool, xrrDisable,                           false)
XIV(int, xineramaPrimaryScreen,                 0)
XIV(int, MenuActivateDelay,                     40)
XIV(int, SubmenuActivateDelay,                  300)
XIV(int, DelayFuzziness,                        10)
XIV(int, ClickMotionDistance,                   4)
XIV(int, ClickMotionDelay,                      200)
XIV(int, MultiClickTime,                        400)
XIV(int, autoScrollStartDelay,                  500)
XIV(int, autoScrollDelay,                       60)
#ifdef CONFIG_TOOLTIP
XIV(int, ToolTipDelay,                          1000)
XIV(int, ToolTipTime,                           0)
#endif

///#warning "move this one back to WM"
XIV(bool, grabRootWindow,                       true)

#ifdef CONFIG_XFREETYPE
XIV(bool, haveXft,                              true)
#endif
#if defined(__linux__) || defined(__FreeBSD__)
XSV(const char *, iconPath,                     "/usr/share/icons/hicolor:/usr/share/icons:/usr/share/pixmaps")
#else
XSV(const char *, iconPath,                     0)
#endif
#define CONFIG_DEFAULT_THEME "default/default.theme"
XSV(const char *, themeName,                    CONFIG_DEFAULT_THEME)
XSV(const char *, xineramaPrimaryScreenName,    0)

enum WMLook {
    lookWin95  = 1 << 0,
    lookMotif  = 1 << 1,
    lookWarp3  = 1 << 2,
    lookWarp4  = 1 << 3,
    lookNice   = 1 << 4,
    lookPixmap = 1 << 5,
    lookMetal  = 1 << 6,
    lookGtk    = 1 << 7,
    lookFlat   = 1 << 8,
};
#define LOOK(looks)             (((looks) & wmLook) != 0)
#define CONFIG_DEFAULT_LOOK     lookNice
XIV(WMLook, wmLook,                             CONFIG_DEFAULT_LOOK)

#ifdef CONFIG_TOOLTIP
XSV(const char *, clrToolTip,                   "rgb:E0/E0/00")
XSV(const char *, clrToolTipText,               "rgb:00/00/00")
#endif
XFV(const char *, toolTipFontName,              FONT(120), "sans-serif:size=12")

#endif
