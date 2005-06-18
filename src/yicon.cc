/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ypixbuf.h"
#include "ypaint.h"
#include "yicon.h"
#include "yapp.h"
#include "sysdep.h"
#include "prefs.h"
#include "yprefs.h"
#include "wmprog.h" // !!! remove this

#include "intl.h"

#ifndef LITE

static bool didInit = false;
static ref<YResourcePaths> iconPaths;

void initIcons() {
    if (!didInit) {
        didInit = true;
        iconPaths = YResourcePaths::subdirs("icons/");
    }
}


YIcon::YIcon(upath filename):
    fSmall(null), fLarge(null), fHuge(null),
    loadedS(false), loadedL(false), loadedH(false),
    fPath(filename), fCached(false)
{
}

YIcon::YIcon(ref<YIconImage> small, ref<YIconImage> large, ref<YIconImage> huge) :
    fSmall(small), fLarge(large), fHuge(huge),
    loadedS(small != null), loadedL(large != null), loadedH(huge != null),
    fPath(NULL), fCached(false)
{
}

YIcon::~YIcon() {
    fHuge = null;
    fLarge = null;
    fSmall = null;
}

upath YIcon::findIcon(upath base, unsigned /*size*/) {
    initIcons();
    /// !!! fix: do this at startup (merge w/ iconPath)
    for (int i = 0; i < iconPaths->getCount(); i++) {
        upath path = iconPaths->getPath(i)->joinPath(upath("/icons/"));
        upath fullpath = findPath(path.path(), R_OK, base, true);

        if (fullpath != null)
            return fullpath;
    }

    return findPath(iconPath, R_OK, base, true);
}

upath YIcon::findIcon(int size) {
    char icons_size[1024];
    cstring cs(fPath.path());

    sprintf(icons_size, "%s_%dx%d.xpm", REDIR_ROOT(cs.c_str()), size, size);

    upath fullpath = findIcon(icons_size, size);
    if (fullpath != null)
        return fullpath;

    if (size == smallSize()) {
        sprintf(icons_size, "%s.xpm", REDIR_ROOT(cs.c_str()));
    } else {
        char name[1024];
        char *p;

        sprintf(icons_size, "%s.xpm", REDIR_ROOT(cs.c_str()));
        p = strrchr(icons_size, '/');
        if (!p)
            p = icons_size;
        else
            p++;
        strcpy(name, p);
        sprintf(p, "mini/%s", name);
    }

    fullpath = findIcon(icons_size, size);
    if (fullpath != null)
        return fullpath;

#ifdef CONFIG_IMLIB
    sprintf(icons_size, "%s", REDIR_ROOT(cs.c_str()));
    fullpath = findIcon(icons_size, size);
    if (fullpath != null)
        return fullpath;
#endif

    MSG(("Icon \"%s\" not found.", cs.c_str()));

    return null;
}

ref<YIconImage> YIcon::loadIcon(int size) {
    ref<YIconImage> icon;

    if (fPath != null) {
        upath fullPath;
        upath loadPath;
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
        if (fPath.isAbsolute() && fPath.fileExists()) {
            loadPath = fPath;
        } else
#endif
        {
            fullPath = findIcon(size);
            if (fullPath != null) {
                loadPath = fullPath;
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
            } else if (size != hugeSize() && (fullPath = findIcon(hugeSize())) != null) {
                loadPath = fullPath;
            } else if (size != largeSize() && (fullPath = findIcon(largeSize())) != null) {
                loadPath = fullPath;
            } else if (size != smallSize() && (fullPath = findIcon(smallSize())) != null) {
                loadPath = fullPath;
#endif
            }
        }
        if (loadPath != null) {
            cstring cs(loadPath.path());
            icon = YIconImage::load(cs.c_str());
            if (icon == null)
                warn(_("Out of memory for pixmap \"%s\""), cs.c_str());
        }
    }

    if (icon != null) {
        icon = icon->scale(size, size);
    }
    return icon;
}

ref<YIconImage> YIcon::huge() {
    if (fHuge == null && !loadedH) {
        fHuge = loadIcon(hugeSize());
        loadedH = true;

#if defined(CONFIG_ANTIALIASING) || defined(CONFIG_IMLIB)
	if (fHuge == null && large() != null)
            fHuge = large()->scale(hugeSize(), hugeSize());

	if (fHuge == null && small() != null)
            fHuge = small()->scale(hugeSize(), hugeSize());
#endif
    }

    return fHuge;
}

ref<YIconImage> YIcon::large() {
    if (fLarge == null && !loadedL) {
        fLarge = loadIcon(largeSize());
        loadedL = true;

#if defined(CONFIG_ANTIALIASING) || defined(CONFIG_IMLIB)
	if (fLarge == null && huge() != null)
            fLarge = huge()->scale(largeSize(), largeSize());

	if (fLarge == null && small() != null)
	    fLarge = small()->scale(largeSize(), largeSize());
#endif
    }

    return fLarge;
}

ref<YIconImage> YIcon::small() {
    if (fSmall == null && !loadedS) {
        fSmall = loadIcon(smallSize());
        loadedS = true;

#if defined(CONFIG_ANTIALIASING) || defined(CONFIG_IMLIB)
	if (fSmall == null && large() != null)
            fSmall = large()->scale(smallSize(), smallSize());
	if (fSmall == null && huge() != null)
            fSmall = huge()->scale(smallSize(), smallSize());
#endif
    }

    return fSmall;
}

ref<YIconImage> YIcon::getScaledIcon(int size) {
    ref<YIconImage> base = null;

    if (size == smallSize())
        base = small();
    else if (size == largeSize())
        base = large();
    else if (size == hugeSize())
        base = huge();

    if (base == null)
        base = huge();
    if (base == null)
        base = large();
    if (base == null)
        base = small();

    if (base != null) {
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
        ref<YIconImage> img = base->scale(size, size);
#else
        ref<YIconImage> img = small();
#endif
        return img;
    }
    return base;
}


static YObjectArray<YIcon> iconCache;

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
        YIcon *found = iconCache.getItem(m);
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

YIcon *YIcon::getIcon(const char *name) {
    int n = cacheFind(name);
    if (n >= 0)
        return iconCache.getItem(n);

    YIcon *newicon = new YIcon(name);
    if (newicon) {
        newicon->setCached(true);
        iconCache.insert(-n - 1, newicon);
    }
    return getIcon(name);
}

void YIcon::freeIcons() {
    while (iconCache.getCount() > 0)
        iconCache.getItem(0)->removeFromCache();
}

int YIcon::smallSize() {
    return smallIconSize;
}

int YIcon::largeSize() {
    return largeIconSize;
}

int YIcon::hugeSize() {
    return hugeIconSize;
}

#endif
