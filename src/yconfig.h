
#undef XSV
#undef XIV
#undef XIV

#ifdef CFGDEF
#define XSV(t,a,b) t a(b);
#else
#define XSV(t,a,b) extern t a;
#endif

#ifndef NO_CONFIGURE
#ifdef CFGDEF
#define XIV(t,a,b) t a(b);
#else
#define XIV(t,a,b) extern t a;
#endif
#else
#ifdef CFGDEF
#define XIV(t,a,b)
#else
#define XIV(t,a,b) static const t a(b);  // I hope this can be optimized away
#endif
#endif

#ifndef __YCONFIG_H__
#define __YCONFIG_H__

#if CONFIG_XFREETYPE >= 2
#define FONT(pt) "-*-sans-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDFONT(pt) "-*-sans-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define TTFONT(pt) "-*-monospace-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDTTFONT(pt) "-*-monospace-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#else
#ifdef FONTS_ADOBE
#define FONT(pt) "-b&h-lucida-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDFONT(pt) "-b&h-lucida-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define TTFONT(pt) "-b&h-lucidatypewriter-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDTTFONT(pt) "-b&h-lucidatypewriter-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#else
#define FONT(pt) "-adobe-helvetica-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDFONT(pt) "-adobe-helvetica-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define TTFONT(pt) "-adobe-courier-medium-r-*-*-*-" #pt "-*-*-*-*-*-*"
#define BOLDTTFONT(pt) "-adobe-courier-bold-r-*-*-*-" #pt "-*-*-*-*-*-*"
#endif
#endif

#define kfShift  1
#define kfCtrl   2
#define kfAlt    4
#define kfMeta   8
#define kfSuper  16
#define kfHyper  32

#ifdef GENPREF
///typedef unsigned int KeySym;
#endif

struct WMKey {
    KeySym key;
    unsigned int mod;
    const char *name;
    bool initial;
};

#ifdef CFGDESC
#define OBV(n,v,d) { cfoption::CF_BOOL, n, { v, { 0, 0, 0 }, { 0, false }, { 0 } }, 0, d }
#define OIV(n,v,m,M,d) { cfoption::CF_INT, n, { 0, { v, m, M }, { 0, false }, { 0 } }, 0, d }
#define OSV(n,v,d) { cfoption::CF_STR, n, { 0, { 0, 0, 0 }, { v, true }, { 0 } }, 0, d }
#define OKV(n,v,d) { cfoption::CF_KEY, n, { 0, { 0, 0, 0 }, { 0, false }, { &v } }, 0, d }
#define OKF(n,f,d) { cfoption::CF_STR, n, { false, { 0, 0, 0 }, { 0, false }, { 0 } }, &f, d }
#define OK0() { cfoption::CF_NONE, 0, { false, { 0, 0, 0 }, { 0, false }, { 0 } }, 0, 0 }

#else
#define OBV(n,v,d) { cfoption::CF_BOOL, n, { v, { 0, 0, 0 }, { 0, false }, { 0 } }, 0 }
#define OIV(n,v,m,M,d) { cfoption::CF_INT, n, { 0, { v, m, M }, { 0, false }, { 0 } }, 0 }
#define OSV(n,v,d) { cfoption::CF_STR, n, { 0, { 0, 0, 0 }, { v, true }, { 0 } }, 0 }
#define OKV(n,v,d) { cfoption::CF_KEY, n, { 0, { 0, 0, 0 }, { 0, false }, { &v } }, 0 }
#define OKF(n,f,d) { cfoption::CF_STR, n, { false, { 0, 0, 0 }, { 0, false }, { 0 } }, &f }
#define OK0() { cfoption::CF_NONE, 0, { false, { 0, 0, 0 }, { 0, false }, { 0 } }, 0 }
#endif

struct cfoption {
    enum { CF_BOOL, CF_INT, CF_STR, CF_KEY, CF_NONE } type;
    const char *name;
    struct {
        bool *bool_value;
        struct { int *int_value; int min, max; } i;
        struct { const char **string_value; bool initial; } s;
        struct { class WMKey *key_value; } k;
    } v;
    void (*notify)(const char *name, const char *value);
#ifdef CFGDESC
    const char *description;
#endif
};

void loadConfig(cfoption *options, const char *fileName);
void freeConfig(cfoption *options);
char *getArgument(char *dest, int maxLen, char *p, bool comma);

#endif
