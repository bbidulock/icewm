/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */	
#include "config.h"
#include "yfull.h"
#include "ypixbuf.h"
#include "ypaint.h"
#include "yxapp.h"
#include "sysdep.h"
#include "prefs.h"
#include "yprefs.h"
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
    return createPixmap(w, h, xapp->depth());
}

Pixmap YPixmap::createPixmap(int w, int h, int depth) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, depth);
}

Pixmap YPixmap::createMask(int w, int h) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, 1);
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
    Graphics(fPixmap, pixbuf.width(), pixbuf.height()).copyPixbuf(pixbuf, 0, 0, fWidth, fHeight, 0, 0, false);
    Graphics(fMask, pixbuf.width(), pixbuf.height()).copyAlphaMask(pixbuf, 0, 0, fWidth, fHeight, 0, 0);
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
    xpmAttributes.colormap  = xapp->colormap();
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    int const rc(XpmReadFileToPixmap(xapp->display(), desktop->handle(),
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
	Graphics g(fMask, fWidth, fHeight);

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
            XFreePixmap(xapp->display(), fPixmap);
        if (fMask != None)
            XFreePixmap(xapp->display(), fMask);
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
	Graphics(nPixmap, dim, height()).repHorz(fPixmap, width(), height(), 0, 0, dim);
    else
	Graphics(nPixmap, width(), dim).repVert(fPixmap, width(), height(), 0, 0, dim);

    if (nMask != None)
	if (horiz)
	    Graphics(nMask, dim, height()).repHorz(fMask, width(), height(), 0, 0, dim);
	else
	    Graphics(nMask, width(), dim).repVert(fMask, width(), height(), 0, 0, dim);

    if (fOwned) {
        if (fPixmap != None)
            XFreePixmap(xapp->display(), fPixmap);
        if (fMask != None)
            XFreePixmap(xapp->display(), fMask);
    }

    fPixmap = nPixmap;
    fMask = nMask;

    (horiz ? fWidth : fHeight) = dim;
}


