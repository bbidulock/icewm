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
    fSmall(NULL), fLarge(NULL), fHuge(NULL),
    loadedS(false), loadedL(false), loadedH(false),
    fPath(newstr(filename)), fCached(false)
{
}

YIcon::YIcon(YIconImage *small, YIconImage *large, YIconImage *huge) :
    fSmall(small), fLarge(large), fHuge(huge), 
    loadedS(small), loadedL(large), loadedH(huge),
    fPath(NULL), fCached(false)
{
}

YIcon::~YIcon() {
    delete fHuge; fHuge = NULL;
    delete fLarge; fLarge = NULL;
    delete fSmall; fSmall = NULL;
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

YIconImage *YIcon::loadIcon(int size) {
    YIconImage *icon = 0;

    if (fPath) {
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
        if(fPath[0] == '/' && isreg(fPath)) {
            if (NULL == (icon = new YIconImage(fPath, size, size)))
                warn(_("Out of memory for pixmap \"%s\""), fPath);
        } else
#endif
        {
            char *fullPath;

            if (NULL != (fullPath = findIcon(size))) {
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
                icon = new YIconImage(fullPath, size, size);
#else
                icon = new YIconImage(fullPath);
#endif
                if (icon == NULL)
                    warn(_("Out of memory for pixmap \"%s\""), fullPath);

                delete[] fullPath;
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
	    } else if (size != hugeSize() && (fullPath = findIcon(hugeSize()))) {
		if (NULL == (icon = new YIconImage(fullPath, size, size)))
		    warn(_("Out of memory for pixmap \"%s\""), fullPath);
	    } else if (size != largeSize() && (fullPath = findIcon(largeSize()))) {
		if (NULL == (icon = new YIconImage(fullPath, size, size)))
		    warn(_("Out of memory for pixmap \"%s\""), fullPath);
	    } else if (size != smallSize() && (fullPath = findIcon(smallSize()))) {
		if (NULL == (icon = new YIconImage(fullPath, size, size)))
		    warn(_("Out of memory for pixmap \"%s\""), fullPath);
#endif
            }
        }
    }
    
    if (NULL != icon && !icon->valid()) {
	delete icon;
	icon = NULL;
    }

    return icon;
}

YIconImage *YIcon::huge() {
    if (fHuge == 0 && !loadedH) {
        fHuge = loadIcon(hugeSize());
	loadedH = true;

#if defined(CONFIG_ANTIALIASING)
	if (fHuge == NULL && (fHuge = large()))
	    fHuge = new YIconImage(*fHuge, hugeSize(), hugeSize());

	if (fHuge == NULL && (fHuge = small()))
	    fHuge = new YIconImage(*fHuge, hugeSize(), hugeSize());
#elif defined(CONFIG_IMLIB)
	if (fHuge == NULL && (fHuge = large()))
	    fHuge = new YIconImage(fHuge->pixmap(), fHuge->mask(),
	    		      fHuge->width(), fHuge->height(),
			      hugeSize(), hugeSize());

	if (fHuge == NULL && (fHuge = small()))
	    fHuge = new YIconImage(fHuge->pixmap(), fHuge->mask(),
			      fHuge->width(), fHuge->height(),
			      hugeSize(), hugeSize());
#endif
    }

    return fHuge;
}

YIconImage *YIcon::large() {
    if (fLarge == 0 && !loadedL) {
        fLarge = loadIcon(largeSize());
	loadedL = true;

#if defined(CONFIG_ANTIALIASING)
	if (fLarge == NULL && (fLarge = huge()))
	    fLarge = new YIconImage(*fLarge, largeSize(), largeSize());

	if (fLarge == NULL && (fLarge = small()))
	    fLarge = new YIconImage(*fLarge, largeSize(), largeSize());
#elif defined(CONFIG_IMLIB)
	if (fLarge == NULL && (fLarge = huge()))
	    fLarge = new YIconImage(fLarge->pixmap(), fLarge->mask(),
			       fLarge->width(), fLarge->height(),
			       largeSize(), largeSize());

	if (fLarge == NULL && (fLarge = small()))
	    fLarge = new YIconImage(fLarge->pixmap(), fLarge->mask(),
			       fLarge->width(), fLarge->height(),
			       largeSize(), largeSize());
#endif
    }

    return fLarge;
}

YIconImage *YIcon::small() {
    if (fSmall == 0 && !loadedS) {
        fSmall = loadIcon(smallSize());
	loadedS = true;

#if defined(CONFIG_ANTIALIASING)
	if (fSmall == NULL && (fSmall = large()))
	    fSmall = new YIconImage(*fSmall, smallSize(), smallSize());

	if (fSmall == NULL && (fSmall = huge()))
	    fSmall = new YIconImage(*fSmall, smallSize(), smallSize());
#elif defined(CONFIG_IMLIB)
	if (fSmall == NULL && (fSmall = large()))
	    fSmall = new YIconImage(fSmall->pixmap(), fSmall->mask(),
			       fSmall->width(), fSmall->height(),
			       smallSize(), smallSize());

	if (fSmall == NULL && (fSmall = huge()))
	    fSmall = new YIconImage(fSmall->pixmap(), fSmall->mask(),
			       fSmall->width(), fSmall->height(),
			       smallSize(), smallSize());
#endif
    }

    return fSmall;
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
