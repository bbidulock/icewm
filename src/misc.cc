/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "base.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>

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

#include "intl.h"
#include "ascii.h"

using namespace ASCII;

#ifdef DEBUG
bool debug = false;
bool debug_z = false;
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
    abort();
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
    gettimeofday(&now, nullptr);
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

    if (str == nullptr)
        return nullptr;

    va_start(ap, str);
    s = str;
    while (s) {
        len += strlen(s);
        s = va_arg(ap, char *);
    }
    va_end(ap);

    if ((p = res = new char[len + 1]) == nullptr)
        return nullptr;

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
    return (str != nullptr ? newstr(str, strlen(str)) : nullptr);
}

char *newstr(char const *str, char const *delim) {
    return (str != nullptr ? newstr(str, strcspn(str, delim)) : nullptr);
}

char *newstr(char const *str, int len) {
    char *s(nullptr);

    if (str != nullptr && len >= 0 && (s = new char[len + 1]) != nullptr) {
        memcpy(s, str, len);
        s[len] = '\0';
    }

    return s;
}

char* demangle(const char* str) {
#ifdef HAVE_GCC_ABI_DEMANGLE
    int status = 0;
    char* c_name = abi::__cxa_demangle(str, nullptr, nullptr, &status);
    if (c_name)
        return c_name;
#endif
    return strdup(str);
}

bool little() {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return true;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return false;
#else
#error undefined byte order
#endif
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
    if (str == nullptr || pfx == nullptr) return -1;
    while (*pfx == *str && *pfx != '\0') ++str, ++pfx;

    return (*pfx == '\0' && strchr(delim, *str) ? 0 : *str - *pfx);
}

char const * strnxt(const char * str, const char * delim) {
    str+= strcspn(str, delim);
    str+= strspn(str, delim);
    return str;
}

#ifndef HAVE_MEMRCHR
void* memrchr(const void* ptr, char chr, size_t num) {
    char* str = (char *) ptr;
    char* q = str + num;
    while (q > str && *--q != chr);
    return q >= str && *q == chr ? q : nullptr;
}
#endif

bool GetShortArgument(char* &ret, const char *name, char** &argpp, char **endpp)
{
    unsigned int alen = strlen(name);
    if (**argpp != '-' || strncmp((*argpp) + 1, name, alen))
        return false;
    char ch = (*argpp)[1 + alen];
    if (ch) {
        ret = (*argpp) + 1 + alen + (ch == '=');
        return true;
    }
    else if (argpp + 1 >= endpp)
        return false;
    ++argpp;
    ret = *argpp;
    return true;
}

bool GetLongArgument(char* &ret, const char *name, char** &argpp, char **endpp)
{
    unsigned int alen = strlen(name);
    if (strncmp(*argpp, "--", 2) || strncmp((*argpp) + 2, name, alen))
        return false;
    char ch = (*argpp)[2 + alen];
    if (ch == '=') {
        ret = (*argpp) + 3 + alen;
        return true;
    }
    if (argpp + 1 >= endpp)
        return false;
    ++argpp;
    ret = *argpp;
    return true;
}

bool GetArgument(char* &ret, const char *sn, const char *ln, char** &arg, char **end)
{
    bool got = false;
    if (arg && *arg && **arg == '-') {
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
        "Copyright 1997-2012 Marko Macek, 2001 Mathias Hasselmann");
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
    if (ApplicationName == nullptr) {
        ApplicationName = my_basename(argv[0]);
    }
    for (char **arg = argv + 1; arg < argv + argc; ++arg) {
        if ('-' == arg[0][0]) {
            char c = ('-' == arg[0][1]) ? arg[0][2] : arg[0][1];
            if (c == '\0') {
                if ('-' == arg[0][1]) {
                    break;
                }
            }
            else if (strchr("h?vVcC", c)) {
                check_help_version(*arg, nonempty(help) ? help :
                    "  -d, --display=NAME    NAME of the X server to use.\n",
                    version);
            }
            else if (c == 'd') {
                char* value(nullptr);
                if (GetArgument(value, "d", "display", arg, argv + argc)) {
                    setenv("DISPLAY", value, 1);
                }
            }
        }
    }
}

#if 1
const char *my_basename(const char *path) {
    const char *base = ::strrchr(path, '/');
    return (base ? base + 1 : path);
}
#else
const char *my_basename(const char *path) {
    return basename(path);
}
#endif

bool isFile(const char* path) {
    struct stat s;
    return stat(path, &s) == 0 && S_ISREG(s.st_mode);
}

bool isExeFile(const char* path) {
    return access(path, X_OK) == 0 && isFile(path);
}

// parse a C-style identifier
char* identifier(const char* text) {
    const char* scan = text;
    if (*scan == '_' || isAlpha(*scan)) {
        while (*++scan == '_' || isAlnum(*scan)) { }
    }
    return text < scan ? newstr(text, int(scan - text)) : nullptr;
}

// free a string when out-of-scope
class strp {
public:
    strp(char* string = nullptr) : ptr(string) { }
    ~strp() { delete[] ptr; }
    operator char*() { return ptr; }
    void operator=(char* p) { ptr = p; }
private:
    char* ptr;
};

// obtain the home directory for a named user
char* userhome(const char* username) {
    const size_t buflen = 1234;
    char buf[buflen];
    passwd pwd, *result = nullptr;
    int get;
    if (nonempty(username)) {
        get = getpwnam_r(username, &pwd, buf, buflen, &result);
    } else {
        get = getpwuid_r(getuid(), &pwd, buf, buflen, &result);
    }
    if (get == 0 && result) {
        return newstr(result->pw_dir);
    } else {
        return nullptr;
    }
}

// expand a leading environment variable
char* dollar_expansion(const char* name) {
    if (name[0] == '$') {
        bool leftb = (name[1] == '{');
        strp ident = identifier(name + 1 + leftb);
        if (ident) {
            size_t idlen = strlen(ident);
            bool right = (name[1 + leftb + idlen] == '}');
            if (leftb == right) {
                char* expand = getenv(ident);
                if (expand) {
                    size_t xlen = strlen(expand);
                    size_t size = xlen + strlen(name + idlen);
                    char* path = new char[size];
                    if (path) {
                        strlcpy(path, expand, size);
                        strlcat(path, name + 1 + leftb + idlen + right, size);
                        return path;
                    }
                }
            }
        }
    }
    return nullptr;
}

// expand a leading tilde+slash or tilde+username
char* tilde_expansion(const char* name) {
    if (name[0] != '~') {
        return nullptr;
    }
    else if (name[1] == '/' || name[1] == '\0') {
        char* home = getenv("HOME");
        strp user;
        if (isEmpty(home)) {
            user = userhome(nullptr);
            home = user;
        }
        if (nonempty(home)) {
            size_t size = strlen(home) + strlen(name);
            char* path = new char[size];
            if (path) {
                strlcpy(path, home, size);
                strlcat(path, name + 1, size);
                return path;
            }
        }
    }
    else {
        strp ident = identifier(name + 1);
        if (ident) {
            size_t idlen = strlen(ident);
            if (name[1 + idlen] == '/' || name[1 + idlen] == '\0') {
                strp home = userhome(ident);
                if (home) {
                    size_t hlen = strlen(home);
                    size_t size = hlen + strlen(name + idlen);
                    char* path = new char[size];
                    if (path) {
                        strlcpy(path, home, size);
                        strlcat(path, name + 1 + idlen, size);
                        return path;
                    }
                }
            }
        }
    }
    return nullptr;
}

// lookup "name" in PATH and return a new string or 0.
char* path_lookup(const char* name) {
    if (isEmpty(name))
        return nullptr;

    if (name[0] == '~') {
        char* expand = tilde_expansion(name);
        if (expand) {
            if (isExeFile(expand)) {
                return expand;
            } else {
                delete[] expand;
            }
        }
        return nullptr;
    }

    if (name[0] == '$') {
        char* expand = dollar_expansion(name);
        if (expand) {
            if (isExeFile(expand)) {
                return expand;
            } else {
                delete[] expand;
            }
        }
        return nullptr;
    }

    if (strchr(name, '/')) {
        return isExeFile(name) ? newstr(name) : nullptr;
    }

    strp env = newstr(getenv("PATH"));
    if (env == nullptr)
        return nullptr;

    const size_t namlen = strlen(name);
    char filebuf[PATH_MAX];

    char *directory, *save = nullptr;
    for (directory = strtok_r(env, ":", &save); directory;
         directory = strtok_r(nullptr, ":", &save))
    {
        size_t dirlen = strlen(directory);
        size_t length = dirlen + namlen + 3;
        if (length < sizeof filebuf) {
            snprintf(filebuf, sizeof filebuf, "%s/%s",
                     dirlen ? directory : ".", name);
            if (isExeFile(filebuf)) {
                return newstr(filebuf);
            }
        }
    }
    return nullptr;
}

#ifdef __gnu_hurd__
const char* getprogname() {
    return ApplicationName;
}
#endif

// get path of executable.
char* progpath() {
#ifdef __linux__
    char* path = program_invocation_name;
    bool fail = isEmpty(path) || access(path, R_OK | X_OK) != 0;
    if (fail) {
        const size_t linksize = 123;
        char link[linksize];
        const size_t procsize = 42;
        char proc[procsize];
        snprintf(proc, procsize, "/proc/%d/exe", int(getpid()));
        ssize_t read = readlink(proc, link, linksize);
        if (inrange<ssize_t>(read, 1, ssize_t(linksize - 1))) {
            link[read] = 0;
            char* annotation = strstr(link, " (deleted)");
            if (annotation && annotation > link) {
                annotation[0] = 0;
            }
            if ((fail = access(link, R_OK | X_OK)) == 0) {
                path = program_invocation_name = newstr(link);
                INFO("1: set program_invocation_name %s", path);
            }
        }
    }
    if (fail && (path = path_lookup(path)) != nullptr) {
        program_invocation_name = path;
        INFO("2: set program_invocation_name %s", path);
    }
#else
    static char* path;
    if (path == 0)
        path = path_lookup(getprogname());
#endif
    return path;
}

void show_backtrace(const int limit) {
#if defined(HAVE_BACKTRACE_SYMBOLS_FD) && defined(HAVE_EXECINFO_H)
    const int asize = Elvis(limit, 20);
    void *array[asize];
    const int count = backtrace(array, asize);
    const char tool[] = "/usr/bin/addr2line";
    char* prog = progpath();
    char* path = prog ? prog : path_lookup("icewm");

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
        FILE* fp = popen(buf, "r");
        if (fp) {
            const int linesize(256);
            char line[linesize];
            int lineCount = 0;
            while (fgets(line, linesize, fp)) {
                ++lineCount;
                if (strncmp(line, "?? ??:0", 7)) {
                    fputs(line, stderr);
                }
            }
            if (pclose(fp) == 0 && lineCount >= count) {
                status = 0;
            }
        }
        else
            status = system(buf);
    }
    if (status) {
        backtrace_symbols_fd(array, count, 2);
    }
    fprintf(stderr, "end\n");
#endif
}

// vim: set sw=4 ts=4 et:
