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
static YResourcePaths iconPaths;

void initIcons() {
    if (!didInit) {
        didInit = true;
        iconPaths.init("icons/");
    }
}


YIcon::YIcon(const char *filename):
    fSmall(null), fLarge(null), fHuge(null),
    loadedS(false), loadedL(false), loadedH(false),
    fPath(newstr(filename)), fCached(false)
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
    if (fPath) { delete[] fPath; fPath = NULL; }
}

char * YIcon::findIcon(char *base, unsigned /*size*/) {
    initIcons();
    /// !!! fix: do this at startup (merge w/ iconPath)
    for (YPathElement const *pe(iconPaths); pe->root; pe++) {
        char *path(pe->joinPath("/icons/"));
        char *fullpath(findPath(path, R_OK, base, true));
        delete[] path;

        if (NULL != fullpath) return fullpath;
    }

    return findPath(iconPath, R_OK, base, true);
}

char * YIcon::findIcon(int size) {
    char icons_size[1024];

    sprintf(icons_size, "%s_%dx%d.xpm", REDIR_ROOT(fPath), size, size);

    char * fullpath(findIcon(icons_size, size));
    if (NULL != fullpath) return fullpath;

    if (size == smallSize()) {
        sprintf(icons_size, "%s.xpm", REDIR_ROOT(fPath));
    } else {
        char name[1024];
        char *p;

        sprintf(icons_size, "%s.xpm", REDIR_ROOT(fPath));
        p = strrchr(icons_size, '/');
        if (!p)
            p = icons_size;
        else
            p++;
        strcpy(name, p);
        sprintf(p, "mini/%s", name);
    }

    if (NULL != (fullpath = findIcon(icons_size, size)))
        return fullpath;

#ifdef CONFIG_IMLIB
    sprintf(icons_size, "%s", REDIR_ROOT(fPath));
    if (NULL != (fullpath = findIcon(icons_size, size)))
        return fullpath;
#endif

    MSG(("Icon \"%s\" not found.", fPath));

    return NULL;
}

ref<YIconImage> YIcon::loadIcon(int size) {
    ref<YIconImage> icon;

    if (fPath) {
        char *fullPath = 0;
        char *loadPath = 0;
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
        if (fPath[0] == '/' && isreg(fPath)) {
            loadPath = fPath;
        } else
#endif
        {
            if ((fullPath = findIcon(size)) != NULL) {
                loadPath = fullPath;
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
            } else if (size != hugeSize() && (fullPath = findIcon(hugeSize()))) {
                loadPath = fullPath;
            } else if (size != largeSize() && (fullPath = findIcon(largeSize()))) {
                loadPath = fullPath;
            } else if (size != smallSize() && (fullPath = findIcon(smallSize()))) {
                loadPath = fullPath;
#endif
            }
        }
        if (loadPath != 0) {
            if (icon.init(new YIconImage(loadPath)) == null)
                warn(_("Out of memory for pixmap \"%s\""), loadPath);
        }
        delete[] fullPath;
    }

    if (icon != null && !icon->valid()) {
        icon = null;
    }

    if (icon != null) {
        icon = YIconImage::scale(icon, size, size);
    }

    return icon;
}

ref<YIconImage> YIcon::huge() {
    if (fHuge == null && !loadedH) {
        fHuge = loadIcon(hugeSize());
        loadedH = true;

#if defined(CONFIG_ANTIALIASING) || defined(CONFIG_IMLIB)
        if (fHuge == null && large() != null)
            fHuge = YIconImage::scale(large(), hugeSize(), hugeSize());

        if (fHuge == null && small() != null)
            fHuge = YIconImage::scale(small(), hugeSize(), hugeSize());
        //#elif defined(CONFIG_IMLIB)
        //      if (fHuge == null && (fHuge = large()) != null)
        //          fHuge.init(new YIconImage(fHuge->pixmap(), fHuge->mask(),
        //                                      fHuge->width(), fHuge->height()),
        //                            hugeSize(), hugeSize());
        //
        //      if (fHuge == null && (fHuge = small()) != null)
        //          fHuge.init(new YIconImage(fHuge->pixmap(), fHuge->mask(),
        //                                      fHuge->width(), fHuge->height(),
        //                                      hugeSize(), hugeSize()));
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
            fLarge = YIconImage::scale(huge(), largeSize(), largeSize());

        if (fLarge == null && small() != null)
            fLarge = YIconImage::scale(small(), largeSize(), largeSize());
        //#elif defined(CONFIG_IMLIB)
        //      if (fLarge == null && (fLarge = huge()))
        //          fLarge = new YIconImage(fLarge->pixmap(), fLarge->mask(),
        //                             fLarge->width(), fLarge->height(),
        //                             largeSize(), largeSize());
        //
        //      if (fLarge == null && (fLarge = small()))
        //          fLarge = new YIconImage(fLarge->pixmap(), fLarge->mask(),
        //                             fLarge->width(), fLarge->height(),
        //                             largeSize(), largeSize());
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
            fSmall = YIconImage::scale(large(), smallSize(), smallSize());

        if (fSmall == null && huge() != null)
            fSmall = YIconImage::scale(huge(), smallSize(), smallSize());
        //#elif defined(CONFIG_IMLIB)
        //      if (fSmall == null && (fSmall = large()))
        //          fSmall = new YIconImage(fSmall->pixmap(), fSmall->mask(),
        //                             fSmall->width(), fSmall->height(),
        //                             smallSize(), smallSize());
        //
        //      if (fSmall == null && (fSmall = huge()) != null)
        //          fSmall = new YIconImage(fSmall->pixmap(), fSmall->mask(),
        //                             fSmall->width(), fSmall->height(),
        //                             smallSize(), smallSize());
#endif
    }

    return fSmall;
}

ref<YIconImage> YIcon::getScaledIcon(int size) {
    //    if (fPath) {
    //        return loadIcon(size);
    //    } else
    {
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
            ref<YIconImage> img = YIconImage::scale(base, size, size);
#else
            ref<YIconImage> img = small();
#endif
            return img;
        }
        return base;
    }
}


static YObjectArray<YIcon> iconCache;

void YIcon::removeFromCache() {
    int n = cacheFind(iconName());
    if (n >= 0) {
        if (fPath) { delete[] fPath; fPath = NULL; }
        iconCache.remove(n);
    }
}

int YIcon::cacheFind(const char *name) {
    int l, r, m;

    l = 0;
    r = iconCache.getCount();
    while (l < r) {
        m = (l + r) / 2;
        YIcon *found = iconCache.getItem(m);
        int cmp = strcmp(name, found->iconName());
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
