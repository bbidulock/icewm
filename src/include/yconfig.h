#ifndef __YCONFIG_H
#define __YCONFIG_H

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
};

class YPixmap;

class YPixmapPrefProperty {
public:
    YPixmapPrefProperty(const char *domain, const char *name, const char *defval, const char *libdir);
    ~YPixmapPrefProperty();

    YPixmap *getPixmap() { if (fPixmap == 0) fetch(); return fPixmap; }
    YPixmap *tiledPixmap(bool horizontal); // a kind of hack? !!!
private:
    const char *fDomain;
    const char *fName;
    YPref *fPref;
    const char *fDefVal;
    const char *fLibDir;
    YPixmap *fPixmap;
    bool fDidTile;

    void fetch();

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
};

#endif
