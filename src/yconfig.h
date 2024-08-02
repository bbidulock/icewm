
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
    t a##Core = b; \
    t a##Xft = c; \
    YFontName a(&a##Core, &a##Xft);
#else
#define XFV(t,a,b,c) \
    extern t a##Core; \
    extern t a##Xft; \
    extern YFontName a;
#endif

#ifdef CFGDEF
#define XIV(t,a,b) t a(b);
#else
#define XIV(t,a,b) extern t a;
#endif

#ifndef YCONFIG_H
#define YCONFIG_H

#include <X11/X.h>
#include "yfontname.h"
#include "wmkey.h"

#define kfShift  1
#define kfCtrl   2
#define kfAlt    4
#define kfMeta   8
#define kfSuper  16
#define kfHyper  32
#define kfAltGr  64

#ifdef CFGDESC
#define DESC(d) d
#else
#define DESC(d) nullptr
#endif

#define OBV(n,v,d)     cfoption(n, sizeof(n), v, DESC(d))
#define OIV(n,v,m,M,d) cfoption(n, sizeof(n), v, m, M, DESC(d))
#define OUV(n,v,m,M,d) cfoption(n, sizeof(n), v, m, M, DESC(d))
#define OSV(n,v,d)     cfoption(n, sizeof(n), v, DESC(d))
#define OFV(n,v,d)     cfoption(n, sizeof(n), v##Core, DESC(d)), \
                       cfoption(n "Xft", sizeof(n "Xft"), v##Xft, DESC(d))
#define OKV(n,v,d)     cfoption(n, sizeof(n), &v, DESC(d))
#define OKF(n,f,d)     cfoption(n, sizeof(n), f, DESC(d))
#define OK0()          cfoption()

struct cfoption {
    typedef void (*notifyfun)(const char* name, const char* value, bool append);
    enum OptionType {
        CF_NONE, CF_BOOL, CF_INT, CF_UINT, CF_STR, CF_KEY, CF_FUNC,
    } type;
    unsigned size;
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

    cfoption(const char* n, size_t z, bool* b, const char* d)
        : type(CF_BOOL), size(z), name(n), description(d) {
        v.b.bool_value = b;
    }
    cfoption(const char* n, size_t z, int* i, int m, int M, const char* d)
        : type(CF_INT), size(z), name(n), description(d) {
        v.i.int_value = i;
        v.i.min = m;
        v.i.max = M;
    }
    cfoption(const char* n, size_t z, unsigned* u, unsigned m, unsigned M, const char* d)
        : type(CF_UINT), size(z), name(n), description(d) {
        v.u.uint_value = u;
        v.u.min = m;
        v.u.max = M;
    }
    cfoption(const char* n, size_t z, const char** s, const char* d)
        : type(CF_STR), size(z), name(n), description(d) {
        v.s.string_value = s;
        v.s.initial = true;
    }
    cfoption(const char* n, size_t z, WMKey* k, const char* d)
        : type(CF_KEY), size(z), name(n), description(d) {
        v.k.key_value = k;
    }
    cfoption(const char* n, size_t z, notifyfun f, const char* d)
        : type(CF_FUNC), size(z), name(n), description(d) {
        v.f.notify = f;
    }
    cfoption() : type(CF_NONE), size(0), name(nullptr), description(nullptr) { }
    bool boolval() const { return *v.b.bool_value; }
    int intval() const { return *v.i.int_value; }
    int intmax() const { return v.i.max; }
    int intmin() const { return v.i.min; }
    unsigned uintval() const { return *v.u.uint_value; }
    unsigned uintmax() const { return v.u.max; }
    unsigned uintmin() const { return v.u.min; }
    const char* str() const { return *v.s.string_value; }
    WMKey* key() const { return v.k.key_value; }
    notifyfun fun() const { return v.f.notify; }

    bool operator==(const cfoption& r) const;
    bool operator!=(const cfoption& r) const { return !operator==(r); }
};

class Argument;
class upath;

class YConfig {
    cfoption* options;
    bool success;
public:
    YConfig(cfoption* options) : options(options), success(false) { }
    YConfig& load(const char* file);
    YConfig& loadTheme();
    YConfig& loadOverride();
    operator bool() const { return success; }

    static void freeConfig(cfoption* options);
    static char* getArgument(Argument* dest, char* p, bool comma = false);
    static bool loadConfigFile(cfoption* options, upath fileName,
                               cfoption* more = nullptr, cfoption* xtra = nullptr);
    static bool findLoadConfigFile(cfoption* options, const char* name);
    static bool findLoadThemeFile(cfoption* options);
    static bool parseKey(const char* arg, KeySym* key, unsigned int* mod);
    static KeySym parseKeySym(const char* arg);
    static size_t cfoptionSize();

private:
    cfoption* findOption(char* name, size_t length);
    static void setOption(char* arg, bool append, cfoption* opt);
    char* parseOption(char* str);
    void parseConfiguration(char* data);
    static char* skipLine(char* p);
};

#endif

// vim: set sw=4 ts=4 et:
