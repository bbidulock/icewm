/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "sysdep.h"
#include "ylib.h"

void *MALLOC(unsigned int len) {
    if (len == 0)
        return NULL;
    return malloc(len);
}

void *REALLOC(void *p, unsigned int new_len) {
    if (p) {
        if (new_len > 0)
            return realloc(p, new_len);
        else {
            FREE(p);
            return NULL;
        }
    } else {
        return MALLOC(new_len);
    }
}

void FREE(void *p) {
    if (p)
        free(p);
}

void *operator new(size_t len) {
    return MALLOC(len);
}

void operator delete (void *p) {
    FREE(p);
}

void *operator new[](size_t len) {
    if (len == 0)
        len = 1;
    return MALLOC(len);
}

void operator delete[](void *p) {
    FREE(p);
}

#ifdef GCC_NO_CPP_RUNTIME
// needed for gcc 3.0 to avoid linking with libstdc++, etc...

// hopefully someday this will not be needed
extern "C" void __cxa_pure_virtual() {
    abort();
}
#endif


#ifdef DEBUG
bool debug = false;
bool debug_z = false;

void msg(const char *msg, ...) {
    va_list ap;

    fprintf(stderr, "icewm: ");
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

void logEvent(XEvent xev) {
    switch (xev.type) {
#if 0
    case CreateNotify:
        msg("window=0x%lX: create parent=0x%lX, (%d:%d-%dx%d) border_width=%d, override_redirect=%s",
            xev.xcreatewindow.window,
            xev.xcreatewindow.parent,
            xev.xcreatewindow.x, xev.xcreatewindow.y,
            xev.xcreatewindow.width, xev.xcreatewindow.height,
            xev.xcreatewindow.border_width,
            xev.xcreatewindow.override_redirect ? "True" : "False");
        break;

    case DestroyNotify:
        msg("window=0x%lX: destroy event=0x%lX",
            xev.xdestroywindow.window,
            xev.xdestroywindow.event);
        break;
#else
    case CreateNotify:
    case DestroyNotify:
        break;
#endif
#if 0
    case MapRequest:
        msg("window=0x%lX: mapRequest parent=0x%lX",
            xev.xmaprequest.window,
            xev.xmaprequest.parent);
        break;
#else
    case MapRequest:
        break;
#endif
#if 0
    case MapNotify:
        msg("window=0x%lX: mapNotify event=0x%lX, override_redirect=%s",
            xev.xmap.window,
            xev.xmap.event,
            xev.xmap.override_redirect ? "True" : "False");
        break;

    case UnmapNotify:
        msg("window=0x%lX: unmapNotify event=0x%lX, from_configure=%s send_event=%s",
            xev.xunmap.window,
            xev.xunmap.event,
            xev.xunmap.from_configure ? "True" : "False",
            xev.xunmap.from_configure ? "True" : "False");
        break;
#else
    case MapNotify:
    case UnmapNotify:
        break;
#endif
#if 0
    case ConfigureRequest:
        msg("window=0x%lX: %s configureRequest parent=0x%lX, (%d:%d-%dx%d) border_width=%d, above=0x%lX, detail=%d, value_mask=0x%lX",
            xev.xconfigurerequest.window,
            xev.xconfigurerequest.send_event ? "synth" : "real",
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
#if 0
    case ConfigureNotify:
        msg("window=0x%lX: configureNotify event=0x%lX, (%d:%d-%dx%d) border_width=%d, above=0x%lX, override_redirect=%s",
            xev.xconfigure.window,
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

void die(int exitcode, const char *msg, ...) {
    va_list ap;

    fprintf(stderr, "icewm: ");
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(exitcode);
}

void warn(const char *msg, ...) {
    va_list ap;

    fprintf(stderr, "icewm: ");
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}


#if 1
char *strJoin(const char *str, ...) {
    va_list ap;
    const char *s;
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
#endif

#if 1
char *newstr(const char *str) {
    if (!str)
        return 0;
    int len = strlen(str) + 1;
    char *s = new char[len];
    if (s)
        memcpy(s, str, len);
    return s;
}

char *newstr(const char *str, int len) {
    if (!str)
        return 0;
    char *s = new char[len + 1];
    if (s) {
        memcpy(s, str, len);
        s[len] = 0;
    }
    return s;
}

static int is_reg(const char *path) {
    struct stat sb;

    if (stat(path, &sb) != 0)
        return 0;
    if (!S_ISREG(sb.st_mode))
        return 0;
    return 1;
}
#endif

#if 1
int findPath(const char *path, int mode, const char *name, char **fullname, bool /*path_relative*/) {
#ifdef __EMX__
    char tmp[1024];
    strcpy(tmp, name);
    if (mode & X_OK)
        strcat(tmp, ".exe");
    name = tmp;
#endif

    //remove!!!
    //fprintf(stderr, "path=%s file=%s\n", path, name);
/*
    if (!path_relative && (strchr(name, '/') != 0
#ifdef __EMX__
        || strchr(name, '\\') != 0
#endif
        ) ||
        (path_relative && name[0] == '/'
#ifdef __EMX__
         || // check for root
#endif
))
*/
    if (name[0] == '/') { // check for root in XFreeOS/2
#ifdef __EMX__
        if (access(name, 0) == 0) {
            *fullname = newstr(name);
            return 1;
        }
#else
        if (access(name, mode) == 0 && is_reg(name)) {
            *fullname = newstr(name);
            return 1;
        }
#endif
    } else {
        if (path == 0)
            return 0;

        char prog[1024];
        const char *p, *q;
        unsigned int len, nameLen = strlen(name);

        if (nameLen > sizeof(prog))
            return 0;

        p = path;
        while (*p) {
            q = p;
            while (*p && *p != PATHSEP)
                p++;

            len = p - q;

            if (len > 0 && len < sizeof(prog) - nameLen - 2) {
                strncpy(prog, q, len);
                if (!ISSLASH(prog[len - 1]))
                    prog[len++] = SLASH;
                strcpy(prog + len, name);

#ifdef __EMX__
                if (access(prog, 0) == 0) {
                    *fullname = newstr(prog);
                    return 1;
                }
#else
                if (access(prog, mode) == 0 && is_reg(prog)) {
                    *fullname = newstr(prog);
                    return 1;
                }
#endif
            }
            if (*p == PATHSEP)
                p++;
        }
    }
    return 0;
}
#endif

