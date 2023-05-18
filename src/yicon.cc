/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ypaint.h"
#include "yicon.h"
#include "prefs.h"
#include "yprefs.h"
#include "yxapp.h"
#include "ypointer.h"
#include "ywordexp.h"
#include "ascii.h"
#include <algorithm>
#include <fnmatch.h>
#include <dirent.h>
#include "intl.h"

// place holder for scalable category, a size beyond normal limits
#define SCALABLE 9000U

IResourceLocator* YIcon::iconResourceLocator;

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
    ".xpm",
#ifdef ICE_SUPPORT_SVG
    ".svg",
#endif
};

static const char subcats[][12] = {
    "actions", "apps", "categories",
    "devices",
    // "emblems",
    // "emotes",
    // "mimetypes",
    "places",
    "status",
    // "ui",
    // "intl",
    "legacy",
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

class UniqueSizes {
public:
    UniqueSizes() : last(0) {
        *this + hugeIconSize
            + largeIconSize
            + smallIconSize
            + menuIconSize

            // last resort
            + 128
            + 64
            + 32

#ifdef ICE_SUPPORT_SVG
            + SCALABLE
#endif
            ;
    }
    bool have(unsigned size) {
        for (unsigned k : *this)
            if (k == size)
                return true;
        return false;
    }
    const unsigned* begin() const { return seen; }
    const unsigned* end() const { return seen + last; }

private:
    static const int atmost = 8;
    unsigned seen[atmost];
    unsigned last;

    UniqueSizes& operator+(unsigned size) {
        if (have(size) == false && last < atmost)
            seen[last++] = size;
        return *this;
    }
};

struct IconCategory {
public:
    MStringArray folders;
    IconCategory(unsigned is) : size(is) {}
    const unsigned size;
};

struct CategoryPool {
    YObjectArray<IconCategory> categories;

    IconCategory anyCategory;
    UniqueSizes uniqueSizes;

    CategoryPool() : anyCategory(0) {
        for (unsigned size : uniqueSizes)
            categories.append(new IconCategory(size));
    }

    bool haveCat(unsigned size) {
        for (auto it: categories)
            if (it->size == size)
                return true;
        return false;
    }

    IconCategory& getCat(unsigned size) {
        for (auto it: categories)
            if (it->size == size)
                return *it;

        IconCategory* cat = new IconCategory(size);
        categories += cat;
        return *cat;
    }
};

class IconDirectory {
private:
    YStringArray names;
public:
    IconDirectory(const char* dirnam) {
        fill(dirnam);
    }
    operator bool() const { return names.nonempty(); }
    const char* const* begin() const { return names.begin(); }
    const char* const* end() const { return names.end(); }
private:
    void fill(const char* dirnam) {
        DIR* dir = opendir(dirnam);
        if (dir) {
            scan(dir);
            closedir(dir);
        }
    }
    void scan(DIR* dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            const char* nam = ent->d_name;
            if (strchr(nam, '.'))
                continue;
#ifdef DT_DIR
            if (ent->d_type && ent->d_type != DT_DIR)
                continue;
#endif
            if (ASCII::isDigit(nam[0])) {
                int i = 1;
                while (nam[i] && (ASCII::isDigit(nam[i]) || nam[i] == 'x'))
                    ++i;
                if (nam[i] == '\0')
                    names += nam;
            }
            else if (ASCII::isLower(nam[0])) {
                int i = 1;
                while (nam[i] && ASCII::isLower(nam[i]))
                    ++i;
                if (nam[i] == '\0')
                    names += nam;
            }
        }
    }
};

class IconPathIndex {
private:
    // pool zero are resource folders,
    // pool one is based on IconPath.
    struct CategoryPool pools[2];
    MStringArray dedupTestPath;

    bool addPath(const mstring& testDir, IconCategory& cat) {
        mstring path(testDir + "/");
        if (find(dedupTestPath, path) < 0) {
            MSG(("adding specific icon directory: %s", path.c_str()));
            dedupTestPath.append(path);
            cat.folders.append(path);
            return true;
        }
        return false;
    };

    unsigned probeAndRegisterXdgFolders(mstring iconPathToken,
                                        bool fromResources)
    {
        unsigned ret = 0;
        IconDirectory entries(iconPathToken);
        if (entries == false)
            return ret;

        for (const char* entry : entries) {
            // try the scalable version if it can handle SVG
            if (strcmp(entry, "scalable") == 0) {
#ifdef ICE_SUPPORT_SVG
                auto& scaleCat = pools[fromResources].getCat(SCALABLE);
                for (const char* sub : subcats) {
                    mstring path(iconPathToken, "/scalable/", sub);
                    if (upath(path).dirExists()) {
                        ret += addPath(path, scaleCat);
                    }
                }
#endif
            }
            else if (strcmp(entry, "base") == 0) {
                mstring basePath(iconPathToken + "/base");
                IconDirectory baseDir(basePath);
                for (const char* base : baseDir) {
                    unsigned size1 = 0, size2 = 0;
                    if (ASCII::isDigit(base[0]) &&
                        sscanf(base, "%ux%u", &size1, &size2) == 2 &&
                        size1 == size2 &&
                        pools[fromResources].haveCat(size1))
                    {
                        IconCategory& cat(pools[fromResources].getCat(size1));
                        mstring testPath(basePath, "/", base);
                        for (const char* sub : subcats) {
                            mstring subPath(testPath, "/", sub);
                            if (upath(subPath).dirExists()) {
                                ret += addPath(subPath, cat);
                            }
                        }
                    }
                }
            }
            else if (ASCII::isDigit(entry[0])) {
                unsigned size1 = 0, size2 = 0;
                if (sscanf(entry, "%ux%u", &size1, &size2) == 2 &&
                    size1 == size2 &&
                    pools[fromResources].haveCat(size1))
                {
                    IconCategory& cat(pools[fromResources].getCat(size1));
                    mstring testPath(iconPathToken, "/", entry);
                    for (const char* sub : subcats) {
                        mstring subPath(testPath, "/", sub);
                        if (upath(subPath).dirExists()) {
                            ret += addPath(subPath, cat);
                        }
                    }
                }
            }
            else if (std::find_if(subcats, subcats + ACOUNT(subcats),
                     [entry](const char* p) { return strcmp(entry, p) == 0; }))
            {
                mstring subcatPath(iconPathToken, "/", entry);
                IconDirectory subDir(subcatPath);
                for (const char* sizeStr : subDir) {
                    char* end = nullptr;
                    long snum = strtol(sizeStr, &end, 10);
                    unsigned size = unsigned(snum);
                    if (0 < snum && *end == '\0' &&
                        pools[fromResources].haveCat(size))
                    {
                        IconCategory& cat(pools[fromResources].getCat(size));
                        mstring sizePath(subcatPath, "/", sizeStr);
                        ret += addPath(sizePath, cat);
                    }
                }
            }
        }

        return ret;
    }

    YArray<const char*> skiplist;
    YArray<const char*> matchlist;

    bool matches(const char* str) {
        for (auto pattern : matchlist)
            if (fnmatch(pattern, str, 0) == 0)
                return true;
        return false;
    }

    bool skipped(const char* str) {
        for (auto pattern : skiplist)
            if (fnmatch(pattern, str, 0) == 0)
                return true;
        return false;
    }

    void probeIconFolder(mstring iPath, bool fromResources) {

        auto& pool = pools[fromResources];

        // try base path in any case (later), for any icon type, loading
        // with the filename expansion scheme
        if (addPath(iPath, pool.anyCategory) == false) {
            return;
        }

        DIR* dir = opendir(iPath);
        if (dir) {
            for (struct dirent* ent; (ent = readdir(dir)) != nullptr; ) {
                const char* nam = ent->d_name;
                if (*nam == '.')
                    continue;
#ifdef DT_DIR
                if (ent->d_type && ent->d_type != DT_DIR)
                    continue;
#endif
                if (matches(nam) && !skipped(nam) && strcmp(nam, "base")) {
                    mstring mstr(iPath, "/", nam);
                    probeAndRegisterXdgFolders(mstr, fromResources);
                }
            }
            closedir(dir);
        }
    }

public:
    IconPathIndex() {
        csmart themesCopy(newstr(iconThemes));
        for (tokens theme(themesCopy, ":"); theme; ++theme) {
            if (theme[0] == '-')
                skiplist.append(theme + 1);
            else
                matchlist.append(theme);
        }

        skiplist.append("*.???");

        // first scan the private resource folders
        MStringArray iceIconPaths;
        if (YIcon::iconResourceLocator == nullptr)
            YIcon::iconResourceLocator = xapp;
        if (YIcon::iconResourceLocator)
            YIcon::iconResourceLocator->subdirs("icons", false, iceIconPaths);
        else {
            upath paths[] = {
                YApplication::getPrivConfDir(),
                YApplication::getConfigDir(),
                YApplication::getLibDir(),
            };
            for (upath& path : paths) {
                if (path != null && path.relative("icons").isExecutable()) {
                    iceIconPaths += path;
                }
            }
        }

        // this returned icewm directories containing "icons" folder
        for (mstring& path : iceIconPaths) {
            probeIconFolder(path + "/icons", true);
        }

        // now test the system icons folders specified by user or defaults
        csmart copy(newstr(iconPath));
        for (tokens folder(copy, ":"); folder; ++folder) {
            probeIconFolder(folder.token(), false);
        }

        dedupTestPath.clear();
        skiplist.clear();
        matchlist.clear();
    }

    upath locateIcon(unsigned size, mstring baseName, bool fromResources) {
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
                mstring szSize(size);
                basePath = mstring(basePath, "_", szSize, "x", szSize);
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
                for (unsigned is : pool.uniqueSizes) {
                    if (is != probeAllButThis &&
                        checkFilesInFolder(folder, is, addSizeSfx))
                        break;
                }
                return res != null;
            }
            return checkFilesInFolder(folder, size, addSizeSfx);
        };
        auto scanList = [&](IconCategory& cat, bool addSizeSfx,
                unsigned probeAllButThis = 0) {

            for (auto& folder : cat.folders) {
                if (smartScanFolder(folder, addSizeSfx, probeAllButThis))
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
            for (unsigned is : pool.uniqueSizes) {
                if (is != size && scanList(pool.getCat(is), false))
                    break;
            }
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

    if (fPath != null && !(loadedS & loadedL & loadedH)) {
        upath loadPath;

        if (fPath.isAbsolute() && fPath.fileExists()) {
            loadPath = fPath;
        } else {
            loadPath = findIcon(size);
        }
        if (loadPath != null) {
            mstring cs(loadPath.path());
            YTraceIcon trace(cs);

#ifdef ICE_SUPPORT_SVG
            mstring ext(loadPath.getExtension().lower());
            if (ext == ".svg")
                icon = YImage::loadsvg(cs);
            else
#endif
                icon = YImage::load(cs);
        }
        else if (XDBG || YTrace::traces("icon")) {
            tlog("icon not found: %s %ux%u", fPath.string(), size, size);
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

ref<YImage> YIcon::huge() {
    if (loadedH == false) {
        fHuge = loadIcon(hugeIconSize);
        loadedH = true;
    }
    return fHuge;
}

ref<YImage> YIcon::large() {
    if (loadedL == false) {
        fLarge = loadIcon(largeIconSize);
        loadedL = true;
    }
    return fLarge;
}

ref<YImage> YIcon::small() {
    if (loadedS == false) {
        fSmall = loadIcon(smallIconSize);
        loadedS = true;
    }
    return fSmall;
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
    iconIndex = null;
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
