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

#ifdef CONFIG_XPM
#include <X11/Xlib.h>
#endif

#ifdef CONFIG_IMLIB
#include <Imlib.h>
#endif

#ifdef CONFIG_GDK_PIXBUF
extern "C" {
#include <gdk-pixbuf/gdk-pixbuf-xlib.h>
}
#endif

class YPixbuf {
public:
#ifdef CONFIG_ANTIALIASING
    typedef unsigned char Pixel;

    YPixbuf(char const * filename);
    YPixbuf(unsigned const width, unsigned const height);
    YPixbuf(YPixbuf const & source,
	    unsigned const width, unsigned const);

    ~YPixbuf();

    void copyArea(YPixbuf const & src, int const sx, int const sy,
    		  unsigned const w, unsigned const h,
		  int const dx, int const dy);
    void copyToDrawable(Drawable drawable, GC gc, int const sx, int const sy,
			unsigned const w, unsigned const h,
			int const dx, int const dy);
#endif

    static bool init();

#ifdef CONFIG_ANTIALIASING
#ifdef CONFIG_XPM
    Pixel * pixels() const { return fPixels; }
    unsigned width() const { return fWidth; }
    unsigned height() const { return fHeight; }
    unsigned rowstride() const { return fRowStride; }

    operator bool() const { return fPixels; }

private:
    unsigned fWidth, fHeight, fRowStride;
    unsigned char * fPixels;
#endif

#ifdef CONFIG_IMLIB
    Pixel * pixels() const { return fImage->rgb_data; }
    unsigned width() const { return fImage->rgb_width; }
    unsigned height() const { return fImage->rgb_height; }
    unsigned rowstride() const { return fImage->rgb_width * 3; }

    operator bool() const { return fImage && fImage->rgb_data; }

private:
    ImlibImage * fImage;
#endif

#ifdef CONFIG_GDK_PIXBUF
    Pixel * pixels() const { return gdk_pixbuf_get_pixels(fPixbuf); }
    unsigned width() const { return gdk_pixbuf_get_width(fPixbuf); }
    unsigned height() const { return gdk_pixbuf_get_height(fPixbuf); }
    unsigned rowstride() const { return gdk_pixbuf_get_rowstride(fPixbuf); }

    operator bool() const { return fPixbuf; }

private:
    GdkPixbuf * fPixbuf;
#endif
#endif
};

#endif
