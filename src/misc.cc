/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "sysdep.h"
#include "ylib.h"
#include "debug.h"
#include "base.h"
#include "intl.h"
#include "ref.h"
#include "yarray.h"
#include <time.h>

#ifdef CONFIG_SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#if defined(__GXX_RTTI) && (__GXX_ABI_VERSION >= 1002)
#define HAVE_GCC_ABI_DEMANGLE
#endif
#ifdef HAVE_GCC_ABI_DEMANGLE
#include <cxxabi.h>
#endif
#if defined(HAVE_BACKTRACE_SYMBOLS_FD) && defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

#ifdef DEBUG
bool debug = false;
bool debug_z = false;
#endif

bool loggingEvents;
bool loggedEventsInited;
#ifdef LOGEVENTS
bool loggedEvents[LASTEvent];
#endif

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
const char* eventName(int eventType) {
    if (inrange(eventType, KeyPress, GenericEvent))
        return eventNames[eventType - KeyPress];
    return "UnknownEvent!";
}

bool initLogEvents() {
#ifdef LOGEVENTS
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
        // setLogEvent(ClientMessage, true);
        // setLogEvent(MappingNotify, true);
        // setLogEvent(GenericEvent, true);

        loggedEventsInited = true;
    }
#endif
    return loggedEventsInited;
}

bool toggleLogEvents() {
    return loggingEvents = !loggingEvents && initLogEvents();
}

void setLogEvent(int evtype, bool enable) {
#ifdef LOGEVENTS
    if ((size_t) evtype < sizeof loggedEvents)
        loggedEvents[evtype] = enable;
    else if (evtype == -1)
        memset(loggedEvents, enable, sizeof loggedEvents);
#else
    (void) evtype;
    (void) enable;
#endif
}

#undef msg
#define msg tlog

inline const char* boolStr(Bool aBool) {
    return aBool ? "True" : "False";
}

void logAny(const union _XEvent& xev) {
    msg("window=0x%lX: %s type=%d, send=%s, #%lu",
        xev.xany.window, eventName(xev.type), xev.type,
        boolStr(xev.xany.send_event), (unsigned long) xev.xany.serial);
}

void logButton(const union _XEvent& xev) {
    msg("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, "
        "(%d:%d %d:%d) state=0x%X button=0x%X same_screen=%s",
        xev.xbutton.window,
        eventName(xev.type),
        xev.xbutton.root,
        xev.xbutton.subwindow,
        xev.xbutton.time,
        xev.xbutton.x, xev.xbutton.y,
        xev.xbutton.x_root, xev.xbutton.y_root,
        xev.xbutton.state,
        xev.xbutton.button,
        boolStr(xev.xbutton.same_screen));
}

void logClientMessage(const union _XEvent& xev) {
    msg("window=0x%lX: clientMessage message_type=0x%lX format=%d",
        xev.xclient.window,
        xev.xclient.message_type,
        xev.xclient.format);
}

void logColormap(const union _XEvent& xev) {
    msg("window=0x%lX: colormapNotify colormap=%ld new=%s state=%d",
        xev.xcolormap.window,
        xev.xcolormap.colormap,
        xev.xcolormap.c_new ? "True" : "False",
        xev.xcolormap.state);
}

void logConfigureNotify(const union _XEvent& xev) {
    msg("window=0x%lX: configureNotify serial=%10lu event=0x%lX, (%+d%+d %dx%d) border_width=%d, above=0x%lX, override_redirect=%s",
        xev.xconfigure.window,
        (unsigned long) xev.xany.serial,
        xev.xconfigure.event,
        xev.xconfigure.x, xev.xconfigure.y,
        xev.xconfigure.width, xev.xconfigure.height,
        xev.xconfigure.border_width,
        xev.xconfigure.above,
        xev.xconfigure.override_redirect ? "True" : "False");
}

void logConfigureRequest(const union _XEvent& xev) {
    msg("window=0x%lX: %s configureRequest serial=%10lu parent=0x%lX, (%+d%+d %dx%d) border_width=%d, above=0x%lX, detail=%d, value_mask=0x%lX %s%s%s%s%s%s",
        xev.xconfigurerequest.window,
        xev.xconfigurerequest.send_event ? "synth" : "real",
        (unsigned long) xev.xany.serial,
        xev.xconfigurerequest.parent,
        xev.xconfigurerequest.x,
        xev.xconfigurerequest.y,
        xev.xconfigurerequest.width,
        xev.xconfigurerequest.height,
        xev.xconfigurerequest.border_width,
        xev.xconfigurerequest.above,
        xev.xconfigurerequest.detail,
        xev.xconfigurerequest.value_mask,
        xev.xconfigurerequest.value_mask & CWX ? "X" : "",
        xev.xconfigurerequest.value_mask & CWY ? "Y" : "",
        xev.xconfigurerequest.value_mask & CWWidth ? "W" : "",
        xev.xconfigurerequest.value_mask & CWHeight ? "H" : "",
        xev.xconfigurerequest.value_mask & CWSibling ? "S" : "",
        xev.xconfigurerequest.value_mask & CWStackMode ? "M" : "");
}

void logCreate(const union _XEvent& xev) {
    msg("window=0x%lX: create serial=%10lu parent=0x%lX, (%+d%+d %dx%d) border_width=%d, override_redirect=%s",
        xev.xcreatewindow.window,
        (unsigned long) xev.xany.serial,
        xev.xcreatewindow.parent,
        xev.xcreatewindow.x, xev.xcreatewindow.y,
        xev.xcreatewindow.width, xev.xcreatewindow.height,
        xev.xcreatewindow.border_width,
        xev.xcreatewindow.override_redirect ? "True" : "False");
}

void logCrossing(const union _XEvent& xev) {
    msg("window=0x%lX: %s serial=%10lu root=0x%lX, subwindow=0x%lX, time=%ld, "
        "(%d:%d %d:%d) mode=%d detail=%d same_screen=%s, focus=%s state=0x%X",
        xev.xcrossing.window,
        eventName(xev.type),
        (unsigned long) xev.xany.serial,
        xev.xcrossing.root,
        xev.xcrossing.subwindow,
        xev.xcrossing.time,
        xev.xcrossing.x, xev.xcrossing.y,
        xev.xcrossing.x_root, xev.xcrossing.y_root,
        xev.xcrossing.mode,
        xev.xcrossing.detail,
        xev.xcrossing.same_screen ? "True" : "False",
        xev.xcrossing.focus ? "True" : "False",
        xev.xcrossing.state);
}

void logDestroy(const union _XEvent& xev) {
    msg("window=0x%lX: destroy serial=%10lu event=0x%lX",
        xev.xdestroywindow.window,
        (unsigned long) xev.xany.serial,
        xev.xdestroywindow.event);
}

void logExpose(const union _XEvent& xev) {
    msg("window=0x%lX: expose (%+d%+d %dx%d) count=%d",
        xev.xexpose.window,
        xev.xexpose.x, xev.xexpose.y, xev.xexpose.width, xev.xexpose.height,
        xev.xexpose.count);
}

void logFocus(const union _XEvent& xev) {
    msg("window=0x%lX: %s mode=%s, detail=%s",
        xev.xfocus.window,
        eventName(xev.type),
        xev.xfocus.mode == NotifyNormal ? "NotifyNormal" :
        xev.xfocus.mode == NotifyWhileGrabbed ? "NotifyWhileGrabbed" :
        xev.xfocus.mode == NotifyGrab ? "NotifyGrab" :
        xev.xfocus.mode == NotifyUngrab ? "NotifyUngrab" : "???",
        xev.xfocus.detail == NotifyAncestor ? "NotifyAncestor" :
        xev.xfocus.detail == NotifyVirtual ? "NotifyVirtual" :
        xev.xfocus.detail == NotifyInferior ? "NotifyInferior" :
        xev.xfocus.detail == NotifyNonlinear ? "NotifyNonlinear" :
        xev.xfocus.detail == NotifyNonlinearVirtual ? "NotifyNonlinearVirtual" :
        xev.xfocus.detail == NotifyPointer ? "NotifyPointer" :
        xev.xfocus.detail == NotifyPointerRoot ? "NotifyPointerRoot" :
        xev.xfocus.detail == NotifyDetailNone ? "NotifyDetailNone" : "???");
}

void logGravity(const union _XEvent& xev) {
    msg("window=0x%lX: gravityNotify serial=%10lu, x=%+d, y=%+d",
        xev.xgravity.window,
        (unsigned long) xev.xany.serial,
        xev.xgravity.x, xev.xgravity.y);
}

void logKey(const union _XEvent& xev) {
    msg("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, (%d:%d %d:%d) state=0x%X keycode=0x%x same_screen=%s",
        xev.xkey.window,
        eventName(xev.type),
        xev.xkey.root,
        xev.xkey.subwindow,
        xev.xkey.time,
        xev.xkey.x, xev.xkey.y,
        xev.xkey.x_root, xev.xkey.y_root,
        xev.xkey.state,
        xev.xkey.keycode,
        xev.xkey.same_screen ? "True" : "False");
}

void logMapRequest(const union _XEvent& xev) {
    msg("window=0x%lX: mapRequest serial=%10lu parent=0x%lX",
        xev.xmaprequest.window,
        (unsigned long) xev.xany.serial,
        xev.xmaprequest.parent);
}

void logMapNotify(const union _XEvent& xev) {
    msg("window=0x%lX: mapNotify serial=%10lu event=0x%lX, override_redirect=%s",
        xev.xmap.window,
        (unsigned long) xev.xany.serial,
        xev.xmap.event,
        xev.xmap.override_redirect ? "True" : "False");
}

void logUnmap(const union _XEvent& xev) {
    msg("window=0x%lX: unmapNotify serial=%10lu event=0x%lX, from_configure=%s send_event=%s",
        xev.xunmap.window,
        (unsigned long) xev.xany.serial,
        xev.xunmap.event,
        xev.xunmap.from_configure ? "True" : "False",
        xev.xunmap.send_event ? "True" : "False");
}

void logMotion(const union _XEvent& xev) {
    msg("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, "
        "(%d:%d %d:%d) state=0x%X is_hint=%s same_screen=%s",
        xev.xmotion.window,
        eventName(xev.type),
        xev.xmotion.root,
        xev.xmotion.subwindow,
        xev.xmotion.time,
        xev.xmotion.x, xev.xmotion.y,
        xev.xmotion.x_root, xev.xmotion.y_root,
        xev.xmotion.state,
        xev.xmotion.is_hint == NotifyHint ? "NotifyHint" : "",
        xev.xmotion.same_screen ? "True" : "False");
}

void logProperty(const union _XEvent& xev) {
    msg("window=0x%lX: propertyNotify atom=0x%lX time=%ld state=%d",
        xev.xproperty.window,
        xev.xproperty.atom,
        xev.xproperty.time,
        xev.xproperty.state);
}

void logReparent(const union _XEvent& xev) {
    msg("window=0x%lX: reparentNotify serial=%10lu event=0x%lX, parent=0x%lX, (%d:%d), override_redirect=%s",
        xev.xreparent.window,
        (unsigned long) xev.xany.serial,
        xev.xreparent.event,
        xev.xreparent.parent,
        xev.xreparent.x, xev.xreparent.y,
        xev.xreparent.override_redirect ? "True" : "False");
}

void logShape(const union _XEvent& xev) {
#ifdef CONFIG_SHAPE
    const XShapeEvent &shp = (const XShapeEvent &)xev;
    msg("window=0x%lX: %s kind=%s %d:%d=%dx%d shaped=%s time=%ld",
        shp.window, "ShapeEvent",
        shp.kind == ShapeBounding ? "ShapeBounding" :
        shp.kind == ShapeClip ? "ShapeClip" : "unknown_shape_kind",
        shp.x, shp.y, shp.width, shp.height, boolstr(shp.shaped), shp.time);
#endif
}

void logVisibility(const union _XEvent& xev) {
    msg("window=0x%lX: visibilityNotify state=%d",
        xev.xvisibility.window,
        xev.xvisibility.state);
}

void logEvent(const union _XEvent& xev) {
#ifdef LOGEVENTS
    if (loggingEvents == false || (size_t) xev.type >= sizeof loggedEvents)
        return;
    if (loggedEventsInited == false && initLogEvents() == false)
        return;
    if (loggedEvents[xev.type] == false)
        return;

    static void (*const loggers[])(const XEvent&) = {
        logAny,               //  0 reserved
        logAny,               //  1 reserved
        logKey,               //  2 KeyPress
        logKey,               //  3 KeyRelease
        logButton,            //  4 ButtonPress
        logButton,            //  5 ButtonRelease
        logMotion,            //  6 MotionNotify
        logAny,               //  7 EnterNotify
        logAny,               //  8 LeaveNotify
        logFocus,             //  9 FocusIn
        logFocus,             // 10 FocusOut
        logAny,               // 11 KeymapNotify
        logExpose,            // 12 Expose
        logAny,               // 13 GraphicsExpose
        logAny,               // 14 NoExpose
        logVisibility,        // 15 VisibilityNotify
        logCreate,            // 16 CreateNotify
        logDestroy,           // 17 DestroyNotify
        logUnmap,             // 18 UnmapNotify
        logMapNotify,         // 19 MapNotify
        logMapRequest,        // 20 MapRequest
        logReparent,          // 21 ReparentNotify
        logConfigureNotify,   // 22 ConfigureNotify
        logConfigureRequest,  // 23 ConfigureRequest
        logGravity,           // 24 GravityNotify
        logAny,               // 25 ResizeRequest
        logAny,               // 26 CirculateNotify
        logAny,               // 27 CirculateRequest
        logProperty,          // 28 PropertyNotify
        logAny,               // 29 SelectionClear
        logAny,               // 30 SelectionRequest
        logAny,               // 31 SelectionNotify
        logColormap,          // 32 ColormapNotify
        logClientMessage,     // 33 ClientMessage
        logAny,               // 34 MappingNotify
        logAny,               // 35 GenericEvent
    };
    loggers[xev.type](xev);
#endif
#undef msg
}

static void endMsg(const char *msg) {
    if (*msg == 0 || msg[strlen(msg)-1] != '\n') {
        fputc('\n', stderr);
    }
    fflush(stderr);
}

void die(int exitcode, char const *msg, ...) {
    fprintf(stderr, "%s: ", ApplicationName);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    endMsg(msg);

    exit(exitcode);
}

void precondition(const char *expr, const char *file, int line) {
    fprintf(stderr, "%s: PRECONDITION FAILED at %s:%d: ( %s )\n",
            ApplicationName, file, line, expr);
    fflush(stderr);

    show_backtrace();
#ifdef HAVE_ABORT
    abort();
#else
    *(char *)0 = 0x42;
#endif
}

void warn(char const *msg, ...) {
    fprintf(stderr, "%s: ", ApplicationName);
    fputs(_("Warning: "), stderr);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    endMsg(msg);
}

void fail(char const *msg, ...) {
    int errcode = errno;
    fprintf(stderr, "%s: ", ApplicationName);
    fputs(_("Warning: "), stderr);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errcode));
    fflush(stderr);
}

void msg(char const *msg, ...) {
    fprintf(stderr, "%s: ", ApplicationName);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    endMsg(msg);
}

void tlog(char const *msg, ...) {
    timeval now;
    gettimeofday(&now, NULL);
    struct tm *loc = localtime(&now.tv_sec);

    fprintf(stderr, "%02d:%02d:%02d.%03u: %s: ", loc->tm_hour,
            loc->tm_min, loc->tm_sec,
            (unsigned)(now.tv_usec / 1000),
            ApplicationName);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    endMsg(msg);
}

char *cstrJoin(char const *str, ...) {
    va_list ap;
    char const *s;
    char *res, *p;
    int len = 0;

    if (str == 0)
        return 0;

    va_start(ap, str);
    s = str;
    while (s) {
        len += strlen(s);
        s = va_arg(ap, char *);
    }
    va_end(ap);

    if ((p = res = new char[len + 1]) == 0)
        return 0;

    va_start(ap, str);
    s = str;
    while (s) {
        len = strlen(s);
        memcpy(p, s, len);
        p += len;
        s = va_arg(ap, char *);
    }
    va_end(ap);
    *p = 0;
    return res;
}

#if (__GNUC__ == 3) || defined(__clang__)

extern "C" void __cxa_pure_virtual() {
    warn("BUG: Pure virtual method called. Terminating.");
    abort();
}

/* time to rewrite in C */

#endif

#ifdef NEED_ALLOC_OPERATORS

static void *MALLOC(unsigned int len) {
    if (len == 0) return 0;
    return malloc(len);
}

static void FREE(void *p) {
    if (p) free(p);
}

void *operator new(size_t len) {
    return MALLOC(len);
}

void *operator new[](size_t len) {
    if (len == 0) len = 1;
    return MALLOC(len);
}

void operator delete (void *p) {
    FREE(p);
}

void operator delete[](void *p) {
    FREE(p);
}

#endif

/* Prefer this as a safer alternative over strcpy. Return strlen(from). */
#if !defined(HAVE_STRLCPY) || !HAVE_STRLCPY
size_t strlcpy(char *dest, const char *from, size_t dest_size)
{
    const char *in = from;
    if (dest_size > 0) {
        char *to = dest;
        char *const stop = to + dest_size - 1;
        while (to < stop && *in)
            *to++ = *in++;
        *to = '\0';
    }
    while (*in) ++in;
    return in - from;
}
#endif

/* Prefer this over strcat. Return strlen(dest) + strlen(from). */
#if !defined(HAVE_STRLCAT) || !HAVE_STRLCAT
size_t strlcat(char *dest, const char *from, size_t dest_size)
{
    char *to = dest;
    char *const stop = to + dest_size - 1;
    while (to < stop && *to) ++to;
    return to - dest + strlcpy(to, from, dest_size - (to - dest));
}
#endif

char *newstr(char const *str) {
    return (str != NULL ? newstr(str, strlen(str)) : NULL);
}

char *newstr(char const *str, char const *delim) {
    return (str != NULL ? newstr(str, strcspn(str, delim)) : NULL);
}

char *newstr(char const *str, int len) {
    char *s(NULL);

    if (str != NULL && len >= 0 && (s = new char[len + 1]) != NULL) {
        memcpy(s, str, len);
        s[len] = '\0';
    }

    return s;
}

char* demangle(const char* str) {
#ifdef HAVE_GCC_ABI_DEMANGLE
    int status = 0;
    char* c_name = abi::__cxa_demangle(str, 0, 0, &status);
    if (c_name)
        return c_name;
#endif
    return strdup(str);
}

unsigned long strhash(const char* str) {
    unsigned long hash = 5381;
    for (; *str; ++str)
        hash = 33 * hash ^ (unsigned char) *str;
    return hash;
}

/*
 *      Returns zero if s2 is a prefix of s1.
 *
 *      Ie the following will match and return 0 with respective given
 *      arguments:
 *
 *              "--interface=/tmp" "--interface"
 */
int strpcmp(char const * str, char const * pfx, char const * delim) {
    if (str == NULL || pfx == NULL) return -1;
    while (*pfx == *str && *pfx != '\0') ++str, ++pfx;

    return (*pfx == '\0' && strchr(delim, *str) ? 0 : *str - *pfx);
}

char const * strnxt(const char * str, const char * delim) {
    str+= strcspn(str, delim);
    str+= strspn(str, delim);
    return str;
}


bool GetShortArgument(char* &ret, const char *name, char** &argpp, char **endpp)
{
        unsigned int alen=strlen(name);
        if (**argpp != '-' || strncmp((*argpp)+1, name, alen))
                return false;
        if (*((*argpp)+1+alen))
        {
                ret=(*argpp)+1+alen;
                return true;
        }
        else if (argpp+1>=endpp)
                return false;
        ++argpp;
        ret=*argpp;
        return true;
}

bool GetLongArgument(char* &ret, const char *name, char** &argpp, char **endpp)
{
        unsigned int alen=strlen(name);
        if (strncmp(*argpp, "--", 2) || strncmp((*argpp)+2, name, alen))
                return false;
        if (*((*argpp)+2+alen) == '=')
        {
                ret=(*argpp)+3+alen;
                return true;
        }
        if (argpp+1>=endpp)
                return false;
        ++argpp;
        ret = *argpp;
        return true;
}

bool GetArgument(char* &ret, const char *sn, const char *ln, char** &arg, char **end)
{
    bool got = false;
    if (**arg == '-') {
        if (arg[0][1] == '-') {
            got = GetLongArgument(ret, ln, arg, end);
        } else {
            got = GetShortArgument(ret, sn, arg, end);
        }
    }
    return got;
}

bool is_short_switch(const char *arg, const char *name)
{
    return arg && *arg == '-' && 0 == strcmp(arg + 1, name);
}

bool is_long_switch(const char *arg, const char *name)
{
    return arg && *arg == '-' && arg[1] == '-' && 0 == strcmp(arg + 2, name);
}

bool is_switch(const char *arg, const char *short_name, const char *long_name)
{
    return is_short_switch(arg, short_name) || is_long_switch(arg, long_name);
}

bool is_copying_switch(const char *arg)
{
    return is_switch(arg, "C", "copying");
}

bool is_help_switch(const char *arg)
{
    return is_switch(arg, "h", "help") || is_switch(arg, "?", "?");
}

bool is_version_switch(const char *arg)
{
    return is_switch(arg, "V", "version");
}

void print_help_exit(const char *help)
{
    printf(_("Usage: %s [OPTIONS]\n"
             "Options:\n"
             "%s"
             "\n"
             "  -C, --copying       Prints license information and exits.\n"
             "  -V, --version       Prints version information and exits.\n"
             "  -h, --help          Prints this usage screen and exits.\n"
             "\n"),
            ApplicationName, help);
    exit(0);
}

void print_version_exit(const char *version)
{
    printf("%s %s, %s.\n", ApplicationName, version,
        "Copyright 1997-2003 Marko Macek, 2001 Mathias Hasselmann");
    exit(0);
}

void print_copying_exit()
{
    printf("%s\n",
    "IceWM is licensed under the GNU Library General Public License.\n"
    "See the file COPYING in the distribution for full details.\n"
    );
    exit(0);
}

void check_help_version(const char *arg, const char *help, const char *version)
{
    if (is_help_switch(arg)) {
        print_help_exit(help);
    }
    if (is_version_switch(arg)) {
        print_version_exit(version);
    }
    if (is_copying_switch(arg)) {
        print_copying_exit();
    }
}

void check_argv(int argc, char **argv, const char *help, const char *version)
{
    if (ApplicationName == NULL) {
        ApplicationName = my_basename(argv[0]);
    }
    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        check_help_version(*arg, (help && *help) ? help :
                "  -d, --display=NAME    NAME of the X server to use.\n",
                version);

        char *value(0);
        if (GetArgument(value, "d", "display", arg, argv + argc)) {
            setenv("DISPLAY", value, 1);
        }
    }
}

#if 0

/*
 *      Counts the tokens separated by delim
 */
unsigned strtoken(const char * str, const char * delim) {
    unsigned count = 0;

    if (str) {
        while (*str) {
            str = strnxt(str, delim);
            ++count;
        }
    }

    return count;
}
#endif

#if 1
const char *my_basename(const char *path) {
    const char *base = ::strrchr(path, DIR_DELIMINATOR);
    return (base ? base + 1 : path);
}
#else
const char *my_basename(const char *path) {
    return basename(path);
}
#endif

#if 0
bool strequal(const char *a, const char *b) {
    return a ? b && !strcmp(a, b) : !b;
}
#endif

#if 0
int strnullcmp(const char *a, const char *b) {
    return a ? (b ? strcmp(a, b) : 1) : (b ? -1 : 0);
}
#endif

bool testOnce(const char* file, const int line) {
    static YArray<unsigned long> list;
    unsigned long hash = strhash(file) * 0x10001 ^ line;
    return find(list, hash) < 0 ? list.append(hash), true : false;
}

void show_backtrace(const int limit) {
#if defined(HAVE_BACKTRACE_SYMBOLS_FD) && defined(HAVE_EXECINFO_H)
    const int asize = Elvis(limit, 20);
    void *array[asize];
    const int count = backtrace(array, asize);
    const char tool[] = "/usr/bin/addr2line";
    const char* path = program_invocation_name;

    fprintf(stderr, "backtrace:\n"); fflush(stderr);

    int status(1);
    if (path && access(path, R_OK) == 0 && access(tool, X_OK) == 0) {
        const size_t bufsize(1234);
        char buf[bufsize];
        snprintf(buf, bufsize, "%s -C -f -p -s -e '%s'", tool, path);
        size_t len = strlen(buf);
        for (int i = 0; i < count && len + 21 < bufsize; ++i) {
            snprintf(buf + len, bufsize - len, " %p", array[i]);
            len += strlen(buf + len);
        }
        status = system(buf);
    }
    if (status) {
        backtrace_symbols_fd(array, count, 2);
    }
    fprintf(stderr, "end\n");
#endif
}

/* read from file descriptor and zero terminate buffer. */
int read_fd(int fd, char *buf, size_t buflen) {
    if (fd >= 0 && buf && buflen) {
        char *ptr = buf;
        ssize_t got = 0, len = (ssize_t)(buflen - 1);
        while (len > 0) {
            if ((got = read(fd, ptr, (size_t) len)) > 0) {
                ptr += got;
                len -= got;
            } else if (got != -1 || errno != EINTR)
                break;
        }
        *ptr = 0;
        return (ptr > buf) ? (int)(ptr - buf) : (int) got;
    }
    return -1;
}

/* read from filename and zero terminate the buffer. */
int read_file(const char *filename, char *buf, size_t buflen) {
    int len = -1, fd = open(filename, O_RDONLY | O_TEXT);
    if (fd >= 0) {
        len = read_fd(fd, buf, buflen);
        close(fd);
    }
    return len;
}

/* read all of filedescriptor and return a zero-terminated new[] string. */
char* load_fd(int fd) {
    struct stat st;
    if (fstat(fd, &st) == 0) {
        if (S_ISREG(st.st_mode) && st.st_size > 0) {
            char* buf = new char[st.st_size + 1];
            if (buf) {
                int len = read_fd(fd, buf, st.st_size + 1);
                if (len == st.st_size) {
                    return buf;
                }
                delete[] buf;
            }
        } else {
            size_t offset = 0;
            size_t bufsiz = 1024;
            char* buf = new char[bufsiz + 1];
            while (buf) {
                int len = read_fd(fd, buf + offset, bufsiz + 1 - offset);
                if (len <= 0 || offset + len < bufsiz) {
                    if (len < 0 && offset == 0) {
                        delete[] buf;
                        buf = 0;
                    }
                    break;
                }
                else {
                    size_t tmpsiz = 2 * bufsiz;
                    char* tmp = new char[tmpsiz + 1];
                    if (tmp) {
                        memcpy(tmp, buf, bufsiz + 1);
                        offset = bufsiz;
                        bufsiz = tmpsiz;
                    }
                    delete[] buf;
                    buf = tmp;
                }
            }
            return buf;
        }
    }
    return 0;
}

/* read a file as a zero-terminated new[] string. */
char* load_text_file(const char *filename) {
    char* buf = 0;
    int fd = open(filename, O_RDONLY | O_TEXT);
    if (fd >= 0) {
        buf = load_fd(fd);
        close(fd);
    }
    return buf;
}


// vim: set sw=4 ts=4 et:
