#ifndef __YPROTO_H
#define __YPROTO_H

extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XATOM_MWM_HINTS;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_CLIPBOARD;

/* Xdnd */
extern Atom XA_XdndAware;
extern Atom XA_XdndEnter;
extern Atom XA_XdndLeave;
extern Atom XA_XdndPosition;
extern Atom XA_XdndStatus;
extern Atom XA_XdndDrop;
extern Atom XA_XdndFinished;
extern Atom XA_XdndSelection;
extern Atom XA_XdndTypeList;

#ifdef GNOME1_HINTS
extern Atom _XA_WIN_PROTOCOLS;
extern Atom _XA_WIN_WORKSPACE;
extern Atom _XA_WIN_WORKSPACE_COUNT;
extern Atom _XA_WIN_WORKSPACE_NAMES;
extern Atom _XA_WIN_WORKAREA;
extern Atom _XA_WIN_LAYER;
extern Atom _XA_WIN_ICONS;
extern Atom _XA_WIN_HINTS;
extern Atom _XA_WIN_STATE;
extern Atom _XA_WIN_SUPPORTING_WM_CHECK;
extern Atom _XA_WIN_CLIENT_LIST;
extern Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
extern Atom _XA_WIN_AREA;
extern Atom _XA_WIN_AREA_COUNT;
#endif
extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_SM_CLIENT_ID;
#ifdef WMSPEC_HINTS
extern Atom _XA_NET_SUPPORTED;                      // OK
extern Atom _XA_NET_CLIENT_LIST;                    // OK (perf: don't update on stacking changes)
extern Atom _XA_NET_CLIENT_LIST_STACKING;           // OK
extern Atom _XA_NET_NUMBER_OF_DESKTOPS;             // implement GET (change count)
///extern Atom _XA_NET_DESKTOP_GEOMETRY;            // not used
///extern Atom _XA_NET_DESKTOP_VIEWPORT;            // not used
extern Atom _XA_NET_CURRENT_DESKTOP;                // OK
extern Atom _XA_NET_ACTIVE_WINDOW;                  // OK
extern Atom _XA_NET_WORKAREA;                       // OK (check min;max_X;Y
extern Atom _XA_NET_SUPPORTING_WM_CHECK;            // OK
//extern Atom _XA_NET_VIRTUAL_ROOTS;                // not used

extern Atom _XA_NET_CLOSE_WINDOW;                   // OK
//extern Atom _XA_NET_WM_MOVERESIZE;                // TODO

extern Atom _XA_NET_WM_NAME;                        // TODO
extern Atom _XA_NET_WM_VISIBLE_NAME;                // TODO
extern Atom _XA_NET_WM_ICON_NAME;                   // TODO
extern Atom _XA_NET_WM_VISIBLE_ICON_NAME;           // TODO

extern Atom _XA_NET_WM_DESKTOP;                     // OK
extern Atom _XA_NET_WM_WINDOW_TYPE;                 // check whether to do dynamic updates
extern Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;         // OK, sets layer only
extern Atom _XA_NET_WM_WINDOW_TYPE_DOCK;            // OK, sets layer only
extern Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;         // TODO
extern Atom _XA_NET_WM_WINDOW_TYPE_MENU;            // TODO
extern Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;          // TODO
extern Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;          // TODO

extern Atom _XA_NET_WM_STATE;                       // OK

extern Atom _XA_NET_WM_STATE_MODAL;                 // TODO
//extern Atom _XA_NET_WM_STATE_STICKY;              // not used
extern Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;        // OK
extern Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;        // OK
extern Atom _XA_NET_WM_STATE_SHADED;                // OK
extern Atom _XA_NET_WM_STATE_SKIP_TASKBAR;          // TODO
//extern Atom _XA_NET_WM_STATE_SKIP_PAGER;          // not used

// _SET would be nice to have
#define _NET_WM_STATE_REMOVE 0                      // OK
#define _NET_WM_STATE_ADD 1                         // OK
#define _NET_WM_STATE_TOGGLE 1                      // OK

extern Atom _XA_NET_WM_STRUT;                       // OK
extern Atom _XA_NET_WM_ICON_GEOMETRY;               // TODO
extern Atom _XA_NET_WM_ICON;                        // TODO
extern Atom _XA_NET_WM_PID;                         // TODO
extern Atom _XA_NET_WM_HANDLED_ICONS;               // TODO
extern Atom _XA_NET_WM_PING;                        // TODO


extern Atom _XA_KDE_NET_SYSTEM_TRAY_WINDOWS;        // TODO
extern Atom _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR;  // TODO

#endif

/* KDE specific */
extern Atom _XA_KWM_WIN_ICON;

extern Atom XA_IcewmWinOptHint;


#endif
