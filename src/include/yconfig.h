#ifndef __YCONFIG_H
#define __YCONFIG_H

class YConfig {
public:
    YConfig(const char *name);
    ~YConfig();

    bool isCached() { return fCached; }
    void setCached(bool cached) { fCached = cached; }
private:
    char *fName;
    bool fCached;
};

class YBoolConfig: public YConfig {
public:
    YBoolConfig(const char *name): YConfig(name) {}
private:
    bool value;
};

#endif
