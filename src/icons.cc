/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */	
#include "config.h"
#include "yfull.h"
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
ImlibData *hImlib = 0;
#endif

#include "intl.h"

Pixmap YPixmap::createPixmap(int w, int h) {
    return XCreatePixmap(app->display(), desktop->handle(), w, h,
	DefaultDepth(app->display(), DefaultScreen(app->display())));
}

Pixmap YPixmap::createMask(int w, int h) {
    return XCreatePixmap(app->display(), desktop->handle(), w, h, 1);
}

YPixmap::YPixmap(const char *fileName) {
#ifdef CONFIG_IMLIB
    if(!hImlib) hImlib=Imlib_init(app->display());

    fOwned = true;

    ImlibImage *im = Imlib_load_image(hImlib, (char *)REDIR_ROOT(fileName));
    if(im) {
        fWidth = im->rgb_width;
        fHeight = im->rgb_height;
        Imlib_render(hImlib, im, fWidth, fHeight);
        fPixmap = (Pixmap)Imlib_move_image(hImlib, im);
        fMask = (Pixmap)Imlib_move_mask(hImlib, im);
        Imlib_destroy_image(hImlib, im);
    } else {
        warn(_("Loading image %s failed"), fileName);
        fPixmap = fMask = None;
        fWidth = fHeight = 16;
    }
#else
#ifdef CONFIG_XPM
    XpmAttributes xpmAttributes;
    int rc;

    xpmAttributes.colormap  = defaultColormap;
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    rc = XpmReadFileToPixmap(app->display(),
                             desktop->handle(),
                             (char *)REDIR_ROOT(fileName), // !!!
                             &fPixmap, &fMask,
                             &xpmAttributes);

    fOwned = true;
    if (rc == 0) {
       fWidth = xpmAttributes.width;
       fHeight = xpmAttributes.height;
    } else {
       fWidth = fHeight = 16; /// should be 0, fix
       fPixmap = fMask = None;
    }

    if (rc != 0)
        warn(_("Load pixmap %s failed with rc=%d"), fileName, rc);
#else
    fWidth = fHeight = 16; /// should be 0, fix
    fPixmap = fMask = None;
#endif
#endif
}

#ifdef CONFIG_IMLIB
/* Load pixmap at specified size */
YPixmap::YPixmap(const char *fileName, int w, int h) {

    if(!hImlib) hImlib = Imlib_init(app->display());

    fOwned = true;
    fWidth = w;
    fHeight = h;

    ImlibImage *im = Imlib_load_image(hImlib, (char *)REDIR_ROOT(fileName));
    if(im) {
        Imlib_render(hImlib, im, fWidth, fHeight);
        fPixmap = (Pixmap) Imlib_move_image(hImlib, im);
        fMask = (Pixmap) Imlib_move_mask(hImlib, im);
        Imlib_destroy_image(hImlib, im);
    } else {
        warn(_("Loading image %s failed"), fileName);
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

YIcon::YIcon(const char *fileName) {
    fNext = firstIcon;
    firstIcon = this;
    loadedS = loadedL = loadedH = false;

    fHuge = fLarge = fSmall = 0;
    fPath = new char [strlen(fileName) + 1];
    if (fPath)
        strcpy(fPath, fileName);
}

YIcon::YIcon(YPixmap *small, YPixmap *large, YPixmap *huge) {
    fSmall = small;
    fLarge = large;
    fHuge = huge;

    loadedS = small;
    loadedL = large;
    loadedH = huge;

    fPath = 0;
    fNext = 0;
}

YIcon::~YIcon() {
    delete fHuge; fHuge = 0;
    delete fLarge; fLarge = 0;
    delete fSmall; fSmall = 0;
    delete fPath; fPath = 0;
}

bool YIcon::findIcon(char *base, char **fullPath, int /*size*/) {
    /// !!! fix: do this at startup (merge w/ iconPath)
    for (YPathElement const *pe(YApplication::iconPaths); pe->root; pe++) {
        char *path(pe->joinPath("/icons/"));

        if (findPath(path, R_OK, base, fullPath, true)) {
            delete[] path;
            return true;
        }

        delete[] path;
    }

    if (findPath(iconPath, R_OK, base, fullPath, true))
        return true;

    return false;
}

bool YIcon::findIcon(char **fullPath, int size) {
    char icons_size[1024];


    sprintf(icons_size, "%s_%dx%d.xpm", REDIR_ROOT(fPath), size, size);

    if (findIcon(icons_size, fullPath, size))
        return true;
    
    if (size == ICON_LARGE) {
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

    if (findIcon(icons_size, fullPath, size))
        return true;

#ifdef CONFIG_IMLIB    
    sprintf(icons_size, "%s", REDIR_ROOT(fPath));
    if (findIcon(icons_size, fullPath, size))
        return true;
#endif

    MSG(("Icon '%s' not found.", fPath));

    return false;
}

YPixmap *YIcon::loadIcon(int size) {
    YPixmap *icon = 0;

    if (fPath && icon == 0) {
#ifdef CONFIG_IMLIB
        if(fPath[0] == '/' && isreg(fPath)) {
            icon = new YPixmap(fPath, size, size);
            if (icon == 0)
                warn(_("Out of memory for pixmap %s"), fPath);
        } else
#endif
        {
            char *fullPath;

            if (findIcon(&fullPath, size)) {
#ifdef CONFIG_IMLIB
                icon = new YPixmap(fullPath, size, size);
#else
                icon = new YPixmap(fullPath);
#endif
                if (icon == 0)
                    warn(_("Out of memory for pixmap %s"), fullPath);
                delete fullPath;
            }
        }
    }
    return icon;
}

YPixmap *YIcon::huge() {
    if (fHuge == 0 && !loadedH) {
        fHuge = loadIcon(ICON_HUGE);
	loadedH = true;

#ifndef CONFIG_XPM
	if (fHuge == NULL && (fHuge = large()))
	    fHuge = new YPixmap(fHuge->pixmap(), fHuge->mask(),
	    		    fHuge->width(), fHuge->height(),
			    ICON_HUGE, ICON_HUGE);

	if (fHuge == NULL && (fHuge = small()))
	    fHuge = new YPixmap(fHuge->pixmap(), fHuge->mask(),
	    		    fHuge->width(), fHuge->height(),
			    ICON_HUGE, ICON_HUGE);
#endif
    }

    return fHuge;
}

YPixmap *YIcon::large() {
    if (fLarge == 0 && !loadedL) {
        fLarge = loadIcon(ICON_LARGE);
	loadedL = true;

#ifndef CONFIG_XPM
	if (fLarge == NULL && (fLarge = huge()))
	    fLarge = new YPixmap(fLarge->pixmap(), fLarge->mask(),
	    		    fLarge->width(), fLarge->height(),
			    ICON_LARGE, ICON_LARGE);

	if (fLarge == NULL && (fLarge = small()))
	    fLarge = new YPixmap(fLarge->pixmap(), fLarge->mask(),
	    		    fLarge->width(), fLarge->height(),
			    ICON_LARGE, ICON_LARGE);
#endif
    }

    return fLarge;
}

YPixmap *YIcon::small() {
    if (fSmall == 0 && !loadedS) {
        fSmall = loadIcon(ICON_SMALL);
	loadedS = true;

#ifndef CONFIG_XPM
	if (fSmall == NULL && (fSmall = large()))
	    fSmall = new YPixmap(fSmall->pixmap(), fSmall->mask(),
	    		    fSmall->width(), fSmall->height(),
			    ICON_SMALL, ICON_SMALL);

	if (fSmall == NULL && (fSmall = huge()))
	    fSmall = new YPixmap(fSmall->pixmap(), fSmall->mask(),
	    		    fSmall->width(), fSmall->height(),
			    ICON_SMALL, ICON_SMALL);
#endif
    }

    return fSmall;
}

YIcon *getIcon(const char *name) {
    for (YIcon * icn(firstIcon); icn; icn = icn->next())
        if (strcmp(name, icn->iconName()) == 0)
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
