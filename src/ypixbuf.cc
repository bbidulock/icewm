/*
 *  IceWM - Implementation of a RGB pixel buffer encapsulating libxpm
 *  Imlib or gdk-pixbuf and a scaler for RGB pixel buffers
 *
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Released under terms of the GNU Library General Public License
 */

#include "config.h"
#include "default.h"
#include "ypixbuf.h"
#include "yapp.h"

#ifdef CONFIG_IMLIB
#endif

#ifdef CONFIG_XPM
bool YPixbuf::init() {
    return false;
}
#endif

#ifdef CONFIG_IMLIB

ImlibData * hImlib(NULL);

bool YPixbuf::init() {
    if (disableImlibCaches) {
	ImlibInitParams parms;
	parms.flags = PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE;
	parms.imagecachesize = 0;
	parms.pixmapcachesize = 0;
	
	hImlib = Imlib_init_with_params(app->display(), &parms);
    } else
	hImlib = Imlib_init(app->display());

    return hImlib;
}

#endif

#ifdef CONFIG_GDK_PIXBUF

bool YPixbuf::init() {
    gdk_pixbuf_xlib_init(app->display(), DefaultScreen(app->display()));

    return false;
}

#endif

#ifdef CONFIG_ANTIALIASING

/******************************************************************************
 * A scaler for RGB pixel buffers
 ******************************************************************************/

struct YScaler {
    typedef unsigned long fixed;
    static fixed const fUnit = 1L << 12;
    static fixed const fPrec = 12;
    enum { R, G, B };

    /**************************************************************************/

    struct AccRow {
        AccRow(unsigned char const * src, unsigned const sLen,
		      unsigned char * dst, unsigned const dLen) {
	    unsigned long acc(0), accL(0), accR(0), accG(0), accB(0);
	    unsigned const inc(dLen), unit(sLen);

	    for (unsigned n(0); n < sLen; ++n, src+= 3) {
		if ((acc+= inc) >= unit) {
		    acc-= unit;
		    fixed const p((acc << fPrec) / unit);
		    fixed const q(fUnit - p);

		    accR+= p * src[R];
		    accG+= p * src[G];
		    accB+= p * src[B];
		    accL+= p;

		    dst[R] = accR / accL;
		    dst[G] = accG / accL;
		    dst[B] = accB / accL;
		    dst+= 3;

		    accR = q * src[R];
		    accG = q * src[G];
		    accB = q * src[B];
		    accL = q;
		} else {
		    accR+= src[R] << fPrec;
		    accG+= src[G] << fPrec;
		    accB+= src[B] << fPrec;
		    accL+= fUnit;
		}
	    }
	}
    };

    template <class RowScaler> struct AccLines {
	AccLines(unsigned char const * src, unsigned const sStep,
			unsigned const sw, unsigned const sh,
			unsigned char * dst, unsigned const dStep,
			unsigned const dw, unsigned const dh) {
	    unsigned const len(dw * 3);
	    unsigned long acc(0), accL(0), * accC(new unsigned long[len]);
	    memset(accC, 0, len * sizeof(*accC));

	    unsigned char * row(new unsigned char[len]);
	    unsigned const inc(dh), unit(sh);

	    for (unsigned n(0); n < sh; ++n, src+= sStep) {
		RowScaler(src, sw, row, dw);

		if ((acc+= inc) >= unit) {
		    acc-= unit;
		    fixed const p((acc << fPrec) / unit);
		    fixed const q(fUnit - p);

		    accL+= p;

		    for (unsigned c(0); c < len; ++c) {
			dst[c] = (accC[c] + p * row[c]) / accL;
		        accC[c] = q * row[c];
		    }

		    dst+= dStep;
		    accL = q;
		} else {
		    for (unsigned c(0); c < len; ++c)
			accC[c]+= row[c] << fPrec;

		    accL+= fUnit;
		}
	    }
	    
	    delete[] row;
	    delete[] accC;
	}
    };

    /**************************************************************************/

    struct CopyRow {
        CopyRow(unsigned char const * src, unsigned const /*sLen*/,
		unsigned char * dst, unsigned const dLen) {
	    memcpy(dst, src, 3 * dLen);
	}
    };

    template <class RowScaler> struct CopyLines {
	CopyLines(unsigned char const * src, unsigned const sStep,
		  unsigned const sw, unsigned const /*sh*/,
		  unsigned char * dst, unsigned const dStep,
		  unsigned const dw, unsigned const dh) {
	    for (unsigned n(0); n < dh; ++n, src+= sStep, dst+= dStep)
		RowScaler(src, sw, dst, dw);
	}
    };
    
    /**************************************************************************/

    struct IntRow {
        IntRow(unsigned char const * src, unsigned const sLen,
		       unsigned char * dst, unsigned const dLen) {
	    unsigned long acc(0);
	    unsigned const inc(sLen - 1), unit(dLen - 1);

	    for (unsigned n(0); n < dLen; ++n, dst+= 3) {
		fixed const p((acc << fPrec) / unit);
		fixed const q(fUnit - p);

		if (p) {
		    dst[R] = (src[R] * q + src[3 + R] * p) >> fPrec;
		    dst[G] = (src[G] * q + src[3 + G] * p) >> fPrec;
		    dst[B] = (src[B] * q + src[3 + B] * p) >> fPrec;
		} else
		    memcpy(dst, src, 3);

		if ((acc+= inc) >= unit) {
		    acc-= unit;
		    src+= 3;
		}
	    }
	}
    };

    template <class RowScaler> struct IntLines {
	IntLines(unsigned char const * src, unsigned const sStep,
		 unsigned const sw, unsigned const sh,
		 unsigned char * dst, unsigned const dStep,
		 unsigned const dw, unsigned const dh) {
	    unsigned const len(dw * 3);
	    unsigned char * a(new unsigned char[len]);
	    unsigned char * b(new unsigned char[len]);

	    RowScaler(src, sw, a, dw);
	    RowScaler(src+= sStep, sw, b, dw);

	    unsigned long acc(0);
	    unsigned const inc(sh - 1), unit(dh - 1);

	    for (unsigned n(dh); n > 1; --n, dst+= dStep) {
		fixed const p((acc << fPrec) / unit);
		fixed const q(fUnit - p);

		if (p)
		    for (unsigned c(0); c < len; ++c)
			dst[c] = (a[c] * q + b[c] * p) >> fPrec;
		else
		    memcpy(dst, a, len);

		if ((acc+= inc) >= unit) {
		    unsigned char * c(a); a = b; b = c;
		    if (n > 2) RowScaler(src+= sStep, sw, b, dw);
		    acc-= unit;
		}
	    }
	    memcpy(dst, a, len);

	    delete[] b;
	    delete[] a;
	}
    };

    /**************************************************************************/

    YScaler(unsigned char const * src, unsigned const sStep,
	    unsigned const sw, unsigned const sh,
	    unsigned char * dst, unsigned const dStep,
	    unsigned const dw, unsigned const dh);
};


YScaler::YScaler(unsigned char const * src, unsigned const sStep,
	    unsigned const sw, unsigned const sh,
	    unsigned char * dst, unsigned const dStep,
	    unsigned const dw, unsigned const dh) {
    if (sh < dh)
	if (sw < dw)
	    IntLines <IntRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
	else if (sw > dw)
	    IntLines <AccRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
	else
	    IntLines <CopyRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
    else if (sh > dh)
	if (sw < dw)
	    AccLines <IntRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
	else if (sw > dw)
	    AccLines <AccRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
	else
	    AccLines <CopyRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
    else
	if (sw < dw)
	    CopyLines <IntRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
	else if (sw > dw)
	    CopyLines <AccRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
	else
	    CopyLines <CopyRow> (src, sStep, sw, sh, dst, dStep, dw, dh);
}

/******************************************************************************
 * Imlib version of the pixel buffer
 ******************************************************************************/

#ifdef CONFIG_IMLIB

YPixbuf::YPixbuf(char const * filename):
    fImage(Imlib_load_image(hImlib, (char*) filename)) {
}

YPixbuf::YPixbuf(unsigned const width, unsigned const height) {
    unsigned char * empty(new unsigned char[width * height * 3]);
    fImage = Imlib_create_image_from_data(hImlib, empty, NULL, width, height);
    delete[] empty;
}

YPixbuf::YPixbuf(YPixbuf const & source,
		 unsigned const width, unsigned const height) {
    const unsigned rowstride(3 * width);
    unsigned char * pixels(new unsigned char[rowstride * height]);

    YScaler(source.pixels(), source.rowstride(),
	    source.width(), source.height(),
	    pixels, rowstride, width, height);

    fImage = Imlib_create_image_from_data(hImlib, pixels, NULL, width, height);
    delete[] pixels;
}

YPixbuf::~YPixbuf() {
    Imlib_kill_image(hImlib, fImage);
}

void YPixbuf::copyArea(YPixbuf const & src,
			    int const sx, int const sy,
			    unsigned const w, unsigned const h,
			    int const dx, int const dy) {
    Pixel * sp(src.pixels() + sy * src.rowstride() + sx * 3);
    Pixel * dp(pixels() + dy * rowstride() + dx * 3);

    for (int y = h; y > 0; --y, sp+= src.rowstride(), dp+= rowstride())
	memcpy(dp, sp, w * 3);
}

void YPixbuf::copyToDrawable(Drawable drawable, GC gc,
			     int const sx, int const sy,
			     unsigned const w, unsigned const h,
			     int const dx, int const dy) {
#if 0			    
    Imlib_render(hImlib, fImage, width(), height());
    Pixmap pixmap(Imlib_move_image(hImlib, fImage));
    XCopyArea(app->display(), pixmap, drawable, gc, sx, sy, w, h, dx, dy);
    Imlib_free_pixmap(hImlib, pixmap);
#else    
    if (fImage) {
        if (fImage->pixmap == None)
	    Imlib_render(hImlib, fImage, width(), height());

	XCopyArea(app->display(), fImage->pixmap, drawable, gc,
		  sx, sy, w, h, dx, dy);
    }
#endif
}

#endif

/******************************************************************************
 * gdk-pixbuf version of the pixel buffer
 ******************************************************************************/

#ifdef CONFIG_GDK_PIXBUF

YPixbuf::YPixbuf(char const * filename):
    fPixbuf(gdk_pixbuf_new_from_file(filename)) {
}

YPixbuf::YPixbuf(unsigned const width, unsigned const height):
    fPixbuf(gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, width, height)) {
}

YPixbuf::~YPixbuf() {
    gdk_pixbuf_unref(fPixbuf);
}

void YPixbuf::copyArea(YPixbuf const & src,
		       int const sx, int const sy,
		       unsigned const w, unsigned const h, 
		       int const dx, int const dy) {
    gdk_pixbuf_copy_area(src.fPixbuf, sx, sy, w, h, fPixbuf, dx, dy);
}

#endif

#endif
