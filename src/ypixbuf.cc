/*
 *  IceWM - Implementation of a RGB pixel buffer encapsulating libxpm
 *  Imlib or gdk-pixbuf and a scaler for RGB pixel buffers
 *
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Released under terms of the GNU Library General Public License
 *
 *  2001/05/30: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *	- libxpm support
 *
 *  2001/05/18: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *	- initial revision (Imlib only)
 */

#include "config.h"
#include "default.h"
#include "intl.h"

#include "ypixbuf.h"
#include "base.h"
#include "yapp.h"

#include "X11/X.h"

#ifdef CONFIG_XPM
#include <string.h>

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
	    if (sh > 1) RowScaler(src+= sStep, sw, b, dw);

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
 * libxpm version of the pixel buffer
 ******************************************************************************/
/* !!! TODO: dithering, 8 bit visuals
 */

#ifdef CONFIG_XPM

#define CHANNEL_MASK(Im,Rm,Gm,Bm) \
    ((Im)->red_mask == (Rm) && \
     (Im)->green_mask == (Gm) && \
     (Im)->blue_mask == (Bm))

/******************************************************************************/

void copyRGB32ToPixbuf(char const * src, unsigned const sStep,
		       unsigned char * dst, unsigned const dStep,
		       unsigned const width, unsigned const height) {
    MSG(("copyRGB32ToPixbuf"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	char const * s(src); unsigned char * d(dst);
	for (unsigned x(width); x-- > 0; s+= 4, dst+= 3) memcpy(d, s, 3);
    }
}

void copyRGB565ToPixbuf(char const * src, unsigned const sStep,
		        unsigned char * dst, unsigned const dStep,
		        unsigned const width, unsigned const height) {
    MSG(("copyRGB565ToPixbuf"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	yuint16 const * s((yuint16*)src); unsigned char * d(dst);
	for (unsigned x(width); x-- > 0; d+= 3, ++s) {
	    d[0] = (*s >> 8) & 0xf8;
	    d[1] = (*s >> 3) & 0xfc;
	    d[2] = (*s << 3) & 0xf8;
	}
    }
}

void copyRGB555ToPixbuf(char const * src, unsigned const sStep,
		        unsigned char * dst, unsigned const dStep,
		        unsigned const width, unsigned const height) {
    MSG(("copyRGB555ToPixbuf"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	yuint16 const * s((yuint16*)src); unsigned char * d(dst);
	for (unsigned x(width); x-- > 0; d+= 3, ++s) {
	    d[0] = (*s >> 7) & 0xf8;
	    d[1] = (*s >> 2) & 0xf8;
	    d[2] = (*s << 3) & 0xf8;
	}
    }
}

template <class Pixel>
void copyRGBAnyToPixbuf(char const * src, unsigned const sStep,
		        unsigned char * dst, unsigned const dStep,
		        unsigned const width, unsigned const height,
			unsigned const rMask, unsigned const gMask,
			unsigned const bMask) {
    warn(_("Using fallback mechanism to convert pixels "
    	   "(depth: %d; red/green/blue-mask: %0*x/%0*x/%0*x)"),
	   sizeof(Pixel) * 8,
	   sizeof(Pixel) * 2, rMask, sizeof(Pixel) * 2, gMask, 
	   sizeof(Pixel) * 2, bMask);

    unsigned const rShift(lowbit(rMask));
    unsigned const gShift(lowbit(gMask));
    unsigned const bShift(lowbit(bMask));

    unsigned const rLoss(7 + rShift - highbit(rMask));
    unsigned const gLoss(7 + gShift - highbit(gMask));
    unsigned const bLoss(7 + bShift - highbit(bMask));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	Pixel const * s((Pixel*)src); unsigned char * d(dst);
	for (unsigned x(width); x-- > 0; d+= 3, ++s) {
	    d[0] = ((*s & rMask) >> rShift) << rLoss;
	    d[1] = ((*s & gMask) >> gShift) << gLoss;
	    d[2] = ((*s & bMask) >> bShift) << bLoss;
	}
    }
}

/******************************************************************************/

void copyPixbufToRGB32(unsigned char const * src, unsigned const sStep,
		       char * dst, unsigned const dStep,
		       unsigned const width, unsigned const height) {
    MSG(("copyPixbufToRGB32"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	unsigned char const * s(src); char * d(dst);
	for (unsigned x(width); x-- > 0; s+= 3, dst+= 4) memcpy(d, s, 3);
    }
}

void copyPixbufToRGB565(unsigned char const * src, unsigned const sStep,
		        char * dst, unsigned const dStep,
		        unsigned const width, unsigned const height) {
    MSG(("copyPixbufToRGB565"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	unsigned char const * s(src); yuint16 * d((yuint16*)dst);
	for (unsigned x(width); x-- > 0; ++d, s+= 3)
	    *d = ((((yuint16)s[0]) << 8) & 0xf800)
	       | ((((yuint16)s[1]) << 3) & 0x07e0)
	       | ((((yuint16)s[2]) >> 3) & 0x001f);
    }
}

void copyPixbufToRGB555(unsigned char const * src, unsigned const sStep,
		        char * dst, unsigned const dStep,
		        unsigned const width, unsigned const height) {
    MSG(("copyPixbufToRGB555"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	unsigned char const * s(src); yuint16 * d((yuint16*)dst);
	for (unsigned x(width); x-- > 0; ++d, s+= 3)
	    *d = ((((yuint16)s[0]) << 7) & 0x7c00)
	       | ((((yuint16)s[1]) << 2) & 0x03e0)
	       | ((((yuint16)s[2]) >> 3) & 0x001f);
    }
}

template <class Pixel>
void copyPixbufToRGBAny(unsigned char const * src, unsigned const sStep,
		        char * dst, unsigned const dStep,
		        unsigned const width, unsigned const height,
			unsigned const rMask, unsigned const gMask,
			unsigned const bMask) {
    warn(_("Using fallback mechanism to convert pixels "
    	   "(depth: %d; red/green/blue-mask: %0*x/%0*x/%0*x)"),
	   sizeof(Pixel) * 8,
	   sizeof(Pixel) * 2, rMask, sizeof(Pixel) * 2, gMask, 
	   sizeof(Pixel) * 2, bMask);

    unsigned const rShift(lowbit(rMask));
    unsigned const gShift(lowbit(gMask));
    unsigned const bShift(lowbit(bMask));

    unsigned const rLoss(7 + rShift - highbit(rMask));
    unsigned const gLoss(7 + gShift - highbit(gMask));
    unsigned const bLoss(7 + bShift - highbit(bMask));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	unsigned char const * s(src); Pixel * d((Pixel*)dst);
	for (unsigned x(width); x-- > 0; s+= 3, ++d)
	    *d = (((((Pixel)s[0]) >> rLoss) << rShift) & rMask)
	       | (((((Pixel)s[1]) >> gLoss) << gShift) & gMask)
	       | (((((Pixel)s[2]) >> bLoss) << bShift) & bMask);
    }
}

/******************************************************************************/

YPixbuf::YPixbuf(char const * filename):
    fPixmap(None) {
    XpmAttributes xpmAttributes;
    xpmAttributes.colormap  = defaultColormap;
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    XImage * image, * mask;
    int const rc(XpmReadFileToImage(app->display(),
				    (char *)REDIR_ROOT(filename), // !!!
				    &image, &mask, &xpmAttributes));

    if (rc == XpmSuccess) {
	fWidth = xpmAttributes.width;
	fHeight = xpmAttributes.height;
	fRowStride = (fWidth * 3 + 3) & ~3;
	fPixels = new unsigned char[fRowStride * fHeight];

	if (image->bitmap_pad == 32)
	    if (CHANNEL_MASK(image, 0xff0000, 0x00ff00, 0x0000ff) ||
		CHANNEL_MASK(image, 0x0000ff, 0x00ff00, 0xff0000))
		copyRGB32ToPixbuf(image->data, image->bytes_per_line,
				  fPixels, fRowStride, fWidth, fHeight);
	    else
		copyRGBAnyToPixbuf<yuint32>(image->data, image->bytes_per_line, 
		    fPixels, fRowStride,fWidth, fHeight,
		    image->red_mask, image->green_mask, image->blue_mask);
	else if (image->bitmap_pad == 16)
	    if (CHANNEL_MASK(image, 0xf800, 0x07e0, 0x001f) ||
		CHANNEL_MASK(image, 0x001f, 0x07e0, 0xf800))
		copyRGB565ToPixbuf(image->data, image->bytes_per_line,
		    fPixels, fRowStride, fWidth, fHeight);
	    else if (CHANNEL_MASK(image, 0x7c00, 0x03e0, 0x001f) ||
		     CHANNEL_MASK(image, 0x001f, 0x03e0, 0x7c00))
		copyRGB555ToPixbuf(image->data, image->bytes_per_line,
		    fPixels, fRowStride, fWidth, fHeight);
	    else
		copyRGBAnyToPixbuf<yuint16>(image->data, image->bytes_per_line,
		     fPixels, fRowStride, fWidth, fHeight,
		     image->red_mask, image->green_mask, image->blue_mask);
	else
	    warn(_("%s:%d: 8 bit visuals are not supported (yet)"),
	    	   __FILE__, __LINE__);

	if (image) XDestroyImage(image);
	if (mask) XDestroyImage(mask);
    } else {
	fWidth = fHeight = fRowStride = 0;
	fPixels = NULL;

        warn(_("Loading of pixmap \"%s\" failed: %s"),
	       filename, XpmGetErrorString(rc));
    }
}

YPixbuf::YPixbuf(unsigned const width, unsigned const height):
    fWidth(width), fHeight(height), fRowStride((width * 3 + 3) & ~3),
    fPixmap(None) {
    fPixels = new unsigned char[fRowStride * fHeight];
}

YPixbuf::YPixbuf(YPixbuf const & source,
		 unsigned const width, unsigned const height):
    fWidth(width), fHeight(height), fRowStride((width * 3 + 3) & ~3),
    fPixmap(None) {
    fPixels = new unsigned char[fRowStride * fHeight];

    YScaler(source.pixels(), source.rowstride(),
	    source.width(), source.height(),
	    fPixels, fRowStride, fWidth, fHeight);
}

YPixbuf::~YPixbuf() {
    if (fPixmap != None)
	XFreePixmap(app->display(), fPixmap);

    delete[] fPixels;
}

void YPixbuf::copyToDrawable(Drawable drawable, GC gc,
			     int const sx, int const sy,
			     unsigned const w, unsigned const h,
			     int const dx, int const dy) {
    if (fPixmap == None) {
    int const depth(DefaultDepth(app->display(), DefaultScreen(app->display())));
    int const bitPadding(depth > 16 ? 32 : depth > 8 ? 16 : 8);
    int const rowStride(((fWidth * bitPadding >> 3) + 3) & ~3);
    char * pixels = new char[rowStride * fHeight];

    fPixmap = XCreatePixmap(app->display(), drawable, fWidth, fHeight, depth);
    Graphics g(fPixmap);    
    g.fillRect(0, 0, fWidth, fHeight);

    XImage * image(XCreateImage(app->display(),
				DefaultVisual(app->display(), 
				    DefaultScreen(app->display())),
				depth, ZPixmap, 0, pixels, fWidth, fHeight, 
				bitPadding, rowStride));

    if (image) {
	if (bitPadding == 32)
	    if (CHANNEL_MASK(image, 0xff0000, 0x00ff00, 0x0000ff) ||
		CHANNEL_MASK(image, 0x0000ff, 0x00ff00, 0xff0000))
		copyPixbufToRGB32(fPixels, fRowStride,
		     image->data, image->bytes_per_line, fWidth, fHeight);
	    else
		copyPixbufToRGBAny<yuint32>(fPixels, fRowStride,
		     image->data, image->bytes_per_line, fWidth, fHeight,
		     image->red_mask, image->green_mask, image->blue_mask);
	else if (bitPadding == 16)
	    if (CHANNEL_MASK(image, 0xf800, 0x07e0, 0x001f) ||
		CHANNEL_MASK(image, 0x001f, 0x07e0, 0xf800))
		copyPixbufToRGB565(fPixels, fRowStride,
		     image->data, image->bytes_per_line, fWidth, fHeight);
	    else if (CHANNEL_MASK(image, 0x7c00, 0x03e0, 0x001f) ||
		     CHANNEL_MASK(image, 0x001f, 0x03e0, 0x7c00))
		copyPixbufToRGB555(fPixels, fRowStride,
		     image->data, image->bytes_per_line, fWidth, fHeight);
	    else
		copyPixbufToRGBAny<yuint16>(fPixels, fRowStride,
		     image->data, image->bytes_per_line, fWidth, fHeight,
		     image->red_mask, image->green_mask, image->blue_mask);
	else
	    warn(_("%s:%d: 8 bit visuals are not supported (yet)"),
	    	   __FILE__, __LINE__);

	Graphics(fPixmap).copyImage(image, 0, 0);
	XDestroyImage(image);
    } else {
	warn(_("Failed to render pixel buffer"));
	delete[] pixels;
    }
    }

    if (fPixmap != None)
	XCopyArea(app->display(), fPixmap, drawable, gc, sx, sy, w, h, dx, dy);
}

#endif

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
