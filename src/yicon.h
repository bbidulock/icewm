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

    enum /* class... or better not, simplify! */ TypeFilter {
        NONE = 0,
        /** Suitable for programs */
        FOR_APP = 1,
        /** Suitable for menu folders */
        FOR_DIR = 2,
        /** Located in IceWM private icon folders */
        FROM_RES = 4,
        /** Located in icon path */
        FROM_PATH = 8,

        FOR_ANY_PURPOSE = FOR_APP | FOR_DIR,
        FROM_ANY_SOURCE = FROM_RES | FROM_RES,
        ALL = FOR_ANY_PURPOSE | FROM_ANY_SOURCE
    };

private:
    ref<YImage> fSmall;
    ref<YImage> fLarge;
    ref<YImage> fHuge;

    bool loadedS;
    bool loadedL;
    bool loadedH;
    bool fCached;

    upath fPath;

    void removeFromCache();
    static int cacheFind(upath name);
    ref<YImage> loadIcon(unsigned size);
};

#endif

// vim: set sw=4 ts=4 et:
