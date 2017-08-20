/*
 *  IceWM - Implementation of a RGB pixel buffer encapsulating
 *  libxpm and Imlib plus a scaler for RGB pixel buffers
 *
 *  Copyright (C) 2002 The Authors of IceWM
 *
 *  Released under terms of the GNU Library General Public License
 *
 *  2001/07/06: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *      - added 12bpp converters
 *
 *  2001/07/05: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *      - fixed some 24bpp oddities
 *
 *  2001/06/12: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *      - 8 bit alpha channel for libxpm version
 *
 *  2001/06/10: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *      - support for 8 bit alpha channels
 *      - from-drawable-constructor for Imlib version
 *
 *  2001/06/03: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *      - libxpm version finished (expect for dithering and 8 bit visuals)
 *
 *  2001/05/30: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *      - libxpm support
 *
 *  2001/05/18: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *      - initial revision (Imlib only)
 */

#include "config.h"
#include "ypixbuf.h"
#include <stdlib.h>
#include <string.h>

typedef unsigned char Pixel;

#if 0

#include "base.h"
#include "yxapp.h"

#include "yprefs.h"
#include "default.h"
#include "intl.h"

#include "X11/X.h"

#if SIZEOF_CHAR == 1
typedef signed char yint8;
typedef unsigned char yuint8;
#else
#error Need typedefs for 8 bit data types
#endif

#if SIZEOF_SHORT == 2
typedef signed short yint16;
typedef unsigned short yuint16;
#else
#error Need typedefs for 16 bit data types
#endif

#if SIZEOF_INT == 4
typedef signed yint32;
typedef unsigned yuint32;
#elif SIZEOF_LONG == 4
typedef signed long yint32;
typedef unsigned long yuint32;
#else
#error Need typedefs for 32 bit data types
#endif

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

bool YPixbuf::init() {
    gdk_pixbuf_xlib_init(xapp->display(), xapp->screen());
    if (true) {
        ImlibInitParams parms;
        parms.flags = PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE | PARAMS_VISUALID;
        parms.imagecachesize = 0;
        parms.pixmapcachesize = 0;
        parms.visualid = xapp->visual()->visualid;
    }

    return true;
}

#endif

/******************************************************************************/

#endif

#if 1

/******************************************************************************
 * A scaler for Grayscale/RGB/RGBA pixel buffers
 ******************************************************************************/

typedef unsigned long fixed;
static fixed const fUnit = 1L << 12;
static fixed const fPrec = 12;
enum { R, G, B, A };

template <class Pixel, int Channels> class YScaler {
public:

    YScaler(Pixel const * src, unsigned const sStep,
            unsigned const sw, unsigned const sh,
            Pixel * dst, unsigned const dStep,
            unsigned const dw, unsigned const dh);

protected:
    YScaler() {}
};

/******************************************************************************/

template <class Pixel, int Channels>
struct YColumnAccumulator : public YScaler <Pixel, Channels> {
    YColumnAccumulator(Pixel const * src, unsigned const sLen,
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

template <class Pixel, int Channels, class RowScaler>
struct YRowAccumulator : public YScaler <Pixel, Channels> {
    YRowAccumulator(Pixel const * src, unsigned const sStep,
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

/******************************************************************************/

template <class Pixel, int Channels>
struct YColumnCopier : public YScaler <Pixel, Channels> {
    YColumnCopier(Pixel const * src, unsigned const /*sLen*/,
                 Pixel * dst, unsigned const dLen) {
        memcpy(dst, src, Channels * sizeof(Pixel) * dLen);
    }
};

template <class Pixel, int Channels, class RowScaler>
struct YRowCopier : public YScaler <Pixel, Channels> {
    YRowCopier(Pixel const * src, unsigned const sStep,
               unsigned const sw, unsigned const /*sh*/,
               Pixel * dst, unsigned const dStep,
               unsigned const dw, unsigned const dh) {
        for (unsigned n(0); n < dh; ++n, src+= sStep, dst+= dStep)
            RowScaler(src, sw, dst, dw);
    }
};

/******************************************************************************/

template <class Pixel, int Channels>
struct YColumnInterpolator : public YScaler <Pixel, Channels> {
    YColumnInterpolator(Pixel const * src, unsigned const sLen,
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

template <class Pixel, int Channels, class RowScaler>
struct YRowInterpolator : public YScaler <Pixel, Channels> {
    YRowInterpolator(Pixel const * src, unsigned const sStep,
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

/******************************************************************************/

template <class Pixel, int Channels>
YScaler<Pixel, Channels>::YScaler
        (Pixel const * src, unsigned const sStep,
         unsigned const sw, unsigned const sh,
         Pixel * dst, unsigned const dStep,
         unsigned const dw, unsigned const dh) {
    if (sh < dh)
        if (sw < dw)
            YRowInterpolator <Pixel, Channels,
            YColumnInterpolator <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
        else if (sw > dw)
            YRowInterpolator <Pixel, Channels,
            YColumnAccumulator <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
        else
            YRowInterpolator <Pixel, Channels,
            YColumnCopier <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
    else if (sh > dh)
        if (sw < dw)
            YRowAccumulator <Pixel, Channels,
            YColumnInterpolator <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
        else if (sw > dw)
            YRowAccumulator <Pixel, Channels,
            YColumnAccumulator <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
        else
            YRowAccumulator <Pixel, Channels,
            YColumnCopier <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
    else
        if (sw < dw)
            YRowCopier <Pixel, Channels,
            YColumnInterpolator <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
        else if (sw > dw)
            YRowCopier <Pixel, Channels,
            YColumnAccumulator <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
        else
            YRowCopier <Pixel, Channels,
            YColumnCopier <Pixel, Channels> >
                (src, sStep, sw, sh, dst, dStep, dw, dh);
}

#endif

#if 0

/******************************************************************************
 * A scaler for RGB pixel buffers
 ******************************************************************************/

/// TODO #warning "fix the optimized versions"
#if 0
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
#endif

template <class Pixel, int Channels>
static void copyRGBAnyToPixbuf(XImage *image, char const * src, unsigned const sStep,
                               unsigned char * dst, unsigned const dStep,
                               unsigned const width, unsigned const height,
                               unsigned const rMask, unsigned const gMask,
                               unsigned const bMask)
{
    MSG((_("Using fallback mechanism to convert pixels "
           "(depth: %d; masks (red/green/blue): %0*x/%0*x/%0*x)"),
         sizeof(Pixel) * 8,
         sizeof(Pixel) * 2, rMask, sizeof(Pixel) * 2, gMask,
         sizeof(Pixel) * 2, bMask));

    unsigned const rShift = lowbit(rMask);
    unsigned const gShift = lowbit(gMask);
    unsigned const bShift = lowbit(bMask);

    unsigned const rLoss = 7 + rShift - highbit(rMask);
    unsigned const gLoss = 7 + gShift - highbit(gMask);
    unsigned const bLoss = 7 + bShift - highbit(bMask);

    unsigned char *d = dst;
    for (unsigned y = height; y > 0; --y, src += sStep, dst += dStep) {
        for (unsigned x = width; x > 0; x--, d += Channels) {
            unsigned long pixel = XGetPixel(image, width - x, height - y);

            ///Pixel const * s = (Pixel*)src;

            d[0] = ((pixel & rMask) >> rShift) << rLoss;
            d[1] = ((pixel & gMask) >> gShift) << gLoss;
            d[2] = ((pixel & bMask) >> bShift) << bLoss;
            if (Channels == 4)
                d[3] = 0;
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
        Visual const * visual(xapp->visual());
        image.red_mask = visual->red_mask;
        image.green_mask = visual->green_mask;
        image.blue_mask = visual->blue_mask;
    }

    if (image.depth > 16) {
#if 0
        if (CHANNEL_MASK(image, 0xff0000, 0x00ff00, 0x0000ff) ||
            CHANNEL_MASK(image, 0x0000ff, 0x00ff00, 0xff0000))
            copyRGB32ToPixbuf<Channels> (image.data, image.bytes_per_line,
                                         pixels, rowstride, width, height);
        else
#endif
            copyRGBAnyToPixbuf<yuint32, Channels>
                (&image, image.data, image.bytes_per_line,
                 pixels, rowstride, width, height,
                 image.red_mask, image.green_mask, image.blue_mask);
    } else if (image.depth > 8) {
#if 0
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
#endif
            copyRGBAnyToPixbuf<yuint16, Channels>
                (&image, image.data, image.bytes_per_line,
                 pixels, rowstride, width, height,
                 image.red_mask, image.green_mask, image.blue_mask);
    } else
        warn(_("%s:%d: %d bit visuals are not supported (yet)"),
             __FILE__, __LINE__, image.depth);

    return pixels;
}


ref<YPixbuf> YPixbuf::load(upath filename) {
    ref<YPixbuf> pix;
    pix.init(new YPixbuf(filename));
    return pix;
}

ref<YPixbuf> YPixbuf::scale(int width, int height) {
    ref<YPixbuf> pix;
    pix.init(this);
    return YPixbuf::scale(pix, width, height);
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
                       int w, int h, int dx, int dy)
{
    gdk_pixbuf_copy_area(src.fImage, sx, sy, w, h, fImage, dx, dy);
#if NOIMLIB
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
#endif
}

#if 0
void YPixbuf::copyAlphaToMask(Pixmap pixmap, GC gc, int sx, int sy,
                              int w, int h, int dx, int dy) {
    if (sx < 0) { dx-= sx; w+= sx; sx = 0; }
    if (sy < 0) { dy-= sy; h+= sy; sy = 0; }
    if (dx < 0) { sx-= dx; w+= dx; dx = 0; }
    if (dy < 0) { sy-= dy; h+= dy; dy = 0; }

    w = min(w, width());
    h = min(h, height());

    XSetForeground(xapp->display(), gc, 1);
    XFillRectangle(xapp->display(), pixmap, gc, dx, dy, w, h);

    if (alpha()) {
        XSetForeground(xapp->display(), gc, 0);

        unsigned const delta(inlineAlpha() ? 4 : 3);
        unsigned const rowStride(inlineAlpha() ? rowstride() : width());
        Pixel const * row(alpha() + sy * rowStride + sx * delta);

        for (unsigned y(h); y; --y, row+= rowStride, ++dy) {
            Pixel const * pixel(row);

            for (int xa(0), xe(0); xe < w; xa = xe + 1) {
                while (xe < w && *pixel++ < 128) ++xe;
                XFillRectangle(xapp->display(), pixmap, gc,
                               dx + xa, dy, xe - xa, 1);
                while (xe < w && *pixel++ >= 128) ++xe;
            }
        }
    }
}
#endif

/******************************************************************************
 * libxpm version of the pixel buffer
 ******************************************************************************/
/* !!! TODO: dithering, 8 bit visuals
 */

#ifdef CONFIG_XPM

/******************************************************************************/

YPixbuf::YPixbuf(upath filename, bool fullAlpha):
    fWidth(0), fHeight(0), fRowStride(0),
    fPixels(NULL), fAlpha(NULL), fPixmap(None)
{
    XpmAttributes xpmAttributes;
    memset(&xpmAttributes, 0, sizeof(xpmAttributes));
    xpmAttributes.colormap  = xapp->colormap();
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    XImage * image(NULL), * alpha(NULL);
    int const rc(XpmReadFileToImage(xapp->display(),
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

        XpmFreeAttributes(&xpmAttributes);
    } else
        warn(_("Loading of pixmap \"%s\" failed: %s"),
               filename, XpmGetErrorString(rc));

    if (image) XDestroyImage(image);
    if (alpha) XDestroyImage(alpha);
}

#if 0
YPixbuf::YPixbuf(char const *filename, int w, int h, bool fullAlpha):
    fWidth(w), fHeight(h), fRowStride(0),
    fPixels(NULL), fAlpha(NULL), fPixmap(None)
{
    ref<YPixbuf> source;
    source.init(new YPixbuf(filename, fullAlpha));
    if (source != null && source->valid()) {
        fRowStride = ((w * (source->alpha() ? 4 : 3) + 3) & ~3),
        fPixels = new Pixel[fRowStride * fHeight];

        if (source->alpha()) {
            fAlpha = fPixels + 3;
            YScaler<Pixel, 4>(source->pixels(), source->rowstride(),
                              source->width(), source->height(),
                              fPixels, fRowStride, fWidth, fHeight);
        } else {
            YScaler<Pixel, 3>(source->pixels(), source->rowstride(),
                              source->width(), source->height(),
                              fPixels, fRowStride, fWidth, fHeight);
        }
    }
}
#endif

YPixbuf::YPixbuf(int const width, int const height):
    fWidth(width), fHeight(height), fRowStride((width * 3 + 3) & ~3),
    fPixels(NULL), fAlpha(NULL), fPixmap(None) {
    fPixels = new Pixel[fRowStride * fHeight];
}

YPixbuf::YPixbuf(const ref<YPixbuf> &source,
                 int const width, int const height):
    fWidth(width), fHeight(height),
    fRowStride((width * (source->alpha() ? 4 : 3) + 3) & ~3),
    fPixels(NULL), fAlpha(NULL), fPixmap(None)
{
    if (source != null && source->valid()) {
        fPixels = new Pixel[fRowStride * fHeight];

        if (source->alpha()) {
            fAlpha = fPixels + 3;
            YScaler<Pixel, 4>(source->pixels(), source->rowstride(),
                              source->width(), source->height(),
                              fPixels, fRowStride, fWidth, fHeight);
        } else
            YScaler<Pixel, 3>(source->pixels(), source->rowstride(),
                              source->width(), source->height(),
                              fPixels, fRowStride, fWidth, fHeight);
    }
}

YPixbuf::YPixbuf(Drawable drawable, Pixmap mask,
                 int dWidth, int dHeight, int w, int h, int x, int y,
                 bool fullAlpha):
    fWidth(0), fHeight(0), fRowStride(0),
    fPixels(NULL), fAlpha(NULL), fPixmap(None)
{

//#warning "!!! remove call to XGetGeometry"
//    Window dRoot; int dWidth, dHeight, dDummy;
//    XGetGeometry(xapp->display(), drawable, &dRoot,
//                 (int*)&dDummy, (int*)&dDummy,
//                 (unsigned int*)&dWidth, (unsigned int*)&dHeight,
//                 (unsigned int*)&dDummy, (unsigned int*)&dDummy);

    MSG(("YPixbuf::YPixbuf: initial: x=%i, y=%i; w=%i, h=%i", x, y, w, h));

    x = clamp(x, 0, (int)dWidth);
    y = clamp(y, 0, (int)dHeight);
    w = min(w, dWidth - x);
    h = min(h, dHeight - y);

    MSG(("YPixbuf::YPixbuf: after clipping: x=%i, y=%i; w=%i, h=%i", x, y, w, h));
    if (!(w && h)) return;

    XImage * image(XGetImage(xapp->display(), drawable, x, y, w, h,
                             AllPlanes, ZPixmap));
    XImage * alpha(fullAlpha && mask != None ?
        XGetImage(xapp->display(), mask, x, y, w, h, AllPlanes, ZPixmap) : NULL);

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
            warn("%s:%d: Failed to copy drawable 0x%x to pixel buffer",
                   __FILE__, __LINE__, mask);
    } else {
        warn("%s:%d: Failed to copy drawable 0x%x to pixel buffer",
               __FILE__, __LINE__, drawable);
    }
}

YPixbuf::~YPixbuf() {
    if (fPixmap != None) {
        if (xapp != 0)
            XFreePixmap(xapp->display(), fPixmap);
        fPixmap = 0;
    }

    delete[] fPixels;
}

void YPixbuf::copyToDrawable(Drawable drawable, GC gc,
                             int const sx, int const sy,
                             int const w, int const h,
                             int const dx, int const dy, bool useAlpha) {
    if (fPixmap == None && fPixels) {
        unsigned const depth(xapp->depth());
        unsigned const pixelSize(depth > 16 ? 4 : depth > 8 ? 2 : 1);
        unsigned const rowStride(((fWidth * pixelSize) + 3) & ~3);
        char * pixels(new char[rowStride * fHeight]);

        fPixmap = YPixmap::createPixmap(fWidth, fHeight);

        XImage * image(XCreateImage(xapp->display(), xapp->visual(),
            depth, ZPixmap, 0, pixels, fWidth, fHeight, 32, rowStride));

        if (image) {
            copyPixbufToImage<3>(fPixels, *image, fRowStride);
            Graphics(fPixmap, fWidth, fHeight).copyImage(image, 0, 0);

            delete[] image->data;
            image->data = NULL;

            XDestroyImage(image);
        } else {
           warn("%s:%d: Failed to copy drawable 0x%x to pixel buffer",
                  __FILE__, __LINE__, drawable);
//         delete[] pixels;
        }
    }

    if (useAlpha && alpha())
        warn("YPixbuf::copyToDrawable with alpha not implemented yet");

    if (fPixmap != None)
        XCopyArea(xapp->display(), fPixmap, drawable, gc, sx, sy, w, h, dx, dy);
}

#endif

/******************************************************************************
 * Imlib version of the pixel buffer
 ******************************************************************************/

#ifdef CONFIG_IMLIB

YPixbuf::YPixbuf(upath filename, bool fullAlpha):
    fImage(NULL), fAlpha(NULL)
{
    cstring cs(filename.path());

    fImage = Imlib_load_image(hImlib, (char *)(cs.c_str()));

    if (NULL == fImage)
        warn(_("Loading of image \"%s\" failed"), cs.c_str());

    if (fullAlpha)
        allocAlphaChannel();
    MSG(("%s %d %d", cs.c_str(), width(), height()));
}

#if 0
YPixbuf::YPixbuf(upath filename, int w, int h, bool fullAlpha):
    fImage(NULL), fAlpha(NULL)
{
    ref<YPixbuf> source;
    source.init(new YPixbuf(filename, fullAlpha));

    if (source != null && source->valid()) {
        if (source->alpha()) {
            fAlpha = new Pixel[w * h];
            YScaler<Pixel, 1>(source->alpha(), source->width(),
                              source->width(), source->height(),
                              fAlpha, w, w, h);
        }

        const unsigned rowstride(3 * w);
        Pixel * pixels(new Pixel[rowstride * h]);

        YScaler<Pixel, 3>(source->pixels(), source->rowstride(),
                          source->width(), source->height(),
                          pixels, rowstride, w, h);

        fImage = Imlib_create_image_from_data(hImlib, pixels, NULL,
                                              w, h);

        delete[] pixels;
    }
}
#endif

YPixbuf::YPixbuf(int const width, int const height) {
    fImage = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 32, width, height);
}

YPixbuf::YPixbuf(const ref<YPixbuf> &source,
                 int const width, int const height):
    fImage(0)
{
    fImage = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 32, width, height);
    gdk_pixbuf_scale(source->fImage, fImage, 0, 0, width, height, 0, 0, source->width(), source->height(), GDK_INTERP_BILINEAR);
#if 0
    if (source != null && source->valid()) {
        if (source->alpha()) {
            fAlpha = new Pixel[width * height];
            YScaler<Pixel, 1>(source->alpha(), source->width(),
                              source->width(), source->height(),
                              fAlpha, width, width, height);
        }

        const unsigned rowstride(3 * width);
        Pixel * pixels(new Pixel[rowstride * height]);

        YScaler<Pixel, 3>(source->pixels(), source->rowstride(),
                          source->width(), source->height(),
                          pixels, rowstride, width, height);

        fImage = Imlib_create_image_from_data(hImlib, pixels, NULL,
                                              width, height);

        delete[] pixels;
    }
#endif
}

#warning "first parameter -> Pixmap" // drawable is broken, really
YPixbuf::YPixbuf(Drawable drawable, Pixmap mask,
                 int dWidth, int dHeight, int w, int h, int x, int y):
    fImage(NULL)
{
    fImage = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 32, dWidth, dHeight);
    fImage = gdk_pixbuf_xlib_get_from_drawable(fImage, drawable, xapp->colormap(), xapp->visual(), x, y, w, h, dWidth, dHeight);
    if (mask != None) {
#warning "FIX MASK HANDLING"
        warn("mask not handled");
    }
#if 0
//#warning "!!! remove call to XGetGeometry"
//    Window dRoot; int dWidth, dHeight, dDummy;
//    XGetGeometry(xapp->display(), drawable, &dRoot,
//                 &dDummy, &dDummy,
//                 (unsigned int*)&dWidth, (unsigned int*)&dHeight, (unsigned int*)&dDummy, (unsigned int*)&dDummy);

    MSG(("YPixbuf::YPixbuf: initial: x=%i, y=%i; w=%i, h=%i", x, y, w, h));

    x = clamp(x, 0, (int)dWidth);
    y = clamp(y, 0, (int)dHeight);
    w = min(w, dWidth - x);
    h = min(h, dHeight - y);

    MSG(("YPixbuf::YPixbuf: after clipping: x=%i, y=%i; w=%i, h=%i", x, y, w, h));
    if (!(w && h)) return;

    XImage * image(XGetImage(xapp->display(), drawable, x, y, w, h,
                             AllPlanes, ZPixmap));

    if (image) {
        MSG(("depth/padding: %d/%d; r/g/b mask: %d/%d/%d",
             image->depth, image->bitmap_pad,
             image->red_mask, image->green_mask, image->blue_mask));

        Pixel * pixels(copyImageToPixbuf<3>(*image, 3 * w));
        fImage = Imlib_create_image_from_data(hImlib, pixels, NULL, w, h);
        delete[] pixels;
        XDestroyImage(image);
    } else {
        warn("%s:%d: Failed to copy drawable 0x%x to pixel buffer (%d:%d-%dx%d",
             __FILE__, __LINE__, drawable, x, y, w, h);
    }

    if (fullAlpha && mask != None) {
        image = XGetImage(xapp->display(), mask, x, y, w, h, AllPlanes, ZPixmap);
        if (image) {
            fAlpha = new Pixel[w * h];
            copyBitmapToPixbuf<1>(image->data, image->bytes_per_line,
                                  fAlpha, w, w, h);
            XDestroyImage(image);
        } else {
            warn("%s:%d: Failed to copy drawable 0x%x to pixel buffer",
                 __FILE__, __LINE__, mask);
        }
    }
#endif
}

YPixbuf::~YPixbuf() {
    gdk_pixbuf_unref(fImage);
    //Imlib_kill_image(hImlib, fImage);
    //delete[] fAlpha;

}

#if 0
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
#endif

void YPixbuf::copyToDrawable(Drawable drawable, GC gc,
                             int const sx, int const sy,
                             int const w, int const h,
                             int const dx, int const dy, bool useAlpha) {
    if (fImage) {
        gdk_pixbuf_xlib_render_to_drawable(fImage, drawable, gc,
                                           sx, sy, dx, dy, w, h,
                                           XLIB_RGB_DITHER_NONE, 0, 0);

#if 0
        if (useAlpha && alpha())
            warn("YPixbuf::copyToDrawable with alpha not implemented yet");

        if (fImage->pixmap == None)
            Imlib_render(hImlib, fImage, width(), height());

        XCopyArea(xapp->display(), fImage->pixmap, drawable, gc,
                  sx, sy, w, h, dx, dy);
#endif
    }
}

#endif

ref<YPixbuf> YPixbuf::scale(ref<YPixbuf> source, int const w, int const h) {
    ref<YPixbuf> scaled;
    if (source->width() != w || source->height() != h) {
        scaled.init(new YPixbuf(source, w, h));
    } else
        scaled = source;
    return scaled;
}

ref<YPixbuf> YPixbuf::createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                           int width, int height,
                                           int nw, int nh)
{
    ref<YPixbuf> scaled;
    scaled.init(new YPixbuf(pix, mask, nw, nh, width, height, 0, 0, true));
    return scaled;
}

void image_init() {
    YPixbuf::init();
}

#endif

void pixbuf_scale(unsigned char *source,
                  int source_rowstride,
                  int source_width,
                  int source_height,
                  unsigned char *dest,
                  int dest_rowstride,
                  int dest_width,
                  int dest_height,
                  bool alpha)
{
    if (alpha) {
        YScaler<Pixel, 4>(source, source_rowstride, source_width, source_height,
                          dest, dest_rowstride, dest_width, dest_height);
    } else {
        YScaler<Pixel, 3>(source, source_rowstride, source_width, source_height,
                          dest, dest_rowstride, dest_width, dest_height);
    }
}
