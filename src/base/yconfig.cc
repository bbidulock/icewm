/*
 * IceWM
 *
 * Copyright (C) 1999 Marko Macek
 */
#include "config.h"
#include "yconfig.h"
#include "ycstring.h"
#include "yapp.h"
#include "base.h"
#include "sysdep.h"

class YPrefDomain {
public:
    YPrefDomain(const char *domain);
    ~YPrefDomain();

    void loadAll();
    const CStr *name() { return fDomain; }
    YPrefDomain *next() { return fNext; }

    YCachedPref *findPref(const char *name);
private:
    friend class YApplication;

    CStr *fDomain;
    YPrefDomain *fNext;
    YCachedPref *fFirstPref;

    void parse(const char *path, char *buf);
    char *parseOption(const char *path, char *str);
    void load(const char *path, const char *name);
private: // not-used
    YPrefDomain(const YPrefDomain &);
    YPrefDomain &operator=(const YPrefDomain &);
};

class YCachedPref {
public:
    YCachedPref(const char *name, const char *value);
    ~YCachedPref();

    const CStr *getName() { return fName; }
    const CStr *getValue() { return fValue; }
    const CStr *getPath() { return fPath; }
    void updateValue(const char *value);
    void updatePath(const char *path);

    void addListener(YPref *pref);
    void removeListener(YPref *pref);
private:
    friend class YApplication;
    friend class YPrefDomain;

    CStr *fName;
    CStr *fValue;
    YPrefDomain *fDomain;
    CStr *fPath;
    YPref *fFirst; // listeners
    YPref *fCurrent; // iterator
    YCachedPref *fNext; // list of prefs
private: // not-used
    YCachedPref(const YCachedPref &);
    YCachedPref &operator=(const YCachedPref &);
};

YPref::YPref(const char *domain, const char *name, YPrefListener *listener) {
    fListener = listener;
    fNext = 0;
    fCachedPref = app->getPref(domain, name);
    if (fListener != 0 && fCachedPref)
        fCachedPref->addListener(this);
}

YPref::~YPref() {
    if (fListener != 0 && fCachedPref)
        fCachedPref->removeListener(this);
    fCachedPref = 0;
    fListener = 0;
}

YCachedPref *YPref::pref() {
    return fCachedPref;
}

const CStr *YPref::getName() {
    return fCachedPref->getName();
}

const CStr *YPref::getValue() {
    return fCachedPref->getValue();
}

const CStr *YPref::getPath() {
    return fCachedPref->getPath();
}

long YPref::getNum(long defValue) {
    const CStr *cv = getValue();
    if (!cv)
        return defValue;

    const char *v = cv->c_str();

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
    const CStr *cv = getValue();
    if (!cv)
        return defValue;
    const char *v = cv->c_str();
    if (!v)
        return defValue;
    if (strcmp(v, "1") == 0 ||
        strcmp(v, "true") == 0 ||
        strcmp(v, "yes") == 0)
        return true;
    return false;
}

const char *YPref::getStr(const char *defValue) {
    const CStr *cv = getValue();
    if (!cv)
        return defValue;
    const char *v = cv->c_str();
    if (!v)
        return defValue;
    return v;
}

void YPref::changed() {
    fListener->prefChanged(this);
}


YCachedPref::YCachedPref(const char *name, const char *value) {
    fName = CStr::newstr(name);
    fValue = value ? CStr::newstr(value) : 0;
    fPath = 0;
    fFirst = 0;
    fprintf(stderr, "pref: %s\n", name);
}

YCachedPref::~YCachedPref() {
    delete fName; fName = 0;
    delete fValue; fValue = 0;
}

void YCachedPref::updateValue(const char *value) {
    delete fValue;
    fValue = CStr::newstr(value);

    YPref *p = fFirst, *c;

    while (p) {
        c = p;
        p = p->fNext;
        c->changed();
    }
}

void YCachedPref::updatePath(const char *path) {
    delete fPath;
    fPath = CStr::newstr(path);
    /// fire the events?
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
        domain = fAppName->c_str();

    while (d) {
        if (strcmp(d->name()->c_str(), domain) == 0)
            return d->findPref(name);
        d = d->next();
    }
    d = new YPrefDomain(domain);
    if (d) {
        d->fNext = fPrefDomains;
        fPrefDomains = d;
        d->loadAll();
        return d->findPref(name);
    }
    return 0;
}

void YApplication::freePrefs() {
    YPrefDomain *d = fPrefDomains, *n;

    while (d) {
        n = d->fNext;
        delete d;
        d = n;
    }
    fPrefDomains = 0;
}

YPrefDomain::YPrefDomain(const char *domain) {
    if (domain)
        fDomain = CStr::newstr(domain);
    else
        fDomain = 0;
    fNext = 0;
    fFirstPref = 0;
}

YPrefDomain::~YPrefDomain() {
    delete fDomain;

    YCachedPref *p = fFirstPref, *n;

    while (p) {
        n = p->fNext;
        delete p;
        p = n;
    }
    fFirstPref = 0;
}

YCachedPref *YPrefDomain::findPref(const char *name) {
    YCachedPref *p;

    p = fFirstPref;
    while (p) {
        if (strcmp(name, p->fName->c_str()) == 0)
            return p;
        p = p->fNext;
    }
    p = new YCachedPref(name, 0);
    p->fNext = fFirstPref;
    fFirstPref = p;
    return p;
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
char *YPrefDomain::parseOption(const char *path, char *str) {
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
        YCachedPref *pref = findPref(name);
        //new YCachedPref(name, argument);
        //if (pref) {
        //    pref->fNext = fFirstPref;
        //    fFirstPref = pref;
        //}
        if (pref) {
            pref->updatePath(path);
            pref->updateValue(argument);
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

void YPrefDomain::parse(const char *path, char *data) {
    char *p = data;

    while (p && *p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || (*p == '\\' && p[1] == '\n'))
            p++;

        if (*p != '#')
            p = parseOption(path, p);
        else {
            while (*p && *p != '\n') {
                if (*p == '\\' && p[1] != 0)
                    p++;
                p++;
            }
        }
    }
}

void YPrefDomain::loadAll() {
    struct timeval start, end, diff;

    gettimeofday(&start, 0);

    char *h = getenv("HOME");
    CStr *d;
    CStr *f;

    //!!! add libDir somehow () ?

    d = CStr::join("/etc/" PNAME "/", NULL);
    f = CStr::join("/etc/" PNAME "/", fDomain->c_str(), ".pref", NULL);
    load(d->c_str(), f->c_str());
    delete f;
    delete d;

    d = CStr::join(h, "/." PNAME "/theme/", NULL);
    f = CStr::join(h, "/." PNAME "/theme/", fDomain->c_str(), ".pref", NULL);
    load(d->c_str(), f->c_str());
    delete f;
    delete d;

    d = CStr::join(h, "/." PNAME "/", NULL);
    f = CStr::join(h, "/." PNAME "/", fDomain->c_str(), ".pref", NULL);
    load(d->c_str(), f->c_str());
    delete f;
    delete d;
    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    fprintf(stderr, "load(%s) in %ld.%06ld\n", fDomain->c_str(), diff.tv_sec, diff.tv_usec);
}

void YPrefDomain::load(const char *path, const char *name) {
    //fprintf(stderr, "file=%s\n", name);
    int fd = open(name, O_RDONLY | O_TEXT);

    if (fd == -1)
        return ;

    struct stat sb;

    if (fstat(fd, &sb) == -1) {
        close(fd);
        return ;
    }

    if (!(sb.st_mode & S_IFREG)) {
        close(fd);
        return ;
    }

    int len = sb.st_size;

    char *buf = new char[len + 1];
    if (buf == 0) {
        close(fd);
        return ;
    }

    if (read(fd, buf, len) != len) {
        close(fd);
        return;
    }

    buf[len] = 0;
    close(fd);
    parse(path, buf);
    delete [] buf;
}

YColorPrefProperty::YColorPrefProperty(const char *domain, const char *name, const char *defval) {
    fDomain = domain;
    fName = name;
    fDefVal = defval;
    fColor = 0;
    fPref = 0;
}

YColorPrefProperty::~YColorPrefProperty() {
    delete fPref; fPref = 0;
    delete fColor; fColor = 0;
}

void YColorPrefProperty::fetch() {
    if (fColor == 0) {
        if (fPref == 0)
            fPref = new YPref(fDomain, fName);
        fColor = new YColor(fPref->getStr(fDefVal));
    }
}

YFontPrefProperty::YFontPrefProperty(const char *domain, const char *name, const char *defval) {
    fDomain = domain;
    fName = name;
    fDefVal = defval;
    fFont = 0;
    fPref = 0;
}

YFontPrefProperty::~YFontPrefProperty() {
    delete fPref; fPref = 0;
    delete fFont; fFont = 0;
}

void YFontPrefProperty::fetch() {
    if (fFont == 0) {
        if (fPref == 0)
            fPref = new YPref(fDomain, fName);
        fFont = YFont::getFont(fPref->getStr(fDefVal));
    }
}

YNumPrefProperty::YNumPrefProperty(const char *domain, const char *name, long defval) {
    fDomain = domain;
    fName = name;
    fDefVal = defval;
    fPref = 0;
    fNum = defval;
}

YNumPrefProperty::~YNumPrefProperty() {
    delete fPref; fPref = 0;
}

void YNumPrefProperty::fetch() {
    if (fPref == 0) {
        fPref = new YPref(fDomain, fName);
        fNum = fPref->getNum(fDefVal);
    }
}

YStrPrefProperty::YStrPrefProperty(const char *domain, const char *name, const char *defval) {
    fDomain = domain;
    fName = name;
    fDefVal = defval;
    fPref = 0;
    fStr = 0;
}

YStrPrefProperty::~YStrPrefProperty() {
    delete fPref; fPref = 0;
    //delete [] fStr; !!!
}

void YStrPrefProperty::fetch() {
    if (fStr == 0) {
        if (fPref == 0)
            fPref = new YPref(fDomain, fName);
        fStr = fPref->getStr(fDefVal); //!!!?
    }
}

YBoolPrefProperty::YBoolPrefProperty(const char *domain, const char *name, bool defval) {
    fDomain = domain;
    fName = name;
    fDefVal = defval;
    fPref = 0;
    fBool = defval;
}

YBoolPrefProperty::~YBoolPrefProperty() {
    delete fPref; fPref = 0;
}

void YBoolPrefProperty::fetch() {
    if (fPref == 0) {
        fPref = new YPref(fDomain, fName);
        if (fPref)
            fBool = fPref->getBool(fDefVal);
    }
}

YPixmapPrefProperty::YPixmapPrefProperty(const char *domain, const char *name, const char *defval, const char *libdir) {
    fDomain = domain;
    fName = name;
    fDefVal = defval;
    fLibDir = libdir;
    fPref = 0;
    fPixmap = 0;
    fDidTile = false;
}

YPixmapPrefProperty::~YPixmapPrefProperty() {
    delete fPixmap; // !!! need pixmap cache
}

void YPixmapPrefProperty::fetch() {
    if (fPref == 0) {
        fPref = new YPref(fDomain, fName);
        if (fPref) {

            // use YFilePath here !!!

            const CStr *path = fPref->getPath();
            const char *name = fPref->getStr(fDefVal), *fn = 0;
            const CStr *p = 0;

            if (name && name[0] == '/')
                fn = name;
            else {
                if (path) {
                    p = CStr::join(path->c_str(), "/", name, 0);
                    fn = p->c_str();
                } else if (fLibDir) {
                    p = CStr::join(fLibDir, "/", name, 0);
                    fn = p->c_str();
                }
            }
            if (fn) {
                fprintf(stderr, "load image(%s:%s)=(%s)\n", fDomain, fName, fn);
                fPixmap = app->loadPixmap(fn);
            }
            delete p;
        }
    }
}

static void replicatePixmap(YPixmap **pixmap, bool horiz) {
    if (*pixmap && (*pixmap)->pixmap()) {
        YPixmap *newpix;
        Graphics *ng;
        int dim;

        if (horiz)
            dim = (*pixmap)->width();
        else
            dim = (*pixmap)->height();

        while (dim < 128) dim *= 2;

        if (horiz)
            newpix = new YPixmap(dim, (*pixmap)->height());
        else
            newpix = new YPixmap((*pixmap)->width(), dim);

        if (!newpix)
            return ;

        ng = new Graphics(newpix);

        if (horiz)
            ng->repHorz(*pixmap, 0, 0, newpix->width());
        else
            ng->repVert(*pixmap, 0, 0, newpix->height());

        delete ng;
        delete *pixmap;
        *pixmap = newpix;
    }
}

// performance optimization for "lazy" theme authors ;)
YPixmap *YPixmapPrefProperty::tiledPixmap(bool horizontal) {
    YPixmap *pix = getPixmap();
    if (pix && !fDidTile) {
        replicatePixmap(&fPixmap, horizontal);
        fDidTile = true;
    }
    return fPixmap;

}

