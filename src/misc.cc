/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "sysdep.h"
#include "ylib.h"
#include "debug.h"

#include "intl.h"
#include "ref.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

extern char const *ApplicationName;

#ifdef DEBUG
bool debug = false;
bool debug_z = false;

void logEvent(const XEvent &xev) {
    switch (xev.type) {
#if 1
    case CreateNotify:
        msg("window=0x%lX: create serial=%10d parent=0x%lX, (%d:%d-%dx%d) border_width=%d, override_redirect=%s",
            xev.xcreatewindow.window,
            xev.xany.serial,
            xev.xcreatewindow.parent,
            xev.xcreatewindow.x, xev.xcreatewindow.y,
            xev.xcreatewindow.width, xev.xcreatewindow.height,
            xev.xcreatewindow.border_width,
            xev.xcreatewindow.override_redirect ? "True" : "False");
        break;

    case DestroyNotify:
        msg("window=0x%lX: destroy serial=%10d event=0x%lX",
            xev.xdestroywindow.window,
            xev.xany.serial,
            xev.xdestroywindow.event);
        break;
#else
    case CreateNotify:
    case DestroyNotify:
        break;
#endif
#if 1
    case MapRequest:
        msg("window=0x%lX: mapRequest serial=%10d parent=0x%lX",
            xev.xmaprequest.window,
            xev.xany.serial,
            xev.xmaprequest.parent);
        break;
#else
    case MapRequest:
        break;
#endif
#if 1
    case MapNotify:
        msg("window=0x%lX: mapNotify serial=%10d event=0x%lX, override_redirect=%s",
            xev.xmap.window,
            xev.xany.serial,
            xev.xmap.event,
            xev.xmap.override_redirect ? "True" : "False");
        break;

    case UnmapNotify:
        msg("window=0x%lX: unmapNotify serial=%10d event=0x%lX, from_configure=%s send_event=%s",
            xev.xunmap.window,
            xev.xany.serial,
            xev.xunmap.event,
            xev.xunmap.from_configure ? "True" : "False",
            xev.xunmap.send_event ? "True" : "False");
        break;
#else
    case MapNotify:
    case UnmapNotify:
        break;
#endif
#if 1
    case ConfigureRequest:
        msg("window=0x%lX: %s configureRequest serial=%10d parent=0x%lX, (%d:%d-%dx%d) border_width=%d, above=0x%lX, detail=%d, value_mask=0x%lX",
            xev.xconfigurerequest.window,
            xev.xconfigurerequest.send_event ? "synth" : "real",
            xev.xany.serial,
            xev.xconfigurerequest.parent,
            xev.xconfigurerequest.x, xev.xconfigurerequest.y,
            xev.xconfigurerequest.width, xev.xconfigurerequest.height,
            xev.xconfigurerequest.border_width,
            xev.xconfigurerequest.above,
            xev.xconfigurerequest.detail,
            xev.xconfigurerequest.value_mask);
        break;
#else
    case ConfigureRequest:
        break;
#endif

#if 0
    case FocusIn:
    case FocusOut:
        msg("window=0x%lX: %s mode=%d, detail=%d",
            xev.xfocus.window,
            (xev.type == FocusIn) ? "focusIn" : "focusOut",
            xev.xfocus.mode,
            xev.xfocus.detail);
        break;
#else
    case FocusIn:
    case FocusOut:
        break;
#endif

#if 0
    case ColormapNotify:
        msg("window=0x%lX: colormapNotify colormap=%ld new=%s state=%d",
            xev.xcolormap.window,
            xev.xcolormap.colormap,
            xev.xcolormap.c_new ? "True" : "False",
            xev.xcolormap.state);
        break;
#else
    case ColormapNotify:
        break;
#endif

#if 1
    case ReparentNotify:
        msg("window=0x%lX: reparentNotify serial=%10d event=0x%lX, parent=0x%lX, (%d:%d), override_redirect=%s",
            xev.xreparent.window,
            xev.xany.serial,
            xev.xreparent.event,
            xev.xreparent.parent,
            xev.xreparent.x, xev.xreparent.y,
            xev.xreparent.override_redirect ? "True" : "False");
        break;
#else
    case ReparentNotify:
        break;
#endif

#if 1
    case ConfigureNotify:
        msg("window=0x%lX: configureNotify serial=%10d event=0x%lX, (%d:%d-%dx%d) border_width=%d, above=0x%lX, override_redirect=%s",
            xev.xconfigure.window,
            xev.xany.serial,
            xev.xconfigure.event,
            xev.xconfigure.x, xev.xconfigure.y,
            xev.xconfigure.width, xev.xconfigure.height,
            xev.xconfigure.border_width,
            xev.xconfigure.above,
            xev.xconfigure.override_redirect ? "True" : "False");
        break;
#else
    case ConfigureNotify:
        break;
#endif

#if 0
    case VisibilityNotify:
        msg("window=0x%lX: visibilityNotify state=%d",
            xev.xvisibility.window,
            xev.xvisibility.state);
        break;
#else
    case VisibilityNotify:
        break;
#endif
#if 1
    case ClientMessage:
        msg("window=0x%lX: clientMessage message_type=0x%lX format=%d",
            xev.xclient.window,
            xev.xclient.message_type,
            xev.xclient.format);
        break;
#else
    case ClientMessage:
        break;
#endif
#if 0
    case PropertyNotify:
        msg("window=0x%lX: propertyNotify atom=0x%lX time=%ld state=%d",
            xev.xproperty.window,
            xev.xproperty.atom,
            xev.xproperty.time,
            xev.xproperty.state);
        break;
#else
    case PropertyNotify:
        break;
#endif
#if 0
    case ButtonPress:
    case ButtonRelease:
        msg("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, (%d:%d %d:%d) state=0x%X detail=0x%X same_screen=%s",
            xev.xbutton.window,
            (xev.type == ButtonPress) ? "buttonPress" : "buttonRelease",
            xev.xbutton.root,
            xev.xbutton.subwindow,
            xev.xbutton.time,
            xev.xbutton.x, xev.xbutton.y,
            xev.xbutton.x_root, xev.xbutton.y_root,
            xev.xbutton.state,
            xev.xbutton.button,
            xev.xbutton.same_screen ? "True" : "False");
        break;
#else
    case ButtonPress:
    case ButtonRelease:
#endif
#if 0
    case MotionNotify:
        msg("window=0x%lX: motionNotify root=0x%lX, subwindow=0x%lX, time=%ld, (%d:%d %d:%d) state=0x%X is_hint=%c same_screen=%s",
            xev.xmotion.window,
            xev.xmotion.root,
            xev.xmotion.subwindow,
            xev.xmotion.time,
            xev.xmotion.x, xev.xmotion.y,
            xev.xmotion.x_root, xev.xmotion.y_root,
            xev.xmotion.state,
            xev.xmotion.is_hint,
            xev.xmotion.same_screen ? "True" : "False");
        break;
#else
    case MotionNotify:
        break;
#endif
#if 0
    case EnterNotify:
    case LeaveNotify:
        msg("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, (%d:%d %d:%d) mode=%d detail=%d same_screen=%s, focus=%s state=0x%X",
            xev.xcrossing.window,
            (xev.type == EnterNotify) ? "enterNotify" : "leaveNotify",
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
        break;
#else
    case EnterNotify:
    case LeaveNotify:
        break;
#endif
#if 0
    case KeyPress:
    case KeyRelease:
        msg("window=0x%lX: %s root=0x%lX, subwindow=0x%lX, time=%ld, (%d:%d %d:%d) state=0x%X keycode=0x%x same_screen=%s",
            xev.xkey.window,
            (xev.type == KeyPress) ? "keyPress" : "keyRelease",
            xev.xkey.root,
            xev.xkey.subwindow,
            xev.xkey.time,
            xev.xkey.x, xev.xkey.y,
            xev.xkey.x_root, xev.xkey.y_root,
            xev.xkey.state,
            xev.xkey.keycode,
            xev.xkey.same_screen ? "True" : "False");
        break;

    case Expose:
        msg("window=0x%lX: expose (%d:%d-%dx%d) count=%d",
            xev.xexpose.window,
            xev.xexpose.x, xev.xexpose.y, xev.xexpose.width, xev.xexpose.height,
            xev.xexpose.count);
        break;
#else
    case KeyPress:
    case KeyRelease:
    case Expose:
        break;
#endif
#if 0
    default:
        msg("window=0x%lX: unknown type=%d", xev.xany.window, xev.type);
        break;
#endif
    }
}
#endif

void die(int exitcode, char const *msg, ...) {
    fprintf(stderr, "%s: ", ApplicationName);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fputs("\n", stderr);

    exit(exitcode);
}

void precondition(char const *msg, ...) {
    fprintf(stderr, "%s: ", ApplicationName);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fputs("\n", stderr);

    *(char *)0 = 0x42;
}

void warn(char const *msg, ...) {
    fprintf(stderr, "%s: ", ApplicationName);
    fputs(_("Warning: "), stderr);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    fputs("\n", stderr);
}

void msg(char const *msg, ...) {
    fprintf(stderr, "%s: ", ApplicationName);

    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fputs("\n", stderr);
}

char *strJoin(char const *str, ...) {
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

#if __GNUC__ == 3

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

char *newstr(char const *str) {
    return (str != NULL ? newstr(str, strlen(str)) : NULL);
}

char *newstr(char const *str, char const *delim) {
    return (str != NULL ? newstr(str, strcspn(str, delim)) : NULL);
}

char *newstr(char const *str, int len) {
    char *s(NULL);

    if (str != NULL && (s = new char[len + 1]) != NULL) {
        memcpy(s, str, len);
        s[len] = '\0';
    }

    return s;
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
    if(str == NULL || pfx == NULL) return -1;
    while(*pfx == *str && *pfx != '\0') ++str, ++pfx;

    return (*pfx == '\0' && strchr(delim, *str) ? 0 : *str - *pfx);
}

char const * strnxt(const char * str, const char * delim) {
    str+= strcspn(str, delim);
    str+= strspn(str, delim);
    return str;

}

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

bool strequal(const char *a, const char *b) {
    return a ? b && !strcmp(a, b) : !b;
}

int strnullcmp(const char *a, const char *b) {
    return a ? (b ? strcmp(a, b) : 1) : (b ? -1 : 0);
}

bool isreg(char const *path) {
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISREG(sb.st_mode));
}
