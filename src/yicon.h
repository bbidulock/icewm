#ifndef YICON_H
#define YICON_H

#include "ypaint.h"
#include "ypixbuf.h"
#include "upath.h"

class YIcon {
public:
    YIcon(upath fileName);
    YIcon(ref<YIconImage> small, ref<YIconImage> large, ref<YIconImage> huge);
    ~YIcon();

    ref<YIconImage> huge();
    ref<YIconImage> large();
    ref<YIconImage> small();

    ref<YIconImage> getScaledIcon(int size);

    upath iconName() const { return fPath; }

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

    upath fPath;
    bool fCached;

    static upath findIcon(char *base, unsigned size);
    upath findIcon(int size);
    void removeFromCache();
    static int cacheFind(upath name);
    ref<YIconImage> loadIcon(int size);
};

#endif
