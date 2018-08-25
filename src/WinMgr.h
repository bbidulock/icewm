#ifndef __WINMGR_H
#define __WINMGR_H

/* all of this is still experimental -- feedback welcome */
/* 0.5
 *    - added WIN_LAYER_MENU
 */
/* 0.4
 *   - added WIN_WORKSPACES for windows appearing on multiple workspaces
 *     at the same time.
 *   - added WinStateDocked
 *   - added WinStateSticky for possible virtual desktops
 *   - changed WIN_ICONS format to be extensible and more complete
 */
/* 0.3
 *   - renamed WinStateSticky -> WinStateAllWorkspaces
 */
/* 0.2
 *   - added separate flags for horizonal/vertical maximized state
 */
/* 0.1 */

/* TODO:
 *   - packed properties for STATE and HINTS (WIN_ALL_STATE and WIN_ALL_HINTS)
 *     currently planned to look like this:
 *      CARD32 nhints
 *        for each hint:
 *         CARD32 atom (hint type)
 *         CARD32 count (data length)
 *         CARD32 data[count]
 *         ...
 *     this has the advantage of being slightly faster, but
 *     demands that all hints are handled at one place. WM ignores
 *     other hints of the same type if these two are present.

 *   - check out WMaker and KDE hints to make a superset of all
 *   - virtual desktop, handling of virtual (>screen) scrolling
 *     desktops and pages (fvwm like?)
 *   - vroot.h?
 *   - MOZILLA_ZORDER (it should be possible to do the same thing
 *      with WIN_* hints)
 *   - there should be a way to coordinate corner/edge points of the frames
 *     (for placement). ICCCM is vague on this issue. icewm prefers to treat
 *     windows as client-area and titlebar when doing the placement (ignoring
 *     the frame)
 *   - hint limit wm key/mouse bindings on a window
 * possible
 *   - what does MOTIF_WM_INFO do?
 *   - WIN_MAXIMIZED_GEOMETRY
 *   - WIN_RESIZE (protocol that allows applications to start sizing the frame)
 *   - WIN_FOCUS (protocol for focusing a window)
 */

#define XA_WIN_PROTOCOLS       "_WIN_PROTOCOLS"
/* Type: array of Atom
 *       set on Root window by the window manager.
 *
 * This property contains all of the protocols/hints supported by
 * the window manager (WM_HINTS, MWM_HINTS, WIN_*, etc.
 */

#define XA_WIN_SUPPORTING_WM_CHECK "_WIN_SUPPORTING_WM_CHECK"
/* XID of window created by WM, used to check */

#define XA_WIN_ICONS           "_WIN_ICONS"
/* Type: array of CARD32
 *       first item is icon count (n)
 *       second item is icon record length (in CARD32s)
 *       this is followed by (n) icon records as follows
 *           pixmap (XID)
 *           mask (XID)
 *           width (CARD32)
 *           height (CARD32)
 *           depth (of pixmap, mask is assumed to be of depth 1) (CARD32)
 *           drawable (screen root drawable of pixmap) (XID)
 *           ... additional fields can be added at the end of this list
 *
 * This property contains additional icons for the application.
 * if this property is set, the WM will ignore default X icon hints
 * and KWM_WIN_ICON hint.
 *
 * Icon Mask can be None if transparency is not required.
 *
 * Icewm currently needs/uses icons of size 16x16 and 32x32 with default
 * depth. Applications are recommended to set several icons sizes
 * (recommended 16x16,32x32,48x48 at least)
 *
 * TODO: WM should present a wishlist of desired icons somehow. (standard
 * WM_ICON_SIZES property is only a partial solution). 16x16 icons can be
 * quite small on large resolutions.
 */

/* workspace */
#define XA_WIN_WORKSPACE       "_WIN_WORKSPACE"
/* Type: CARD32
 *       Root Window: current workspace, set by the window manager
 *
 *       Application Window: current workspace. The default workspace
 *       can be set by applications, but they must use ClientMessages to
 *       change them. When window is mapped, the propery should only be
 *       updated by the window manager.
 *
 * Applications wanting to change the current workspace should send
 * a ClientMessage to the Root window like this:
 *     xev.type = ClientMessage;
 *     xev.window = root_window or toplevel_window;
 *     xev.message_type = _XA_WIN_WORKSPACE;
 *     xev.format = 32;
 *     xev.data.l[0] = workspace;
 *     xev.data.l[1] = timeStamp;
 *     XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
 *
 */

#define XA_WIN_WORKSPACE_COUNT "_WIN_WORKSPACE_COUNT"
/* Type: CARD32
 *       workspace count, set by window manager
 *
 * NOT FINALIZED/IMPLEMENTED YET
 */

#define XA_WIN_WORKSPACE_NAMES "_WIN_WORKSPACE_NAMES"
/* Type: StringList (TextPropery)
 *
 *
 * IMPLEMENTED but not FINALIZED.
 * perhaps the name should be separate for each workspace (like KDE).
 * this where WIN_WORKSPACE_COUNT comes into play.
 */

#define WinWorkspaceInvalid    -1L

/* workspaces */
#define XA_WIN_WORKSPACES "_WIN_WORKSPACES"
/* Type: array of CARD32
 *       bitmask of workspaces that application appears on
 *
 * Applications must still set WIN_WORKPACE property to define
 * the default window (if the WM has to switch workspaces) and
 * specify the workspace for WMs that do not support this property.
 *
 * The same policy applies as for WIN_WORKSPACE: default can be set by
 * applications, but is only changed by the window manager after the
 * window is mapped.
 *
 * Applications wanting to change the current workspace should send
 * a ClientMessage to the Root window like this:
 *     xev.type = ClientMessage;
 *     xev.window = root_window or toplevel_window;
 *     xev.message_type = _XA_WIN_WORKSPACES_ADD; // or _REMOVE
 *     xev.format = 32;
 *     xev.data.l[0] = index; // index of item
 *     xev.data.l[1] = bitmask; // to assign, or, or reset
 *     xev.data.l[2] = timestamp; // of event that caused operation
 *     XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
 */
#define XA_WIN_WORKSPACES_ADD "_WIN_WORKSPACES_ADD"
#define XA_WIN_WORKSPACES_REMOVE "_WIN_WORKSPACES_REMOVE"

/* layer */
#define XA_WIN_LAYER           "_WIN_LAYER"
/* Type: CARD32
 *       window layer
 *
 * Window layer.
 * Windows with LAYER=WinLayerDock determine size of the Work Area
 * (WIN_WORKAREA). Windows below dock layer are resized to the size
 * of the work area when maximized. Windows above dock layer are
 * maximized to the entire screen space.
 *
 * The default can be set by application, but when window is mapped
 * only window manager can change this. If an application wants to change
 * the window layer it should send the ClientMessage to the root window
 * like this:
 *     xev.type = ClientMessage;
 *     xev.window = toplevel_window;
 *     xev.message_type = _XA_WIN_LAYER;
 *     xev.format = 32;
 *     xev.data.l[0] = layer;
 *     xev.data.l[1] = timeStamp;
 *     XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
 *
 * TODO: A few available layers could be used for WMaker menus/submenus
 * (comments?)
 *
 * Partially implemented. Currently requires all docked windows to be
 * sticky (only one workarea for all workspaces). Otherwise non-docked sticky
 * windows could (?) move when switching workspaces (annoying).
 */

#define WinLayerCount          16
#define WinLayerInvalid        -1L

#define WinLayerDesktop        0L
#define WinLayerBelow          2L
#define WinLayerNormal         4L
#define WinLayerOnTop          6L
#define WinLayerDock           8L
#define WinLayerAboveDock      10L
#define WinLayerMenu           12L
#define WinLayerFullscreen     14L // hack, for now
#define WinLayerAboveAll       15L // for taskbar auto hide

/* task bar tray */
#define XA_WIN_TRAY             "_ICEWM_TRAY"
/* Type: CARD32
 *       window task bar tray option
 *
 * Window with tray=Ignore (default) has its window button only on TaskPane.
 * Window with tray=Minimized has its icon on TrayPane and if it is
 * not minimized it has also window button on TaskPane (no button for
 * minimized window).
 * Window with tray=Exclusive has only its icon on TrayPane and there is no
 * window button on TaskPane.
 *
 * The default can be set by application, but when window is mapped
 * only window manager can change this. If an application wants to change
 * the window tray option it should send the ClientMessage to the root window
 * like this:
 *
 *     xev.type = ClientMessage;
 *     xev.window = toplevel_window;
 *     xev.message_type = _XA_WIN_TRAY;
 *     xev.format = 32;
 *     xev.data.l[0] = tray_opt;
 *     xev.data.l[1] = timeStamp;
 *     XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
 */

#define WinTrayOptionCount      3
#define WinTrayInvalid          -1L

#define WinTrayIgnore           0L
#define WinTrayMinimized        1L
#define WinTrayExclusive        2L

/* state */
#define XA_WIN_STATE           "_WIN_STATE"

/* Type CARD32[2]
 *      window state. First CARD32 is the mask of set/supported states,
 *      the second one is the state.
 *
 * The default value for this property can be set by applications.
 * When window is mapped, the property is updated by the window manager
 * as necessary. Applications can request the state change by sending
 * the client message to the root window like this:
 *
 *   xev.type = ClientMessage;
 *   xev.window = toplevel_window;
 *   xev.message_type = _XA_WIN_WORKSPACE;
 *   xev.format = 32;
 *   xev.data.l[0] = mask; // mask of the states to change
 *   xev.data.l[1] = state; // new state values
 *   xev.data.l[2] = timeStamp;
 *   XSendEvent(display, root, False, SubstructureNotifyMask, (XEvent *) &xev);
 */
#if 0
// From wm-comp.ps:
#define WIN_STATE_STICKY            (1<<0) /* everyone knows sticky */
#define WIN_STATE_MINIMIZED         (1<<1) /* Reserved - definition is unclear */
#define WIN_STATE_MAXIMIZED_VERT    (1<<2) /* window in maximized V state */
#define WIN_STATE_MAXIMIZED_HORIZ   (1<<3) /* window in maximized H state */
#define WIN_STATE_HIDDEN            (1<<4) /* not on taskbar but window visible */
#define WIN_STATE_SHADED            (1<<5) /* shaded (MacOS / Afterstep style) */
#define WIN_STATE_HID_WORKSPACE     (1<<6) /* not on current desktop */
#define WIN_STATE_HID_TRANSIENT     (1<<7) /* owner of transient is hidden */
#define WIN_STATE_FIXED_POSITION    (1<<8) /* window is fixed in position even */
#define WIN_STATE_ARRANGE_IGNORE    (1<<9) /* ignore for auto arranging */
#endif

#define WinStateSticky         (1 << 0)   /* sticks to virtual desktop */
#define WinStateMinimized      (1 << 1)   /* to iconbox,taskbar,... */
#define WinStateMaximizedVert  (1 << 2)   /* maximized vertically */
#define WinStateMaximizedHoriz (1 << 3)   /* maximized horizontally */
#define WinStateHidden         (1 << 4)   /* not on taskbar if any, but still accessible */
#define WinStateRollup         (1 << 5)   /* only titlebar visible */
#define WinStateHidWorkspace   (1 << 6)   /* not on current desktop */
#define WinStateHidTransient   (1 << 7)   /* owner of transient is hidden */
#define WinStateFixedPosition  (1 << 8)   /* fixed position on virtual desktop*/
#define WinStateArrangeIgnore  (1 << 9)   /* ignore for auto arranging */
//#define WinStateDocked         (1 << 9) /* docked, ignore my area for maximizing */
#define WinStateFocused        (1 << 21)  /* has the focus */
#define WinStateUrgent         (1 << 22)  /* demands attention */
#define WinStateSkipPager      (1 << 23)  /* skip pager */
#define WinStateSkipTaskBar    (1 << 24)  /* skip taskbar */
#define WinStateModal          (1 << 25)  /* modal */
#define WinStateBelow          (1 << 26)  /* below layer */
#define WinStateAbove          (1 << 27)  /* above layer */
#define WinStateFullscreen     (1 << 28)  /* fullscreen (no lauout limits) */
#define WinStateWasHidden      (1 << 29)  /* was hidden when parent was minimized/hidden */
#define WinStateWasMinimized   (1 << 30)  /* was minimized when parent was minimized/hidden */
#define WinStateWithdrawn      (1 << 31)  /* managed, but not available to user */

#define WIN_STATE_ALL (WinStateSticky | WinStateMinimized |\
                       WinStateMaximizedVert | WinStateMaximizedHoriz |\
                       WinStateHidden | WinStateRollup | WinStateHidWorkspace |\
                       WinStateHidTransient | WinStateFixedPosition |\
                       WinStateArrangeIgnore)


/* hints */
// From wm-comp.ps:
// #define WIN_HINTS_SKIP_FOCUS        (1<<0) /* "alt-tab" skips this win */
// #define WIN_HINTS_SKIP_WINLIST      (1<<1) /* do not show in window list */
// #define WIN_HINTS_SKIP_TASKBAR      (1<<2) /* do not show on taskbar */
// #define WIN_HINTS_GROUP_TRANSIENT   (1<<3) /* Reserved - definition is unclear */
// #define WIN_HINTS_FOCUS_ON_CLICK    (1<<4) /* app only accepts focus if clicked */

#define XA_WIN_HINTS            "_WIN_HINTS"
#define WinHintsSkipFocus       (1 << 0) /* "alt-tab" skips this win */
#define WinHintsSkipWindowMenu  (1 << 1) /* do not show in window list */
#define WinHintsSkipTaskBar     (1 << 2) /* do not show on taskbar */
#define WinHintsGroupTransient  (1 << 3) /* Reserved - definition is unclear */
#define WinHintsFocusOnClick    (1 << 4) /* app only accepts focus when clicked */
#define WinHintsDoNotCover      (1 << 5) /* attempt to not cover this window */
#define WinHintsDockHorizontal  (1 << 6) /* docked horizontally */

#define WIN_HINTS_ALL (WinHintsSkipFocus | WinHintsSkipWindowMenu |\
                       WinHintsSkipTaskBar | WinHintsGroupTransient |\
                       WinHintsFocusOnClick | WinHintsDoNotCover)

/* Type CARD32[2]
 *      additional window hints
 *
 *      Handling of this propery is very similiar to WIN_STATE.
 *
 * NOT IMPLEMENTED YET.
 */
/* WinHintsDockHorizontal -- not used
 * This state is necessary for correct WORKAREA negotiation when
 * a window is positioned in a screen corner. If set, it determines how
 * the place where window is subtracted from workare.
 *
 * Imagine a square docklet in the corner of the screen (several WMaker docklets
 * are like this).
 *
 * HHHHD
 * ....V
 * ....V
 * ....V
 *
 * If WinStateDockHorizontal is set, the WORKAREA will consist of area
 * covered by '.' and 'V', otherwise the WORKAREA will consist of area
 * covered by '. and 'H';
 *
 * currently hack is used where: w>h -> horizontal dock, else vertical
 */

/* work area of current workspace -- */
#define XA_WIN_WORKAREA        "_WIN_WORKAREA"
/*
 * CARD32[4]
 *     minX, minY, maxX, maxY of workarea.
 *     set/updated only by the window manager
 *
 * When windows are maximized they occupy the entire workarea except for
 * the titlebar at the top (in icewm window frame is not visible).
 *
 * Note: when WORKAREA changes, the application window are automatically
 * repositioned and maximized windows are also resized.
 */

#define XA_WIN_CLIENT_LIST    "_WIN_CLIENT_LIST"
/*
 * XID[]
 *
 * list of clients the WM is currently managing
 */


/* hack for gmc */
#define XA_WIN_DESKTOP_BUTTON_PROXY "_WIN_DESKTOP_BUTTON_PROXY"

/* not really used: */
#define XA_WIN_AREA "_WIN_AREA"
#define XA_WIN_AREA_COUNT "_WIN_AREA_COUNT"

#define XA_WIN_APP_STATE "_WIN_APP_STATE"
#define XA_WIN_EXPANDED_SIZE "_WIN_EXPANDED_SIZE"

#endif

// vim: set sw=4 ts=4 et:
