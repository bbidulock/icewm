/*
 * IceWM
 *
 * Copyright (C) 1999 Marko Macek
 */
#include "config.h"
#include "yconfig.h"
#include "yapp.h"
#include "base.h"
#include "sysdep.h"

class YPrefDomain {
public:
    YPrefDomain(const char *domain);
    ~YPrefDomain();

    void load();
    const char *name() { return fDomain; }
    YPrefDomain *next() { return fNext; }

    YCachedPref *findPref(const char *name);
private:
    friend class YApplication;

    char *fDomain;
    YPrefDomain *fNext;
    YCachedPref *fFirstPref;

    char *findPrefsFile(const char *name);
    void parse(char *buf);
    char *parseOption(char *str);
};

class YCachedPref {
public:
    YCachedPref(const char *name, const char *value);

    const char *getName() { return fName; }
    const char *getValue() { return fValue; }
    void updateValue(const char *value);

    void addListener(YPref *pref);
    void removeListener(YPref *pref);
private:
    friend class YApplication;
    friend class YPrefDomain;

    char *fName;
    char *fValue;
    YPrefDomain *fDomain;
    YPref *fFirst; // listeners
    YPref *fCurrent; // iterator
    YCachedPref *fNext; // list of prefs
};

YPref::YPref(const char *domain, const char *name, YPrefListener *listener) {
    fListener = listener;
    fNext = 0;
    fPref = app->getPref(domain, name);
    if (fListener != 0)
        fPref->addListener(this);
}

YPref::~YPref() {
    if (fListener != 0)
        fPref->removeListener(this);
    fPref = 0;
    fListener = 0;
}

const char *YPref::getName() {
    return fPref->getName();
}

const char *YPref::getValue() {
    return fPref->getValue();
}

long YPref::getNum(long defValue) {
    const char *v = getValue();
    if (!v)
        return defValue;

    if (*v == 0)
        return defValue;

    long val;
    char *endptr;

    val = strtol(v, &endptr, 0);

    if (endptr != NULL && *endptr != '\0')
        return defValue;

    if (val != LONG_MAX && val != LONG_MIN);
        return val;

    return defValue;
}

bool YPref::getBool(bool defValue) {
    const char *v = getValue();
    if (!v)
        return defValue;
    if (strcmp(v, "1") == 0 ||
        strcmp(v, "true") == 0 ||
        strcmp(v, "yes") == 0)
        return true;
    return false;
}

const char *YPref::getStr(const char *defValue) {
    const char *v = getValue();
    if (!v)
        return defValue;
    return v;
}

void YPref::changed() {
    fListener->prefChanged(this);
}


YCachedPref::YCachedPref(const char *name, const char *value) {
    fName = newstr(name);
    fValue = value ? newstr(value) : 0;
    fFirst = 0;
}

void YCachedPref::updateValue(const char *value) {
    delete [] fValue;
    fValue = newstr(value);

    YPref *p = fFirst, *c;

    while (p) {
        c = p;
        p = p->fNext;
        c->changed();
    }
}

void YCachedPref::addListener(YPref *pref) {
    assert(pref->fNext == 0);
    pref->fNext = fFirst;
    fFirst = pref;
}

void YCachedPref::removeListener(YPref *pref) {
    YPref **p = &fFirst;
    while (*p) {
        if (*p == pref) {
            *p = (*p)->fNext;
            break;
        }
        p = &((*p)->fNext);
    }
    pref->fNext = 0;
}

YCachedPref *YApplication::getPref(const char *domain, const char *name) {
    YPrefDomain *d = fPrefDomains;

    if (domain == 0)
        domain = fAppName;

    while (d) {
        if (strcmp(d->name(), domain) == 0)
            return d->findPref(name);
        d = d->next();
    }
    d = new YPrefDomain(domain);
    if (d) {
        d->fNext = fPrefDomains;
        fPrefDomains = d;
        d->load();
        return d->findPref(name);
    }
    return 0;
}

YPrefDomain::YPrefDomain(const char *domain) {
    if (domain)
        fDomain = newstr(domain);
    else
        fDomain = 0;
    fNext = 0;
    fFirstPref = 0;
}

YPrefDomain::~YPrefDomain() {
    delete [] fDomain;
}

YCachedPref *YPrefDomain::findPref(const char *name) {
    YCachedPref *p;

    p = fFirstPref;
    while (p) {
        if (strcmp(name, p->fName) == 0)
            return p;
        p = p->fNext;
    }
    p = new YCachedPref(name, 0);
    p->fNext = fFirstPref;
    fFirstPref = p;
    return p;
}

char *YPrefDomain::findPrefsFile(const char *name) {
    char *p, *h;

    h = getenv("HOME");
    if (h) {
        p = strJoin(h, "/.iprefs/", name, NULL);
        if (access(p, R_OK) == 0)
            return p;
        delete p;
    }
    return 0;
}

static char *getArgument(char *dest, int maxLen, char *p, bool comma) {
    char *d;
    int len = 0;
    int in_str = 0;
    
    while (*p && (*p == ' ' || *p == '\t'))
        p++;

    d = dest;
    len = 0;
    while (*p && len < maxLen - 1 &&
           (in_str || (*p != ' ' && *p != '\t' && *p != '\n' && (!comma || *p != ','))))
    {
        if (in_str && *p == '\\' && p[1]) {
            p++;
            char c = *p++; // *++p++ doesn't work :(

            switch (c) {
            case 'a': *d++ = '\a'; break;
            case 'b': *d++ = '\b'; break;
            case 'e': *d++ = 27; break;
            case 'f': *d++ = '\f'; break;
            case 'n': *d++ = '\n'; break;
            case 'r': *d++ = '\r'; break;
            case 't': *d++ = '\t'; break;
            case 'v': *d++ = '\v'; break;
            case 'x':
#define UNHEX(c) \
    (\
    ((c) >= '0' && (c) <= '9') ? (c) - '0' : \
    ((c) >= 'A' && (c) <= 'F') ? (c) - 'A' + 0xA : \
    ((c) >= 'a' && (c) <= 'f') ? (c) - 'a' + 0xA : 0 \
    )
                if (p[0] && p[1]) { // only two digits taken
                    int a = UNHEX(p[0]);
                    int b = UNHEX(p[1]);

                    int n = (a << 4) + b;

                    p += 3;
                    *d++ = (unsigned char)(n & 0xFF);

                    a -= '0';
                    if (a > '9')
                        a = a + '0' - 'A';
                    break;
                }
            default:
                *d++ = c;
                break;
            }
            len++;
        } else if (*p == '"') {
            in_str = !in_str;
            p++;
        } else {
            *d++ = *p++;
            len++;
        }
    }
    *d = 0;

    return p;
}

// parse option name and argument
// name is string without spaces up to =
// option is a " quoted string or characters up to next space
char *YPrefDomain::parseOption(char *str) {
    char name[64];
    char argument[256];
    char *p = str;
    unsigned int len = 0;

    while (*p && *p != '=' && *p != ' ' && *p != '\t' && len < sizeof(name) - 1)
        p++, len++;

    strncpy(name, str, len);
    name[len] = 0;

    while (*p && *p != '=')
        p++;
    if (*p != '=')
        return 0;
    p++;

    do {
        p = getArgument(argument, sizeof(argument), p, true);

        //p = setOption(name, argument, p);
        YCachedPref *pref = new YCachedPref(name, argument);
        if (pref) {
            pref->fNext = fFirstPref;
            fFirstPref = pref;
        }

        if (p == 0)
            return 0;

        while (*p && (*p == ' ' || *p == '\t'))
            p++;

        if (*p != ',')
            break;
        p++;
    } while (1);

    return p;
}

void YPrefDomain::parse(char *data) {
    char *p = data;

    while (p && *p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || (*p == '\\' && p[1] == '\n'))
            p++;
        
        if (*p != '#')
            p = parseOption(p);
        else {
            while (*p && *p != '\n') {
                if (*p == '\\' && p[1] != 0)
                    p++;
                p++;
            }
        }
    }
}

void YPrefDomain::load() {
    char *f = findPrefsFile(fDomain);
    if (f == 0)
        return ;

    int fd = open(f, O_RDONLY | O_TEXT);
    delete [] f;

    if (fd == -1)
        return ;

    struct stat sb;

    if (fstat(fd, &sb) == -1)
        return ;

    int len = sb.st_size;

    char *buf = new char[len + 1];
    if (buf == 0)
        return ;

    if (read(fd, buf, len) != len)
        return;

    buf[len] = 0;
    close(fd);
    parse(buf);
    delete buf;
}
