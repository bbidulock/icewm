/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */	
#include "config.h"
#include "yfull.h"
#include "ypaint.h"
#include "yapp.h"
#include "sysdep.h"
#include "prefs.h"
#include "wmprog.h" // !!! remove this

#ifdef XPM
#include <X11/xpm.h>
#endif

#ifdef IMLIB
#include <Imlib.h>
static ImlibData *hImlib = 0;
#endif

#include "intl.h"

YPixmap::YPixmap(const char *fileName) {
#ifdef IMLIB
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
#ifdef XPM
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

#ifdef IMLIB
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

#ifdef IMLIB
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
	
	YPixmap mask(XCreatePixmap(app->display(), desktop->handle(),
		     fWidth, fHeight, 1), None, fWidth, fHeight);
	Graphics *g = new Graphics(&mask);

	YColor solid((unsigned long) 1);
        g->setColor(&solid);
	g->fillRect(0, 0, fWidth, fHeight);

	YColor transparent((unsigned long) 0);
        g->setColor(&transparent);
//
// nested rendering loop inspired by gdk-pixbuf
//
	unsigned char *px = sc->rgb_data;
	for (unsigned y = 0; y < fHeight; ++y)
	    for (unsigned xa = 0, xe; xa < fWidth; xa = xe) {
		while (xa < fWidth && *px < 128) ++xa, px+= 3;
		xe = xa;
		while (xe < fWidth && *px >= 128) ++xe, px+= 3;
		g->drawLine(xa, y, xe - 1, y);
	    }

	fMask = mask.pixmap ();
	Imlib_destroy_image(hImlib, sc);
    }
}
#endif

YPixmap::YPixmap(int w, int h, bool mask) {
    fOwned = true;
    fWidth = w;
    fHeight = h;

    fPixmap = XCreatePixmap(app->display(), desktop->handle(),
                            fWidth, fHeight, DefaultDepth(app->display(),
			    DefaultScreen(app->display())));
    fMask = (mask
	? XCreatePixmap(app->display(), desktop->handle(),
		        fWidth, fHeight, DefaultDepth(app->display(),
					 DefaultScreen(app->display())))
	: None);
}

YPixmap::~YPixmap() {
    if (fOwned) {
        if (fPixmap != None)
            XFreePixmap(app->display(), fPixmap);
        if (fMask != None)
            XFreePixmap(app->display(), fMask);
    }
}

void loadPixmap(pathelem *pe, const char *base, const char *name, YPixmap **pixmap) {
    char *path;

    *pixmap = 0;

    for (; pe->root; pe++) {
        path = joinPath(pe, base, name);

        if (is_reg(path)) {
            *pixmap = new YPixmap(path);

            if (*pixmap == 0)
                die(1, _("Out of memory for pixmap %s"), path);

            delete path;
            return ;
        }
        delete path;
    }
#ifdef DEBUG
    if (debug)
        warn(_("Could not find pixmap %s"), name);
#endif
}

#ifndef LITE
YIcon *firstIcon = 0;

YIcon::YIcon(const char *fileName) {
    fNext = firstIcon;
    firstIcon = this;
    loadedS = loadedL = false;

    fLarge = fSmall = 0;
    fPath = new char [strlen(fileName) + 1];
    if (fPath)
        strcpy(fPath, fileName);
}

YIcon::YIcon(YPixmap *small, YPixmap *large) {
    fSmall = small;
    fLarge = large;
    loadedS = loadedL = true;
    fPath = 0;
    fNext = 0;
}

YIcon::~YIcon() {
    delete fLarge; fLarge = 0;
    delete fSmall; fSmall = 0;
    delete fPath; fPath = 0;
}

bool YIcon::findIcon(char *base, char **fullPath, int /*size*/) {
    /// !!! fix: do this at startup (merge w/ iconPath)
    pathelem *pe = icon_paths;
    for (; pe->root; pe++) {
        char *path = joinPath(pe, "/icons/", "");

        if (findPath(path, R_OK, base, fullPath, true)) {
            delete path;
            return true;
        }
        delete path;
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

#ifdef IMLIB    
    sprintf(icons_size, "%s", REDIR_ROOT(fPath));
    if (findIcon(icons_size, fullPath, size))
        return true;
#endif

    MSG(("Icon '%s' not found.", fPath));

    return false;
}

YPixmap *YIcon::loadIcon(int size) {
    YPixmap *icon = 0;

    if (icon == 0) {
#ifdef IMLIB
        if(fPath[0] == '/' && is_reg(fPath)) {
            icon = new YPixmap(fPath, size, size);
            if (icon == 0)
                warn(_("Out of memory for pixmap %s"), fPath);
        } else
#endif
        {
            char *fullPath;

            if (findIcon(&fullPath, size)) {
#ifdef IMLIB
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

YPixmap *YIcon::large() {
    if (fLarge == 0 && !loadedL)
        fLarge = loadIcon(ICON_LARGE);
    loadedL = true;
    return fLarge;
}

YPixmap *YIcon::small() {
    if (fSmall == 0 && !loadedS)
        fSmall = loadIcon(ICON_SMALL);
    loadedS = true;
    //return large(); // for testing menus...
    return fSmall;
}

YIcon *getIcon(const char *name) {
    YIcon *icn = firstIcon;

    while (icn) {
        if (strcmp(name, icn->iconName()) == 0)
            return icn;
        icn = icn->next();
    }
    return new YIcon(name);
}

void freeIcons() {
    YIcon *icn, *next;

    for (icn = firstIcon; icn; icn = next) {
        next = icn->next();
        delete icn;
    }
}
#endif
