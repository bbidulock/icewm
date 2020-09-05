#include "config.h"
#include "yxapp.h"
#include "yfull.h"
#include "ymenu.h"
#include "wmmgr.h"
#include "MwmUtil.h"
#include "ypointer.h"
#include "yxcontext.h"
#include "guievent.h"
#include "intl.h"
#undef override
#include <X11/Xproto.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

YXApplication *xapp = nullptr;

YDesktop *desktop = nullptr;
YContext<YWindow> windowContext;

YCursor YXApplication::leftPointer;
YCursor YXApplication::rightPointer;
YCursor YXApplication::movePointer;
bool YXApplication::synchronizeX11;
bool YXApplication::alphaBlending;

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
Atom _XATOM_MWM_INFO;
Atom _XA_WINDOW_ROLE;
Atom _XA_SM_CLIENT_ID;
Atom _XA_ICEWM_ACTION;
Atom _XA_ICEWM_GUIEVENT;
Atom _XA_ICEWM_HINT;
Atom _XA_ICEWM_FONT_PATH;
Atom _XA_ICEWMBG_IMAGE;
Atom _XA_XROOTPMAP_ID;
Atom _XA_XROOTCOLOR_PIXEL;
Atom _XA_GDK_TIMESTAMP_PROP;
Atom _XA_CLIPBOARD;
Atom _XA_MANAGER;
Atom _XA_TARGETS;
Atom _XA_XEMBED;
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

YExtension composite;
YExtension damage;
YExtension fixes;
YExtension render;
YExtension shapes;
YExtension xrandr;
YExtension xinerama;

#ifdef DEBUG
int xeventcount = 0;
#endif

class YClipboard: public YWindow {
public:
    void setData(mstring data) {
        fData = data;
        if (length() == 0)
            clearSelection(false);
        else
            acquireSelection(false);
    }
    void handleSelectionClear(const XSelectionClearEvent &clear) {
        if (clear.selection == _XA_CLIPBOARD) {
            fData = null;
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
                (request.target == XA_STRING ||
                 request.target == _XA_UTF8_STRING) &&
                length() > 0)
            {
                unsigned char *data =
                    reinterpret_cast<unsigned char *>(
                            const_cast<char *>(fData.c_str()));
                XChangeProperty(xapp->display(),
                                request.requestor,
                                request.property,
                                request.target,
                                8, PropModeReplace,
                                data, length());
            } else if (request.selection == _XA_CLIPBOARD &&
                       request.target == _XA_TARGETS &&
                       length() > 0)
            {
                Atom targets[] = {
                    XA_STRING,
                    _XA_UTF8_STRING,
                };
                unsigned char* data =
                    reinterpret_cast<unsigned char *>(targets);
                const int count = int ACOUNT(targets);

                XChangeProperty(xapp->display(),
                                request.requestor,
                                request.property,
                                request.target,
                                32, PropModeReplace,
                                data, count);
            } else {
                notify.property = None;
            }

            XSendEvent(xapp->display(), notify.requestor, False, None,
                       reinterpret_cast<XEvent *>(&notify));
        }
    }

    int length() const {
        return fData.length();
    }

private:
    mstring fData;
};

YAtomName YXApplication::atom_info[] = {
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
        { &_XA_ICEWM_GUIEVENT                   , XA_GUI_EVENT_NAME                     },
        { &_XA_ICEWM_HINT                       , "_ICEWM_WINOPTHINT"                   },
        { &_XA_ICEWM_FONT_PATH                  , "ICEWM_FONT_PATH"                     },
        { &_XA_ICEWMBG_IMAGE                    , "_ICEWMBG_IMAGE"                     },
        { &_XA_XROOTPMAP_ID                     , "_XROOTPMAP_ID"                       },
        { &_XA_XROOTCOLOR_PIXEL                 , "_XROOTCOLOR_PIXEL"                   },
        { &_XA_GDK_TIMESTAMP_PROP               , "GDK_TIMESTAMP_PROP"                  },
        { &_XATOM_MWM_HINTS                     , _XA_MOTIF_WM_HINTS                    },
        { &_XATOM_MWM_INFO                      , _XA_MOTIF_WM_INFO                     },

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
        { &_XA_MANAGER                          , "MANAGER"                             },
        { &_XA_XEMBED                           , "_XEMBED"                             },
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

void YXApplication::initAtoms() {
    unsigned int i;

#ifdef HAVE_XINTERNATOMS
    const char *names[ACOUNT(atom_info)];
    Atom atoms[ACOUNT(atom_info)];

    for (i = 0; i < ACOUNT(atom_info); i++)
        names[i] = atom_info[i].name;

    XInternAtoms(xapp->display(), const_cast<char **>(names),
                 ACOUNT(atom_info), False, atoms);

    for (i = 0; i < ACOUNT(atom_info); i++)
        *(atom_info[i].atom) = atoms[i];
#else
    for (i = 0; i < ACOUNT(atom_info); i++)
        *(atom_info[i].atom) = XInternAtom(xapp->display(),
                                           atom_info[i].name, False);
#endif

    qsort(atom_info, ACOUNT(atom_info), sizeof(atom_info[0]), sortAtoms);
    setAtomName(atomName);
}

int YXApplication::sortAtoms(const void* p1, const void* p2) {
    const YAtomName* n1 = static_cast<const YAtomName*>(p1);
    const YAtomName* n2 = static_cast<const YAtomName*>(p2);
    const Atom a1 = *n1->atom;
    const Atom a2 = *n2->atom;
    return int(long(a1) - long(a2));
}

const char* YXApplication::atomName(Atom atom) {
    int lo = 0, hi = int ACOUNT(atom_info);
    while (lo < hi) {
        int pv = (lo + hi) / 2;
        if (atom < *atom_info[pv].atom)
            hi = pv;
        else if (atom > *atom_info[pv].atom)
            lo = pv + 1;
        else
            return atom_info[pv].name;
    }
    static char buf[32];
    snprintf(buf, sizeof buf, "Atom(%lu)", atom);
    return buf;
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

bool YXApplication::hasControlAlt(unsigned state) const {
    return xapp->AltMask && hasbits((state & KeyMask), ControlMask | AltMask);
}

bool YXApplication::hasWinMask(unsigned state) const {
    return xapp->WinMask && hasbit((state & KeyMask), WinMask);
}

void YXApplication::dispatchEvent(YWindow *win, XEvent &xev) {
    if (xev.type == KeyPress || xev.type == KeyRelease) {
        YWindow *w = win;

        if (w && (fGrabWindow == nullptr || fGrabTree)) {
            if (w->toplevel())
                w = w->toplevel();

            if (w->getFocusWindow())
                w = w->getFocusWindow();
        }

        for (; w && (w->handleKey(xev.xkey) == false); w = w->parent()) {
            if (fGrabTree && w == fXGrabWindow)
                break;
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
                    win.ptr = nullptr;
                else
                    win.ptr = fGrabWindow;
            }
        } else {
            if ( ! windowContext.find(xev.xbutton.window, &win.ptr))
            {
                if (xev.type == EnterNotify || xev.type == LeaveNotify)
                    win.ptr = nullptr;
                else
                    win.ptr = fGrabWindow;
            }
        }
        if (win.ptr == nullptr)
            return ;
        {
            YWindow *p = win.ptr;
            for (; p; p = p->parent()) {
                if (p == fXGrabWindow)
                    break;
            }
            if (p == nullptr) {
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

bool YXApplication::grabEvents(YWindow *win, Cursor ptr,
        unsigned long eventMask, bool grabMouse, bool grabKeyboard, bool grabTree)
{
    if (fGrabWindow || !win)
        return false;

    fGrabTree = grabTree;
    fGrabMouse = grabMouse;
    if (grabMouse) {
        int rc = XGrabPointer(display(), win->handle(), grabTree,
                              eventMask, GrabModeSync, GrabModeAsync,
                              None, ptr, CurrentTime);
        if (rc) {
            MSG(("grab status = %d\x7", rc));
            return false;
        }
    }
    else {
        XChangeActivePointerGrab(display(), eventMask, ptr, CurrentTime);
    }

    if (grabKeyboard) {
        int rc = XGrabKeyboard(display(), win->handle(), grabTree,
                               GrabModeSync, GrabModeAsync, CurrentTime);
        if (rc) {
            MSG(("grab status = %d\x7", rc));
            if (grabMouse) {
                XUngrabPointer(display(), CurrentTime);
                fGrabMouse = false;
            }
            return false;
        }
    }
    XAllowEvents(xapp->display(), SyncPointer, CurrentTime);

    fXGrabWindow = win;
    fGrabWindow = win;
    return true;
}

bool YXApplication::releaseEvents() {
    if (fGrabWindow == nullptr)
        return false;

    fGrabWindow = nullptr;
    fXGrabWindow = nullptr;
    fGrabTree = false;
    if (fGrabMouse) {
        XUngrabPointer(display(), CurrentTime);
        fGrabMouse = false;
    }
    XUngrabKeyboard(display(), CurrentTime);

    return true;
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

Time YXApplication::getEventTime(const char *) const {
    return lastEventTime;
}

bool YXApplication::haveColormaps(Display* dpy) {
    XVisualInfo pattern = { nullptr, None, DefaultScreen(dpy), 0, };
    int i = 0, num = 0, mask = VisualScreenMask;
    xsmart<XVisualInfo> info(XGetVisualInfo(dpy, mask, &pattern, &num));
    for (; i < num && notbit(info[i].c_class, 1); ++i);
    return i < num;
}

Visual* YXApplication::visualForDepth(unsigned depth) const {
    Visual* vis =
        depth == 32 ? fVisual32 :
        depth == 24 ? fVisual24 :
        depth == unsigned(DefaultDepth(display(), screen())) ?
                 DefaultVisual(display(), screen()) :
                 CopyFromParent;
    return vis;
}

Colormap YXApplication::colormapForDepth(unsigned depth) const {
    Colormap cmap =
        depth == 32 ? fColormap32 :
        depth == 24 ? fColormap24 :
        depth == unsigned(DefaultDepth(display(), screen())) ?
                 DefaultColormap(display(), screen()) :
                 CopyFromParent;
    return cmap;
}

Colormap YXApplication::colormapForVisual(Visual* visual) const {
    Colormap cmap =
        visual == fVisual32 ? fColormap32 :
        visual == fVisual24 ? fColormap24 :
        visual == DefaultVisual(display(), screen()) ?
                  DefaultColormap(display(), screen()) :
                  CopyFromParent;
    return cmap;
}

XRenderPictFormat* YXApplication::formatForDepth(unsigned depth) const {
    XRenderPictFormat* format =
        depth == 32 ? fFormat32 :
        depth == 24 ? fFormat24 :
        nullptr;
    return format;
}

XRenderPictFormat* YXApplication::findFormat(int depth) const {
    XRenderPictFormat* format = nullptr;
    if (depth == 32)
        format = XRenderFindStandardFormat(fDisplay, PictStandardARGB32);
    if (depth == 24)
        format = XRenderFindStandardFormat(fDisplay, PictStandardRGB24);
    return format;
}

Visual* YXApplication::findVisual(int depth) const {
    Visual* found = nullptr;
    XRenderPictFormat* pictFormat = findFormat(depth);
    if (pictFormat) {
        XVisualInfo pattern = {
            found, None, fScreen, depth, TrueColor, None, None, None, 0, 8
        };
        int count = 0, mask = VisualDepthMask | VisualScreenMask |
                              VisualClassMask | VisualBitsPerRGBMask;
        xsmart<XVisualInfo> info(
                XGetVisualInfo(fDisplay, mask, &pattern, &count));
        for (int i = 0; i < count && found == nullptr; ++i) {
            XRenderPictFormat* format =
                XRenderFindVisualFormat(fDisplay, info[i].visual);
            if (format == pictFormat) {
                found = info[i].visual;
            }
        }
    }
    if (found == nullptr) {
        XVisualInfo pattern = {
            found, 0, fScreen, depth, TrueColor, 0xff0000, 0xff00, 0xff, 0, 8
        };
        int mask = VisualScreenMask | VisualDepthMask | VisualClassMask
                 | VisualRedMaskMask | VisualGreenMaskMask
                 | VisualBlueMaskMask | VisualBitsPerRGBMask;
        int count = 0;
        xsmart<XVisualInfo> info(
                XGetVisualInfo(fDisplay, mask, &pattern, &count));
        if (count && info) {
            found = info->visual;
        }
    }
    if (found == nullptr && depth == DefaultDepth(fDisplay, fScreen)) {
        found = DefaultVisual(fDisplay, fScreen);
    }
    return found;
}

int YXApplication::cmapError(Display *disp, XErrorEvent *xerr) {
    // Ignore create colormap error.
    // This may occur with Xdmx for 32-bit visuals.
    // Ignore for now, unless problems do show up.
    return Success;
}

Colormap YXApplication::getColormap(int depth) const {
    Colormap cmap = None;
    Visual* visual = depth == 32 ? fVisual32
                   : depth == 24 ? fVisual24 : nullptr;
    if (visual == DefaultVisual(fDisplay, fScreen)) {
        cmap = DefaultColormap(fDisplay, fScreen);
    }
    else if (visual) {
        XErrorHandler old = XSetErrorHandler(cmapError);
        cmap = XCreateColormap(fDisplay, fRoot, visual, AllocNone);
        XSync(fDisplay, False);
        XSetErrorHandler(old);
    }
    else if (depth == DefaultDepth(fDisplay, fScreen)) {
        cmap = DefaultColormap(fDisplay, fScreen);
    }
    return cmap;
}

void YXApplication::alert() {
    XBell(display(), 100);
}

void YXApplication::setClipboardText(mstring data) {
    fClip->setData(data);
}

void YXApplication::dropClipboard() {
    fClip = null;
}

const char* YXApplication::getHelpText() {
    return _(
    "  -d, --display=NAME  NAME of the X server to use.\n"
    "  --sync              Synchronize X11 commands.\n"
    );
}

const char*
YXApplication::parseArgs(int argc, char **argv, const char *displayName) {
    for (char ** arg = argv + 1; arg < argv + argc; ++arg) {
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
            else if (GetArgument(value, "d", "display", arg, argv + argc)) {
                if (isEmpty(displayName))
                    displayName = value;
            }
            else if (is_long_switch(*arg, "sync"))
                synchronizeX11 = true;
            else if (is_long_switch(*arg, "alpha"))
                alphaBlending = true;
        }
    }

    return displayName;
}

Display* YXApplication::openDisplay(const char* displayName) {
    if (nonempty(displayName))
        setenv("DISPLAY", displayName, True);

    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr)
        die(1, _("Can't open display: %s. X must be running and $DISPLAY set."),
            displayName ? displayName : _("<none>"));

    if (synchronizeX11)
        XSynchronize(display, True);

    XSetErrorHandler(errorHandler);

    initExtensions(display);

    return display;
}

YXApplication::YXApplication(int *argc, char ***argv, const char *displayName):
    YApplication(argc, argv),

    fDisplay( openDisplay( parseArgs(*argc, *argv, displayName))),
    fScreen( DefaultScreen(fDisplay)),
    fRoot( RootWindow(fDisplay, fScreen)),
    fFormat32( findFormat(32)),
    fFormat24( findFormat(24)),
    fVisual32( findVisual(32)),
    fVisual24( findVisual(24)),
    fColormap32( getColormap(32)),
    fColormap24( getColormap(24)),
    fAlpha( alphaBlending && fVisual32 && fColormap32 ),
    fDepth( fAlpha ? 32 : fVisual24 ? 24 : DefaultDepth(fDisplay, fScreen)),
    fVisual( visualForDepth(fDepth)),
    fColormap( colormapForDepth(fDepth)),
    fHasColormaps( haveColormaps(display())),
    fBlack( BlackPixel(display(), screen())),
    fWhite( WhitePixel(display(), screen())),

    lastEventTime(CurrentTime),
    fPopup(nullptr),
    xfd(this),
    fXGrabWindow(nullptr),
    fGrabWindow(nullptr),
    fGrabTree(false),
    fGrabMouse(false),
    fReplayEvent(false)
{
    xapp = this;
    xfd.registerPoll(ConnectionNumber(display()));

    new YDesktop(nullptr, root());
    extern void image_init();
    image_init();

    initAtoms();
    initModifiers();
    initPointers();
}

void YExtension::init(Display* dis, QueryFunc ext, QueryFunc ver) {
    supported = (*ext)(dis, &eventBase, &errorBase)
             && (*ver)(dis, &versionMajor, &versionMinor);
}

void YXApplication::initExtensions(Display* dpy) {

    composite.init(dpy, XCompositeQueryExtension, XCompositeQueryVersion);
    damage.init(dpy, XDamageQueryExtension, XDamageQueryVersion);
    fixes.init(dpy, XFixesQueryExtension, XFixesQueryVersion);
    render.init(dpy, XRenderQueryExtension, XRenderQueryVersion);

#ifdef CONFIG_SHAPE
    shapes.init(dpy, XShapeQueryExtension, XShapeQueryVersion);
#endif

#ifdef CONFIG_XRANDR
    xrandr.init(dpy, XRRQueryExtension, XRRQueryVersion);
    xrandr.supported = (12 <= 10 * xrandr.versionMajor + xrandr.versionMinor);
#endif

#ifdef XINERAMA
    xinerama.init(dpy, XineramaQueryExtension, XineramaQueryVersion);
    xinerama.supported = (xinerama.supported && XineramaIsActive(dpy));
#endif
}

YXApplication::~YXApplication() {
    if (fColormap32 != CopyFromParent)
        XFreeColormap(xapp->display(), fColormap32);

    xfd.unregisterPoll();
    XCloseDisplay(display());
    xapp = nullptr;
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

#if LOGEVENTS
        if (loggingEvents) {
            if (xev.type < LASTEvent)
                logEvent(xev);
#ifdef CONFIG_SHAPE
            else if (shapes.isEvent(xev.type, ShapeNotify))
                logShape(xev);
#endif
#ifdef CONFIG_XRANDR
            else if (xrandr.isEvent(xev.type, RRScreenChangeNotify))
                logRandrScreen(xev);
            else if (xrandr.isEvent(xev.type, RRNotify))
                logRandrNotify(xev);
#endif
        }
#endif

        if (filterEvent(xev)) {
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
    } window = { nullptr };

    if (windowContext.find(xwindow, &window.ptr))
    {
        if ((xev.type == KeyPress || xev.type == KeyRelease)
            && window.ptr->toplevel())
        {
            YWindow *w = window.ptr->toplevel();

            if (w->getFocusWindow())
                w = w->getFocusWindow();

            dispatchEvent(w, xev);
        } else {
            window.ptr->handleEvent(xev);
        }
    } else {
        if (xev.type == MapRequest) {
            // !!! java seems to do this ugliness
            //YFrameWindow *f = getFrame(xev.xany.window);
            TLOG(("APP BUG? mapRequest for window %lX sent to destroyed frame %lX!",
                xev.xmaprequest.parent,
                xev.xmaprequest.window));
            desktop->handleEvent(xev);
        } else if (xev.type == ConfigureRequest) {
            TLOG(("APP BUG? configureRequest for window %lX sent to destroyed frame %lX!",
                xev.xmaprequest.parent,
                xev.xmaprequest.window));
            desktop->handleEvent(xev);
        }
        else if (xev.type == ClientMessage && desktop) {
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

int YXApplication::handleError(XErrorEvent* xev) {
    return BadImplementation;
}

int YXApplication::errorHandler(Display* display, XErrorEvent* xev) {
    int rc = xapp->handleError(xev);
    if (rc == Success)
        return rc;

    XDBG {
        char message[80], req[80], number[80];

        snprintf(number, sizeof number, "%d", xev->request_code);
        XGetErrorDatabaseText(display, "XRequest", number, "", req, sizeof req);
        if (req[0] == 0)
            snprintf(req, sizeof req, "[request_code=%d]", xev->request_code);

        if (XGetErrorText(display, xev->error_code, message, sizeof message))
            *message = '\0';

        tlog("X error %s(0x%lx): %s, #%lu, %+ld, %+ld.",
             req, xev->resourceid, message, xev->serial,
             long(NextRequest(display)) - long(xev->serial),
             long(LastKnownRequestProcessed(display)) - long(xev->serial));

#if defined(DEBUG) || defined(PRECON)
        if (xapp->synchronized()) {
            switch (xev->request_code) {
                case X_GetWindowAttributes:
                    break;
                case X_GetImage:
                case X_CreateGC:
                    show_backtrace();
                    break;
                default:
                    show_backtrace();
                    break;
            }
        }
        else if (ONCE) {
            TLOG(("unsynchronized"));
        }
#endif
    }

    if (rc == BadImplementation)
        xapp->exit(rc);
    return rc;
}

void YXPoll::notifyRead() {
    owner()->handleXEvents();
}

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

YTextProperty::YTextProperty(const char* str) {
    encoding = XA_STRING;
    format = 8;
    if (str) {
        nitems = strlen(str);
        value = new unsigned char[1 + nitems];
        if (value) memcpy(value, str, 1 + nitems);
    } else {
        nitems = 0;
        value = nullptr;
    }
}

YTextProperty::~YTextProperty() {
    if (value) delete[] value;
}

void YProperty::discard() {
    if (fData) {
        XFree(fData);
        fData = nullptr;
        fSize = None;
        fType = None;
    }
}

const YProperty& YProperty::update() {
    discard();
    int fmt = 0;
    if (XGetWindowProperty(xapp->display(), fWind, fProp, 0L, fLimit, fDelete,
                           fKind, &fType, &fmt, &fSize, &fMore, &fData) ==
        Success && fData && fSize && fmt == fBits && (fKind == fType || !fKind))
    {
    } else {
        discard();
    }
    return *this;
}

// vim: set sw=4 ts=4 et:
