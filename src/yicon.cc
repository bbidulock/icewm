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
#include "ywordexp.h"

#include "intl.h"

#include <fnmatch.h>

#include <vector>
#include <initializer_list>
#include <functional>
#include <set>

// place holder for scalable category, a size beyond normal limits
#define SCALABLE 9000

YIcon::YIcon(upath filename) :
        fSmall(null), fLarge(null), fHuge(null), loadedS(false), loadedL(false),
        loadedH(false), fCached(false), fPath(filename.expand())
{
    // don't attempt to load if icon is disabled
    if (fPath == "none" || fPath == "-")
        loadedS = loadedL = loadedH = true;
}

YIcon::YIcon(ref<YImage> small, ref<YImage> large, ref<YImage> huge) :
        fSmall(small), fLarge(large), fHuge(huge), loadedS(small != null),
        loadedL(large != null), loadedH(huge != null), fCached(false),
        fPath(null) {
}

YIcon::~YIcon() {
}

static const char iconExts[][5] = {
    ".png",
#if defined(CONFIG_GDK_PIXBUF_XLIB) && defined(CONFIG_LIBRSVG)
    ".svg",
#endif
    ".xpm",
};

static const mstring subcats[] = {
    "/apps", "/categories", "/places", "/devices", "/status",
};

static bool hasImageExtension(const upath& base) {
    auto baseExt(base.getExtension());
    if (baseExt.length() != 4)
        return false;
    for (auto& pe : iconExts) {
        if (baseExt.equals(pe, 4))
            return true;
    }
    return false;
}

// calls callback on all unique values until callback returns false
static void iterUniqueSizeRev(std::function<bool(unsigned)> f, unsigned toSkip) {
    // prefer bigger size, in case scaling is necessary
    unsigned consideredIconSizes[] = {
#if defined(CONFIG_GDK_PIXBUF_XLIB) && defined(CONFIG_LIBRSVG)
        unsigned(SCALABLE),
#endif
        hugeIconSize, largeIconSize, smallIconSize,
        menuIconSize,

        // last resort
        128,
        64,
        32,
    };
    static unsigned seen[10];
    static unsigned last;
    if (last == 0) {
        for (auto size : consideredIconSizes) {
            unsigned find = 0;
            while (find < last && size != seen[find]) {
                ++find;
            }
            if (find == last) {
                seen[last++] = size;
            }
        }
    }

    for (unsigned i = 0; i < last; ++i) {
        if (seen[i] != toSkip) {
            f(seen[i]);
        }
    }
}

class IconPathIndex {
public:
    struct IconCategory {
    public:
        struct entry {
            mstring path;
#ifdef SUPPORT_XDG_ICON_TYPE_CATEGORIES
            int itype; // actually a bit field from IconTypeFilter
#endif
        };
        std::vector<entry> folders;
        unsigned size;
        IconCategory(unsigned is) : size(is) {}
    };
    struct Pool {
        std::vector<IconCategory> categories;
        IconCategory anyCategory;
        Pool() : anyCategory(0) {
            iterUniqueSizeRev([&](unsigned is) {
                    categories.emplace_back(is); return true;
                }, -1);
        }
        IconCategory& getCat(unsigned is)
        {
            for (auto& it: categories)
                if (it.size == is)
                    return it;
            categories.emplace_back(is);
            return *categories.rbegin();
        }
    } pools[2]; // zero are resource folders, one is based on IconPath

    IconPathIndex() {
        std::set<mstring> dedupTestPath;
        auto add = [&dedupTestPath](IconCategory& cat,
                IconCategory::entry&& el) {

            if (dedupTestPath.insert(el.path).second) {
                MSG(("adding specific icon directory: %s", el.path.c_str()));
                cat.folders.emplace_back(std::move(el));
            }
        };

        auto probeAndRegisterXdgFolders = [this, &add](
                const mstring& iconPathToken, bool fromResources) -> unsigned {

            // stop early because this is obviously matching a file!
            if (hasImageExtension(upath(iconPathToken)))
                return 0;
            unsigned ret = 0;

            auto gotcha = [&](const mstring& testDir, IconCategory& cat) {
                if (!upath(testDir).dirExists())
                    return false;

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
#error FIXME: an enum value for "status"
                }
                add(cat, IconCategory::entry { testDir + "/",
                        flags });
#else
                add(cat, IconCategory::entry { testDir + "/" });
#endif
                return true;
            };
            // try the scalable version if can handle SVG
#if defined(CONFIG_GDK_PIXBUF_XLIB) && defined(CONFIG_LIBRSVG)
            auto& scaleCat = pools[fromResources].getCat(SCALABLE);
            for (auto& contentDir : subcats) {
                ret += gotcha(mstring(iconPathToken) + "/scalable" + contentDir,
                        scaleCat);
            }
#endif

            for (auto& kv : pools[fromResources].categories) {
                // special handling above
                if (kv.size == SCALABLE)
                    continue;

                mstring szSize(long(kv.size));

                for (auto& contentDir : subcats) {
                    for (auto& testDir : {
                        // XXX: optimize concatenation with MStringBuilder
                        mstring(iconPathToken) + "/" + szSize + "x" + szSize
                        + contentDir,
                        mstring(iconPathToken) + "/base/"
                        + szSize + "x" + szSize + contentDir,
                        // some old themes contain just one dimension
                        // and a different naming convention
                        mstring(iconPathToken) + contentDir + "/" + szSize
                    }) {
                        ret += gotcha(testDir, kv);
                    }
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
            // try base path in any case (later), for any icon type, loading
            // with the filename expansion scheme
            add(pool.anyCategory, IconCategory::entry {
                iPath + "/"
#ifdef SUPPORT_XDG_ICON_TYPE_CATEGORIES
                , YIcon::FOR_ANY_PURPOSE //| privFlag
#endif
            });

            for (auto& themeExprTok : matchlist) {
                // probe the folder like it was specified by user directly up to
                // the theme location and then also look for themes (by name)
                // underneath that folder

                unsigned nFoundForFolder = 0;

                for (auto themeExpr : { iPath, iPath + "/" + themeExprTok }) {

                    // were already XDG-like found by fishing in the simple
                    // attempt?
                    if (nFoundForFolder)
                        continue;

                    wordexp_t exp;
                    if (wordexp(themeExpr, &exp, WRDE_NOCMD) == 0) {
                        for (size_t i = 0; i < size_t(exp.we_wordc); ++i) {
                            auto match = exp.we_wordv[i];
                            // get theme name from folder base name
                            auto bname = strrchr(match, (unsigned) '/');
                            if (!bname)
                                continue;
                            bname++;
                            int keep = 1; // non-zero to consider it
                            for (auto& blistPattern : skiplist) {
                                keep = fnmatch(blistPattern, bname, 0);
                                if (!keep)
                                    break;
                            }

                            // found a potential theme folder to consider?
                            // does even the entry folder exist or is this a
                            // dead reference?
                            if (keep && upath(match).dirExists()) {

                                nFoundForFolder +=
                                        probeAndRegisterXdgFolders(match,
                                                fromResources);
                            }
                        }
                        wordfree(&exp);
                    }
                    else { // wordexp failed?
                        nFoundForFolder += probeAndRegisterXdgFolders(themeExpr,
                                fromResources);
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

    upath locateIcon(int size, mstring baseName, bool fromResources) {
        bool hasSuffix = hasImageExtension(baseName);
        auto& pool = pools[fromResources];
        upath res;

        // for compaction reasons, the lambdas return true on success,
        // but the success is only found in _this_ lambda only,
        // and this is the only one which touches `result`!
        auto checkFile = [&](upath path) {
            return path.fileExists() ? (res = path, true) : false;
        };
        auto checkFilesAtBasePath = [&](mstring basePath, unsigned size,
                bool addSizeSfx) {
            // XXX: optimize string concatenation? Or go back to plain printf?
            if (addSizeSfx) {
                basePath += "_";
                basePath += mstring(long(size)) + "x" + mstring(long(size));
            }
            for (auto& imgExt : iconExts) {
                if (checkFile(basePath + imgExt))
                    return true;
            }
            return false;
        };
        auto checkFilesInFolder = [&](const mstring& dirPath, unsigned size,
                bool addSizeSfx) {
            return checkFilesAtBasePath(dirPath + baseName, size, addSizeSfx);
        };
        auto smartScanFolder = [&](const mstring& folder, bool addSizeSfx,
                unsigned probeAllButThis = 0) {

            // full file name with suffix -> no further size/type extensions
            if (hasSuffix)
                return checkFile(folder + baseName);

            if (probeAllButThis) {
                iterUniqueSizeRev([&](unsigned is) {
                    return ! checkFilesInFolder(folder, is, addSizeSfx);
                }, probeAllButThis);
                return res != null;
            }
            return checkFilesInFolder(folder, size, addSizeSfx);
        };
        auto scanList = [&](IconCategory& cat, bool addSizeSfx,
                unsigned probeAllButThis = 0) {

            for (auto& folder : cat.folders) {
                if (smartScanFolder(folder.path, addSizeSfx, probeAllButThis))
                    return true;
            }
            return false;
        };

        if (upath(baseName).isAbsolute())
        {
            // subset of the resolution scheme to the one below
            // and applied to just one folder; shifting the parts to emulate
            // the same concatenation results
            auto what = baseName;
            baseName = mstring();
            return (smartScanFolder(what, false, false)
                    || hasSuffix
                    || smartScanFolder(what, true, 0)
                    || smartScanFolder(what, true, size)) ? res : null;
        }

        // Order of preferences:
        // in <size> without size-suffix
        // in any-folders with size-suffix
        // in all-sizes-excl.ours-excl.0, without suffix
        // in 0 with size-suffix like all-sizes-excl.ours-excl.0

        if (res == null)
            scanList(pool.getCat(size), false);
        // plain check in IconPath folders
        if (res == null && !hasSuffix)
            scanList(pool.anyCategory, true);
        if (res == null)
            scanList(pool.anyCategory, false);
        if (res == null) {
            iterUniqueSizeRev([&](unsigned is) {
                return !scanList(pool.getCat(is), false);
            }, size);
        }
        if (res == null && !hasSuffix)
            scanList(pool.anyCategory, true, size);
        return res;
    }
};
static lazy<IconPathIndex> iconIndex;

upath YIcon::findIcon(unsigned size) {
    // XXX: also differentiate between purpose (menu folder or program)

    // search in our resource paths, fallback to IconPath
    auto ret = iconIndex->locateIcon(size, fPath, true);
    if (ret == null) {
        ret = iconIndex->locateIcon(size, fPath, false);
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
        else {
            TLOG(("Icon not found: %s", fPath.string()));
        }

    }
    // if the image data which was found in the expected file does not really
    // match the filename, scale the data to fit
    if (icon != null) {
        if (size != icon->width() || size != icon->height()) {
            icon = icon->scale(size, size);
        }
    }

    return icon;
}


ref<YImage> YIcon::bestLoad(int size, ref<YImage>& img, bool& flag) {
    if (flag == false) {
        img = loadIcon(size);
        flag = true;
    }
    return img;
}

ref<YImage> YIcon::getScaledIcon(unsigned size) {
    if (size == smallSize() && (loadedS ? fSmall != null : small() != null))
        return fSmall;
    if (size == largeSize() && (loadedL ? fLarge != null : large() != null))
        return fLarge;
    if (size == hugeSize() && (loadedH ? fHuge != null : huge() != null))
        return fHuge;

    ref<YImage> base;
    if (size < smallSize() && (loadedS ? fSmall != null : small() != null))
        if ((base = fSmall->scale(size, size)) != null)
            return base;
    if (size < largeSize() && (loadedL ? fLarge != null : large() != null))
        if ((base = fLarge->scale(size, size)) != null)
            return base;
    if (size < hugeSize() && (loadedH ? fHuge != null : huge() != null))
        if ((base = fHuge->scale(size, size)) != null)
            return base;

    if ((base = loadIcon(size)) != null)
        return base;

    if (loadedH ? fHuge != null : huge() != null)
        if ((base = fHuge->scale(size, size)) != null)
            return base;
    if (loadedL ? fLarge != null : large() != null)
        if ((base = fLarge->scale(size, size)) != null)
            return base;
    if (loadedS ? fSmall != null : small() != null)
        if ((base = fSmall->scale(size, size)) != null)
            return base;

    return null;
}

static YRefArray<YIcon> iconCache;

void YIcon::removeFromCache() {
    int n = iconCache.find(this);
    if (n >= 0) {
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
    iconCache.clear();
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

bool YIcon::draw(Graphics& g, int x, int y, int size) {
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
