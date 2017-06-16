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
#include <time.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#if defined(__linux__) && defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

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
#if 0
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
#if 1
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
#if 1
    case EnterNotify:
    case LeaveNotify:
        msg("window=0x%lX: %s serial=%10d root=0x%lX, subwindow=0x%lX, time=%ld, (%d:%d %d:%d) mode=%d detail=%d same_screen=%s, focus=%s state=0x%X",
            xev.xcrossing.window,
            (xev.type == EnterNotify) ? "enterNotify" : "leaveNotify",
            xev.xany.serial,
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
#else
    case KeyPress:
    case KeyRelease:
        break;
#endif
#if 0
    case Expose:
        msg("window=0x%lX: expose (%d:%d-%dx%d) count=%d",
            xev.xexpose.window,
            xev.xexpose.x, xev.xexpose.y, xev.xexpose.width, xev.xexpose.height,
            xev.xexpose.count);
        break;
#else
    case Expose:
        break;
#endif
#if 1
    default:
        msg("window=0x%lX: unknown type=%d", xev.xany.window, xev.type);
        break;
#endif
    }
}
#endif

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
    time_t now = time(NULL);
    struct tm *loc = localtime(&now);

    fprintf(stderr, "%02d:%02d:%02d: %s: ", loc->tm_hour,
            loc->tm_min, loc->tm_sec, ApplicationName);

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

/* Prefer this over strcat. Return strlen(dest) + strlen(from). */
size_t strlcat(char *dest, const char *from, size_t dest_size)
{
    char *to = dest;
    char *const stop = to + dest_size - 1;
    while (to < stop && *to) ++to;
    return to - dest + strlcpy(to, from, dest_size - (to - dest));
}

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


bool GetShortArgument(char* &ret, const char *name, char** &argpp, char **endpp)
{
	unsigned int alen=strlen(name);
	if(**argpp != '-' || strncmp((*argpp)+1, name, alen))
		return false;
	if(*((*argpp)+1+alen))
	{
		ret=(*argpp)+1+alen;
		return true;
	}
	else if(argpp+1>=endpp)
		return false;
	++argpp;
	ret=*argpp;
	return true;
}

bool GetLongArgument(char* &ret, const char *name, char** &argpp, char **endpp)
{
	unsigned int alen=strlen(name);
	if(strncmp(*argpp, "--", 2) || strncmp((*argpp)+2, name, alen))
		return false;
	if(*((*argpp)+2+alen) == '=')
	{
		ret=(*argpp)+3+alen;
		return true;
	}
	if(argpp+1>=endpp)
		return false;
	++argpp;
	ret = *argpp;
	return true;
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
             "  -V, --version       Prints version information and exits.\n"
             "  -h, --help          Prints this usage screen and exits.\n"
             "\n"),
            ApplicationName, help);
    exit(1);
}

void print_version_exit(const char *version)
{
    printf("%s %s, %s.\n", ApplicationName, version,
        "Copyright 1997-2003 Marko Macek, 2001 Mathias Hasselmann");
    exit(1);
}

void check_help_version(const char *arg, const char *help, const char *version)
{
    if (is_help_switch(arg)) {
        print_help_exit(help);
    }
    if (is_version_switch(arg)) {
        print_version_exit(version);
    }
}

void check_argv(int argc, char **argv, const char *help, const char *version)
{
    if (ApplicationName == NULL) {
        ApplicationName = my_basename(argv[0]);
    }
    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        check_help_version(*arg, (help && *help) ? help :
                "  --display=NAME      NAME of the X server to use.\n",
                version);

        char *value(0);
        if (GetLongArgument(value, "display", arg, argv + argc)) {
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

void show_backtrace() {
#if defined(__linux__) && defined(HAVE_EXECINFO_H)
    void *array[20];

    fprintf(stderr, "\nbacktrace:\n");
    int size = backtrace(array, ACOUNT(array));
    backtrace_symbols_fd(array, size, 2);
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
        if (S_ISREG(st.st_mode)) {
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
            size_t bufsiz = 4096;
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

