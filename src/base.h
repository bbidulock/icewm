#ifndef __BASE_H
#define __BASE_H

#if ( __GNUC__ == 3 && __GNUC_MINOR__ > 0 ) || __GNUC__ > 3
#define ICEWM_deprecated __attribute__((deprecated))
#else
#define ICEWM_deprecated
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

#if 1
char *newstr(char const *str);
char *newstr(char const *str, int len);
char *newstr(char const *str, char const *delim);
char *cstrJoin(char const *str, ...);
#endif

#if 0
bool isempty(char const *str);
bool isreg(char const *path);
#endif

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

void die(int exitcode, char const *msg, ...);
void warn(char const *msg, ...);
void msg(char const *msg, ...);
void precondition(char const *msg, ...);
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

#if 0
inline bool strIsEmpty(char const *str) {
    if (str) while (*str) if (*str++ > ' ') return false;
    return true;
}
#endif

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

/******************************************************************************/

#if 1

/// this should be abstracted somehow (maybe in yapp)
#define GET_SHORT_ARGUMENT(Name) \
    (!strncmp(*arg, "-" Name, 3) ? *++arg : NULL)
#define GET_LONG_ARGUMENT(Name) \
    (!strpcmp(*arg, "--" Name, "=") ? \
      ('=' == (*arg)[sizeof(Name) + 1] ? (*arg) + sizeof(Name) + 2 : *++arg) \
      : \
      NULL)

#define IS_SHORT_SWITCH(Name)  (0 == strcmp(*arg, "-" Name))
#define IS_LONG_SWITCH(Name)   (0 == strcmp(*arg, "--" Name))
#define IS_SWITCH(Short, Long) (IS_SHORT_SWITCH(Short) || \
                                IS_LONG_SWITCH(Long))
#endif

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
