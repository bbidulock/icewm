/*
 *  IceWM - Implementation of a RGB pixel buffer encapsulating
 *  libxpm and Imlib plus a scaler for RGB pixel buffers
 *
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Released under terms of the GNU Library General Public License
 *
 *  2001/07/06: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *	- added 12bpp converters
 *
 *  2001/07/05: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *	- fixed some 24bpp oddities
 *
 *  2001/06/12: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *	- 8 bit alpha channel for libxpm version
 *
 *  2001/06/10: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *	- support for 8 bit alpha channels
 *	- from-drawable-constructor for Imlib version
 *
 *  2001/06/03: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *	- libxpm version finished (expect for dithering and 8 bit visuals)
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

/******************************************************************************/

#define CHANNEL_MASK(I,R,G,B) \
    ((I).red_mask == (R) && (I).green_mask == (G) && (I).blue_mask == (B))

/******************************************************************************/

#ifdef CONFIG_XPM
#include <string.h>

bool YPixbuf::init() {
    return false;
}
#endif

/******************************************************************************/

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

/******************************************************************************/

#ifdef CONFIG_ANTIALIASING

/******************************************************************************
 * A scaler for Grayscale/RGB/RGBA pixel buffers
 ******************************************************************************/

template <class Pixel, int Channels> struct YScaler {
    typedef unsigned long fixed;
    static fixed const fUnit = 1L << 12;
    static fixed const fPrec = 12;
    enum { R, G, B, A };

    /**************************************************************************/

    struct AccRow {
        AccRow(Pixel const * src, unsigned const sLen,
	       Pixel * dst, unsigned const dLen) {
	    unsigned long acc(0), accL(0), accR(0), accG(0), accB(0), accA(0);
	    unsigned const inc(dLen), unit(sLen);

	    for (unsigned n(0); n < sLen; ++n, src+= Channels) {
		if ((acc+= inc) >= unit) {
		    acc-= unit;
		    fixed const p((acc << fPrec) / unit);
		    fixed const q(fUnit - p);

		    if (Channels == 1) {
			accA+= p * *src;
			accL+= p;
			*dst = accA / accL;
			accA = q * *src;
		    } else if (Channels == 3) {
			accR+= p * src[R];
			accG+= p * src[G];
			accB+= p * src[B];

			accL+= p;

			dst[R] = accR / accL;
			dst[G] = accG / accL;
			dst[B] = accB / accL;

			accR = q * src[R];
			accG = q * src[G];
			accB = q * src[B];
		    } else if (Channels == 4) {
			accR+= p * src[R];
			accG+= p * src[G];
			accB+= p * src[B];
			accA+= p * src[A];

			accL+= p;

			dst[R] = accR / accL;
			dst[G] = accG / accL;
			dst[B] = accB / accL;
			dst[A] = accA / accL;

			accR = q * src[R];
			accG = q * src[G];
			accB = q * src[B];
			accA = q * src[A];
		    }

		    dst+= Channels;
		    accL = q;
		} else {
		    if (Channels == 1) {
			accA+= *src << fPrec;
		    } else if (Channels == 3) {
			accR+= src[R] << fPrec;
			accG+= src[G] << fPrec;
			accB+= src[B] << fPrec;
		    } else if (Channels == 4) {
			accR+= src[R] << fPrec;
			accG+= src[G] << fPrec;
			accB+= src[B] << fPrec;
			accA+= src[A] << fPrec;
		    }

		    accL+= fUnit;
		}
	    }
	}
    };

    template <class RowScaler> struct AccLines {
	AccLines(Pixel const * src, unsigned const sStep,
		 unsigned const sw, unsigned const sh,
		 Pixel * dst, unsigned const dStep,
		 unsigned const dw, unsigned const dh) {
	    unsigned const len(dw * Channels);
	    unsigned long acc(0), accL(0), * accC(new unsigned long[len]);
	    memset(accC, 0, len * sizeof(*accC));

	    Pixel * row(new Pixel[len]);
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
        CopyRow(Pixel const * src, unsigned const /*sLen*/,
		Pixel * dst, unsigned const dLen) {
	    memcpy(dst, src, Channels * sizeof(Pixel) * dLen);
	}
    };

    template <class RowScaler> struct CopyLines {
	CopyLines(Pixel const * src, unsigned const sStep,
		  unsigned const sw, unsigned const /*sh*/,
		  Pixel * dst, unsigned const dStep,
		  unsigned const dw, unsigned const dh) {
	    for (unsigned n(0); n < dh; ++n, src+= sStep, dst+= dStep)
		RowScaler(src, sw, dst, dw);
	}
    };
    
    /**************************************************************************/

    struct IntRow {
        IntRow(Pixel const * src, unsigned const sLen,
	       Pixel * dst, unsigned const dLen) {
	    unsigned long acc(0);
	    unsigned const inc(sLen - 1), unit(dLen - 1);

	    for (unsigned n(0); n < dLen; ++n, dst+= Channels) {
		fixed const p((acc << fPrec) / unit);
		fixed const q(fUnit - p);

		if (p) {
		    if (Channels == 1) {
			*dst = (src[0] * q + src[1] * p) >> fPrec;
		    } else if (Channels == 3) {
			dst[R] = (src[R] * q + src[3 + R] * p) >> fPrec;
			dst[G] = (src[G] * q + src[3 + G] * p) >> fPrec;
			dst[B] = (src[B] * q + src[3 + B] * p) >> fPrec;
		    } else if (Channels == 4) {
			dst[R] = (src[R] * q + src[4 + R] * p) >> fPrec;
			dst[G] = (src[G] * q + src[4 + G] * p) >> fPrec;
			dst[B] = (src[B] * q + src[4 + B] * p) >> fPrec;
			dst[A] = (src[A] * q + src[4 + A] * p) >> fPrec;
		    }
		} else
		    memcpy(dst, src, Channels * sizeof(Pixel));

		if ((acc+= inc) >= unit) {
		    acc-= unit;
		    src+= Channels;
		}
	    }
	}
    };

    template <class RowScaler> struct IntLines {
	IntLines(Pixel const * src, unsigned const sStep,
		 unsigned const sw, unsigned const sh,
		 Pixel * dst, unsigned const dStep,
		 unsigned const dw, unsigned const dh) {
	    unsigned const len(dw * Channels);
	    Pixel * a(new Pixel[len]);
	    Pixel * b(new Pixel[len]);

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
		    memcpy(dst, a, len * sizeof(Pixel));

		if ((acc+= inc) >= unit) {
		    Pixel * c(a); a = b; b = c;
		    if (n > 2) RowScaler(src+= sStep, sw, b, dw);
		    acc-= unit;
		}
	    }
	    memcpy(dst, a, len * sizeof(Pixel));

	    delete[] b;
	    delete[] a;
	}
    };

    /**************************************************************************/

    YScaler(Pixel const * src, unsigned const sStep,
	    unsigned const sw, unsigned const sh,
	    Pixel * dst, unsigned const dStep,
	    unsigned const dw, unsigned const dh);
};

template <class Pixel, int Channels>
YScaler<Pixel, Channels>::YScaler
	(Pixel const * src, unsigned const sStep,
	 unsigned const sw, unsigned const sh,
	 Pixel * dst, unsigned const dStep,
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
 * A scaler for RGB pixel buffers
 ******************************************************************************/

template <int Channels>
static void copyRGB32ToPixbuf(char const * src, unsigned const sStep,
			      unsigned char * dst, unsigned const dStep,
			      unsigned const width, unsigned const height) {
    MSG(("copyRGB32ToPixbuf"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	char const * s(src); unsigned char * d(dst);
	for (unsigned x(width); x-- > 0; s+= 4, d+= Channels)
	{
		d[0] = s[2];
		d[1] = s[1];
		d[2] = s[0];
	}
    }
}

template <int Channels>
static void copyRGB565ToPixbuf(char const * src, unsigned const sStep,
			       unsigned char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height) {
    MSG(("copyRGB565ToPixbuf"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	yuint16 const * s((yuint16*)src); unsigned char * d(dst);
	for (unsigned x(width); x-- > 0; d+= Channels, ++s) {
	    d[0] = (*s >> 8) & 0xf8;
	    d[1] = (*s >> 3) & 0xfc;
	    d[2] = (*s << 3) & 0xf8;
	}
    }
}

template <int Channels>
static void copyRGB555ToPixbuf(char const * src, unsigned const sStep,
			       unsigned char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height) {
    MSG(("copyRGB555ToPixbuf"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	yuint16 const * s((yuint16*)src); unsigned char * d(dst);
	for (unsigned x(width); x-- > 0; d+= Channels, ++s) {
	    d[0] = (*s >> 7) & 0xf8;
	    d[1] = (*s >> 2) & 0xf8;
	    d[2] = (*s << 3) & 0xf8;
	}
    }
}

template <int Channels>
static void copyRGB444ToPixbuf(char const * src, unsigned const sStep,
			       unsigned char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height) {
    MSG(("copyRGB444ToPixbuf"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	yuint16 const * s((yuint16*)src); unsigned char * d(dst);
	for (unsigned x(width); x-- > 0; d+= Channels, ++s) {
	    d[0] = (*s >> 4) & 0xf0;
	    d[1] =  *s       & 0xf0;
	    d[2] = (*s << 4) & 0xf0;
	}
    }
}

template <class Pixel, int Channels>
static void copyRGBAnyToPixbuf(char const * src, unsigned const sStep,
			       unsigned char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height,
			       unsigned const rMask, unsigned const gMask,
			       unsigned const bMask) {
    warn(_("Using fallback mechanism to convert pixels "
    	   "(depth: %d; masks (red/green/blue): %0*x/%0*x/%0*x)"),
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
	for (unsigned x(width); x-- > 0; d+= Channels, ++s) {
	    d[0] = ((*s & rMask) >> rShift) << rLoss;
	    d[1] = ((*s & gMask) >> gShift) << gLoss;
	    d[2] = ((*s & bMask) >> bShift) << bLoss;
	}
    }
}

template <int Channels>
static void copyBitmapToPixbuf(char const * src, unsigned const sStep,
			       unsigned char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height) {
    MSG(("copyBitmapToPixbuf<%d>(%p,%d,%p,%d,%d,%d)", Channels,
    	 src, sStep, dst, dStep, width, height));

    for (unsigned y(height); y; --y, src+= sStep, dst+= dStep) {
	char const * s(src); unsigned char * d(dst);
	for (unsigned x(width), t(*s), m(1); x; --x, d+= Channels, m<<= 1) {
	    if (m & 256) { m = 1; t = *(++s); }
	    *d = (t & m ? 255 : 0);
	}
    }
}

/******************************************************************************/

template <int Channels>
static YPixbuf::Pixel * copyImageToPixbuf(XImage & image,
					  unsigned const rowstride) {
    unsigned const width(image.width), height(image.height);
    YPixbuf::Pixel * pixels = new YPixbuf::Pixel[rowstride * height];

    if (!(image.red_mask && image.green_mask && image.blue_mask)) {
	Visual const * visual(app->visual());
	image.red_mask = visual->red_mask;
	image.green_mask = visual->green_mask;
	image.blue_mask = visual->blue_mask;
    }

    if (image.depth > 16) {
	if (CHANNEL_MASK(image, 0xff0000, 0x00ff00, 0x0000ff) ||
	    CHANNEL_MASK(image, 0x0000ff, 0x00ff00, 0xff0000))
	    copyRGB32ToPixbuf<Channels> (image.data, image.bytes_per_line,
					 pixels, rowstride, width, height);
	else
	    copyRGBAnyToPixbuf<yuint32, Channels>
		(image.data, image.bytes_per_line, 
		 pixels, rowstride, width, height,
		 image.red_mask, image.green_mask, image.blue_mask);
    } else if (image.depth > 8) {
	if (CHANNEL_MASK(image, 0xf800, 0x07e0, 0x001f) ||
	    CHANNEL_MASK(image, 0x001f, 0x07e0, 0xf800))
	    copyRGB565ToPixbuf<Channels> (image.data, image.bytes_per_line,
					  pixels, rowstride, width, height);
	else if (CHANNEL_MASK(image, 0x7c00, 0x03e0, 0x001f) ||
		 CHANNEL_MASK(image, 0x001f, 0x03e0, 0x7c00))
	    copyRGB555ToPixbuf<Channels> (image.data, image.bytes_per_line,
					  pixels, rowstride, width, height);
	else if (CHANNEL_MASK(image, 0xf00, 0x0f0, 0x00f) ||
		 CHANNEL_MASK(image, 0x00f, 0x0f0, 0xf00))
	    copyRGB444ToPixbuf<Channels> (image.data, image.bytes_per_line,
					  pixels, rowstride, width, height);
	else
	    copyRGBAnyToPixbuf<yuint16, Channels>
		(image.data, image.bytes_per_line,
		 pixels, rowstride, width, height,
		 image.red_mask, image.green_mask, image.blue_mask);
    } else
	warn(_("%s:%d: %d bit visuals are not supported (yet)"),
	     __FILE__, __LINE__, image.depth);

    return pixels;
}

/******************************************************************************/
/******************************************************************************/

#ifdef CONFIG_XPM

template <int Channels>
static void copyPixbufToRGB32(unsigned char const * src, unsigned const sStep,
			      char * dst, unsigned const dStep,
			      unsigned const width, unsigned const height) {
    MSG(("copyPixbufToRGB32"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	unsigned char const * s(src); char * d(dst);
	for (unsigned x(width); x-- > 0; s+= Channels, d+= 4)
	{
		d[0] = s[2];
		d[1] = s[1];
		d[2] = s[0];
	}		
    }
}

template <int Channels>
static void copyPixbufToRGB565(unsigned char const * src, unsigned const sStep,
			       char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height) {
    MSG(("copyPixbufToRGB565"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	unsigned char const * s(src); yuint16 * d((yuint16*)dst);
	for (unsigned x(width); x-- > 0; ++d, s+= Channels)
	    *d = ((((yuint16)s[0]) << 8) & 0xf800)
	       | ((((yuint16)s[1]) << 3) & 0x07e0)
	       | ((((yuint16)s[2]) >> 3) & 0x001f);
    }
}

template <int Channels>
static void copyPixbufToRGB555(unsigned char const * src, unsigned const sStep,
			       char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height) {
    MSG(("copyPixbufToRGB555"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	unsigned char const * s(src); yuint16 * d((yuint16*)dst);
	for (unsigned x(width); x-- > 0; ++d, s+= Channels)
	    *d = ((((yuint16)s[0]) << 7) & 0x7c00)
	       | ((((yuint16)s[1]) << 2) & 0x03e0)
	       | ((((yuint16)s[2]) >> 3) & 0x001f);
    }
}

template <int Channels>
static void copyPixbufToRGB444(unsigned char const * src, unsigned const sStep,
			       char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height) {
    MSG(("copyPixbufToRGB444"));

    for (unsigned y(height); y > 0; --y, src+= sStep, dst+= dStep) {
	unsigned char const * s(src); yuint16 * d((yuint16*)dst);
	for (unsigned x(width); x-- > 0; ++d, s+= Channels)
	    *d = ((((yuint16)s[0]) << 4) & 0xf00)
	       |  (((yuint16)s[1])       & 0x0f0)
	       | ((((yuint16)s[2]) >> 4) & 0x00f);
    }
}

template <class Pixel, int Channels>
static void copyPixbufToRGBAny(unsigned char const * src, unsigned const sStep,
			       char * dst, unsigned const dStep,
			       unsigned const width, unsigned const height,
			       unsigned const rMask, unsigned const gMask,
			       unsigned const bMask) {
    warn(_("Using fallback mechanism to convert pixels "
    	   "(depth: %d; masks (red/green/blue): %0*x/%0*x/%0*x)"),
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
	for (unsigned x(width); x-- > 0; s+= Channels, ++d)
	    *d = (((((Pixel)s[0]) >> rLoss) << rShift) & rMask)
	       | (((((Pixel)s[1]) >> gLoss) << gShift) & gMask)
	       | (((((Pixel)s[2]) >> bLoss) << bShift) & bMask);
    }
}

/******************************************************************************/

template <int Channels>
static void copyPixbufToImage(YPixbuf::Pixel const * pixels,
			      XImage & image, unsigned const rowstride) {
    unsigned const width(image.width), height(image.height);

    if (image.depth > 16) {
	if (CHANNEL_MASK(image, 0xff0000, 0x00ff00, 0x0000ff) ||
	    CHANNEL_MASK(image, 0x0000ff, 0x00ff00, 0xff0000))
	    copyPixbufToRGB32<Channels> (pixels, rowstride,
		     image.data, image.bytes_per_line, width, height);
	else
	    copyPixbufToRGBAny<yuint32, Channels> (pixels, rowstride,
		image.data, image.bytes_per_line, width, height,
		image.red_mask, image.green_mask, image.blue_mask);
    } else if (image.depth > 8) {
	if (CHANNEL_MASK(image, 0xf800, 0x07e0, 0x001f) ||
	    CHANNEL_MASK(image, 0x001f, 0x07e0, 0xf800))
	    copyPixbufToRGB565<Channels> (pixels, rowstride,
		image.data, image.bytes_per_line, width, height);
	else if (CHANNEL_MASK(image, 0x7c00, 0x03e0, 0x001f) ||
		 CHANNEL_MASK(image, 0x001f, 0x03e0, 0x7c00))
	    copyPixbufToRGB555<Channels> (pixels, rowstride,
		image.data, image.bytes_per_line, width, height);
	else if (CHANNEL_MASK(image, 0xf00, 0x0f0, 0x00f) ||
		 CHANNEL_MASK(image, 0x00f, 0x0f0, 0xf00))
	    copyPixbufToRGB444<Channels> (pixels, rowstride,
		image.data, image.bytes_per_line, width, height);
	else
	    copyPixbufToRGBAny<yuint16, Channels> (pixels, rowstride,
		image.data, image.bytes_per_line, width, height,
		image.red_mask, image.green_mask, image.blue_mask);
    } else
	warn(_("%s:%d: %d bit visuals are not supported (yet)"),
	     __FILE__, __LINE__, image.depth);
}

#endif

/******************************************************************************
 * shared code
 ******************************************************************************/

void YPixbuf::copyArea(YPixbuf const & src, int sx, int sy,
		       unsigned w, unsigned h, int dx, int dy) {
    if (sx < 0) { dx-= sx; w+= sx; sx = 0; }
    if (sy < 0) { dy-= sy; h+= sy; sy = 0; }
    if (dx < 0) { sx-= dx; w+= dx; dx = 0; }
    if (dy < 0) { sy-= dy; h+= dy; dy = 0; }
    
    w = min(min(w, width()), src.width());
    h = min(min(h, height()), src.height());

    if (src.alpha()) {
	unsigned const deltaS(src.inlineAlpha() ? 4 : 3);
	unsigned const deltaA(src.inlineAlpha() ? 4 : 1);
	unsigned const deltaD(alpha () && inlineAlpha() ? 4 : 3);
	
	unsigned const sRowStride(src.rowstride());
	unsigned const aRowStride(src.inlineAlpha() ? src.rowstride()
						    : src.width());
	unsigned const dRowStride(rowstride());

	Pixel const * sp(src.pixels() + sy * sRowStride + sx * deltaS);
	Pixel const * ap(src.alpha() + sy * aRowStride + sx * deltaA);
	Pixel * dp(pixels() + dy * dRowStride + dx * deltaD);

	for (unsigned y(h); y; --y, sp+= sRowStride, ap+= aRowStride,
				    dp+= dRowStride) {
	    Pixel const * s(sp), * a(ap); Pixel * d(dp);
	    for (unsigned x(w); x; --x, s+= deltaS, a+= deltaA, d+= deltaD) {
		unsigned p(*a), q(255 - p);
		d[0] = (p * s[0] + q * d[0]) / 255;
		d[1] = (p * s[1] + q * d[1]) / 255;
		d[2] = (p * s[2] + q * d[2]) / 255;
	    }
	}

	if (alpha()) {
	    unsigned const deltaS(src.inlineAlpha() ? 4 : 3);
	    unsigned const deltaD(inlineAlpha() ? 4 : 3);

	    unsigned const sRowStride(src.inlineAlpha() ? src.rowstride()
						        : src.width());
	    unsigned const dRowStride(inlineAlpha() ? rowstride()
						    : width());

	    Pixel const * sp(src.alpha() + sy * sRowStride + sx * deltaS);
	    Pixel * dp(alpha() + dy * dRowStride + dx * deltaD);

	    for (unsigned y(h); y; --y, sp+= sRowStride, dp+= dRowStride) {
		Pixel const * s(sp); Pixel * d(dp);
		for (unsigned x(w); x; --x, s+= deltaS, d+= deltaD) {
		    unsigned p(*s), q(255 - p);
		    *d = (p * *s + q * *d) / 255;
		}
	    }
	}
    } else {
	Pixel const * sp(src.pixels() + sy * src.rowstride() + sx * 3);
	Pixel * dp(pixels() + dy * rowstride() + dx * 3);

	for (int y = h; y > 0; --y, sp+= src.rowstride(), dp+= rowstride())
	    memcpy(dp, sp, w * 3);
    }
}

/******************************************************************************
 * libxpm version of the pixel buffer
 ******************************************************************************/
/* !!! TODO: dithering, 8 bit visuals
 */

#ifdef CONFIG_XPM

/******************************************************************************/

YPixbuf::YPixbuf(char const * filename, bool fullAlpha):
    fWidth(0), fHeight(0), fRowStride(0), 
    fPixels(NULL), fAlpha(NULL), fPixmap(None) {
    XpmAttributes xpmAttributes;
    xpmAttributes.colormap  = app->colormap();
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    XImage * image(NULL), * alpha(NULL);
    int const rc(XpmReadFileToImage(app->display(),
				    (char *)REDIR_ROOT(filename), // !!!
				    &image, &alpha, &xpmAttributes));

    if (rc == XpmSuccess) {
	fWidth = xpmAttributes.width;
	fHeight = xpmAttributes.height;
	fRowStride = (fWidth * (fullAlpha && alpha ? 4 : 3) + 3) & ~3;
	
	if (fullAlpha && alpha) {
	    fPixels = copyImageToPixbuf<4>(*image, fRowStride);
	    fAlpha = fPixels + 3;

	    copyBitmapToPixbuf<4>(alpha->data, alpha->bytes_per_line,
			          fAlpha, fRowStride, fWidth, fHeight);
	} else
	    fPixels = copyImageToPixbuf<3>(*image, fRowStride);

    } else
        warn(_("Loading of pixmap \"%s\" failed: %s"),
	       filename, XpmGetErrorString(rc));

    if (image) XDestroyImage(image);
    if (alpha) XDestroyImage(alpha);
}

YPixbuf::YPixbuf(unsigned const width, unsigned const height):
    fWidth(width), fHeight(height), fRowStride((width * 3 + 3) & ~3),
    fPixels(NULL), fAlpha(NULL), fPixmap(None) {
    fPixels = new Pixel[fRowStride * fHeight];
}

YPixbuf::YPixbuf(YPixbuf const & source,
		 unsigned const width, unsigned const height):
    fWidth(width), fHeight(height), 
    fRowStride((width * (source.alpha() ? 4 : 3) + 3) & ~3),
    fPixels(NULL), fAlpha(NULL), fPixmap(None) {

    if (source) {
	fPixels = new Pixel[fRowStride * fHeight];
	    
	if (source.alpha()) {
	    fAlpha = fPixels + 3;
	    YScaler<Pixel, 4>(source.pixels(), source.rowstride(),
			      source.width(), source.height(),
			      fPixels, fRowStride, fWidth, fHeight);
	} else
	    YScaler<Pixel, 3>(source.pixels(), source.rowstride(),
			      source.width(), source.height(),
			      fPixels, fRowStride, fWidth, fHeight);
    }
}

YPixbuf::YPixbuf(Drawable drawable, Pixmap mask,
		 unsigned w, unsigned h, int x, int y,
		 bool fullAlpha) :
    fWidth(0), fHeight(0), fRowStride(0),
    fPixels(NULL), fAlpha(NULL), fPixmap(None) {

    Window dRoot; unsigned dWidth, dHeight, dDummy;
    XGetGeometry(app->display(), drawable, &dRoot,
    		 (int*)&dDummy, (int*)&dDummy,
		 &dWidth, &dHeight, &dDummy, &dDummy);

    MSG(("YPixbuf::YPixbuf: initial: x=%i, y=%i; w=%i, h=%i", x, y, w, h));

    x = clamp(x, 0, (int)dWidth);
    y = clamp(y, 0, (int)dHeight);
    w = min(w, dWidth - x);
    h = min(h, dHeight - y);

    MSG(("YPixbuf::YPixbuf: after clipping: x=%i, y=%i; w=%i, h=%i", x, y, w, h));

    XImage * image(XGetImage(app->display(), drawable, x, y, w, h,
    			     AllPlanes, ZPixmap));
    XImage * alpha(fullAlpha && mask != None ? 
	XGetImage(app->display(), mask, x, y, w, h, AllPlanes, ZPixmap) : NULL);

    if (image) {
	MSG(("depth/padding: %d/%d; r/g/b mask: %d/%d/%d",
	     image->depth, image->bitmap_pad,
	     image->red_mask, image->green_mask, image->blue_mask));

	fWidth = w;
	fHeight = h;

	if (fullAlpha && alpha) {
	    fRowStride = (fWidth * 4 + 3) & ~3;
	    fPixels = copyImageToPixbuf<4>(*image, fRowStride);
	    fAlpha = fPixels + 3;

	    copyBitmapToPixbuf<4>(alpha->data, alpha->bytes_per_line,
			          fAlpha, fRowStride, w, h);

	    XDestroyImage(image);
	    XDestroyImage(alpha);
	} else {
	    fRowStride = (fWidth * 3 + 3) & ~3;
	    fPixels = copyImageToPixbuf<3>(*image, fRowStride);
	    XDestroyImage(image);
	}
	
	if (fullAlpha && mask != None && alpha == NULL)
	    warn(_("%s:%d: Failed to copy drawable 0x%x to pixel buffer"),
		   __FILE__, __LINE__, mask);
    } else {
	warn(_("%s:%d: Failed to copy drawable 0x%x to pixel buffer"),
	       __FILE__, __LINE__, drawable);
    }
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
    if (fPixmap == None && fPixels) {
	unsigned const depth(app->depth());
	unsigned const pixelSize(depth > 16 ? 4 : depth > 8 ? 2 : 1);
	unsigned const rowStride(((fWidth * pixelSize) + 3) & ~3);
	char * pixels(new char[rowStride * fHeight]);

	fPixmap = YPixmap::createPixmap(fWidth, fHeight);

	XImage * image(XCreateImage(app->display(), app->visual(),
	    depth, ZPixmap, 0, pixels, fWidth, fHeight, 32, rowStride));

	if (image) {
	    copyPixbufToImage<3>(fPixels, *image, fRowStride);
	    Graphics(fPixmap).copyImage(image, 0, 0);

	    delete[] image->data;
	    image->data = NULL;

	    XDestroyImage(image);
        } else {
	   warn(_("%s:%d: Failed to copy drawable 0x%x to pixel buffer"),
	   	  __FILE__, __LINE__, drawable);
//	   delete[] pixels;
	}
    }
    
    if (alpha())
	warn("YPixbuf::copyToDrawable with alpha not implemented yet");

    if (fPixmap != None)
	XCopyArea(app->display(), fPixmap, drawable, gc, sx, sy, w, h, dx, dy);
}

#endif

/******************************************************************************
 * Imlib version of the pixel buffer
 ******************************************************************************/

#ifdef CONFIG_IMLIB

YPixbuf::YPixbuf(char const * filename, bool fullAlpha):
    fImage(Imlib_load_image(hImlib, (char*) filename)), fAlpha(NULL) {

    if (NULL == fImage)
        warn(_("Loading of image \"%s\" failed"), filename);
	
    if (fullAlpha) allocAlphaChannel();
}

YPixbuf::YPixbuf(unsigned const width, unsigned const height):
    fAlpha(NULL) {
    Pixel * empty(new Pixel[width * height * 3]);
    fImage = Imlib_create_image_from_data(hImlib, empty, NULL, width, height);
    delete[] empty;
}

YPixbuf::YPixbuf(YPixbuf const & source,
		 unsigned const width, unsigned const height):
    fImage(NULL), fAlpha(NULL) {
    if (source) {
	if (source.alpha()) {
	    fAlpha = new Pixel[width * height];
	    YScaler<Pixel, 1>(source.alpha(), source.width(),
			      source.width(), source.height(),
			      fAlpha, width, width, height);
	}

	const unsigned rowstride(3 * width);
	Pixel * pixels(new Pixel[rowstride * height]);

	YScaler<Pixel, 3>(source.pixels(), source.rowstride(),
			  source.width(), source.height(),
			  pixels, rowstride, width, height);

	fImage = Imlib_create_image_from_data(hImlib, pixels, NULL,
					      width, height);

	delete[] pixels;
    }
}

YPixbuf::YPixbuf(Drawable drawable, Pixmap mask,
		 unsigned w, unsigned h, int x, int y,
		 bool fullAlpha) :
    fImage(NULL), fAlpha(NULL) {
    Window dRoot; unsigned dWidth, dHeight, dDummy;
    XGetGeometry(app->display(), drawable, &dRoot,
    		 (int*)&dDummy, (int*)&dDummy,
		 &dWidth, &dHeight, &dDummy, &dDummy);

    MSG(("YPixbuf::YPixbuf: initial: x=%i, y=%i; w=%i, h=%i", x, y, w, h));

    x = clamp(x, 0, (int)dWidth);
    y = clamp(y, 0, (int)dHeight);
    w = min(w, dWidth - x);
    h = min(h, dHeight - y);

    MSG(("YPixbuf::YPixbuf: after clipping: x=%i, y=%i; w=%i, h=%i", x, y, w, h));
    if (!(w && h)) return;    

    XImage * image(XGetImage(app->display(), drawable, x, y, w, h,
    			     AllPlanes, ZPixmap));

    if (image) {
	MSG(("depth/padding: %d/%d; r/g/b mask: %d/%d/%d",
	     image->depth, image->bitmap_pad,
	     image->red_mask, image->green_mask, image->blue_mask));

	Pixel * pixels(copyImageToPixbuf<3>(*image, 3 * w));
	fImage = Imlib_create_image_from_data(hImlib, pixels, NULL, w, h);
	delete[] pixels;
	XDestroyImage(image);
    } else
	warn(_("%s:%d: Failed to copy drawable 0x%x to pixel buffer"),
	       __FILE__, __LINE__, drawable);
    
    if (fullAlpha && mask != None) {
	image = XGetImage(app->display(), mask, x, y, w, h, AllPlanes, ZPixmap);
	if (image) {
	    fAlpha = new Pixel[w * h];
	    copyBitmapToPixbuf<1>(image->data, image->bytes_per_line,
			          fAlpha, w, w, h);
	    XDestroyImage(image);
	} else
	    warn(_("%s:%d: Failed to copy drawable 0x%x to pixel buffer"),
	           __FILE__, __LINE__, mask);
    }
}

YPixbuf::~YPixbuf() {
    Imlib_kill_image(hImlib, fImage);
    delete[] fAlpha;
}

void YPixbuf::allocAlphaChannel() {
    if (fImage) {
	ImlibColor alpha;
	Imlib_get_image_shape(hImlib, fImage, &alpha);
	
	if (alpha.r != -1 && alpha.g != -1 && alpha.b != -1) {
	    unsigned n(height() * width());

	    Pixel * a(fAlpha = new Pixel[n]);
	    Pixel * p(pixels());

	    while(n) {
		unsigned len;
		Pixel * q;

		for (len = 0, q = p; n && (alpha.r == q[0] &&
					   alpha.g == q[1] &&
					   alpha.b == q[2]); --n, ++len, q+=3);

		memset(a, 0, len); a+= len;
		memset(p, 128, q - p); p = q;

		for (len = 0; n && (alpha.r != p[0] ||
				    alpha.g != p[1] ||
				    alpha.b != p[2]); --n, ++len, p+=3);

		memset(a, 255, len); a+= len;
	    };
	}
    }
}

void YPixbuf::copyToDrawable(Drawable drawable, GC gc,
			     int const sx, int const sy,
			     unsigned const w, unsigned const h,
			     int const dx, int const dy) {
    if (fImage) {
	if (alpha())
	    warn("YPixbuf::copyToDrawable with alpha not implemented yet");

	if (fImage->pixmap == None)
	    Imlib_render(hImlib, fImage, width(), height());

	XCopyArea(app->display(), fImage->pixmap, drawable, gc,
		  sx, sy, w, h, dx, dy);
    }
}

#endif

#endif
