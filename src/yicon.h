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

    static class IResourceLocator* iconResourceLocator;
    static ref<YIcon> getIcon(const char *name);
    static void freeIcons();
    bool isCached() const { return fCached; }
    void setCached(bool cached) { fCached = cached; }

    static unsigned menuSize() {
        extern unsigned menuIconSize;
        return menuIconSize; }
    static unsigned smallSize() {
        extern unsigned smallIconSize;
        return smallIconSize; }
    static unsigned largeSize() {
        extern unsigned largeIconSize;
        return largeIconSize; }
    static unsigned hugeSize() {
        extern unsigned hugeIconSize;
        return hugeIconSize; }
    static void fixIconSizes();
    static unsigned fixIconSize(unsigned size);

    bool draw(Graphics &g, int x, int y, unsigned size);
    upath findIcon(unsigned size);
    static bool supportSVG();

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
    Picture pSmall;
    Picture pLarge;
    Picture pHuge;
    Picture pOther;
    unsigned otherSize;

    bool loadedS;
    bool loadedL;
    bool loadedH;
    bool fCached;

    upath fPath;

    void removeFromCache();
    static int cacheFind(upath name);
    ref<YImage> loadIcon(unsigned size);

    static bool fSupportSVG;
    static bool fExamineSVG;
};

#endif

// vim: set sw=4 ts=4 et:
