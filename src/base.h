#ifndef BASE_H
#define BASE_H

#include "debug.h"
#include <stddef.h>

#ifndef __GNUC__
#define __attribute__(a)
#endif

/*** Essential Arithmetic Functions *******************************************/

/*
 * Decimal digits required to write the largest element of type:
 * bits(Type) * (2.5 = 5/2 ~ (8 * ln(2) / ln(10)))
 */
#define DECIMAL_DIGIT_COUNT(Type) ((sizeof(Type) * 5 + 1) / 2)

template <class T>
inline T min(T a, T b) {
    return (a < b ? a : b);
}

template <class T>
inline T max(T a, T b) {
    return (a < b ? b : a);
}

template <class T>
inline void swap(T& a, T& b) {
    T t(a); a = b; b = t;
}

template <class T>
inline T clamp(T value, T minimum, T maximum) {
    return max(min(value, maximum), minimum);
}

template <class T>
inline bool inrange(T value, T lower, T upper) {
    return !(value < lower) && !(upper < value);
}

template <class T>
inline T abs(T v) {
    return (v < 0 ? -v : v);
}

// https://en.wikipedia.org/wiki/Elvis_operator
template <class T>
inline T Elvis(T a, T b) {
    return a ? a : b;
}

template <class T>
inline T non_zero(T x) {
    return Elvis(x, static_cast<T>(1));
}

template <class L, class R>
class pair {
public:
    L left;
    R right;
    pair(const L& l, const R& r) : left(l), right(r) { }
};

/*** String Functions *********************************************************/

/* Prefer this as a safer alternative over strcpy. Return strlen(from). */
#if !defined(HAVE_STRLCPY) || !HAVE_STRLCPY
size_t strlcpy(char *dest, const char *from, size_t dest_size);
#endif

/* Prefer this over strcat. Return strlen(dest) + strlen(from). */
#if !defined(HAVE_STRLCAT) || !HAVE_STRLCAT
size_t strlcat(char *dest, const char *from, size_t dest_size);
#endif

char *newstr(char const *str);
char *newstr(char const *str, int len);
char *newstr(char const *str, char const *delim);
char *cstrJoin(char const *str, ...);

char* demangle(const char* str);
unsigned long strhash(const char* str);

inline bool nonempty(const char* s) { return s && *s; }
inline bool isEmpty(const char* s) { return !(s && *s); }

#ifndef HAVE_MEMRCHR
void* memrchr(const void*, char, size_t);
#endif

/*** Message Functions ********************************************************/

void die(int exitcode, char const *msg, ...) __attribute__((format(printf, 2, 3) ));
void warn(char const *msg, ...) __attribute__((format(printf, 1, 2) ));
void fail(char const *msg, ...) __attribute__((format(printf, 1, 2) ));
void msg(char const *msg, ...) __attribute__((format(printf, 1, 2) ));
void tlog(char const *msg, ...) __attribute__((format(printf, 1, 2) ));
void precondition(const char *expr, const char *file, int line);

/*** User Environment ********************************************************/

char* userhome(const char* username);
char* dollar_expansion(const char* name);
char* tilde_expansion(const char* name);
char* path_lookup(const char* name);

char* progpath();
void show_backtrace(const int limit = 0);

#define DEPRECATE(x) \
    do { \
    if (x) warn("Deprecated option: " #x); \
    } while (0)

/*** Misc Stuff (clean up!!!) *************************************************/

#define ACOUNT(x) (sizeof(x)/sizeof(x[0]))
#define BUFNSIZE(x) x, sizeof(x)

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

inline const char* boolstr(bool bval) {
    return bval ? "true" : "false";
}

template <class T>
inline char const * niceUnit(T & val, char const * const units[],
                             T const lim = 10240, T const div = 1024) {
    char const * uname(nullptr);

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

// be true just once: only the first evaluation
#define ONCE    testOnce(__FILE__, __LINE__)

bool testOnce(const char* file, const int line);

bool little();

/*** Bit Operations ***********************************************************/

template <class M, class B>
inline bool hasbit(M mask, B bits) {
    return (mask & static_cast<M>(bits)) != 0;
}

template <class M, class B>
inline bool hasbits(M mask, B bits) {
    return (mask & static_cast<M>(bits)) == static_cast<M>(bits);
}

template <class M, class B>
inline bool notbit(M mask, B bits) {
    return (mask & static_cast<M>(bits)) == 0;
}

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
    while (!(mask & ((static_cast<T>(1)) << bit)) && bit < sizeof(mask) * 8) ++bit;
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
    while (!(mask & ((static_cast<T>(1)) << bit)) && bit > 0) --bit;
#endif

    return bit;
}

/*** argc/argv processing *****************************************************/

extern char const *ApplicationName;

bool GetShortArgument(char* &ret, const char *name, char** &argpp, char ** endpp);
bool GetLongArgument(char* &ret, const char *name, char** &argpp, char ** endpp);
bool GetArgument(char* &ret, const char *sn, const char *ln, char** &arg, char **end);
bool is_short_switch(const char *arg, const char *name);
bool is_long_switch(const char *arg, const char *name);
bool is_switch(const char *arg, const char *short_name, const char *long_name);
bool is_copying_switch(const char *arg);
bool is_help_switch(const char *arg);
bool is_version_switch(const char *arg);
void print_copying_exit();
void print_help_exit(const char *help);
void print_version_exit(const char *version);
void check_help_version(const char *arg, const char *help, const char *version);
void check_argv(int argc, char **argv, const char *help, const char *version);

inline int intersection(int s1, int e1, int s2, int e2) {
    return max(0, min(e1, e2) - max(s1, s2));
}

#endif

// vim: set sw=4 ts=4 et:
