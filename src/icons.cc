/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */	
#include "config.h"
#include "yfull.h"
#include "ypixbuf.h"
#include "ypaint.h"
#include "yapp.h"
#include "sysdep.h"
#include "prefs.h"
#include "wmprog.h" // !!! remove this

#ifdef CONFIG_XPM
#include <X11/xpm.h>
#endif

#ifdef CONFIG_IMLIB
#include <Imlib.h>
extern ImlibData *hImlib;
#endif

#include "intl.h"

Pixmap YPixmap::createPixmap(int w, int h) {
    return createPixmap(w, h, app->depth());
}

Pixmap YPixmap::createPixmap(int w, int h, int depth) {
    return XCreatePixmap(app->display(), desktop->handle(), w, h, depth);
}

Pixmap YPixmap::createMask(int w, int h) {
    return XCreatePixmap(app->display(), desktop->handle(), w, h, 1);
}

YPixmap::YPixmap(YPixmap const & pixmap):
    fPixmap(pixmap.fPixmap), fMask(pixmap.fMask),
    fWidth(pixmap.fWidth), fHeight(pixmap.fHeight),
    fOwned(false) {
}

#ifdef CONFIG_ANTIALIASING
YPixmap::YPixmap(YPixbuf & pixbuf):
    fPixmap(createPixmap(pixbuf.width(), pixbuf.height())), 
    fMask(pixbuf.alpha() ? createMask(pixbuf.width(), pixbuf.height()) : None),
    fWidth(pixbuf.width()), fHeight(pixbuf.height()),
    fOwned(true) {
    Graphics(fPixmap).copyPixbuf(pixbuf, 0, 0, fWidth, fHeight, 0, 0, false);
    Graphics(fMask).copyAlphaMask(pixbuf, 0, 0, fWidth, fHeight, 0, 0);
}
#endif

YPixmap::YPixmap(const char *filename):
    fOwned(true) {
#if defined(CONFIG_IMLIB)
    ImlibImage *im(Imlib_load_image(hImlib, (char *)REDIR_ROOT(filename)));

    if (im) {
        fWidth = im->rgb_width;
        fHeight = im->rgb_height;
        Imlib_render(hImlib, im, fWidth, fHeight);
        fPixmap = (Pixmap)Imlib_move_image(hImlib, im);
        fMask = (Pixmap)Imlib_move_mask(hImlib, im);
        Imlib_destroy_image(hImlib, im);
    } else {
        warn(_("Loading of image \"%s\" failed"), filename);
        fPixmap = fMask = None;
        fWidth = fHeight = 16;
    }
#elif defined(CONFIG_XPM)
    XpmAttributes xpmAttributes;
    memset(&xpmAttributes, 0, sizeof(xpmAttributes));
    xpmAttributes.colormap  = app->colormap();
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    int const rc(XpmReadFileToPixmap(app->display(), desktop->handle(),
				     (char *)REDIR_ROOT(filename), // !!!
				     &fPixmap, &fMask, &xpmAttributes));
    if (rc == XpmSuccess) {
	fWidth = xpmAttributes.width;
        fHeight = xpmAttributes.height;
        XpmFreeAttributes(&xpmAttributes);
    } else {
        warn(_("Loading of pixmap \"%s\" failed: %s"),
	       filename, XpmGetErrorString(rc));

	fWidth = fHeight = 16; /// should be 0, fix
	fPixmap = fMask = None;
    }
#else
    fWidth = fHeight = 16; /// should be 0, fix
    fPixmap = fMask = None;
#endif
}

#ifdef CONFIG_IMLIB
/* Load pixmap at specified size */
YPixmap::YPixmap(const char *filename, int w, int h) {
    fOwned = true;
    fWidth = w;
    fHeight = h;
    
    ImlibImage *im = Imlib_load_image(hImlib, (char *)REDIR_ROOT(filename));
    if(im) {
        Imlib_render(hImlib, im, fWidth, fHeight);
        fPixmap = (Pixmap) Imlib_move_image(hImlib, im);
        fMask = (Pixmap) Imlib_move_mask(hImlib, im);
        Imlib_destroy_image(hImlib, im);
    } else {
        warn(_("Loading of image \"%s\" failed"), filename);
        fPixmap = fMask = None;
    }
}
#endif

YPixmap::YPixmap(Pixmap pixmap, Pixmap mask, int w, int h) {
    fOwned = false;
    fWidth = w;
    fHeight = h;
    fPixmap = pixmap;
    fMask = mask;
}

#ifdef CONFIG_IMLIB
YPixmap::YPixmap(Pixmap pixmap, Pixmap mask, int w, int h,
		 int wScaled, int hScaled) {
    fOwned = true;
    fWidth = wScaled;
    fHeight = hScaled;
    fPixmap = fMask = None;

    ImlibImage *im = 
	Imlib_create_image_from_drawable (hImlib, pixmap, 0, 0, 0, w, h);

    if (im == 0) {
        warn (_("Imlib: Acquisition of X pixmap failed"));
	return;
    }

    Imlib_render(hImlib, im, fWidth, fHeight);
    fPixmap = Imlib_move_image(hImlib, im);
    Imlib_destroy_image(hImlib, im);

    if (fPixmap == 0) {
        warn (_("Imlib: Imlib image to X pixmap mapping failed"));
	return;
    }
    
    if (mask) {
	im = Imlib_create_image_from_drawable (hImlib, mask, 0, 0, 0, w, h);

	if (im == 0) {
	    warn (_("Imlib: Acquisition of X pixmap failed"));
	    return;
	}
//
// Initialization of a bilevel pixmap
//
	ImlibImage *sc = 
	    Imlib_clone_scaled_image (hImlib, im, fWidth, fHeight);
	Imlib_destroy_image(hImlib, im);
	
	fMask = createMask(fWidth, fHeight);
	Graphics g(fMask);

        g.setColor(YColor::white);
	g.fillRect(0, 0, fWidth, fHeight);

        g.setColor(YColor::black);
//
// nested rendering loop inspired by gdk-pixbuf
//
	unsigned char *px = sc->rgb_data;
	for (unsigned y = 0; y < fHeight; ++y)
	    for (unsigned xa = 0, xe; xa < fWidth; xa = xe) {
		while (xa < fWidth && *px < 128) ++xa, px+= 3;
		xe = xa;
		while (xe < fWidth && *px >= 128) ++xe, px+= 3;
		g.drawLine(xa, y, xe - 1, y);
	    }

	Imlib_destroy_image(hImlib, sc);
    }
}
#endif

YPixmap::YPixmap(int w, int h, bool mask) {
    fOwned = true;
    fWidth = w;
    fHeight = h;

    fPixmap = createPixmap(fWidth, fHeight);
    fMask = mask ? createPixmap(fWidth, fHeight) : None;
}

YPixmap::~YPixmap() {
    if (fOwned) {
        if (fPixmap != None)
            XFreePixmap(app->display(), fPixmap);
        if (fMask != None)
            XFreePixmap(app->display(), fMask);
    }
}

void YPixmap::replicate(bool horiz, bool copyMask) {
    if (this == NULL || pixmap() == None || (fMask == None && copyMask))
	return;
	
    int dim(horiz ? width() : height());
    if (dim >= 128) return;
    dim = 128 + dim - 128 % dim;

    Pixmap nPixmap(horiz ? createPixmap(dim, height())
    			 : createPixmap(width(), dim));
    Pixmap nMask(copyMask ? (horiz ? createMask(dim, height())
				   : createMask(width(), dim)) : None);

    if (horiz)
	Graphics(nPixmap).repHorz(fPixmap, width(), height(), 0, 0, dim);
    else
	Graphics(nPixmap).repVert(fPixmap, width(), height(), 0, 0, dim);

    if (nMask != None)
	if (horiz)
	    Graphics(nMask).repHorz(fMask, width(), height(), 0, 0, dim);
	else
	    Graphics(nMask).repVert(fMask, width(), height(), 0, 0, dim);

    if (fOwned) {
        if (fPixmap != None)
            XFreePixmap(app->display(), fPixmap);
        if (fMask != None)
            XFreePixmap(app->display(), fMask);
    }

    fPixmap = nPixmap;
    fMask = nMask;

    (horiz ? fWidth : fHeight) = dim;
}


#ifndef LITE
YIcon *firstIcon = 0;

YIcon::YIcon(const char *filename):
    fSmall(NULL), fLarge(NULL), fHuge(NULL),
    loadedS(false), loadedL(false), loadedH(false),
    fPath(newstr(filename)), fNext(firstIcon) {
    firstIcon = this;
}

YIcon::YIcon(Image * small, Image * large, Image * huge) :
    fSmall(small), fLarge(large), fHuge(huge), 
    loadedS(small), loadedL(large), loadedH(huge),
    fPath(NULL), fNext(NULL) {
}

YIcon::~YIcon() {
    if (this != firstIcon) {
        for (YIcon * icn(firstIcon); NULL != icn; icn = icn->fNext) {
            if (icn->fNext == this) {
                icn->fNext = fNext;
                break;
            }
        }
    } else {
        firstIcon = fNext;
    }

    delete fHuge; fHuge = NULL;
    delete fLarge; fLarge = NULL;
    delete fSmall; fSmall = NULL;
    delete[] fPath; fPath = NULL;
}

char * YIcon::findIcon(char *base, unsigned /*size*/) {
    /// !!! fix: do this at startup (merge w/ iconPath)
    for (YPathElement const *pe(YApplication::iconPaths); pe->root; pe++) {
        char *path(pe->joinPath("/icons/"));
	char *fullpath(findPath(path, R_OK, base, true));
	delete[] path;
	
        if (NULL != fullpath) return fullpath;
    }

    return findPath(iconPath, R_OK, base, true);
}

char * YIcon::findIcon(unsigned size) {
    char icons_size[1024];

    sprintf(icons_size, "%s_%dx%d.xpm", REDIR_ROOT(fPath), size, size);

    char * fullpath(findIcon(icons_size, size));
    if (NULL != fullpath) return fullpath;
    
    if (size == sizeLarge) {
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

YIcon::Image * YIcon::loadIcon(unsigned size) {
    YIcon::Image * icon(NULL);

    if (fPath) {
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
        if(fPath[0] == '/' && isreg(fPath)) {
            if (NULL == (icon = new Image(fPath, size, size)))
                warn(_("Out of memory for pixmap \"%s\""), fPath);
        } else
#endif
        {
            char *fullPath;

            if (NULL != (fullPath = findIcon(size))) {
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
                icon = new Image(fullPath, size, size);
#else
                icon = new Image(fullPath);
#endif
                if (icon == NULL)
                    warn(_("Out of memory for pixmap \"%s\""), fullPath);

                delete[] fullPath;
#if defined(CONFIG_IMLIB) || defined(CONFIG_ANTIALIASING)
	    } else if (size != sizeHuge && (fullPath = findIcon(sizeHuge))) {
		if (NULL == (icon = new Image(fullPath, size, size)))
		    warn(_("Out of memory for pixmap \"%s\""), fullPath);
	    } else if (size != sizeLarge && (fullPath = findIcon(sizeLarge))) {
		if (NULL == (icon = new Image(fullPath, size, size)))
		    warn(_("Out of memory for pixmap \"%s\""), fullPath);
	    } else if (size != sizeSmall && (fullPath = findIcon(sizeSmall))) {
		if (NULL == (icon = new Image(fullPath, size, size)))
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

YIcon::Image * YIcon::huge() {
    if (fHuge == 0 && !loadedH) {
        fHuge = loadIcon(sizeHuge);
	loadedH = true;

#if defined(CONFIG_ANTIALIASING)
	if (fHuge == NULL && (fHuge = large()))
	    fHuge = new Image(*fHuge, sizeHuge, sizeHuge);

	if (fHuge == NULL && (fHuge = small()))
	    fHuge = new Image(*fHuge, sizeHuge, sizeHuge);
#elif defined(CONFIG_IMLIB)
	if (fHuge == NULL && (fHuge = large()))
	    fHuge = new Image(fHuge->pixmap(), fHuge->mask(),
	    		      fHuge->width(), fHuge->height(),
			      sizeHuge, sizeHuge);

	if (fHuge == NULL && (fHuge = small()))
	    fHuge = new Image(fHuge->pixmap(), fHuge->mask(),
			      fHuge->width(), fHuge->height(),
			      sizeHuge, sizeHuge);
#endif
    }

    return fHuge;
}

YIcon::Image * YIcon::large() {
    if (fLarge == 0 && !loadedL) {
        fLarge = loadIcon(sizeLarge);
	loadedL = true;

#if defined(CONFIG_ANTIALIASING)
	if (fLarge == NULL && (fLarge = huge()))
	    fLarge = new Image(*fLarge, sizeLarge, sizeLarge);

	if (fLarge == NULL && (fLarge = small()))
	    fLarge = new Image(*fLarge, sizeLarge, sizeLarge);
#elif defined(CONFIG_IMLIB)
	if (fLarge == NULL && (fLarge = huge()))
	    fLarge = new Image(fLarge->pixmap(), fLarge->mask(),
			       fLarge->width(), fLarge->height(),
			       sizeLarge, sizeLarge);

	if (fLarge == NULL && (fLarge = small()))
	    fLarge = new Image(fLarge->pixmap(), fLarge->mask(),
			       fLarge->width(), fLarge->height(),
			       sizeLarge, sizeLarge);
#endif
    }

    return fLarge;
}

YIcon::Image * YIcon::small() {
    if (fSmall == 0 && !loadedS) {
        fSmall = loadIcon(sizeSmall);
	loadedS = true;

#if defined(CONFIG_ANTIALIASING)
	if (fSmall == NULL && (fSmall = large()))
	    fSmall = new Image(*fSmall, sizeSmall, sizeSmall);

	if (fSmall == NULL && (fSmall = huge()))
	    fSmall = new Image(*fSmall, sizeSmall, sizeSmall);
#elif defined(CONFIG_IMLIB)
	if (fSmall == NULL && (fSmall = large()))
	    fSmall = new Image(fSmall->pixmap(), fSmall->mask(),
			       fSmall->width(), fSmall->height(),
			       sizeSmall, sizeSmall);

	if (fSmall == NULL && (fSmall = huge()))
	    fSmall = new Image(fSmall->pixmap(), fSmall->mask(),
			       fSmall->width(), fSmall->height(),
			       sizeSmall, sizeSmall);
#endif
    }

    return fSmall;
}

YIcon *getIcon(const char *name) {
    for (YIcon * icn(firstIcon); icn; icn = icn->next())
        if (icn->iconName() && !strcmp(name, icn->iconName()))
            return icn;

    return new YIcon(name);
}

void freeIcons() {
    for (YIcon * icn(firstIcon), * next; icn; icn = next) {
        next = icn->next();
        delete icn;
    }
}
#endif
