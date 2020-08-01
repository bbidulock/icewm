#ifndef YICON_H
#define YICON_H

class YIcon: public refcounted {
public:
    YIcon(upath fileName);
    YIcon(ref<YImage> small, ref<YImage> large, ref<YImage> huge);
    ~YIcon();

    ref<YImage> huge() { return bestLoad(hugeSize(), fHuge, loadedH); }
    ref<YImage> large() { return bestLoad(largeSize(), fLarge, loadedL); }
    ref<YImage> small() { return bestLoad(smallSize(), fSmall, loadedS); }


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

#ifdef SUPPORT_XDG_ICON_TYPE_CATEGORIES
    enum /* class... or better not, simplify! */ TypeFilter {
        NONE = 0,
        /** Suitable for programs */
        FOR_APPS = 1,
        /** Suitable for menu folders */
        FOR_MENUCATS = 2,
        FOR_PLACES = 4,
        FOR_DEVICES = 8,

        FOR_ANY_PURPOSE = FOR_APPS | FOR_DEVICES | FOR_MENUCATS | FOR_APPS,
        ALL = FOR_ANY_PURPOSE // | FROM_ANY_SOURCE
    };
#endif

private:
    ref<YImage> fSmall;
    ref<YImage> fLarge;
    ref<YImage> fHuge;

    bool loadedS;
    bool loadedL;
    bool loadedH;
    bool fCached;

    upath fPath;

    ref<YImage> bestLoad(int size, ref<YImage>& img, bool& flag);

    void removeFromCache();
    static int cacheFind(upath name);
    ref<YImage> loadIcon(unsigned size);
};

#endif

// vim: set sw=4 ts=4 et:
