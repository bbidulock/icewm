/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ypaint.h"
#include "yicon.h"
#include "yapp.h"
#include "sysdep.h"
#include "prefs.h"
#include "yprefs.h"
#include "ypaths.h"
#include "ypointer.h"
#include <wordexp.h>

#include "intl.h"

#include <fnmatch.h>

#include <vector>
#include <array>
#include <initializer_list>

YIcon::YIcon(upath filename) :
        fSmall(null), fLarge(null), fHuge(null), loadedS(false), loadedL(false),
        loadedH(false), fCached(false), fPath(filename.expand()) {
}

YIcon::YIcon(ref<YImage> small, ref<YImage> large, ref<YImage> huge) :
        fSmall(small), fLarge(large), fHuge(huge), loadedS(small != null),
        loadedL(large != null), loadedH(huge != null), fCached(false),
        fPath(null) {
}

YIcon::~YIcon() {
    fHuge = null;
    fLarge = null;
    fSmall = null;
}

static const char iconExts[][5] = { ".png",
#if defined(CONFIG_GDK_PIXBUF_XLIB) && defined(CONFIG_LIBRSVG)
        ".svg",
#endif
        ".xpm" };

inline bool HasImageExtension(const upath &base) {
    const auto baseExt(base.getExtension());
    if (baseExt.length() != 4)
        return false;
    for (const auto &pe : iconExts) {
        if (baseExt.equals(pe, 4))
            return true;
    }
    return false;
}

class ZIconPathIndex {

public:

    struct IconCategory {
    private:
        mutable std::vector<mstring> suffixCache;
        unsigned sizetype = 0;

    public:
        unsigned getSize() const {
            return sizetype;
        }
        IconCategory(unsigned size = 0) :
                sizetype(size) {
        }
        struct entry {
            mstring path;
            int itype; // actually a bit field from IconTypeFilter
        };
        const std::vector<mstring>& getExtendedSuffixes() const;

        std::vector<entry> folders;
    };

    std::vector<IconCategory> categories;
    // catch all folders without sizetyped subdirs
    IconCategory legacyDirs;

    IconCategory& getCat(unsigned size) {
        for (auto &cat : categories) {
            if (cat.getSize() == size)
                return cat;
        }
        return legacyDirs;
    }

    std::array<IconCategory*,2> getCatAndDefault(unsigned size) {
        for (auto &cat : categories) {
            if (cat.getSize() == size)
                return {&cat, &legacyDirs};
        }
        return {&legacyDirs, nullptr};
    }

    void init() {

        static bool once = false;
        if (once)
            return;
        once = true;

        // add unique(!) category
        for (auto size : { hugeIconSize, largeIconSize, smallIconSize,
                menuIconSize }) {

            if (&legacyDirs == &(getCat(size))) {
                categories.emplace_back(IconCategory(size));
            }
        }

        auto probeAndRegisterXdgFolders = [this](const mstring &what,
                YIcon::TypeFilter privFlag) {

            // stop early because this is obviously matching a file!
            if (HasImageExtension(upath(what)))
                return;

            for (auto &cat : categories) {
                mstring szSize(long(cat.getSize()));
                // assume that if the folder is there for any usable role then it wil be ok to search for icons
                for (const auto &contentDir : { "/apps", "/categories" }) {
                    auto testDir = mstring(what) + "/" + szSize + "x" + szSize
                            + contentDir;
                    if (upath(testDir).dirExists()) {
                        // finally!
                        auto flags = privFlag | (contentDir[1] == 'a' ?
                                        YIcon::FOR_APP : YIcon::FOR_DIR);
                        cat.folders.emplace_back(IconCategory::entry {
                            testDir + "/", flags });
                    }
                }
            }
        };

        std::vector<const char*> skiplist, matchlist;

        char *save = nullptr;
        csmart themesCopy(newstr(iconThemes));
        for (auto *tok = strtok_r(themesCopy, ":", &save); tok;
                tok = strtok_r(0, ":", &save)) {
            if (tok[0] == '-')
                skiplist.emplace_back(tok + 1);
            else
                matchlist.emplace_back(tok);
        }

        auto probeIconFolder = [&](mstring iPath, YIcon::TypeFilter privFlag) {

            iPath += "/";

            // try base path in any case (later), for any icon type
            legacyDirs.folders.emplace_back(IconCategory::entry {
                iPath,
                YIcon::FOR_ANY_PURPOSE | privFlag
            });

            for (const auto &themeExprTok : matchlist) {
                mstring themeExpr = iPath + themeExprTok;
                wordexp_t exp;
                if (wordexp(themeExpr, &exp, WRDE_NOCMD) == 0) {
                    for (unsigned i = 0; i < exp.we_wordc; ++i) {
                        auto match = exp.we_wordv[i];
                        // get theme name from folder base name
                        auto bname = strrchr(match, '/');
                        if (!bname)
                            continue;
                        bname++;
                        int keep = 1; // non-zero to consider it
                        for (const auto &blistPattern : skiplist) {
                            keep = fnmatch(blistPattern, bname, 0);
                            if (!keep)
                                break;
                        }
                        // found a potential theme folder to consider?
                        if (keep)
                            probeAndRegisterXdgFolders(match, privFlag);
                    }
                    wordfree(&exp);
                } else { // wordexp failed?
                    probeAndRegisterXdgFolders(themeExpr, privFlag);
                }
            }
        };

        // first scan the private resource folders
        auto iceIconPaths = YResourcePaths::subdirs("icons");
        if (iceIconPaths != null) {
            // this returned icewm directories containing "icons" folder
            for (int i = 0; i < iceIconPaths->getCount(); ++i) {
                probeIconFolder(iceIconPaths->getPath(i) + "/icons",
                        YIcon::FROM_RES);
            }
        }
        // now test the system icons folders specified by user or defaults
        csmart copy(newstr(iconPath));
        for (auto *itok = strtok_r(copy, ":", &save); itok;
                itok = strtok_r(0, ":", &save)) {

            probeIconFolder(itok, YIcon::FROM_PATH);
        }

    }

    upath locateIcon(int size, const mstring &baseName, unsigned filter) {
        bool hasSuffix = HasImageExtension(baseName);
        for (auto pCat : getCatAndDefault(size)) {

            if (!pCat)
                continue;

            for (const auto &el : pCat->folders) {

                if (0 == (filter & el.itype))
                    continue;

                // XXX: optimize string concatenation?
                mstring path(el.path + baseName);
                if (hasSuffix) {
                    upath testPath(path);
                    if (testPath.fileExists())
                        return testPath;
                } else {
                    for (const mstring &sfx : pCat->getExtendedSuffixes()) {
                        upath testPath(path + sfx);
                        //printf("probing %s\n", testPath.string());
                        //bool dbgCheck = 0 == strcmp("/usr/share/icons/hicolor/32x32/apps/firefox.png", testPath.string());
                        if (testPath.fileExists())
                            return testPath;
                    }
                }
            }
        }
        return null;
    }
} iconIndex;

const std::vector<mstring>& ZIconPathIndex::IconCategory::getExtendedSuffixes()
const {
    if (!suffixCache.empty())
        return suffixCache;
    // if untyped folder, try size-specific suffixes first
    if (sizetype == 0) {
        for (auto cat : iconIndex.categories) {
            mstring sDim(long(cat.getSize()));
            for (const auto &ex : iconExts)
                suffixCache.emplace_back(mstring("_") + sDim + "x" + sDim + ex);
        }
    }
    for (const auto &sfx : iconExts)
        suffixCache.emplace_back(sfx);
    return suffixCache;
}

upath YIcon::findIcon(unsigned size) {

    iconIndex.init();
    // XXX: also differentiate between purpose (menu folder or program)
    auto ret = iconIndex.locateIcon(size, fPath, FROM_RES);
    if (ret == null) {
        ret = iconIndex.locateIcon(size, fPath, FROM_PATH);
    }
    if (ret == null) {
        MSG(("Icon \"%s\" not found.", fPath.string()));
    }

    return ret;
}

ref<YImage> YIcon::loadIcon(unsigned size) {
    ref<YImage> icon;

    if (fPath != null) {
        upath loadPath;

        if (fPath.isAbsolute() && fPath.fileExists()) {
            loadPath = fPath;
        } else {
            loadPath = findIcon(size);
        }
        if (loadPath != null) {
            auto cs(loadPath.path());
            YTraceIcon trace(cs);
            icon = YImage::load(cs);
        }
    }
    // if the image data which was found in the expected file does not really match the filename, scale the data to fit
    if (icon != null) {
        if (size != icon->width() || size != icon->height()) {
            icon = icon->scale(size, size);
        }
    }

    return icon;
}

ref<YImage> YIcon::huge() {
    if (fHuge == null && !loadedH) {
        fHuge = loadIcon(hugeSize());
        loadedH = true;

        if (fHuge == null && large() != null)
            fHuge = large()->scale(hugeSize(), hugeSize());

        if (fHuge == null && small() != null)
            fHuge = small()->scale(hugeSize(), hugeSize());
    }

    return fHuge;
}

ref<YImage> YIcon::large() {
    if (fLarge == null && !loadedL) {
        fLarge = loadIcon(largeSize());
        loadedL = true;

        if (fLarge == null && huge() != null)
            fLarge = huge()->scale(largeSize(), largeSize());

        if (fLarge == null && small() != null)
            fLarge = small()->scale(largeSize(), largeSize());
    }

    return fLarge;
}

ref<YImage> YIcon::small() {
    if (fSmall == null && !loadedS) {
        fSmall = loadIcon(smallSize());
        loadedS = true;

        if (fSmall == null && large() != null)
            fSmall = large()->scale(smallSize(), smallSize());
        if (fSmall == null && huge() != null)
            fSmall = huge()->scale(smallSize(), smallSize());
    }

    return fSmall;
}

ref<YImage> YIcon::getScaledIcon(unsigned size) {
    ref<YImage> base;

    if (size == smallSize())
        base = small();
    else if (size == largeSize())
        base = large();
    else if (size == hugeSize())
        base = huge();

    if (base == null) {
        base = huge() != null ? fHuge : large() != null ? fLarge : small();
    }

    if (base != null) {
        if (size != base->width() || size != base->height()) {
            base = base->scale(size, size);
        }
    }
    return base;
}

static YRefArray<YIcon> iconCache;

void YIcon::removeFromCache() {
    int n = cacheFind(iconName());
    if (n >= 0) {
        fPath = null;
        iconCache.remove(n);
    }
}

int YIcon::cacheFind(upath name) {
    int l, r, m;

    l = 0;
    r = iconCache.getCount();
    while (l < r) {
        m = (l + r) / 2;
        ref<YIcon> found = iconCache.getItem(m);
        int cmp = name.path().compareTo(found->iconName().path());
        if (cmp == 0) {
            return m;
        } else if (cmp < 0)
            r = m;
        else
            l = m + 1;
    }
    return -(l + 1);
}

ref<YIcon> YIcon::getIcon(const char *name) {
    int n = cacheFind(name);
    if (n >= 0)
        return iconCache.getItem(n);

    ref<YIcon> newicon(new YIcon(name));
    if (newicon != null) {
        newicon->setCached(true);
        iconCache.insert(-n - 1, newicon);
    }
    return newicon;
}

void YIcon::freeIcons() {
    for (int k = iconCache.getCount(); --k >= 0;) {
        ref<YIcon> icon = iconCache.getItem(k);
        icon->fPath = null;
        icon->fSmall = null;
        icon->fLarge = null;
        icon->fHuge = null;
        icon = null;
        iconCache.remove(k);
    }
}

unsigned YIcon::menuSize() {
    return menuIconSize;
}

unsigned YIcon::smallSize() {
    return smallIconSize;
}

unsigned YIcon::largeSize() {
    return largeIconSize;
}

unsigned YIcon::hugeSize() {
    return hugeIconSize;
}

bool YIcon::draw(Graphics &g, int x, int y, int size) {
    ref<YImage> image = getScaledIcon(size);
    if (image != null) {
        if (!doubleBuffer) {
            g.drawImage(image, x, y);
        } else {
            g.compositeImage(image, 0, 0, size, size, x, y);
        }
        return true;
    }
    return false;
}

// vim: set sw=4 ts=4 et:
