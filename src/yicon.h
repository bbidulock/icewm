#ifndef YICON_H
#define YICON_H

class YIcon {
public:
    YIcon(char const *fileName);
    YIcon(YIconImage *small, YIconImage *large, YIconImage *huge);
    ~YIcon();

    YIconImage *huge();
    YIconImage *large();
    YIconImage *small();

    char const *iconName() const { return fPath; }

    static YIcon *getIcon(const char *name);
    static void freeIcons();
    bool isCached() { return fCached; }
    void setCached(bool cached) { fCached = cached; }

    static int smallSize();
    static int largeSize();
    static int hugeSize();

private:
    YIconImage *fSmall;
    YIconImage *fLarge;
    YIconImage *fHuge;

    bool loadedS;
    bool loadedL;
    bool loadedH;

    char *fPath;
    bool fCached;

    char *findIcon(char *base, unsigned size);
    char *findIcon(int size);
    void removeFromCache();
    static int cacheFind(const char *name);
    YIconImage *loadIcon(int size);
};

#endif
