#ifndef __BASE_H
#define __BASE_H

/*** Atomar Data Types ********************************************************/

#ifdef NEED_BOOL
	typedef  { false = 0, true = 1 } bool;
#endif

#if SIZEOF_CHAR == 1
	typedef signed char yint8;
	typedef unsigned char yuint8;
#else
	#error Need typedefs for 8 bit data types
#endif

#if SIZEOF_SHORT == 2
	typedef signed short yint16;
	typedef unsigned short yuint16;
#else
	#error Need typedefs for 16 bit data types
#endif

#if SIZEOF_INT == 4
	typedef signed yint32;
	typedef unsigned yuint32;
#elif SIZEOF_LONG == 4
	typedef signed long yint32;
	typedef unsigned long yuint32;
#else
	#error Need typedefs for 32 bit data types
#endif

/*** Essential Arithmetic Functions *******************************************/

/*
 * Decimal digits required to write the largest element of type:
 * bits(Type) * (2.5 = 5/2 ~ (ln(2) / ln(10)))
 */
#define DIGIT_COUNT(Type) ((sizeof(Type) * 5 + 1) / 2)

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

char *newstr(char const *str);
char *newstr(char const *str, int len);
char *newstr(char const *str, char const *delim);
char *strJoin(char const *str, ...);

bool isempty(char const *str);
bool isreg(char const *path);

/*
 * Convert unsigned to string
 */
template <class T>
inline char * utoa(T u, char * s, unsigned const len) {
    if (len > DIGIT_COUNT(T)) {
	*(s+= DIGIT_COUNT(u) + 1) = '\0';
	do { *--s = '0' + u % 10; } while (u/= 10);
	return s;
    } else
	return 0;
}

template <class T>
static char const * utoa(T u) {
    static char s[DIGIT_COUNT(int) + 1];
    return utoa(u, s, sizeof(s));
}

/*
 * Convert signed to string
 */
template <class T>
inline char * itoa(T i, char * s, unsigned const len, bool sign = false) {
    if (len > DIGIT_COUNT(T) + 1) {
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
    static char s[DIGIT_COUNT(int) + 2];
    return itoa(i, s, sizeof(s), sign);
}

/*** Message Functions ********************************************************/

void die(int exitcode, char const *msg, ...);
void warn(char const *msg, ...);
void msg(char const *msg, ...);

/*** Allocation Functions *****************************************************/

// !!! remove this
void *MALLOC(unsigned int len);
void *REALLOC(void *p, unsigned int new_len);
void FREE(void *p);

/*** Misc Stuff (clean up!!!) *************************************************/

#define ACOUNT(x) (sizeof(x)/sizeof(x[0]))

#ifndef DIR_DELIMINATOR
# define DIR_DELIMINATOR	'/'
#endif

#ifndef ISBLANK
# define ISBLANK(c)	(((c) == ' ') || ((c) == '\t'))
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
#define KEY_MODMASK(x) ((x) & (app->KeyMask))
#define BUTTON_MASK(x) ((x) & (app->ButtonMask))
#define BUTTON_MODMASK(x) ((x) & (app->ButtonKeyMask))
#define IS_BUTTON(s,b) (BUTTON_MODMASK(s) == (b))

#define ISMASK(w,e,n) (((w) & ~(n)) == (e))
#define HASMASK(w,e,n) ((((w) & ~(n)) & (e)) == (e))

#define ISLOWER(c) ((c) >= 'a' && (c) <= 'z')
#define TOUPPER(c) (ISLOWER(c) ? (c) - 'a' + 'A' : (c))

inline bool strIsEmpty(char const *str) {
    if (str) while (*str)
	if (*str++ > ' ') return false;

    return true;
}

int strpcmp(char const *str, char const *pfx, char const *delim = "=:");
unsigned strTokens(const char * str, const char * delim = " \t");
char const * strnxt(const char * str, const char * delim = " \t");
extern "C" char * basename(char const * filename);

inline int unhex(char c) {
    return ((c >= '0' && c <= '9') ? c - '0' :
	    (c >= 'A' && c <= 'F') ? c - 'A' + 10 :
	    (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1);
}

template <class T>
inline char const * niceUnit(T & val, char const * const units[],
			     T const lim = 10240, T const div = 1024) {
    char const * uname(0);

    if (units && *units) {
        uname = *units++;
	while (val >= lim && *units) {
	    uname = *units++;
	    val/= div;
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

#define THROW(Result) { rc = (Result); goto exceptionHandler; }
#define TRY(Command) { if ((rc = (Command))) THROW(rc); }
#define CATCH(Handler) { exceptionHandler: { Handler } return rc; }

#include "debug.h"

#endif
