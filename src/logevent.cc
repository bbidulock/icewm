#include "config.h"
#include "base.h"
#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>
#ifdef CONFIG_SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef CONFIG_XRANDR
#include <X11/extensions/Xrandr.h>
#endif
#include "logevent.h"

bool loggingEvents;
bool loggedEventsInited;

static const char* emptyAtom(Atom atom) { return ""; }
static AtomNameFunc atomName = emptyAtom;
void setAtomName(AtomNameFunc func) { atomName = func; }
const char* getAtomName(unsigned long atom) {
    return atomName ? atomName(atom) : "";
}

#if LOGEVENTS

bool loggedEvents[LASTEvent];

static const char eventNames[][17] = {
    "KeyPress",             //  2
    "KeyRelease",           //  3
    "ButtonPress",          //  4
    "ButtonRelease",        //  5
    "MotionNotify",         //  6
    "EnterNotify",          //  7
    "LeaveNotify",          //  8
    "FocusIn",              //  9
    "FocusOut",             // 10
    "KeymapNotify",         // 11
    "Expose",               // 12
    "GraphicsExpose",       // 13
    "NoExpose",             // 14
    "VisibilityNotify",     // 15
    "CreateNotify",         // 16
    "DestroyNotify",        // 17
    "UnmapNotify",          // 18
    "MapNotify",            // 19
    "MapRequest",           // 20
    "ReparentNotify",       // 21
    "ConfigureNotify",      // 22
    "ConfigureRequest",     // 23
    "GravityNotify",        // 24
    "ResizeRequest",        // 25
    "CirculateNotify",      // 26
    "CirculateRequest",     // 27
    "PropertyNotify",       // 28
    "SelectionClear",       // 29
    "SelectionRequest",     // 30
    "SelectionNotify",      // 31
    "ColormapNotify",       // 32
    "ClientMessage",        // 33
    "MappingNotify",        // 34
    "GenericEvent",         // 35
};
static const char* eventName(int eventType) {
    if (inrange(eventType, KeyPress, GenericEvent))
        return eventNames[eventType - KeyPress];
    return "UnknownEvent!";
}

void setLogEvent(int evtype, bool enable) {
    if (size_t(evtype) < sizeof loggedEvents)
        loggedEvents[evtype] = enable;
    else if (evtype == -1)
        memset(loggedEvents, enable, sizeof loggedEvents);
}

inline const char* boolStr(Bool aBool) {
    return aBool ? "True" : "False";
}

void logAny(const XAnyEvent& xev) {
    tlog("window=0x%lX: %s type=%d, send=%s, #%lu",
        xev.window, eventName(xev.type), xev.type,
        boolStr(xev.send_event), (unsigned long) xev.serial);
}

void logButton(const XButtonEvent& xev) {
    tlog("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, "
        "(%d:%d %d:%d) state=0x%X button=%d same_screen=%s",
        xev.window,
        eventName(xev.type),
        xev.root,
        xev.subwindow,
        xev.time,
        xev.x, xev.y,
        xev.x_root, xev.y_root,
        xev.state,
        xev.button,
        boolStr(xev.same_screen));
}

void logColormap(const XColormapEvent& xev) {
    tlog("window=0x%lX: colormapNotify colormap=%ld new=%s state=%d",
        xev.window,
        xev.colormap,
        xev.c_new ? "True" : "False",
        xev.state);
}

void logConfigureNotify(const XConfigureEvent& xev) {
    tlog("window=0x%lX: configureNotify serial=%lu event=0x%lX, (%+d%+d %dx%d) border_width=%d, above=0x%lX, override_redirect=%s",
        xev.window,
        (unsigned long) xev.serial,
        xev.event,
        xev.x, xev.y,
        xev.width, xev.height,
        xev.border_width,
        xev.above,
        xev.override_redirect ? "True" : "False");
}

void logConfigureRequest(const XConfigureRequestEvent& xev) {
    const int size = 256;
    char buf[size];
    int len = snprintf(buf, size, "window=0x%lX: %s configureRequest",
                       xev.window, xev.send_event ? "synth" : "real");
    len += snprintf(buf + len, size - len, " serial=%lu parent=0x%lX",
                    (unsigned long) xev.serial, xev.parent);
    if ((xev.value_mask & CWX) && 0 < len && len < size)
        len += snprintf(buf + len, size - len, " X=%d", xev.x);
    if ((xev.value_mask & CWY) && 0 < len && len < size)
        len += snprintf(buf + len, size - len, " Y=%d", xev.y);
    if ((xev.value_mask & CWWidth) && 0 < len && len < size)
        len += snprintf(buf + len, size - len, " Width=%d", xev.width);
    if ((xev.value_mask & CWHeight) && 0 < len && len < size)
        len += snprintf(buf + len, size - len, " Height=%d", xev.height);
    if ((xev.value_mask & CWBorderWidth) && 0 < len && len < size)
        len += snprintf(buf + len, size - len, " Border=%d", xev.border_width);
    if ((xev.value_mask & CWSibling) && 0 < len && len < size)
        len += snprintf(buf + len, size - len, " Sibling=0x%lX", xev.above);
    if ((xev.value_mask & CWStackMode) && 0 < len && len < size)
        len += snprintf(buf + len, size - len, " StackMode=%s",
                        xev.detail == Above ? "Above" :
                        xev.detail == Below ? "Below" :
                        xev.detail == TopIf ? "TopIf" :
                        xev.detail == BottomIf ? "BottomIf" :
                        xev.detail == Opposite ? "Opposite" : "Invalid");
    tlog("%s", (0 < len && len < size) ? buf : "lcrbug");
}

void logCreate(const XCreateWindowEvent& xev) {
    tlog("window=0x%lX: create serial=%lu parent=0x%lX, (%+d%+d %dx%d) border_width=%d, override_redirect=%s",
        xev.window,
        (unsigned long) xev.serial,
        xev.parent,
        xev.x, xev.y,
        xev.width, xev.height,
        xev.border_width,
        xev.override_redirect ? "True" : "False");
}

void logCrossing(const XCrossingEvent& xev) {
    tlog("window=0x%06lX: %s serial=%lu root=0x%lX, subwindow=0x%lX, time=%ld, "
        "(%d:%d %d:%d) mode=%s detail=%s same_screen=%s, focus=%s state=0x%X",
        xev.window,
        eventName(xev.type),
        (unsigned long) xev.serial,
        xev.root,
        xev.subwindow,
        xev.time,
        xev.x, xev.y,
        xev.x_root, xev.y_root,
        xev.mode == NotifyNormal ? "Normal" :
        xev.mode == NotifyGrab ? "Grab" :
        xev.mode == NotifyUngrab ? "Ungrab" :
        xev.mode == NotifyWhileGrabbed ? "Grabbed" : "Unknown",
        xev.detail == NotifyAncestor ? "Ancestor" :
        xev.detail == NotifyVirtual ? "Virtual" :
        xev.detail == NotifyInferior ? "Inferior" :
        xev.detail == NotifyNonlinear ? "Nonlinear" :
        xev.detail == NotifyNonlinearVirtual ? "NonlinearVirtual" :
        xev.detail == NotifyPointer ? "Pointer" :
        xev.detail == NotifyPointerRoot ? "PointerRoot" :
        xev.detail == NotifyDetailNone ? "DetailNone" : "Unknown",
        xev.same_screen ? "True" : "False",
        xev.focus ? "True" : "False",
        xev.state);
}

void logDestroy(const XDestroyWindowEvent& xev) {
    tlog("window=0x%lX: destroy serial=%lu event=0x%lX",
        xev.window,
        (unsigned long) xev.serial,
        xev.event);
}

void logExpose(const XExposeEvent& xev) {
    tlog("window=0x%lX: expose (%+d%+d %dx%d) count=%d",
        xev.window,
        xev.x, xev.y, xev.width, xev.height,
        xev.count);
}

void logFocus(const XFocusChangeEvent& xev) {
    tlog("window=0x%lX: %s mode=%s, detail=%s",
        xev.window,
        eventName(xev.type),
        xev.mode == NotifyNormal ? "NotifyNormal" :
        xev.mode == NotifyWhileGrabbed ? "NotifyWhileGrabbed" :
        xev.mode == NotifyGrab ? "NotifyGrab" :
        xev.mode == NotifyUngrab ? "NotifyUngrab" : "???",
        xev.detail == NotifyAncestor ? "NotifyAncestor" :
        xev.detail == NotifyVirtual ? "NotifyVirtual" :
        xev.detail == NotifyInferior ? "NotifyInferior" :
        xev.detail == NotifyNonlinear ? "NotifyNonlinear" :
        xev.detail == NotifyNonlinearVirtual ? "NotifyNonlinearVirtual" :
        xev.detail == NotifyPointer ? "NotifyPointer" :
        xev.detail == NotifyPointerRoot ? "NotifyPointerRoot" :
        xev.detail == NotifyDetailNone ? "NotifyDetailNone" : "???");
}

void logGravity(const XGravityEvent& xev) {
    tlog("window=0x%lX: gravityNotify serial=%lu, x=%+d, y=%+d",
        xev.window,
        (unsigned long) xev.serial,
        xev.x, xev.y);
}

void logKey(const XKeyEvent& xev) {
    tlog("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, (%d:%d %d:%d) state=0x%X keycode=0x%x same_screen=%s",
        xev.window,
        eventName(xev.type),
        xev.root,
        xev.subwindow,
        xev.time,
        xev.x, xev.y,
        xev.x_root, xev.y_root,
        xev.state,
        xev.keycode,
        xev.same_screen ? "True" : "False");
}

void logMapRequest(const XMapRequestEvent& xev) {
    tlog("window=0x%lX: mapRequest serial=%lu parent=0x%lX",
        xev.window,
        (unsigned long) xev.serial,
        xev.parent);
}

void logMapNotify(const XMapEvent& xev) {
    tlog("window=0x%lX: mapNotify serial=%lu event=0x%lX, override_redirect=%s",
        xev.window,
        (unsigned long) xev.serial,
        xev.event,
        xev.override_redirect ? "True" : "False");
}

void logUnmap(const XUnmapEvent& xev) {
    tlog("window=0x%lX: unmapNotify serial=%lu event=0x%lX, from_configure=%s send_event=%s",
        xev.window,
        (unsigned long) xev.serial,
        xev.event,
        xev.from_configure ? "True" : "False",
        xev.send_event ? "True" : "False");
}

void logMotion(const XMotionEvent& xev) {
    tlog("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, "
        "(%d:%d %d:%d) state=0x%X is_hint=%s same_screen=%s",
        xev.window,
        eventName(xev.type),
        xev.root,
        xev.subwindow,
        xev.time,
        xev.x, xev.y,
        xev.x_root, xev.y_root,
        xev.state,
        xev.is_hint == NotifyHint ? "NotifyHint" : "",
        xev.same_screen ? "True" : "False");
}

void logProperty(const XPropertyEvent& xev) {
    tlog("window=0x%lX: propertyNotify %s time=%ld state=%s",
        xev.window,
        atomName(xev.atom),
        xev.time,
        xev.state == PropertyNewValue ? "NewValue" :
        xev.state == PropertyDelete ? "Delete" : "?");
}

void logReparent(const XReparentEvent& xev) {
    tlog("window=0x%lX: reparentNotify serial=%lu event=0x%lX, parent=0x%lX, (%d:%d), override_redirect=%s",
        xev.window,
        (unsigned long) xev.serial,
        xev.event,
        xev.parent,
        xev.x, xev.y,
        xev.override_redirect ? "True" : "False");
}

void logVisibility(const XVisibilityEvent& xev) {
    tlog("window=0x%lX: visibilityNotify state=%s",
        xev.window,
        xev.state == VisibilityPartiallyObscured ? "partial" :
        xev.state == VisibilityFullyObscured ? "obscured" :
        xev.state == VisibilityUnobscured ? "unobscured" : "bogus"
        );
}

#ifdef CONFIG_SHAPE
void logShape(const XEvent& xev) {
    const XShapeEvent &shp = (const XShapeEvent &)xev;
    tlog("window=0x%lX: %s kind=%s %d:%d=%dx%d shaped=%s time=%ld",
        shp.window, "ShapeEvent",
        shp.kind == ShapeBounding ? "ShapeBounding" :
        shp.kind == ShapeClip ? "ShapeClip" : "unknown_shape_kind",
        shp.x, shp.y, shp.width, shp.height, boolstr(shp.shaped), shp.time);
}
#endif

#ifdef CONFIG_XRANDR
void logRandrScreen(const XEvent& xev) {
    const XRRScreenChangeNotifyEvent& evt =
        (const XRRScreenChangeNotifyEvent &)xev;
    tlog("window=0x%lX: %s index=%u order=%u "
        "rotation=%u width=%dpx(%dmm) height=%dpx(%dmm)",
        evt.window, "XRRScreenChangeNotifyEvent",
        evt.size_index, evt.subpixel_order, (evt.rotation & 15) * 45,
        evt.width, evt.mwidth, evt.height, evt.mheight
       );
}
#endif

#ifdef CONFIG_XRANDR
void logRandrNotify(const XEvent& xev) {
    const XRRNotifyEvent& nev = (const XRRNotifyEvent &)xev;
    if (nev.subtype == RRNotify_CrtcChange) {
        const XRRCrtcChangeNotifyEvent& e =
            (const XRRCrtcChangeNotifyEvent &) xev;
        tlog("window=0x%lX: %s crtc=%lu mode=%lu rotation=%u %ux%u+%d+%d",
            e.window, "XRRCrtcChangeNotifyEvent",
            e.crtc, e.mode, (e.rotation & 15) * 45, e.width, e.height, e.x, e.y
           );
    }
    else if (nev.subtype == RRNotify_OutputChange) {
        const XRROutputChangeNotifyEvent& e =
            (const XRROutputChangeNotifyEvent &) xev;
        tlog("window=0x%lX: %s output=%lu crtc=%lu mode=%lu "
            "rotation=%u connection=%s subpixel=%u",
            e.window, "XRROutputChangeNotifyEvent",
            e.output, e.crtc, e.mode, (e.rotation & 15) * 45,
            e.connection == RR_Connected ? "RR_Connected" :
            e.connection == RR_Disconnected ? "RR_Disconnected" :
            e.connection == RR_UnknownConnection ? "RR_UnknownConnection" :
            "unknown", e.subpixel_order
           );
    }
    else if (nev.subtype == RRNotify_OutputProperty) {
        const XRROutputPropertyNotifyEvent& e =
            (const XRROutputPropertyNotifyEvent &) xev;
        tlog("window=0x%lX: %s output=%lu property=%lu state=%s",
            e.window, "XRROutputPropertyNotifyEvent",
            e.output, e.property,
            e.state == PropertyNewValue ? "NewValue" :
            e.state == PropertyDelete ? "Deleted" :
            "unknown"
           );
    }
#ifdef RRNotify_ProviderChange
    else if (nev.subtype == RRNotify_ProviderChange) {
        const XRRProviderChangeNotifyEvent& e =
            (const XRRProviderChangeNotifyEvent &) xev;
        tlog("window=0x%lX: %s provider=%lu current_role=%u",
            e.window, "XRRProviderChangeNotifyEvent",
            e.provider, e.current_role
           );
    }
#endif
#ifdef RRNotify_ProviderProperty
    else if (nev.subtype == RRNotify_ProviderProperty) {
        const XRRProviderPropertyNotifyEvent& e =
            (const XRRProviderPropertyNotifyEvent &) xev;
        tlog("window=0x%lX: %s provider=%lu property=%lu state=%s",
            e.window, "XRRProviderPropertyNotifyEvent",
            e.provider, e.property,
            e.state == PropertyNewValue ? "NewValue" :
            e.state == PropertyDelete ? "Deleted" :
            "unknown"
           );
    }
#endif
#ifdef RRNotify_ResourceChange
    else if (nev.subtype == RRNotify_ResourceChange) {
        const XRRResourceChangeNotifyEvent& e =
            (const XRRResourceChangeNotifyEvent &) xev;
        tlog("window=0x%lX: %s",
            e.window, "XRRResourceChangeNotifyEvent"
           );
    }
#endif
}
#endif

#endif

#if LOGEVENTS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

void logEvent(const XEvent& xev) {
#if LOGEVENTS
    typedef void (*fun)(const XEvent&);
    static void (*const loggers[])(const XEvent&) = {
        (fun) logAny,               //  0 reserved
        (fun) logAny,               //  1 reserved
        (fun) logKey,               //  2 KeyPress
        (fun) logKey,               //  3 KeyRelease
        (fun) logButton,            //  4 ButtonPress
        (fun) logButton,            //  5 ButtonRelease
        (fun) logMotion,            //  6 MotionNotify
        (fun) logCrossing,          //  7 EnterNotify
        (fun) logCrossing,          //  8 LeaveNotify
        (fun) logFocus,             //  9 FocusIn
        (fun) logFocus,             // 10 FocusOut
        (fun) logAny,               // 11 KeymapNotify
        (fun) logExpose,            // 12 Expose
        (fun) logAny,               // 13 GraphicsExpose
        (fun) logAny,               // 14 NoExpose
        (fun) logVisibility,        // 15 VisibilityNotify
        (fun) logCreate,            // 16 CreateNotify
        (fun) logDestroy,           // 17 DestroyNotify
        (fun) logUnmap,             // 18 UnmapNotify
        (fun) logMapNotify,         // 19 MapNotify
        (fun) logMapRequest,        // 20 MapRequest
        (fun) logReparent,          // 21 ReparentNotify
        (fun) logConfigureNotify,   // 22 ConfigureNotify
        (fun) logConfigureRequest,  // 23 ConfigureRequest
        (fun) logGravity,           // 24 GravityNotify
        (fun) logAny,               // 25 ResizeRequest
        (fun) logAny,               // 26 CirculateNotify
        (fun) logAny,               // 27 CirculateRequest
        (fun) logProperty,          // 28 PropertyNotify
        (fun) logAny,               // 29 SelectionClear
        (fun) logAny,               // 30 SelectionRequest
        (fun) logAny,               // 31 SelectionNotify
        (fun) logColormap,          // 32 ColormapNotify
        (fun) logClientMessage,     // 33 ClientMessage
        (fun) logAny,               // 34 MappingNotify
        (fun) logAny,               // 35 GenericEvent
    };
    if (loggingEvents && size_t(xev.type) < sizeof loggedEvents &&
        (loggedEventsInited || initLogEvents()) && loggedEvents[xev.type])
    {
        loggers[xev.type](xev);
    }
#endif
}

#if LOGEVENTS
#pragma GCC diagnostic pop
#endif

bool initLogEvents() {
#if LOGEVENTS
    if (loggedEventsInited == false) {
        memset(loggedEvents, false, sizeof loggedEvents);

        // setLogEvent(KeyPress, true);
        // setLogEvent(KeyRelease, true);
        setLogEvent(ButtonPress, true);
        setLogEvent(ButtonRelease, true);
        // setLogEvent(MotionNotify, true);
        setLogEvent(EnterNotify, true);
        setLogEvent(LeaveNotify, true);
        // setLogEvent(FocusIn, true);
        // setLogEvent(FocusOut, true);
        // setLogEvent(KeymapNotify, true);
        // setLogEvent(Expose, true);
        // setLogEvent(GraphicsExpose, true);
        // setLogEvent(NoExpose, true);
        // setLogEvent(VisibilityNotify, true);
        setLogEvent(CreateNotify, true);
        setLogEvent(DestroyNotify, true);
        setLogEvent(UnmapNotify, true);
        setLogEvent(MapNotify, true);
        setLogEvent(MapRequest, true);
        setLogEvent(ReparentNotify, true);
        setLogEvent(ConfigureNotify, true);
        setLogEvent(ConfigureRequest, true);
        // setLogEvent(GravityNotify, true);
        // setLogEvent(ResizeRequest, true);
        // setLogEvent(CirculateNotify, true);
        // setLogEvent(CirculateRequest, true);
        // setLogEvent(PropertyNotify, true);
        // setLogEvent(SelectionClear, true);
        // setLogEvent(SelectionRequest, true);
        // setLogEvent(SelectionNotify, true);
        // setLogEvent(ColormapNotify, true);
        setLogEvent(ClientMessage, true);
        // setLogEvent(MappingNotify, true);
        // setLogEvent(GenericEvent, true);

        loggedEventsInited = true;
    }
#endif
    return loggedEventsInited;
}

bool toggleLogEvents() {
#if LOGEVENTS
    loggingEvents = !loggingEvents && initLogEvents();
#endif
    return loggingEvents;
}

void logClientMessage(const XClientMessageEvent& event) {
    const char* name = atomName(event.message_type);
    const long* data = event.data.l;
    char head[64];
    snprintf(head, sizeof head, "window=0x%lx: ", event.window);
    if (strcmp(name, "_NET_WM_STATE") == 0) {
        const char* op =
            data[0] == 0 ? "REMOVE" :
            data[0] == 1 ? "ADD" :
            data[0] == 2 ? "TOGGLE" : "?";
        const char* p1 = data[1] ? atomName(data[1]) : "";
        const char* p2 = data[2] ? atomName(data[2]) : "";
        tlog("%sClientMessage %s %d data=%s,%s,%s\n",
                head, name, event.format, op, p1, p2);
    }
    else if (strcmp(name, "_WM_CHANGE_STATE") == 0) {
        const char* op =
            data[0] == 0 ? "WithdrawnState" :
            data[0] == 1 ? "NormalState" :
            data[0] == 3 ? "IconicState" : "?";
        tlog("%sClientMessage %s %s\n", head, name, op);
    }
    else {
        tlog("%sClientMessage %s fmt=%d data=%ld,0x%lx,0x%lx",
            head, name, event.format, data[0], data[1], data[2]);
    }
}

