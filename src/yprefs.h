#ifndef __YPREFS_H
#define __YPREFS_H

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
XSV(const char *, iconPath,                     0)
#define CONFIG_DEFAULT_THEME "icedesert/default.theme"
XSV(const char *, themeName,                    CONFIG_DEFAULT_THEME)
XSV(const char *, xineramaPrimaryScreenName,    0)

#define CONFIG_DEFAULT_LOOK lookNice

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
#define CONFIG_LOOK_FLAT

#ifndef WMLOOK
#define WMLOOK
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
#ifdef CONFIG_LOOK_FLAT
    lookFlat,
#endif
    lookMAX
} WMLook;
#endif

XIV(WMLook, wmLook,                             CONFIG_DEFAULT_LOOK)

#ifdef CONFIG_LOOK_WIN95
#define LOOK_IS_WIN95           (wmLook == lookWin95)
#define CASE_LOOK_WIN95         case lookWin95
#else
#define LOOK_IS_WIN95 false
#define CASE_LOOK_WIN95
#endif

#ifdef CONFIG_LOOK_MOTIF
#define LOOK_IS_MOTIF           (wmLook == lookMotif)
#define CASE_LOOK_MOTIF         case lookMotif
#else
#define LOOK_IS_MOTIF           false
#define CASE_LOOK_MOTIF
#endif

#ifdef CONFIG_LOOK_WARP3
#define LOOK_IS_WARP3           (wmLook == lookWarp3)
#define CASE_LOOK_WARP3         case lookWarp3
#else
#define LOOK_IS_WARP3 false
#define CASE_LOOK_WARP3
#endif

#ifdef CONFIG_LOOK_WARP4
#define LOOK_IS_WARP4           (wmLook == lookWarp4)
#define CASE_LOOK_WARP4         case lookWarp4
#else
#define LOOK_IS_WARP4 false
#define CASE_LOOK_WARP4
#endif

#ifdef CONFIG_LOOK_NICE
#define LOOK_IS_NICE            (wmLook == lookNice)
#define CASE_LOOK_NICE          case lookNice
#else
#define LOOK_IS_NICE false
#define CASE_LOOK_NICE
#endif

#ifdef CONFIG_LOOK_PIXMAP
#define LOOK_IS_PIXMAP          (wmLook == lookPixmap)
#define CASE_LOOK_PIXMAP        case lookPixmap
#else
#define LOOK_IS_PIXMAP false
#define CASE_LOOK_PIXMAP
#endif

#ifdef CONFIG_LOOK_METAL
#define LOOK_IS_METAL           (wmLook == lookMetal)
#define CASE_LOOK_METAL         case lookMetal
#else
#define LOOK_IS_METAL false
#define CASE_LOOK_METAL
#endif

#ifdef CONFIG_LOOK_GTK
#define LOOK_IS_GTK             (wmLook == lookGtk)
#define CASE_LOOK_GTK           case lookGtk
#else
#define LOOK_IS_GTK false
#define CASE_LOOK_GTK
#endif

#ifdef CONFIG_LOOK_FLAT
#define LOOK_IS_FLAT            (wmLook == lookFlat)
#define CASE_LOOK_FLAT          case lookFlat
#else
#define LOOK_IS_FLAT false
#define CASE_LOOK_FLAT
#endif

#ifdef CONFIG_TOOLTIP
XSV(const char *, clrToolTip,                   "rgb:E0/E0/00")
XSV(const char *, clrToolTipText,               "rgb:00/00/00")
#endif
XFV(const char *, toolTipFontName,              FONT(120), "sans-serif:size=12")

#endif
