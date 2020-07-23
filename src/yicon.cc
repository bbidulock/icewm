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
#include <set>
#include <map>

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
// XXX: this can be expressed better with constexpr but C++11 did not support complex
// constexpr functions yet! Maybe should be replaced with a static array and some preprocessor
// hackery checking for the same values.
static const std::set<unsigned> dedupSizes = {hugeIconSize, largeIconSize, smallIconSize, menuIconSize};

class ZIconPathIndex {

public:

    struct IconCategory {
    private:
        mutable std::vector<mstring> suffixCache;

    public:
        struct entry {
            mstring path;
#ifdef SUPPORT_XDG_ICON_TYPE_CATEGORIES
            int itype; // actually a bit field from IconTypeFilter
#endif
        };

        std::vector<entry> folders;
    };
    struct Pool {
        std::map<unsigned, IconCategory> categories;
        Pool() {
            // add unique(!) category
            for (auto size : dedupSizes)
                categories[size] = IconCategory();
            categories[0] = IconCategory();
        }
    } pools[2]; // zero are resource folders, one is based on IconPath


    void init() {

        static bool once = false;
        if (once)
            return;
        once = true;
        std::set<mstring> dedupTestPath;
        auto add = [&dedupTestPath](IconCategory& cat, IconCategory::entry&& el) {
            if(dedupTestPath.insert(el.path).second) cat.folders.emplace_back(std::move(el));
        };

        auto probeAndRegisterXdgFolders = [this, &add](const mstring &what,
                bool fromResources) -> unsigned {

            // stop early because this is obviously matching a file!
            if (HasImageExtension(upath(what)))
                return 0;
            unsigned ret = 0;
            for (auto &kv : pools[fromResources].categories) {
                auto& cat = kv.second;
                mstring szSize(long(kv.first));

                for (const auto &contentDir : { "/apps", "/categories", "/places", "/devices" }) {
                    for (const auto &testDir : {

                            mstring(what) + "/" + szSize + "x" + szSize + contentDir,
                            mstring(what) + "/base/" + szSize + "x" + szSize + contentDir,

                            // some old themes contain just one dimension and inverted order
                            mstring(what) + contentDir + "/" + szSize
                    }) {
                        if (upath(testDir).dirExists()) {
                            // finally!
#ifdef SUPPORT_XDG_ICON_TYPE_CATEGORIES
                            int flags = 0;
                            switch (contentDir[1]) {
                            case 'a':
                                flags |= YIcon::FOR_APPS;
                                break;
                            case 'p':
                                flags |= YIcon::FOR_PLACES;
                                break;
                            case 'd':
                                flags |= YIcon::FOR_DEVICES;
                                break;
                            case 'c':
                                flags |= YIcon::FOR_MENUCATS;
                                break;
                            }
                            add(cat, IconCategory::entry { testDir + "/", flags });
#else
                            add(cat, IconCategory::entry { testDir + "/"});
#endif
                            ret++;
                            goto FOUND_FOR_SIZE_AND_PURPOSE;
                        }
                    }

                    FOUND_FOR_SIZE_AND_PURPOSE: ;
                }
            }
            return ret;
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

        auto probeIconFolder = [&](mstring iPath, bool fromResources) {

            auto& pool = pools[fromResources];
            // try base path in any case (later), for any icon type, loading with the filename expansion scheme
            add(pool.categories[0], IconCategory::entry {
                iPath + "/"
#ifdef SUPPORT_XDG_ICON_TYPE_CATEGORIES
                , YIcon::FOR_ANY_PURPOSE //| privFlag
#endif
            });

            for (const auto &themeExprTok : matchlist) {
                // probe the folder like it was specified by user directly up to the theme location
                // and then also look for themes (by name) underneath that folder

                unsigned nFoundForFolder = 0;

                for (auto themeExpr : { iPath, iPath + "/" + themeExprTok }) {

                    // were already XDG-like found by fishing in the simple attempt?
                    if(nFoundForFolder)
                        continue;

                    wordexp_t exp;
                    if (wordexp(themeExpr, &exp, WRDE_NOCMD) == 0) {
                        for (unsigned i = 0; i < exp.we_wordc; ++i) {
                            auto match = exp.we_wordv[i];
                            // get theme name from folder base name
                            auto bname = strrchr(match, (unsigned) '/');
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
                            // does even the entry folder exist or is this a dead reference?
                            if (keep && upath(match).dirExists())
                                nFoundForFolder += probeAndRegisterXdgFolders(match, fromResources);
                        }
                        wordfree(&exp);
                    } else { // wordexp failed?
                        nFoundForFolder += probeAndRegisterXdgFolders(themeExpr, fromResources);
                    }
                }
            }
        };

        // first scan the private resource folders
        auto iceIconPaths = YResourcePaths::subdirs("icons");
        if (iceIconPaths != null) {
            // this returned icewm directories containing "icons" folder
            for (int i = 0; i < iceIconPaths->getCount(); ++i) {
                probeIconFolder(iceIconPaths->getPath(i) + "/icons",
                        true);
            }
        }
        // now test the system icons folders specified by user or defaults
        csmart copy(newstr(iconPath));
        for (auto *itok = strtok_r(copy, ":", &save); itok;
                itok = strtok_r(0, ":", &save)) {

            probeIconFolder(itok, false);
        }
    }

    upath locateIcon(int size, const mstring &baseName, bool fromResources) {
        bool hasSuffix = HasImageExtension(baseName);
        auto &pool = pools[fromResources];
        upath result = null;

        auto checkFile = [&](const mstring &path) {
            upath testPath(path);
            if (testPath.fileExists())
                result = testPath;
        };
        auto checkFilesInFolder = [&](const mstring &dirPath, unsigned size, bool addSizeSfx) {
            // XXX: optimize string concatenation? Or go back to plain printf?
            mstring imgPath(dirPath + baseName);
            if (addSizeSfx) {
                imgPath += "_";
                imgPath += mstring(long(size)) + "x" + mstring(long(size));
            }
            for (const auto &imgExt : iconExts) {
                checkFile(imgPath + imgExt);
                if (result != null)
                    break;
            }
        };
        auto scanList = [&](IconCategory &cat, bool addSizeSfx, unsigned probeAllButThis = 0) {
            for (const auto &el : cat.folders) {

                if (hasSuffix) {
                    checkFile(el.path + baseName);
                    if (result != null)
                        break;
                }

                // starting with bigger size so we get those for scaling first, in case that becomes needed
                if (probeAllButThis) {
                    for (auto it = dedupSizes.rend(); it != dedupSizes.rbegin(); it++) {
                        if (int(*it) == size)
                            continue;
                        checkFilesInFolder(el.path, *it, addSizeSfx);
                        if (result != null)
                            break;
                    }
                } else
                    checkFilesInFolder(el.path, size, addSizeSfx);

                if (result != null)
                    break;
            }
        };


        // Order of preferences:
        // in <size> without size-suffix
        // in 0 with size-suffix
        // in all-sizes-excl.ours-excl.0, without suffix
        // in 0 with size-suffix like all-sizes-excl.ours-excl.0
        //
        // <RANT> actually we need generator functions which C++ doesn't have

        if (result == null)
            scanList(pool.categories[size], false);
        if (result == null && !hasSuffix)
            scanList(pool.categories[0], true);
        if (result == null) {
            for (auto testSize : dedupSizes) {
                if (int(testSize) != size)
                    scanList(pool.categories[testSize], false);
                if (result != null)
                    break;
            }
        }
        if (result == null && !hasSuffix)
            scanList(pool.categories[0], true, size);
        return result;
    }
} iconIndex;


upath YIcon::findIcon(unsigned size) {

    iconIndex.init();
    // XXX: also differentiate between purpose (menu folder or program)
    auto ret = iconIndex.locateIcon(size, fPath, true);
    if (ret == null) {
        ret = iconIndex.locateIcon(size, fPath, false);
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
