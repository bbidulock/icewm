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

    YPref *fNext;
    YPrefListener *fListener;
    YCachedPref *fPref;
};

#endif
