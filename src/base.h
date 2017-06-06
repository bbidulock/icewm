#ifndef __BASE_H
#define __BASE_H

#include <stddef.h>

#ifndef __GNUC__
#define __attribute__(a)
#endif

/*** Atomar Data Types ********************************************************/

#ifdef NEED_BOOL
typedef  { false = 0, true = 1 } bool;
#endif

/*** Essential Arithmetic Functions *******************************************/

/*
 * Decimal digits required to write the largest element of type:
 * bits(Type) * (2.5 = 5/2 ~ (ln(2) / ln(10)))
 */
#define DECIMAL_DIGIT_COUNT(Type) ((sizeof(Type) * 5 + 1) / 2)

template <class T>
inline T min(T a, T b) {
    return (a < b ? a : b);
}

template <class T>
inline T max(T a, T b) {
    return (a > b ? a : b);
}

template <class T>
inline T clamp(T value, T minimum, T maximum) {
    return max(min(value, maximum), minimum);
}

template <class T>
inline T abs(T v) {
    return (v < 0 ? -v : v);
}

/*** String Functions *********************************************************/

/* Prefer this as a safer alternative over strcpy. Return strlen(from). */
size_t strlcpy(char *dest, const char *from, size_t dest_size);
/* Prefer this over strcat. Return strlen(dest) + strlen(from). */
size_t strlcat(char *dest, const char *from, size_t dest_size);

char *newstr(char const *str);
char *newstr(char const *str, int len);
char *newstr(char const *str, char const *delim);
char *cstrJoin(char const *str, ...);

#if 0
/*
 * Convert unsigned to string
 */
template <class T>
inline char * utoa(T u, char * s, unsigned const len) {
    if (len > DECIMAL_DIGIT_COUNT(T)) {
        *(s += DECIMAL_DIGIT_COUNT(u) + 1) = '\0';
        do { *--s = '0' + u % 10; } while (u /= 10);
        return s;
    } else
        return 0;
}

template <class T>
static char const * utoa(T u) {
    static char s[DECIMAL_DIGIT_COUNT(int) + 1];
    return utoa(u, s, sizeof(s));
}

/*
 * Convert signed to string
 */
template <class T>
inline char * itoa(T i, char * s, unsigned const len, bool sign = false) {
    if (len > DECIMAL_DIGIT_COUNT(T) + 1) {
        if (i < 0) {
            s = utoa(-i, s, len);
            *--s = '-';
        } else {
            s = utoa(i, s, len);
            if (sign) *--s = '+';
        }

        return s;
    } else
        return 0;
}

template <class T>
static char const * itoa(T i, bool sign = false) {
    static char s[DECIMAL_DIGIT_COUNT(int) + 2];
    return itoa(i, s, sizeof(s), sign);
}
#endif

/*** Message Functions ********************************************************/

void die(int exitcode, char const *msg, ...) __attribute__((format(printf, 2, 3) ));
void warn(char const *msg, ...) __attribute__((format(printf, 1, 2) ));
void fail(char const *msg, ...) __attribute__((format(printf, 1, 2) ));
void msg(char const *msg, ...) __attribute__((format(printf, 1, 2) ));
void tlog(char const *msg, ...) __attribute__((format(printf, 1, 2) ));
void precondition(const char *expr, const char *file, int line);
void show_backtrace();

#define DEPRECATE(x) \
    do { \
    if (x) warn("Deprecated option: " #x); \
    } while (0)

/*** Misc Stuff (clean up!!!) *************************************************/

#define ACOUNT(x) (sizeof(x)/sizeof(x[0]))

#ifndef DIR_DELIMINATOR
#define DIR_DELIMINATOR '/'
#endif

extern "C" {
#ifdef __EMX__
char* __XOS2RedirRoot(char const*);
#define REDIR_ROOT(path) __XOS2RedirRoot(path)
#else
#define REDIR_ROOT(path) (path)
#endif
}

//!!! clean these up
#define KEY_MODMASK(x) ((x) & (xapp->KeyMask))
#define BUTTON_MASK(x) ((x) & (xapp->ButtonMask))
#define BUTTON_MODMASK(x) ((x) & (xapp->ButtonKeyMask))
#define IS_BUTTON(s,b) (BUTTON_MODMASK(s) == (b))

#define ISMASK(w,e,n) (((w) & ~(n)) == (e))
#define HASMASK(w,e,n) ((((w) & ~(n)) & (e)) == (e))

int strpcmp(char const *str, char const *pfx, char const *delim = "=:");
#if 0
unsigned strtoken(const char *str, const char *delim = " \t");
#endif
char const * strnxt(const char *str, const char *delim = " \t");
const char *my_basename(const char *filename);

#if 0
bool strequal(const char *a, const char *b);
int strnullcmp(const char *a, const char *b);
#endif

template <class T>
inline char const * niceUnit(T & val, char const * const units[],
                             T const lim = 10240, T const div = 1024) {
    char const * uname(0);

    if (units && *units) {
        uname = *units++;
        while (val >= lim && *units) {
            uname = *units++;
            /* precise rounding errs */
            val = (val + div / 2) / div;
        }
    }
    
    return uname;
}

/*** Bit Operations ***********************************************************/

/*
 * Returns the lowest bit set in mask.
 */
template <class T> 
inline unsigned lowbit(T mask) {
#if defined(CONFIG_X86_ASM) && defined(__i386__) && \
    defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ > 208)
    unsigned bit;
    asm ("bsf %1,%0" : "=r" (bit) : "r" (mask));
#else
    unsigned bit(0); 
    while(!(mask & (1 << bit)) && bit < sizeof(mask) * 8) ++bit;
#endif

    return bit;
}

/*
 * Returns the highest bit set in mask.
 */
template <class T> 
inline unsigned highbit(T mask) {
#if defined(CONFIG_X86_ASM) && defined(__i386__) && \
    defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ > 208)
    unsigned bit;
    asm ("bsr %1,%0" : "=r" (bit) : "r" (mask));
#else
    unsigned bit(sizeof(mask) * 8 - 1);
    while(!(mask & (1 << bit)) && bit > 0) --bit;
#endif

    return bit;
}

/*** argc/argv processing *****************************************************/

extern char const *ApplicationName;

bool GetShortArgument(char* &ret, const char *name, char** &argpp, char ** endpp);
bool GetLongArgument(char* &ret, const char *name, char** &argpp, char ** endpp);
bool is_short_switch(const char *arg, const char *name);
bool is_long_switch(const char *arg, const char *name);
bool is_switch(const char *arg, const char *short_name, const char *long_name);
bool is_help_switch(const char *arg);
bool is_version_switch(const char *arg);
void print_help_exit(const char *help);
void print_version_exit(const char *version);
void check_help_version(const char *arg, const char *help, const char *version);
void check_argv(int argc, char **argv, const char *help, const char *version);

/*** file handling ************************************************************/

/* read from file descriptor and zero terminate buffer. */
int read_fd(int fd, char *buf, size_t buflen);

/* read from filename and zero terminate the buffer. */
int read_file(const char *filename, char *buf, size_t buflen);

/* read all of filedescriptor and return a zero-terminated new[] string. */
char* load_fd(int fd);

/* read a file as a zero-terminated new[] string. */
char* load_text_file(const char *filename);

/******************************************************************************/

#include "debug.h"

inline int intersection(int s1, int e1, int s2, int e2) {
    int s, e;

    if (s1 > e2)
        return 0;
    if (s2 > e1)
        return 0;

    /* start */
    if (s2 > s1)
        s = s2;
    else
        s = s1;

    /* end */
    if (e1 < e2)
        e = e1;
    else
        e = e2;
    if (e > s)
        return e - s;
    else
        return 0;
}

#endif
