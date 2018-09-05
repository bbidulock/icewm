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

#include "intl.h"

static ref<YResourcePaths> iconPaths;

static void initIconPaths() {
    if (iconPaths == null) {
        iconPaths = YResourcePaths::subdirs("icons/");
    }
}


YIcon::YIcon(upath filename):
    fSmall(null), fLarge(null), fHuge(null),
    loadedS(false), loadedL(false), loadedH(false),
    fPath(filename), fCached(false)
{
}

YIcon::YIcon(ref<YImage> small, ref<YImage> large, ref<YImage> huge) :
    fSmall(small), fLarge(large), fHuge(huge),
    loadedS(small != null), loadedL(large != null), loadedH(huge != null),
    fPath(null), fCached(false)
{
}

YIcon::~YIcon() {
    fHuge = null;
    fLarge = null;
    fSmall = null;
}

static upath joinPath(const upath& dir, const upath& name) {
    if (dir == null)
        return name;

    if (name.isAbsolute())
        return name;

    return dir.relative(name);
}

static inline bool isIconFile(const upath& name) {
    bool exist = name.fileExists();
    return exist;
}

upath YIcon::findIcon(upath dir, upath base, unsigned size) {
    char iconName[1024];
    const size_t iconSize = sizeof iconName;
    const cstring cbase(base.string());
    const char* cBaseStr = cbase.c_str();
    static const char iconExts[][5] = { ".png",
#if defined(CONFIG_GDK_PIXBUF_XLIB) && defined(CONFIG_LIBRSVG)
            ".svg",
#endif
            ".xpm"
    };
    static const int numIconExts = (int) ACOUNT(iconExts);

    upath fullpath(joinPath(dir, base));
    if (isIconFile(fullpath))
        return fullpath;

    bool hasImageExtension = false;
    const cstring cbaseExt(base.getExtension());
    if (cbaseExt.length() == 4) {
        for (int i = 0; i < numIconExts; ++i) {
            hasImageExtension |= (0 == strcmp(iconExts[i], cbaseExt));
        }
    }

    // XXX: actually, we should distinguish by purpose (app, category, mimetype, etc.)
    // For now, check by the same schema hoping that the file name provides uniquie identity information.
    static const char* xdg_icon_patterns[] = {
            "/%ux%u/apps/%s",
            "/%ux%u/categories/%s",
            0
    };
    static const char* xdg_folder_patterns[] = {
            "/%ux%u/apps",
            "/%ux%u/categories",
            0
    };


    if (hasImageExtension) {
        for (const char **p = xdg_icon_patterns; *p; ++p) {
            snprintf(iconName, iconSize, *p, size, size, cBaseStr);
            fullpath = dir + iconName;
            if (isIconFile(fullpath))
                return fullpath;
        }
    }
    else if (base.path().endsWith("/") == false) {
        for (int i = 0; i < numIconExts; ++i) {
            snprintf(iconName, iconSize,
                    "%s_%ux%u%s", cBaseStr, size, size, iconExts[i]);
            fullpath = joinPath(dir, iconName);
            if (isIconFile(fullpath))
                return fullpath;
        }

        for (int i = 0; i < numIconExts; ++i) {
            fullpath = joinPath(dir, base.addExtension(iconExts[i]));
            if (isIconFile(fullpath))
                return fullpath;
        }

        for (const char **p = xdg_folder_patterns; *p; ++p) {
            snprintf(iconName, iconSize, *p, size, size);
            upath apps(dir + iconName);
            if (apps.dirExists()) {
                for (int i = 0; i < numIconExts; ++i) {
                    snprintf(iconName, iconSize, "/%s%s", cBaseStr,
                            iconExts[i]);
                    fullpath = apps + iconName;
                    if (isIconFile(fullpath))
                        return fullpath;
                }
            }
        }
    }

    return null;
}

upath YIcon::findIcon(unsigned size) {
    initIconPaths();

    mstring copy(iconPath), part;
    while (copy.splitall(PATHSEP, &part, &copy)) {
        if (part.nonempty()) {
            upath path(part);
            if (path.dirExists()) {
                upath fullpath(findIcon(path, fPath, size));
                if (fullpath != null) {
                    return fullpath;
                }
            }
        }
    }

    copy = iconPath;
    for (int i = 0; i < iconPaths->getCount(); i++) {
        upath path(iconPaths->getPath(i) + "icons");
        int k = copy.find(path);
        if (k >= 0 && (k == 0 || copy[k - 1] == PATHSEP)) {
            int ch = copy[path.length()];
            if (ch == -1 || ch == PATHSEP) {
                continue;
            }
            if (ch == SLASH) {
                int ch2 = copy[path.length() + 1];
                if (ch2 == -1 || ch2 == PATHSEP) {
                    continue;
                }
            }
        }
        path = findIcon(path, fPath, size);
        if (path != null)
            return path;
    }

    MSG(("Icon \"%s\" not found.", fPath.string().c_str()));

    return null;
}

ref<YImage> YIcon::loadIcon(unsigned size) {
    ref<YImage> icon;

    if (fPath != null) {
        upath loadPath;

        if (fPath.isAbsolute() && fPath.fileExists()) {
            loadPath = fPath;
        } else {
            const unsigned sizes[] = {
                size, hugeSize(), largeSize(), smallSize()
            };
            for (int i = 0; i < (int) ACOUNT(sizes); ++i) {
                int k = i;
                while (--k >= 0 && sizes[k] != sizes[i]) { }
                if (k < 0) {
                    loadPath = findIcon(size);
                    if (loadPath != null) {
                        break;
                    }
                }
            }
        }
        if (loadPath != null) {
            cstring cs(loadPath.path());
            icon = YImage::load(cs.c_str());
        }
    }
#if 1
    if (icon != null) {
        icon = icon->scale(size, size);
    }
#endif
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
    ref<YImage> base = null;

#if 1
    if (size == smallSize())
        base = small();
    else if (size == largeSize())
        base = large();
    else if (size == hugeSize())
        base = huge();
#endif

    if (base == null)
        base = huge();
    if (base == null)
        base = large();
    if (base == null)
        base = small();

    if (base != null) {
        ref<YImage> img = base->scale(size, size);
        return img;
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

    ref<YIcon>newicon(new YIcon(name));
    if (newicon != null) {
        newicon->setCached(true);
        iconCache.insert(-n - 1, newicon);
    }
    return newicon;
}

void YIcon::freeIcons() {
    for (int k = iconCache.getCount(); --k >= 0; ) {
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
