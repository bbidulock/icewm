/*
 *  IceWM - Definition of a RGB pixel buffer encapsulating libxpm, Imlib
 *  or gdk-pixbuf and a scaler for RGB pixel buffers
 *
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Released under terms of the GNU Library General Public License
 */

#ifndef __YPIXBUF_H
#define __YPIXBUF_H

#include "ref.h"
#include "upath.h"

#if 0
#ifdef CONFIG_XPM
#include <X11/xpm.h>
#endif
#endif

#if 0
#ifdef CONFIG_IMLIB
extern "C" {
#include <gdk-pixbuf/gdk-pixbuf-xlib.h>
}
#endif
#endif

#if 0

class YPixbuf: public refcounted {
public:
    static ref<YPixbuf> create(int w, int h, bool mask = false);
    static ref<YPixbuf> load(upath filename);
    ref<YPixbuf> scale(int width, int height);
    static ref<YPixbuf> createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                      int width, int height,
                                                      int nw, int nh);
#ifdef CONFIG_ANTIALIASING
    typedef unsigned char Pixel;

    YPixbuf(upath filename, bool fullAlpha = true);
    YPixbuf(int const width, int const height);
    YPixbuf(Drawable drawable, Pixmap mask,
            int dWidth, int dHeight, int width, int height, int x = 0, int y = 0);

    ~YPixbuf();

    void copyArea(YPixbuf const & src, int sx, int sy,
                  int w, int h, int dx, int dy);
    void copyToDrawable(Drawable drawable, GC gc, int const sx, int const sy,
                        int const w, int const h,
                        int const dx, int const dy, bool useAlpha = true);
    void copyAlphaToMask(Pixmap pixmap, GC gc, int sx, int sy,
                         int w, int h,
                         int dx, int dy);
#endif
#if defined(CONFIG_ANTIALIASING) || defined(CONFIG_IMLIB)
    static ref<YPixbuf> scale(ref<YPixbuf> source, int const width, int const height);
private:
    YPixbuf::YPixbuf(const ref<YPixbuf> &source,
                     int const width, int const height);
public:
#endif

    static bool init();

#ifdef CONFIG_ANTIALIASING
#ifdef CONFIG_XPM
    Pixel * pixels() const { return fPixels; }
    Pixel * alpha() const { return fAlpha; }
    int width() const { return fWidth; }
    int height() const { return fHeight; }
    int rowstride() const { return fRowStride; }

    bool valid() const { return fPixels; }
    operator bool() const { return valid(); }
    bool inlineAlpha() const { return true; };

    Pixmap renderPixmap();

private:
    int fWidth, fHeight, fRowStride;
    Pixel * fPixels, * fAlpha;

    Pixmap fPixmap;
#endif

#ifdef CONFIG_IMLIB
#if 0
    Pixel * pixels() const { return fImage ? fImage->rgb_data : NULL; }
    Pixel * alpha() const { return fAlpha; }
    int rowstride() const { return fImage ? fImage->rgb_width * 3 : 0; }
#endif
    int width() const { return fImage ? gdk_pixbuf_get_width(fImage) : 0; }
    int height() const { return fImage ? gdk_pixbuf_get_height(fImage) : 0; }

    bool valid() const { return fImage != NULL; }
    operator bool() const { return valid(); }
    bool inlineAlpha() const { return false; };

private:
    void allocAlphaChannel();

    friend class YPixmap;
    GdkPixbuf * fImage;
#endif

#ifdef CONFIG_GDK_PIXBUF
    Pixel * pixels() const { return gdk_pixbuf_get_pixels(fPixbuf); }
    int width() const { return gdk_pixbuf_get_width(fPixbuf); }
    int height() const { return gdk_pixbuf_get_height(fPixbuf); }
    int rowstride() const { return gdk_pixbuf_get_rowstride(fPixbuf); }

    operator bool() const { return fPixbuf; }

private:
    GdkPixbuf * fPixbuf;
#endif
#endif
};

#endif

#endif
