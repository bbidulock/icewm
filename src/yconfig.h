
#undef XSV
#undef XFV
#undef XIV

#ifdef CFGDEF
#define XSV(t,a,b) t a(b);
#else
#define XSV(t,a,b) extern t a;
#endif

#ifdef CFGDEF
#define XFV(t,a,b,c) \
    t a(b); \
    t a##Xft = c;
#else
#define XFV(t,a,b,c) \
    extern t a; \
    extern t a##Xft;
#endif

#define XFA(a) a, a##Xft

#ifdef CFGDEF
#define XIV(t,a,b) t a(b);
#else
#define XIV(t,a,b) extern t a;
#endif

#ifndef __YCONFIG_H__
#define __YCONFIG_H__

#ifdef CONFIG_XFREETYPE
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
#define kfAltGr  64

struct WMKey {
    KeySym key;
    unsigned int mod;
    const char *name;
    bool initial;

    bool eq(KeySym k, unsigned m) const { return key == k && mod == m; }
    bool operator==(const WMKey& o) const { return eq(o.key, o.mod); }
    bool operator!=(const WMKey& o) const { return !eq(o.key, o.mod); }
};

#ifdef CFGDESC
#define DESC(d) d
#else
#define DESC(d) ((const char *) 0)
#endif

#define OBV(n,v,d)     cfoption(n, v, DESC(d))
#define OIV(n,v,m,M,d) cfoption(n, v, m, M, DESC(d))
#define OUV(n,v,m,M,d) cfoption(n, v, m, M, DESC(d))
#define OSV(n,v,d)     cfoption(n, v, DESC(d))
#define OFV(n,v,d)     cfoption(n, v, DESC(d)), \
                       cfoption(n "Xft", v##Xft, DESC(d))
#define OKV(n,v,d)     cfoption(n, &v, DESC(d))
#define OKF(n,f,d)     cfoption(n, f, DESC(d))
#define OK0()          cfoption()

struct cfoption {
    typedef void (*notifyfun)(const char* name, const char* value, bool append);
    enum {
        CF_NONE, CF_BOOL, CF_INT, CF_UINT, CF_STR, CF_KEY, CF_FUNC,
    } type;
    const char *name;
    const char *description;
    union {
        struct { int *int_value; int min, max; } i;
        struct { unsigned *uint_value; unsigned min, max; } u;
        struct { const char **string_value; bool initial; } s;
        struct { WMKey *key_value; } k;
        struct { bool *bool_value; } b;
        struct { notifyfun notify; } f;
    } v;

    cfoption(const char* n, bool* b, const char* d)
        : type(CF_BOOL), name(n), description(d) {
        v.b.bool_value = b;
    }
    cfoption(const char* n, int* i, int m, int M, const char* d)
        : type(CF_INT), name(n), description(d) {
        v.i.int_value = i;
        v.i.min = m;
        v.i.max = M;
    }
    cfoption(const char* n, unsigned* u, unsigned m, unsigned M, const char* d)
        : type(CF_UINT), name(n), description(d) {
        v.u.uint_value = u;
        v.u.min = m;
        v.u.max = M;
    }
    cfoption(const char* n, const char** s, const char* d)
        : type(CF_STR), name(n), description(d) {
        v.s.string_value = s;
        v.s.initial = true;
    }
    cfoption(const char* n, WMKey* k, const char* d)
        : type(CF_KEY), name(n), description(d) {
        v.k.key_value = k;
    }
    cfoption(const char* n, notifyfun f, const char* d)
        : type(CF_FUNC), name(n), description(d) {
        v.f.notify = f;
    }
    cfoption() : type(CF_NONE), name(0), description(0) { }
    bool boolval() { return *v.b.bool_value; }
    int intval() { return *v.i.int_value; }
    unsigned uintval() { return *v.u.uint_value; }
    const char* str() { return *v.s.string_value; }
    WMKey* key() { return v.k.key_value; }
    notifyfun fun() { return v.f.notify; }
};

class Argument;
class IApp;
class upath;

class YConfig {
public:
    static bool loadConfigFile(cfoption *options, upath fileName);
    static void freeConfig(cfoption *options);
    static char *getArgument(Argument *dest, char *p, bool comma = false);
    static bool findLoadConfigFile(IApp *app, cfoption *options, upath name);
    static bool findLoadThemeFile(IApp *app, cfoption *options, upath name);
    static void parseConfiguration(cfoption *options, char *data);
    static bool parseKey(const char *arg, KeySym *key, unsigned int *mod);
    static size_t cfoptionSize();
};

#endif

// vim: set sw=4 ts=4 et:
