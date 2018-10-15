#include "config.h"
#include "yxapp.h"
#include "yfull.h"
#include "ymenu.h"
#include "wmmgr.h"
#include "MwmUtil.h"
#include "ypointer.h"
#include "yxcontext.h"

#ifdef CONFIG_RENDER
#include <X11/extensions/Xrender.h>
#endif

#include <sys/resource.h>

#include "intl.h"

YXApplication *xapp = 0;

YDesktop *desktop = 0;
YContext<YWindow> windowContext;

YCursor YXApplication::leftPointer;
YCursor YXApplication::rightPointer;
YCursor YXApplication::movePointer;

Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_CLASS;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_WM_CLIENT_MACHINE;
Atom _XA_WM_COLORMAP_NOTIFY;
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_COMMAND;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_DESKTOP;
Atom _XA_WM_HINTS;
Atom _XA_WM_ICON_NAME;
Atom _XA_WM_ICON_SIZE;
Atom _XA_WM_LOCALE_NAME;
Atom _XA_WM_NAME;
Atom _XA_WM_NORMAL_HINTS;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_SIZE_HINTS;
Atom _XA_WM_STATE;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_TRANSIENT_FOR;
Atom _XA_WM_WINDOW_ROLE;
Atom _XA_WM_ZOOM_HINTS;

Atom _XATOM_MWM_HINTS;
//Atom _XA_MOTIF_WM_INFO;!!!
Atom _XA_WINDOW_ROLE;
Atom _XA_SM_CLIENT_ID;
Atom _XA_ICEWM_ACTION;
Atom _XA_CLIPBOARD;
Atom _XA_TARGETS;
Atom _XA_XEMBED_INFO;
Atom _XA_UTF8_STRING;

Atom _XA_WIN_APP_STATE;
Atom _XA_WIN_AREA_COUNT;
Atom _XA_WIN_AREA;
Atom _XA_WIN_CLIENT_LIST;
Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
Atom _XA_WIN_EXPANDED_SIZE;
Atom _XA_WIN_HINTS;
Atom _XA_WIN_ICONS;
Atom _XA_WIN_LAYER;
Atom _XA_WIN_PROTOCOLS;
Atom _XA_WIN_STATE;
Atom _XA_WIN_SUPPORTING_WM_CHECK;
Atom _XA_WIN_TRAY;
Atom _XA_WIN_WORKAREA;
Atom _XA_WIN_WORKSPACE_COUNT;
Atom _XA_WIN_WORKSPACE_NAMES;
Atom _XA_WIN_WORKSPACE;

Atom _XA_NET_ACTIVE_WINDOW;
Atom _XA_NET_CLIENT_LIST;
Atom _XA_NET_CLIENT_LIST_STACKING;
Atom _XA_NET_CLOSE_WINDOW;
Atom _XA_NET_CURRENT_DESKTOP;
Atom _XA_NET_DESKTOP_GEOMETRY;
Atom _XA_NET_DESKTOP_LAYOUT;
Atom _XA_NET_DESKTOP_NAMES;
Atom _XA_NET_DESKTOP_VIEWPORT;
Atom _XA_NET_FRAME_EXTENTS;
Atom _XA_NET_MOVERESIZE_WINDOW;
Atom _XA_NET_NUMBER_OF_DESKTOPS;
Atom _XA_NET_PROPERTIES;
Atom _XA_NET_REQUEST_FRAME_EXTENTS;
Atom _XA_NET_RESTACK_WINDOW;
Atom _XA_NET_SHOWING_DESKTOP;
Atom _XA_NET_STARTUP_ID;
Atom _XA_NET_STARTUP_INFO_BEGIN;
Atom _XA_NET_STARTUP_INFO;
Atom _XA_NET_SUPPORTED;
Atom _XA_NET_SUPPORTING_WM_CHECK;
Atom _XA_NET_SYSTEM_TRAY_MESSAGE_DATA;
Atom _XA_NET_SYSTEM_TRAY_OPCODE;
Atom _XA_NET_SYSTEM_TRAY_ORIENTATION;
Atom _XA_NET_SYSTEM_TRAY_VISUAL;
Atom _XA_NET_VIRTUAL_ROOTS;
Atom _XA_NET_WM_ACTION_ABOVE;
Atom _XA_NET_WM_ACTION_BELOW;
Atom _XA_NET_WM_ACTION_CHANGE_DESKTOP;
Atom _XA_NET_WM_ACTION_CLOSE;
Atom _XA_NET_WM_ACTION_FULLSCREEN;
Atom _XA_NET_WM_ACTION_MAXIMIZE_HORZ;
Atom _XA_NET_WM_ACTION_MAXIMIZE_VERT;
Atom _XA_NET_WM_ACTION_MINIMIZE;
Atom _XA_NET_WM_ACTION_MOVE;
Atom _XA_NET_WM_ACTION_RESIZE;
Atom _XA_NET_WM_ACTION_SHADE;
Atom _XA_NET_WM_ACTION_STICK;
Atom _XA_NET_WM_ALLOWED_ACTIONS;
Atom _XA_NET_WM_BYPASS_COMPOSITOR;
Atom _XA_NET_WM_DESKTOP;
Atom _XA_NET_WM_FULL_PLACEMENT;
Atom _XA_NET_WM_FULLSCREEN_MONITORS;
Atom _XA_NET_WM_HANDLED_ICONS;
Atom _XA_NET_WM_ICON_GEOMETRY;
Atom _XA_NET_WM_ICON_NAME;
Atom _XA_NET_WM_ICON;
Atom _XA_NET_WM_MOVERESIZE;
Atom _XA_NET_WM_NAME;
Atom _XA_NET_WM_OPAQUE_REGION;
Atom _XA_NET_WM_PID;
Atom _XA_NET_WM_PING;
Atom _XA_NET_WM_STATE;
Atom _XA_NET_WM_STATE_ABOVE;
Atom _XA_NET_WM_STATE_BELOW;
Atom _XA_NET_WM_STATE_DEMANDS_ATTENTION;
Atom _XA_NET_WM_STATE_FOCUSED;
Atom _XA_NET_WM_STATE_FULLSCREEN;
Atom _XA_NET_WM_STATE_HIDDEN;
Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;
Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;
Atom _XA_NET_WM_STATE_MODAL;
Atom _XA_NET_WM_STATE_SHADED;
Atom _XA_NET_WM_STATE_SKIP_PAGER;
Atom _XA_NET_WM_STATE_SKIP_TASKBAR;
Atom _XA_NET_WM_STATE_STICKY;
Atom _XA_NET_WM_STRUT;
Atom _XA_NET_WM_STRUT_PARTIAL;
Atom _XA_NET_WM_SYNC_REQUEST;
Atom _XA_NET_WM_SYNC_REQUEST_COUNTER;
Atom _XA_NET_WM_USER_TIME;
Atom _XA_NET_WM_USER_TIME_WINDOW;
Atom _XA_NET_WM_VISIBLE_ICON_NAME;
Atom _XA_NET_WM_VISIBLE_NAME;
Atom _XA_NET_WM_WINDOW_OPACITY;
Atom _XA_NET_WM_WINDOW_TYPE;
Atom _XA_NET_WM_WINDOW_TYPE_COMBO;
Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;
Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;
Atom _XA_NET_WM_WINDOW_TYPE_DND;
Atom _XA_NET_WM_WINDOW_TYPE_DOCK;
Atom _XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
Atom _XA_NET_WM_WINDOW_TYPE_MENU;
Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;
Atom _XA_NET_WM_WINDOW_TYPE_NOTIFICATION;
Atom _XA_NET_WM_WINDOW_TYPE_POPUP_MENU;
Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;
Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;
Atom _XA_NET_WM_WINDOW_TYPE_TOOLTIP;
Atom _XA_NET_WM_WINDOW_TYPE_UTILITY;
Atom _XA_NET_WORKAREA;

Atom _XA_KWM_DOCKWINDOW;
Atom _XA_KWM_WIN_ICON;

Atom _XA_KDE_NET_SYSTEM_TRAY_WINDOWS;
Atom _XA_KDE_NET_WM_FRAME_STRUT;
Atom _XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR;
Atom _XA_KDE_NET_WM_WINDOW_TYPE_OVERRIDE;
Atom _XA_KDE_SPLASH_PROGRESS;
Atom _XA_KDE_WM_CHANGE_STATE;

Atom XA_XdndAware;
Atom XA_XdndDrop;
Atom XA_XdndEnter;
Atom XA_XdndFinished;
Atom XA_XdndLeave;
Atom XA_XdndPosition;
Atom XA_XdndProxy;
Atom XA_XdndStatus;

#ifdef CONFIG_RENDER
int renderSupported;
int renderEventBase, renderErrorBase;
int renderVersionMajor, renderVersionMinor;
#endif

#ifdef CONFIG_SHAPE
int shapesSupported;
int shapeEventBase, shapeErrorBase;
int shapeVersionMajor, shapeVersionMinor;
#endif

#ifdef CONFIG_XRANDR
int xrandrSupported;
int xrandrEventBase, xrandrErrorBase;
int xrandrVersionMajor, xrandrVersionMinor;
bool xrandr12 = false;
#endif

#ifdef DEBUG
int xeventcount = 0;
#endif

class YClipboard: public YWindow {
public:
    YClipboard(): YWindow() {
        fLen = 0;
        fData = 0;
    }
    ~YClipboard() {
        if (fData)
            delete [] fData;
        fData = 0;
        fLen = 0;
    }

    void setData(const char *data, int len) {
        if (fData)
            delete [] fData;
        fLen = len;
        fData = new char[len];
        if (fData)
            memcpy(fData, data, len);
        if (fLen == 0)
            clearSelection(false);
        else
            acquireSelection(false);
    }
    void handleSelectionClear(const XSelectionClearEvent &clear) {
        if (clear.selection == _XA_CLIPBOARD) {
            if (fData)
                delete [] fData;
            fLen = 0;
            fData = 0;
        }
    }
    void handleSelectionRequest(const XSelectionRequestEvent &request) {
        if (request.selection == _XA_CLIPBOARD) {
            XSelectionEvent notify;

            notify.type = SelectionNotify;
            notify.requestor = request.requestor;
            notify.selection = request.selection;
            notify.target = request.target;
            notify.time = request.time;
            notify.property = request.property;

            if (request.selection == _XA_CLIPBOARD &&
                request.target == XA_STRING &&
                fLen > 0)
            {
                XChangeProperty(xapp->display(),
                                request.requestor,
                                request.property,
                                request.target,
                                8, PropModeReplace,
                                (unsigned char *)(fData ? fData : ""),
                                fLen);
            } else if (request.selection == _XA_CLIPBOARD &&
                       request.target == _XA_TARGETS &&
                       fLen > 0)
            {
                Atom type = XA_STRING;

                XChangeProperty(xapp->display(),
                                request.requestor,
                                request.property,
                                request.target,
                                32, PropModeReplace,
                                (unsigned char *)&type, 1);
            } else {
                notify.property = None;
            }

            XSendEvent(xapp->display(), notify.requestor, False, 0L, (XEvent *)&notify);
        }
    }

private:
    int fLen;
    char *fData;
};


void YXApplication::initAtoms() {
    struct {
        Atom *atom;
        const char *name;
    } atom_info[] = {
        { &_XA_WM_CHANGE_STATE                  , "WM_CHANGE_STATE"                     },
        { &_XA_WM_CLASS                         , "WM_CLASS"                            },
        { &_XA_WM_CLIENT_LEADER                 , "WM_CLIENT_LEADER"                    },
        { &_XA_WM_CLIENT_MACHINE                , "WM_CLIENT_MACHINE"                   },
        { &_XA_WM_COLORMAP_NOTIFY               , "WM_COLORMAP_NOTIFY"                  },
        { &_XA_WM_COLORMAP_WINDOWS              , "WM_COLORMAP_WINDOWS"                 },
        { &_XA_WM_COMMAND                       , "WM_COMMAND"                          },
        { &_XA_WM_DELETE_WINDOW                 , "WM_DELETE_WINDOW"                    },
        { &_XA_WM_DESKTOP                       , "WM_DESKTOP"                          },
        { &_XA_WM_HINTS                         , "WM_HINTS"                            },
        { &_XA_WM_ICON_NAME                     , "WM_ICON_NAME"                        },
        { &_XA_WM_ICON_SIZE                     , "WM_ICON_SIZE"                        },
        { &_XA_WM_LOCALE_NAME                   , "WM_LOCALE_NAME"                      },
        { &_XA_WM_NAME                          , "WM_NAME"                             },
        { &_XA_WM_NORMAL_HINTS                  , "WM_NORMAL_HINTS"                     },
        { &_XA_WM_PROTOCOLS                     , "WM_PROTOCOLS"                        },
        { &_XA_WM_SIZE_HINTS                    , "WM_SIZE_HINTS"                       },
        { &_XA_WM_STATE                         , "WM_STATE"                            },
        { &_XA_WM_TAKE_FOCUS                    , "WM_TAKE_FOCUS"                       },
        { &_XA_WM_TRANSIENT_FOR                 , "WM_TRANSIENT_FOR"                    },
        { &_XA_WM_WINDOW_ROLE                   , "WM_WINDOW_ROLE"                      },
        { &_XA_WM_ZOOM_HINTS                    , "WM_ZOOM_HINTS"                       },

        { &_XA_WINDOW_ROLE                      , "WINDOW_ROLE"                         },
        { &_XA_SM_CLIENT_ID                     , "SM_CLIENT_ID"                        },
        { &_XA_ICEWM_ACTION                     , "_ICEWM_ACTION"                       },
        { &_XATOM_MWM_HINTS                     , _XA_MOTIF_WM_HINTS                    },

        { &_XA_KWM_DOCKWINDOW                   , "KWM_DOCKWINDOW"                      },
        { &_XA_KWM_WIN_ICON                     , "KWM_WIN_ICON"                        },

        { &_XA_KDE_NET_SYSTEM_TRAY_WINDOWS      , "_KDE_NET_SYSTEM_TRAY_WINDOWS"        },
        { &_XA_KDE_NET_WM_FRAME_STRUT           , "_KDE_NET_WM_FRAME_STRUT"             },
        { &_XA_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR"  },
        { &_XA_KDE_NET_WM_WINDOW_TYPE_OVERRIDE  , "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE"    },
        { &_XA_KDE_SPLASH_PROGRESS              , "_KDE_SPLASH_PROGRESS"                },
        { &_XA_KDE_WM_CHANGE_STATE              , "_KDE_WM_CHANGE_STATE"                },

        { &_XA_WIN_APP_STATE                    , XA_WIN_APP_STATE                      },
        { &_XA_WIN_AREA_COUNT                   , XA_WIN_AREA_COUNT                     },
        { &_XA_WIN_AREA                         , XA_WIN_AREA                           },
        { &_XA_WIN_CLIENT_LIST                  , XA_WIN_CLIENT_LIST                    },
        { &_XA_WIN_DESKTOP_BUTTON_PROXY         , XA_WIN_DESKTOP_BUTTON_PROXY           },
        { &_XA_WIN_EXPANDED_SIZE                , XA_WIN_EXPANDED_SIZE                  },
        { &_XA_WIN_HINTS                        , XA_WIN_HINTS                          },
        { &_XA_WIN_ICONS                        , XA_WIN_ICONS                          },
        { &_XA_WIN_LAYER                        , XA_WIN_LAYER                          },
        { &_XA_WIN_PROTOCOLS                    , XA_WIN_PROTOCOLS                      },
        { &_XA_WIN_STATE                        , XA_WIN_STATE                          },
        { &_XA_WIN_SUPPORTING_WM_CHECK          , XA_WIN_SUPPORTING_WM_CHECK            },
        { &_XA_WIN_TRAY                         , XA_WIN_TRAY                           },
        { &_XA_WIN_WORKAREA                     , XA_WIN_WORKAREA                       },
        { &_XA_WIN_WORKSPACE_COUNT              , XA_WIN_WORKSPACE_COUNT                },
        { &_XA_WIN_WORKSPACE_NAMES              , XA_WIN_WORKSPACE_NAMES                },
        { &_XA_WIN_WORKSPACE                    , XA_WIN_WORKSPACE                      },

        { &_XA_NET_ACTIVE_WINDOW                , "_NET_ACTIVE_WINDOW"                  },
        { &_XA_NET_CLIENT_LIST                  , "_NET_CLIENT_LIST"                    },
        { &_XA_NET_CLIENT_LIST_STACKING         , "_NET_CLIENT_LIST_STACKING"           },
        { &_XA_NET_CLOSE_WINDOW                 , "_NET_CLOSE_WINDOW"                   },
        { &_XA_NET_CURRENT_DESKTOP              , "_NET_CURRENT_DESKTOP"                },
        { &_XA_NET_DESKTOP_GEOMETRY             , "_NET_DESKTOP_GEOMETRY"               },
        { &_XA_NET_DESKTOP_LAYOUT               , "_NET_DESKTOP_LAYOUT"                 },
        { &_XA_NET_DESKTOP_NAMES                , "_NET_DESKTOP_NAMES"                  },
        { &_XA_NET_DESKTOP_VIEWPORT             , "_NET_DESKTOP_VIEWPORT"               },
        { &_XA_NET_FRAME_EXTENTS                , "_NET_FRAME_EXTENTS"                  },
        { &_XA_NET_MOVERESIZE_WINDOW            , "_NET_MOVERESIZE_WINDOW"              },
        { &_XA_NET_NUMBER_OF_DESKTOPS           , "_NET_NUMBER_OF_DESKTOPS"             },
        { &_XA_NET_PROPERTIES                   , "_NET_PROPERTIES"                     },
        { &_XA_NET_REQUEST_FRAME_EXTENTS        , "_NET_REQUEST_FRAME_EXTENTS"          },
        { &_XA_NET_RESTACK_WINDOW               , "_NET_RESTACK_WINDOW"                 },
        { &_XA_NET_SHOWING_DESKTOP              , "_NET_SHOWING_DESKTOP"                },
        { &_XA_NET_STARTUP_ID                   , "_NET_STARTUP_ID"                     },
        { &_XA_NET_STARTUP_INFO                 , "_NET_STARTUP_INFO"                   },
        { &_XA_NET_STARTUP_INFO_BEGIN           , "_NET_STARTUP_INFO_BEGIN"             },
        { &_XA_NET_SUPPORTED                    , "_NET_SUPPORTED"                      },
        { &_XA_NET_SUPPORTING_WM_CHECK          , "_NET_SUPPORTING_WM_CHECK"            },
        { &_XA_NET_SYSTEM_TRAY_MESSAGE_DATA     , "_NET_SYSTEM_TRAY_MESSAGE_DATA"       },
        { &_XA_NET_SYSTEM_TRAY_OPCODE           , "_NET_SYSTEM_TRAY_OPCODE"             },
        { &_XA_NET_SYSTEM_TRAY_ORIENTATION      , "_NET_SYSTEM_TRAY_ORIENTATION"        },
        { &_XA_NET_SYSTEM_TRAY_VISUAL           , "_NET_SYSTEM_TRAY_VISUAL"             },
        { &_XA_NET_VIRTUAL_ROOTS                , "_NET_VIRTUAL_ROOTS"                  },
        { &_XA_NET_WM_ACTION_ABOVE              , "_NET_WM_ACTION_ABOVE"                },
        { &_XA_NET_WM_ACTION_BELOW              , "_NET_WM_ACTION_BELOW"                },
        { &_XA_NET_WM_ACTION_CHANGE_DESKTOP     , "_NET_WM_ACTION_CHANGE_DESKTOP"       },
        { &_XA_NET_WM_ACTION_CLOSE              , "_NET_WM_ACTION_CLOSE"                },
        { &_XA_NET_WM_ACTION_FULLSCREEN         , "_NET_WM_ACTION_FULLSCREEN"           },
        { &_XA_NET_WM_ACTION_MAXIMIZE_HORZ      , "_NET_WM_ACTION_MAXIMIZE_HORZ"        },
        { &_XA_NET_WM_ACTION_MAXIMIZE_VERT      , "_NET_WM_ACTION_MAXIMIZE_VERT"        },
        { &_XA_NET_WM_ACTION_MINIMIZE           , "_NET_WM_ACTION_MINIMIZE"             },
        { &_XA_NET_WM_ACTION_MOVE               , "_NET_WM_ACTION_MOVE"                 },
        { &_XA_NET_WM_ACTION_RESIZE             , "_NET_WM_ACTION_RESIZE"               },
        { &_XA_NET_WM_ACTION_SHADE              , "_NET_WM_ACTION_SHADE"                },
        { &_XA_NET_WM_ACTION_STICK              , "_NET_WM_ACTION_STICK"                },
        { &_XA_NET_WM_ALLOWED_ACTIONS           , "_NET_WM_ALLOWED_ACTIONS"             },
        { &_XA_NET_WM_BYPASS_COMPOSITOR         , "_NET_WM_BYPASS_COMPOSITOR"           },
        { &_XA_NET_WM_DESKTOP                   , "_NET_WM_DESKTOP"                     },
        { &_XA_NET_WM_FULL_PLACEMENT            , "_NET_WM_FULL_PLACEMENT"              },
        { &_XA_NET_WM_FULLSCREEN_MONITORS       , "_NET_WM_FULLSCREEN_MONITORS"         },
        { &_XA_NET_WM_HANDLED_ICONS             , "_NET_WM_HANDLED_ICONS"               },
        { &_XA_NET_WM_ICON_GEOMETRY             , "_NET_WM_ICON_GEOMETRY"               },
        { &_XA_NET_WM_ICON_NAME                 , "_NET_WM_ICON_NAME"                   },
        { &_XA_NET_WM_ICON                      , "_NET_WM_ICON"                        },
        { &_XA_NET_WM_MOVERESIZE                , "_NET_WM_MOVERESIZE"                  },
        { &_XA_NET_WM_NAME                      , "_NET_WM_NAME"                        },
        { &_XA_NET_WM_OPAQUE_REGION             , "_NET_WM_OPAQUE_REGION"               },
        { &_XA_NET_WM_PID                       , "_NET_WM_PID"                         },
        { &_XA_NET_WM_PING                      , "_NET_WM_PING"                        },
        { &_XA_NET_WM_STATE                     , "_NET_WM_STATE"                       },
        { &_XA_NET_WM_STATE_ABOVE               , "_NET_WM_STATE_ABOVE"                 },
        { &_XA_NET_WM_STATE_BELOW               , "_NET_WM_STATE_BELOW"                 },
        { &_XA_NET_WM_STATE_DEMANDS_ATTENTION   , "_NET_WM_STATE_DEMANDS_ATTENTION"     },
        { &_XA_NET_WM_STATE_FOCUSED             , "_NET_WM_STATE_FOCUSED"               },
        { &_XA_NET_WM_STATE_FULLSCREEN          , "_NET_WM_STATE_FULLSCREEN"            },
        { &_XA_NET_WM_STATE_HIDDEN              , "_NET_WM_STATE_HIDDEN"                },
        { &_XA_NET_WM_STATE_MAXIMIZED_HORZ      , "_NET_WM_STATE_MAXIMIZED_HORZ"        },
        { &_XA_NET_WM_STATE_MAXIMIZED_VERT      , "_NET_WM_STATE_MAXIMIZED_VERT"        },
        { &_XA_NET_WM_STATE_MODAL               , "_NET_WM_STATE_MODAL"                 },
        { &_XA_NET_WM_STATE_SHADED              , "_NET_WM_STATE_SHADED"                },
        { &_XA_NET_WM_STATE_SKIP_PAGER          , "_NET_WM_STATE_SKIP_PAGER"            },
        { &_XA_NET_WM_STATE_SKIP_TASKBAR        , "_NET_WM_STATE_SKIP_TASKBAR"          },
        { &_XA_NET_WM_STATE_STICKY              , "_NET_WM_STATE_STICKY"                },
        { &_XA_NET_WM_STRUT                     , "_NET_WM_STRUT"                       },
        { &_XA_NET_WM_STRUT_PARTIAL             , "_NET_WM_STRUT_PARTIAL"               },
        { &_XA_NET_WM_SYNC_REQUEST              , "_NET_WM_SYNC_REQUEST"                },
        { &_XA_NET_WM_SYNC_REQUEST_COUNTER      , "_NET_WM_SYNC_REQUEST_COUNTER"        },
        { &_XA_NET_WM_USER_TIME                 , "_NET_WM_USER_TIME"                   },
        { &_XA_NET_WM_USER_TIME_WINDOW          , "_NET_WM_USER_TIME_WINDOW"            },
        { &_XA_NET_WM_VISIBLE_ICON_NAME         , "_NET_WM_VISIBLE_ICON_NAME"           },
        { &_XA_NET_WM_VISIBLE_NAME              , "_NET_WM_VISIBLE_NAME"                },
        { &_XA_NET_WM_WINDOW_OPACITY            , "_NET_WM_WINDOW_OPACITY"              },
        { &_XA_NET_WM_WINDOW_TYPE               , "_NET_WM_WINDOW_TYPE"                 },
        { &_XA_NET_WM_WINDOW_TYPE_COMBO         , "_NET_WM_WINDOW_TYPE_COMBO"           },
        { &_XA_NET_WM_WINDOW_TYPE_DESKTOP       , "_NET_WM_WINDOW_TYPE_DESKTOP"         },
        { &_XA_NET_WM_WINDOW_TYPE_DIALOG        , "_NET_WM_WINDOW_TYPE_DIALOG"          },
        { &_XA_NET_WM_WINDOW_TYPE_DND           , "_NET_WM_WINDOW_TYPE_DND"             },
        { &_XA_NET_WM_WINDOW_TYPE_DOCK          , "_NET_WM_WINDOW_TYPE_DOCK"            },
        { &_XA_NET_WM_WINDOW_TYPE_DROPDOWN_MENU , "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU"   },
        { &_XA_NET_WM_WINDOW_TYPE_MENU          , "_NET_WM_WINDOW_TYPE_MENU"            },
        { &_XA_NET_WM_WINDOW_TYPE_NORMAL        , "_NET_WM_WINDOW_TYPE_NORMAL"          },
        { &_XA_NET_WM_WINDOW_TYPE_NOTIFICATION  , "_NET_WM_WINDOW_TYPE_NOTIFICATION"    },
        { &_XA_NET_WM_WINDOW_TYPE_POPUP_MENU    , "_NET_WM_WINDOW_TYPE_POPUP_MENU"      },
        { &_XA_NET_WM_WINDOW_TYPE_SPLASH        , "_NET_WM_WINDOW_TYPE_SPLASH"          },
        { &_XA_NET_WM_WINDOW_TYPE_TOOLBAR       , "_NET_WM_WINDOW_TYPE_TOOLBAR"         },
        { &_XA_NET_WM_WINDOW_TYPE_TOOLTIP       , "_NET_WM_WINDOW_TYPE_TOOLTIP"         },
        { &_XA_NET_WM_WINDOW_TYPE_UTILITY       , "_NET_WM_WINDOW_TYPE_UTILITY"         },
        { &_XA_NET_WORKAREA                     , "_NET_WORKAREA"                       },

        { &_XA_CLIPBOARD                        , "CLIPBOARD"                           },
        { &_XA_XEMBED_INFO                      , "_XEMBED_INFO"                        },
        { &_XA_TARGETS                          , "TARGETS"                             },
        { &_XA_UTF8_STRING                      , "UTF8_STRING"                         },

        { &XA_XdndAware                         , "XdndAware"                           },
        { &XA_XdndDrop                          , "XdndDrop"                            },
        { &XA_XdndEnter                         , "XdndEnter"                           },
        { &XA_XdndFinished                      , "XdndFinished"                        },
        { &XA_XdndLeave                         , "XdndLeave"                           },
        { &XA_XdndPosition                      , "XdndPosition"                        },
        { &XA_XdndProxy                         , "XdndProxy"                           },
        { &XA_XdndStatus                        , "XdndStatus"                          }
    };
    unsigned int i;

#ifdef HAVE_XINTERNATOMS
    const char *names[ACOUNT(atom_info)];
    Atom atoms[ACOUNT(atom_info)];

    for (i = 0; i < ACOUNT(atom_info); i++)
        names[i] = atom_info[i].name;

    XInternAtoms(xapp->display(), (char **)names, ACOUNT(atom_info), False, atoms);

    for (i = 0; i < ACOUNT(atom_info); i++)
        *(atom_info[i].atom) = atoms[i];
#else
    for (i = 0; i < ACOUNT(atom_info); i++)
        *(atom_info[i].atom) = XInternAtom(xapp->display(),
                                           atom_info[i].name, False);
#endif
}

void YXApplication::initPointers() {
    osmart<YCursorLoader> l(YCursor::newLoader());
    leftPointer  = l->load("left.xpm",  XC_left_ptr);
    rightPointer = l->load("right.xpm", XC_right_ptr);
    movePointer  = l->load("move.xpm",  XC_fleur);
}

void YXApplication::initModifiers() {
    XModifierKeymap *xmk = XGetModifierMapping(xapp->display());
    AltMask = MetaMask = WinMask = SuperMask = HyperMask =
        NumLockMask = ScrollLockMask = ModeSwitchMask = 0;

    if (xmk) {
        KeyCode *c = xmk->modifiermap;

        for (int m = 0; m < 8; m++)
            for (int k = 0; k < xmk->max_keypermod; k++, c++) {
                if (*c == NoSymbol)
                    continue;
                KeySym kc = XkbKeycodeToKeysym(xapp->display(), *c, 0, 0);
                if (kc == NoSymbol)
                    kc = XkbKeycodeToKeysym(xapp->display(), *c, 0, 1);
                if (kc == XK_Num_Lock && NumLockMask == 0)
                    NumLockMask = (1 << m);
                if (kc == XK_Scroll_Lock && ScrollLockMask == 0)
                    ScrollLockMask = (1 << m);
                if ((kc == XK_Alt_L || kc == XK_Alt_R) && AltMask == 0)
                    AltMask = (1 << m);
                if ((kc == XK_Meta_L || kc == XK_Meta_R) && MetaMask == 0)
                    MetaMask = (1 << m);
                if ((kc == XK_Super_L || kc == XK_Super_R) && SuperMask == 0)
                    SuperMask = (1 << m);
                if ((kc == XK_Hyper_L || kc == XK_Hyper_R) && HyperMask == 0)
                    HyperMask = (1 << m);
                if ((kc == XK_Mode_switch || kc == XK_ISO_Level3_Shift) && ModeSwitchMask == 0)
                    ModeSwitchMask = (1 << m);
            }

        XFreeModifiermap(xmk);
    }
    if (MetaMask == AltMask)
        MetaMask = 0;

    MSG(("alt:%d meta:%d super:%d hyper:%d mode:%d num:%d scroll:%d",
         AltMask, MetaMask, SuperMask, HyperMask, ModeSwitchMask,
         NumLockMask, ScrollLockMask));

    // some hacks for "broken" modifier configurations
    if (HyperMask == SuperMask)
        HyperMask = 0;

    // this basically does what <0.9.13 versions did
    if (AltMask != 0 && MetaMask == Mod1Mask) {
        MetaMask = AltMask;
        AltMask = Mod1Mask;
    }

    if (AltMask == 0 && MetaMask != 0) {
        if (MetaMask != Mod1Mask) {
            AltMask = Mod1Mask;
        }
        else {
            AltMask = MetaMask;
            MetaMask = 0;
        }
    }

    if (AltMask == 0)
        AltMask = Mod1Mask;

    if (ModeSwitchMask & (AltMask | MetaMask | SuperMask | HyperMask))
        ModeSwitchMask = 0;

    PRECONDITION(xapp->AltMask != 0);
    PRECONDITION(xapp->AltMask != ShiftMask);
    PRECONDITION(xapp->AltMask != ControlMask);
    PRECONDITION(xapp->AltMask != xapp->MetaMask);

    KeyMask =
        ControlMask |
        ShiftMask |
        AltMask |
        MetaMask |
        SuperMask |
        HyperMask |
        ModeSwitchMask;

    ButtonMask =
        Button1Mask |
        Button2Mask |
        Button3Mask |
        Button4Mask |
        Button5Mask;

    ButtonKeyMask = KeyMask | ButtonMask;

#if 0
    KeySym wl = XKeycodeToKeysym(app->display(), 115, 0);
    KeySym wr = XKeycodeToKeysym(app->display(), 116, 0);

    if (wl == XK_Super_L) {
    } else if (wl == XK_Meta_L) {
    }
#endif
    // this will do for now, but we should actualy check the keycodes
    Win_L = Win_R = 0;

    if (SuperMask != 0) {
        WinMask = SuperMask;

        Win_L = XK_Super_L;
        Win_R = XK_Super_R;
    }
    MSG(("alt:%d meta:%d super:%d hyper:%d win:%d mode:%d num:%d scroll:%d",
         AltMask, MetaMask, SuperMask, HyperMask, WinMask, ModeSwitchMask,
         NumLockMask, ScrollLockMask));

}

void YXApplication::dispatchEvent(YWindow *win, XEvent &xev) {
    if (xev.type == KeyPress || xev.type == KeyRelease) {
        YWindow *w = win;

        if (!(fGrabWindow != 0 && !fGrabTree)) {
            if (w->toplevel())
                w = w->toplevel();

            if (w->getFocusWindow() != 0)
                w = w->getFocusWindow();
        }

        while (w && (w->handleKey(xev.xkey) == false)) {
            if (fGrabTree && w == fXGrabWindow)
                break;
            w = w->parent();
        }
    } else {
        Window child;

        if (xev.type == MotionNotify) {
            if (xev.xmotion.window != win->handle()) {
                if (XTranslateCoordinates(xapp->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xmotion.x, xev.xmotion.y,
                                          &xev.xmotion.x, &xev.xmotion.y, &child) == True)
                    xev.xmotion.window = win->handle();
                else
                    return ;
            }
        } else if (xev.type == ButtonPress || xev.type == ButtonRelease ||
                   xev.type == EnterNotify || xev.type == LeaveNotify)
        {
            if (xev.xbutton.window != win->handle()) {
                if (XTranslateCoordinates(xapp->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xbutton.x, xev.xbutton.y,
                                          &xev.xbutton.x, &xev.xbutton.y, &child) == True)
                    xev.xbutton.window = win->handle();
                else
                    return ;
            }
        } else if (xev.type == KeyPress || xev.type == KeyRelease) {
            if (xev.xkey.window != win->handle()) {
                if (XTranslateCoordinates(xapp->display(),
                                          xev.xany.window, win->handle(),
                                          xev.xkey.x, xev.xkey.y,
                                          &xev.xkey.x, &xev.xkey.y, &child) == True)
                    xev.xkey.window = win->handle();
                else
                    return ;
            }
        }
        win->handleEvent(xev);
    }
}

void YXApplication::handleGrabEvent(YWindow *winx, XEvent &xev) {
    struct {
        YWindow *ptr;
    } win = { winx };

    PRECONDITION(win.ptr != 0);
    if (fGrabTree) {
        if (xev.xbutton.subwindow != None) {
            if ( ! windowContext.find(xev.xbutton.subwindow, &win.ptr))
            {
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    win.ptr = 0;
                else
                    win.ptr = fGrabWindow;
            }
        } else {
            if ( ! windowContext.find(xev.xbutton.window, &win.ptr))
            {
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    win.ptr = 0;
                else
                    win.ptr = fGrabWindow;
            }
        }
        if (win.ptr == 0)
            return ;
        {
            YWindow *p = win.ptr;
            while (p) {
                if (p == fXGrabWindow)
                    break;
                p = p->parent();
            }
            if (p == 0) {
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    return ;
                else
                    win.ptr = fGrabWindow;
            }
        }
        if (xev.type == EnterNotify || xev.type == LeaveNotify)
            if (win.ptr != fGrabWindow)
                return ;
        if (fGrabWindow != fXGrabWindow)
            win.ptr = fGrabWindow;
    }
    dispatchEvent(win.ptr, xev);
}

void YXApplication::replayEvent() {
    if (!fReplayEvent) {
        fReplayEvent = true;
        XAllowEvents(xapp->display(), ReplayPointer, CurrentTime);
    }
}

void YXApplication::captureGrabEvents(YWindow *win) {
    if (fGrabWindow == fXGrabWindow && fGrabTree) {
        fGrabWindow = win;
    }
}

void YXApplication::releaseGrabEvents(YWindow *win) {
    if (win == fGrabWindow && fGrabTree) {
        fGrabWindow = fXGrabWindow;
    }
}

int YXApplication::grabEvents(YWindow *win, Cursor ptr, unsigned int eventMask, int grabMouse, int grabKeyboard, int grabTree) {
    int rc;

    if (fGrabWindow != 0)
        return 0;
    if (win == 0)
        return 0;

    fGrabTree = grabTree;
    if (grabMouse) {
        fGrabMouse = 1;
        rc = XGrabPointer(display(), win->handle(),
                          grabTree ? True : False,
                          eventMask,
                          GrabModeSync, GrabModeAsync,
                          None, ptr, CurrentTime);

        if (rc != Success) {
            MSG(("grab status = %d\x7", rc));
            return 0;
        }
    } else {
        fGrabMouse = 0;

        XChangeActivePointerGrab(display(),
                                 eventMask,
                                 ptr, CurrentTime);
    }

    if (grabKeyboard) {
        rc = XGrabKeyboard(display(), win->handle(),
                           ///False,
                           grabTree ? True : False,
                           GrabModeSync, GrabModeAsync, CurrentTime);
        if (rc != Success && grabMouse) {
            MSG(("grab status = %d\x7", rc));
            XUngrabPointer(display(), CurrentTime);
            return 0;
        }
    }
    XAllowEvents(xapp->display(), SyncPointer, CurrentTime);

    fXGrabWindow = win;
    fGrabWindow = win;
    return 1;
}

int YXApplication::releaseEvents() {
    if (fGrabWindow == 0)
        return 0;
    fGrabWindow = 0;
    fXGrabWindow = 0;
    fGrabTree = 0;
    if (fGrabMouse) {
        XUngrabPointer(display(), CurrentTime);
        fGrabMouse = 0;
    }
    XUngrabKeyboard(display(), CurrentTime);

    return 1;
}

void YXApplication::afterWindowEvent(XEvent & /*xev*/) {
}

bool YXApplication::filterEvent(const XEvent &xev) {
    if (xev.type == MappingNotify) {
        MSG(("MappingNotify"));
        XMappingEvent xmapping = xev.xmapping;
        XRefreshKeyboardMapping(&xmapping);

        initModifiers();

        desktop->grabKeys();
        return true;
    }
    return false;
}

void YXApplication::saveEventTime(const XEvent &xev) {
    switch (xev.type) {
    case ButtonPress:
    case ButtonRelease:
        lastEventTime = xev.xbutton.time;
        break;

    case MotionNotify:
        lastEventTime = xev.xmotion.time;
        break;

    case KeyPress:
    case KeyRelease:
        lastEventTime = xev.xkey.time;
        break;

    case EnterNotify:
    case LeaveNotify:
        lastEventTime = xev.xcrossing.time;
        break;

    case PropertyNotify:
        lastEventTime = xev.xproperty.time;
        break;

    case SelectionClear:
        lastEventTime = xev.xselectionclear.time;
        break;

    case SelectionRequest:
        lastEventTime = xev.xselectionrequest.time;
        break;

    case SelectionNotify:
        lastEventTime = xev.xselection.time;
        break;
    }
}

Time YXApplication::getEventTime(const char */*debug*/) const {
    return lastEventTime;
}

bool YXApplication::hasColormap() {
    XVisualInfo pattern;
    pattern.screen = DefaultScreen(display());

    int nVisuals;
    bool rc = false;

    XVisualInfo *first_visual(XGetVisualInfo(display(), VisualScreenMask,
                                              &pattern, &nVisuals));
    XVisualInfo *visual = first_visual;

    while (visual && nVisuals--) {
        if (visual->c_class & 1)
            rc = true;
        visual++;
    }

    if (first_visual)
        XFree(first_visual);

    return rc;
}


void YXApplication::alert() {
    XBell(display(), 100);
}

void YXApplication::setClipboardText(const ustring &data) {
    if (fClip == 0)
        fClip = new YClipboard();
    if (!fClip)
        return ;
    cstring s(data);
    fClip->setData(s.c_str(), s.c_str_len());
}

const char* YXApplication::getHelpText() {
    return _(
    "  -d, --display=NAME  NAME of the X server to use.\n"
    "  --sync              Synchronize X11 commands.\n"
    );
}

YXApplication::AppArgs
YXApplication::parseArgs(int *argc, char ***argv, const char *displayName) {
    AppArgs appArgs = { displayName, None, };

    for (char ** arg = *argv + 1; arg < *argv + *argc; ++arg) {
        if (**arg == '-') {
            char *value;
            if (is_help_switch(*arg)) {
                print_help_exit(getHelpText());
            }
            else if (is_version_switch(*arg)) {
                print_version_exit(VERSION);
            }
            else if (is_copying_switch(*arg)) {
                print_copying_exit();
            }
            else if (GetArgument(value, "d", "display", arg, *argv+*argc)) {
                appArgs.displayName = value;
            }
            else if (is_long_switch(*arg, "sync"))
                appArgs.runSynchronized = true;
        }
    }

    if (appArgs.displayName == None)
        appArgs.displayName = getenv("DISPLAY");
    else
        setenv("DISPLAY", appArgs.displayName, True);

    return appArgs;
}

Display* YXApplication::openDisplay() {
    Display* display = XOpenDisplay(fArgs.displayName);
    if (display == 0)
        die(1, _("Can't open display: %s. X must be running and $DISPLAY set."),
            fArgs.displayName ? fArgs.displayName : _("<none>"));

    if (fArgs.runSynchronized)
        XSynchronize(display, True);

    return display;
}

YXApplication::YXApplication(int *argc, char ***argv, const char *displayName):
    YApplication(argc, argv),

    fArgs( parseArgs(argc, argv, displayName)),
    fDisplay( openDisplay()),
    fScreen( DefaultScreen(display())),
    fRoot(  RootWindow(display(), screen())),
    fDepth( DefaultDepth(display(), screen())),
    fVisual( DefaultVisual(display(), screen())),
    fColormap( DefaultColormap(display(), screen())),
    fBlack( BlackPixel(display(), screen())),
    fWhite( WhitePixel(display(), screen())),

    lastEventTime(CurrentTime),
    fPopup(0),
    fGrabTree(0),
    fXGrabWindow(0),
    fGrabMouse(0),
    fGrabWindow(0),
    fClip(0),
    fReplayEvent(false)
{
    xapp = this;
    xfd.registerPoll(this, ConnectionNumber(display()));

    new YDesktop(0, root());
    extern void image_init();
    image_init();

    initAtoms();
    initModifiers();
    initPointers();
    initExtensions();
}

void YXApplication::initExtensions() {

#ifdef CONFIG_SHAPE
    if ((shapesSupported = XShapeQueryExtension(display(),
                                           &shapeEventBase, &shapeErrorBase)))
    {
        XShapeQueryVersion(display(),
                &shapeVersionMajor, &shapeVersionMinor);
    }
#endif

#ifdef CONFIG_RENDER
    if ((renderSupported = XRenderQueryExtension(display(),
                    &renderEventBase, &renderErrorBase)))
    {
        XRenderQueryVersion(display(),
                &renderVersionMajor, &renderVersionMinor);
    }
#endif

#ifdef CONFIG_XRANDR
    if ((xrandrSupported = XRRQueryExtension(display(),
                                        &xrandrEventBase, &xrandrErrorBase)))
    {
        XRRQueryVersion(display(), &xrandrVersionMajor, &xrandrVersionMinor);

        MSG(("XRRVersion: %d %d", xrandrVersionMajor, xrandrVersionMinor));
        if (12 <= 10 * xrandrVersionMajor + xrandrVersionMinor)
            xrandr12 = true;
    }
#endif
}

YXApplication::~YXApplication() {
    xfd.unregisterPoll();
    XCloseDisplay(display());
    xapp = 0;
}

bool YXApplication::handleXEvents() {
    const int prratio = 3;
    int retrieved = 0;
    for (; retrieved < XPending(display()); retrieved += prratio - 1) {
        XEvent xev;

        XNextEvent(display(), &xev);
#ifdef DEBUG
        xeventcount++;
#endif
        //msg("%d", xev.type);

        saveEventTime(xev);

        logEvent(xev);

        if (filterEvent(xev)) {
            ;
        } else {
            bool ge = xev.type == ButtonPress ||
                      xev.type == ButtonRelease ||
                      xev.type == MotionNotify ||
                      xev.type == KeyPress ||
                      xev.type == KeyRelease /*||
                      xev.type == EnterNotify ||
                      xev.type == LeaveNotify*/;

            fReplayEvent = false;

            if (fPopup && ge) {
                handleGrabEvent(fPopup, xev);
            } else if (fGrabWindow && ge) {
                handleGrabEvent(fGrabWindow, xev);
            } else {
                handleWindowEvent(xev.xany.window, xev);
            }
            if (fGrabWindow) {
                if (xev.type == ButtonPress ||
                    xev.type == ButtonRelease ||
                    xev.type == MotionNotify)
                {
                    if (!fReplayEvent) {
                        XAllowEvents(xapp->display(), SyncPointer, CurrentTime);
                    }
                }
            }
        }
        XFlush(display());
    }
    return retrieved > 0;
}

bool YXApplication::handleIdle() {
    return handleXEvents();
}

void YXApplication::handleWindowEvent(Window xwindow, XEvent &xev) {
    struct {
        YWindow *ptr;
    } window = { 0 };

    if (windowContext.find(xwindow, &window.ptr))
    {
        if ((xev.type == KeyPress || xev.type == KeyRelease)
            && window.ptr->toplevel() != 0)
        {
            YWindow *w = window.ptr;

            w = w->toplevel();

            if (w->getFocusWindow() != 0)
                w = w->getFocusWindow();

            dispatchEvent(w, xev);
        } else {
            window.ptr->handleEvent(xev);
        }
    } else {
        if (xev.type == MapRequest) {
            // !!! java seems to do this ugliness
            //YFrameWindow *f = getFrame(xev.xany.window);
            tlog("APP BUG? mapRequest for window %lX sent to destroyed frame %lX!",
                xev.xmaprequest.parent,
                xev.xmaprequest.window);
            desktop->handleEvent(xev);
        } else if (xev.type == ConfigureRequest) {
            tlog("APP BUG? configureRequest for window %lX sent to destroyed frame %lX!",
                xev.xmaprequest.parent,
                xev.xmaprequest.window);
            desktop->handleEvent(xev);
        }
        else if (xev.type == ClientMessage && desktop != 0) {
            Atom mesg = xev.xclient.message_type;
            if (mesg == _XA_NET_REQUEST_FRAME_EXTENTS) {
                desktop->handleEvent(xev);
            }
            else
            {
                MSG(("Unknown client message %ld, win 0x%lX, data %ld,%ld",
                     mesg, xev.xclient.window,
                     xev.xclient.data.l[0], xev.xclient.data.l[1]));
            }
        }
        else if (xev.type != DestroyNotify) {
            MSG(("unknown window 0x%lX event=%d", xev.xany.window, xev.type));
        }
    }
    if (xev.type == KeyPress || xev.type == KeyRelease) ///!!!
        afterWindowEvent(xev);
}

void YXApplication::flushXEvents() {
    XFlush(display());
}

void YXPoll::notifyRead() {
    owner()->handleXEvents();
}

void YXPoll::notifyWrite() { }

bool YXPoll::forRead() {
    return true;
}

bool YXPoll::forWrite() { return false; }

void YAtom::atomize() {
    if (screen) {
        char buf[256];
        snprintf(buf, sizeof buf, "%s%d", name, xapp->screen());
        atom = xapp->atom(buf);
    } else {
        atom = xapp->atom(name);
    }
}

YAtom::operator Atom() {
    if (atom == None)
        atomize();
    return atom;
}

// vim: set sw=4 ts=4 et:
