#ifndef __YCONFIG_H
#define __YCONFIG_H

class YPref;
class YCachedPref;

class YPrefListener {
public:
    virtual void prefChanged(YPref *pref) = 0;
};

class YPref {
public:
    YPref(const char *domain, const char *name, YPrefListener *listener = 0);
    ~YPref();

    const char *getName();
    const char *getValue();

    long getNum(long defValue);
    bool getBool(bool defValue);
    const char *getStr(const char *defValue);
    //void setValue(const char *value);
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
    const char *fDefVal;
    YPref *fPref;
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
    const char *fDefVal;
    YPref *fPref;
    YFont *fFont;

    void fetch();
};

#endif
