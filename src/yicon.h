#ifndef YICON_H
#define YICON_H

#include "ypaint.h"
#include "ypixbuf.h"

class YIcon {
public:
    YIcon(char const *fileName);
    YIcon(ref<YIconImage> small, ref<YIconImage> large, ref<YIconImage> huge);
    ~YIcon();

    ref<YIconImage> huge();
    ref<YIconImage> large();
    ref<YIconImage> small();

    ref<YIconImage> getScaledIcon(int size);

    char const *iconName() const { return fPath; }

    static YIcon *getIcon(const char *name);
    static void freeIcons();
    bool isCached() { return fCached; }
    void setCached(bool cached) { fCached = cached; }

    static int smallSize();
    static int largeSize();
    static int hugeSize();

private:
    ref<YIconImage> fSmall;
    ref<YIconImage> fLarge;
    ref<YIconImage> fHuge;

    bool loadedS;
    bool loadedL;
    bool loadedH;

    char *fPath;
    bool fCached;

    char *findIcon(char *base, unsigned size);
    char *findIcon(int size);
    void removeFromCache();
    static int cacheFind(const char *name);
    ref<YIconImage> loadIcon(int size);
};

#endif
