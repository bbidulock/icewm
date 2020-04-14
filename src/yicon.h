#ifndef YICON_H
#define YICON_H

class YIcon: public refcounted {
public:
    YIcon(upath fileName);
    YIcon(ref<YImage> small, ref<YImage> large, ref<YImage> huge);
    ~YIcon();

    ref<YImage> huge();
    ref<YImage> large();
    ref<YImage> small();

    ref<YImage> getScaledIcon(unsigned size);

    upath iconName() const { return fPath; }

    static ref<YIcon> getIcon(const char *name);
    static void freeIcons();
    bool isCached() { return fCached; }
    void setCached(bool cached) { fCached = cached; }

    static unsigned menuSize();
    static unsigned smallSize();
    static unsigned largeSize();
    static unsigned hugeSize();

    bool draw(Graphics &g, int x, int y, int size);
    upath findIcon(unsigned size);

private:
    ref<YImage> fSmall;
    ref<YImage> fLarge;
    ref<YImage> fHuge;

    bool loadedS;
    bool loadedL;
    bool loadedH;
    bool fCached = false;

    upath fPath;

    void removeFromCache();
    static int cacheFind(upath name);
    ref<YImage> loadIcon(unsigned size);
};

#endif

// vim: set sw=4 ts=4 et:
