#ifndef __YCONFIG_H
#define __YCONFIG_H

#pragma interface

class YPref;
class YCachedPref;
class CStr;

class YPrefListener {
public:
    virtual void prefChanged(YPref *pref) = 0;
};

class YPref {
public:
    YPref(const char *domain, const char *name, YPrefListener *listener = 0);
    ~YPref();

    const CStr *getName();
    const CStr *getValue();

    long getNum(long defValue);
    bool getBool(bool defValue);
    const char *getStr(const char *defValue);
    //void setValue(const char *value);

    const CStr *getPath();
private:

    friend class YCachedPref;

    void changed();
    YCachedPref *pref();

    YPref *fNext;
    YPrefListener *fListener;
    YCachedPref *fCachedPref;
private: // not-used
    YPref(const YPref &);
    YPref &operator=(const YPref &);
};

class YColor;

class YColorPrefProperty {
public:
    YColorPrefProperty(const char *domain, const char *name, const char *defval);
    ~YColorPrefProperty();

    YColor *getColor() { if (fColor == 0) fetch(); return fColor; }
private:
    const char *fDomain;
    const char *fName;
    YPref *fPref;
    const char *fDefVal;
    YColor *fColor;

    void fetch();
private: // not-used
    YColorPrefProperty(const YColorPrefProperty &);
    YColorPrefProperty &operator=(const YColorPrefProperty &);
};

class YFont;

class YFontPrefProperty {
public:
    YFontPrefProperty(const char *domain, const char *name, const char *defval);
    ~YFontPrefProperty();

    YFont *getFont() { if (fFont == 0) fetch(); return fFont; }
private:
    const char *fDomain;
    const char *fName;
    YPref *fPref;
    const char *fDefVal;
    YFont *fFont;

    void fetch();
private: // not-used
    YFontPrefProperty(const YFontPrefProperty &);
    YFontPrefProperty &operator=(const YFontPrefProperty &);
};

class YPixmap;

class YPixmapPrefProperty {
public:
    YPixmapPrefProperty(const char *domain, const char *name, const char *defval, const char *libdir = 0);
    ~YPixmapPrefProperty();

    YPixmap *getPixmap() { if (fPixmap == 0) fetch(); return fPixmap; }
    YPixmap *tiledPixmap(bool horizontal);
private:
    const char *fDomain;
    const char *fName;
    YPref *fPref;
    const char *fDefVal;
    const char *fLibDir;
    YPixmap *fPixmap;
    bool fDidTile;

    void fetch();
private: // not-used
    YPixmapPrefProperty(const YPixmapPrefProperty &);
    YPixmapPrefProperty &operator=(const YPixmapPrefProperty &);
};

class YBoolPrefProperty {
public:
    YBoolPrefProperty(const char *domain, const char *name, bool defval);
    ~YBoolPrefProperty();

    long getBool() { if (fPref == 0) fetch(); return fBool; }
private:
    const char *fDomain;
    const char *fName;
    YPref *fPref;
    bool fDefVal;
    bool fBool;

    void fetch();
private: // not-used
    YBoolPrefProperty(const YBoolPrefProperty &);
    YBoolPrefProperty &operator=(const YBoolPrefProperty &);
};

class YNumPrefProperty {
public:
    YNumPrefProperty(const char *domain, const char *name, long defval);
    ~YNumPrefProperty();

    long getNum() { if (fPref == 0) fetch(); return fNum; }
private:
    const char *fDomain;
    const char *fName;
    YPref *fPref;
    long fDefVal;
    long fNum;

    void fetch();
private: // not-used
    YNumPrefProperty(const YNumPrefProperty &);
    YNumPrefProperty &operator=(const YNumPrefProperty &);
};

class YStrPrefProperty {
public:
    YStrPrefProperty(const char *domain, const char *name, const char *defval);
    ~YStrPrefProperty();

    const char *getStr() { if (fStr == 0) fetch(); return fStr; }
private:
    const char *fDomain;
    const char *fName;
    YPref *fPref;
    const char *fDefVal;
    const char *fStr;

    void fetch();
private: // not-used
    YStrPrefProperty(const YStrPrefProperty &);
    YStrPrefProperty &operator=(const YStrPrefProperty &);
};

#endif
