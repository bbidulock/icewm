#include "config.h"

#ifdef CONFIG_GDK_PIXBUF_XLIB

#include "yimage.h"
#include "yxapp.h"
#include <stdlib.h>

extern "C" {
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
}

class YImageGDK: public YImage {
public:
    YImageGDK(unsigned width, unsigned height, GdkPixbuf *pixbuf):
        YImage(width, height)
    {
        fPixbuf = pixbuf;
    }
    virtual ~YImageGDK() {
        g_object_unref(G_OBJECT(fPixbuf));
    }
    virtual ref<YPixmap> renderToPixmap(unsigned depth);
    virtual ref<YImage> scale(unsigned width, unsigned height);
    virtual void draw(Graphics &g, int dx, int dy);
    virtual void draw(Graphics &g, int x, int y,
                       unsigned w, unsigned h, int dx, int dy);
    virtual void composite(Graphics &g, int x, int y,
                            unsigned w, unsigned h, int dx, int dy);
    virtual bool valid() const { return fPixbuf != 0; }
private:
    GdkPixbuf *fPixbuf;
};

ref<YImage> YImage::load(upath filename) {
    ref<YImage> image;
    GError *gerror = 0;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename.string(), &gerror);

    if (pixbuf != NULL) {
        image.init(new YImageGDK(gdk_pixbuf_get_width(pixbuf),
                                 gdk_pixbuf_get_height(pixbuf),
                                 pixbuf));
        return image;
    }

    // support themes with indirect XPM images, like OnyX:
    const int lim = 64;
    for (int k = 9; --k > 0 && inrange(int(filename.fileSize()), 5, lim); ) {
        fileptr fp(filename.fopen("r"));
        if (fp == 0)
            break;

        char buf[lim];
        if (fgets(buf, lim, fp) == 0)
            break;

        mstring match(mstring(buf).match("^[a-z][-_a-z0-9]*\\.xpm$", "i"));
        if (match == null)
            break;

        filename = filename.parent().relative(match);
        if (filename.fileSize() > lim)
            return load(filename);
    }
    return image;
}

ref<YImage> YImageGDK::scale(unsigned w, unsigned h) {
    ref<YImage> image;
    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_scale_simple(fPixbuf,
                                     w, h,
                                     GDK_INTERP_BILINEAR);
    if (pixbuf != NULL) {
        image.init(new YImageGDK(w, h, pixbuf));
    }

    return image;
}

ref<YImage> YImage::createFromPixmap(ref<YPixmap> pixmap) {
    return createFromPixmapAndMask(pixmap->pixmap(),
                                   pixmap->mask(),
                                   pixmap->width(),
                                   pixmap->height());
}

ref<YImage> YImage::createFromPixmapAndMask(Pixmap pixmap, Pixmap mask,
                                            unsigned width, unsigned height)
{
    ref<YImage> image;
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                       width, height);


    if (pixbuf) {
        pixbuf =
            gdk_pixbuf_xlib_get_from_drawable(pixbuf,
                                              pixmap,
                                              xlib_rgb_get_cmap(),
                                              xlib_rgb_get_visual(),
                                              0, 0,
                                              0, 0,
                                              width,
                                              height);

        if (mask != None) {
            XImage *image = XGetImage(xapp->display(), mask,
                                      0, 0, width, height,
                                      AllPlanes, ZPixmap);
            guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

            if (image) {
                //unsigned char *pix = image->data;
                for (unsigned r = 0; r < height; r++) {
                    for (unsigned c = 0; c < width; c++) {
                        unsigned int pix = XGetPixel(image, c, r);
                        pixels[c * 4 + 3] = (unsigned char)(pix ? 255 : 0);
                    }
                    pixels += gdk_pixbuf_get_rowstride(pixbuf);
                    //pix += image->bytes_per_line;
                }
                XDestroyImage(image);
            }
        }

        image.init(new YImageGDK(width,
                                 height,
                                 pixbuf));
    }
    return image;
}

ref<YImage> YImage::createFromIconProperty(long *prop_pixels,
                                           unsigned width, unsigned height)
{
    ref<YImage> image;
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                       width, height);

    if (!pixbuf)
        return null;

    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    for (unsigned r = 0; r < height; r++) {
        for (unsigned c = 0; c < width; c++) {
            unsigned long pix =
                prop_pixels[c + r * width];
            pixels[c * 4 + 2] = (unsigned char)(pix & 0xFF);
            pixels[c * 4 + 1] = (unsigned char)((pix >> 8) & 0xFF);
            pixels[c * 4] = (unsigned char)((pix >> 16) & 0xFF);
            pixels[c * 4 + 3] = (unsigned char)((pix >> 24) & 0xFF);
        }
        pixels += gdk_pixbuf_get_rowstride(pixbuf);
    }
    image.init(new YImageGDK(width,
                             height,
                             pixbuf));
    return image;
}

ref<YImage> YImage::createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                  unsigned width, unsigned height,
                                                  unsigned nw, unsigned nh)
{
    ref<YImage> image = createFromPixmapAndMask(pix, mask, width, height);
    if (image != null)
        image = image->scale(nw, nh);
    return image;
}

ref<YPixmap> YImageGDK::renderToPixmap(unsigned depth) {
    Pixmap pixmap = None, mask = None;

    if (depth == (unsigned) xlib_rgb_get_depth()) {
        gdk_pixbuf_xlib_render_pixmap_and_mask(fPixbuf, &pixmap, &mask, 128);
    }
    else if (depth == 32) {
        int width = int(this->width());
        int height = int(this->height());
        XImage* image = XCreateImage(xapp->display(), xapp->visual(), depth,
                                     ZPixmap, 0, NULL, width, height, 8, 0);
        if (image)
            image->data = (char *) calloc(image->bytes_per_line * height, 1);
        XImage* imask = XCreateImage(xapp->display(), xapp->visual(), 1,
                                     XYPixmap, 0, NULL, width, height, 8, 0);
        if (imask)
            imask->data = (char *) calloc(imask->bytes_per_line * height, 1);

        if (image && image->data && imask && imask->data) {
            if (4 != gdk_pixbuf_get_n_channels(fPixbuf) ||
                1 != gdk_pixbuf_get_has_alpha(fPixbuf)) {
                static int atmost(3);
                if (atmost-- > 0)
                    tlog("gdk pixbuf channels %d != 4 || has_alpha %d != 1",
                         gdk_pixbuf_get_n_channels(fPixbuf),
                         gdk_pixbuf_get_has_alpha(fPixbuf));
            }
            guchar *pixels = gdk_pixbuf_get_pixels(fPixbuf);

            for (int r = 0; r < height; r++) {
                for (int c = 0; c < width; c++) {
                    XPutPixel(image, c, r,
                              (pixels[c * 4] << 16) +
                              (pixels[c * 4 + 1] << 8) +
                              (pixels[c * 4 + 2] << 0) +
                              (pixels[c * 4 + 3] << 24));
                    XPutPixel(imask, c, r, pixels[c * 4 + 3] >= 128);
                }
                pixels += gdk_pixbuf_get_rowstride(fPixbuf);
            }

            pixmap = XCreatePixmap(xapp->display(), xapp->root(),
                                   width, height, depth);
            GC gc = XCreateGC(xapp->display(), pixmap, None, None);
            XPutImage(xapp->display(), pixmap, gc, image,
                      0, 0, 0, 0, width, height);
            XFreeGC(xapp->display(), gc);

            mask = XCreatePixmap(xapp->display(), xapp->root(),
                                 width, height, 1);
            gc = XCreateGC(xapp->display(), mask, None, None);
            XPutImage(xapp->display(), mask, gc, imask,
                      0, 0, 0, 0, width, height);
            XFreeGC(xapp->display(), gc);

            XDestroyImage(image);
            XDestroyImage(imask);
        }
    }

    return createPixmap(pixmap, mask, width(), height(), depth);
}

ref<YPixmap> YImage::createPixmap(Pixmap pixmap, Pixmap mask,
                                  unsigned w, unsigned h, unsigned depth) {
    ref<YPixmap> n;

    n.init(new YPixmap(pixmap, mask, w, h, depth, ref<YImage>(this)));
    return n;
}

void YImageGDK::draw(Graphics &g, int dx, int dy) {
#if 1
    composite(g, 0, 0, width(), height(), dx, dy);
#else
    gdk_pixbuf_xlib_render_to_drawable_alpha(fPixbuf, g.drawable(), //g.handleX(),
                                             0, 0, dx, dy, width(), height(),
                                             GDK_PIXBUF_ALPHA_FULL,
                                             128,
                                             XLIB_RGB_DITHER_NORMAL, 0, 0);
#endif
}

void YImageGDK::draw(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy) {
#if 1
    composite(g, x, y, w, h, dx, dy);
#else
    gdk_pixbuf_xlib_render_to_drawable_alpha(fPixbuf, g.drawable(), //g.handleX(),
                                             x, y, dx, dy, w, h,
                                             GDK_PIXBUF_ALPHA_BILEVEL, 128,
                                             XLIB_RGB_DITHER_NORMAL, 0, 0);
#endif
}

void YImageGDK::composite(Graphics &g, int x, int y, unsigned width, unsigned height, int dx, int dy) {
    int w = (int) width;
    int h = (int) height;

    //MSG(("composite -- %d %d %d %d | %d %d", x, y, w, h, dx, dy));
    if (g.xorigin() > dx) {
        if (w <= g.xorigin() - dx)
            return;
        w -= g.xorigin() - dx;
        x += g.xorigin() - dx;
        dx = g.xorigin();
    }
    if (g.yorigin() > dy) {
        if (h <= g.xorigin() - dx)
            return;
        h -= g.yorigin() - dy;
        y += g.yorigin() - dy;
        dy = g.yorigin();
    }
    if (dx + w > (int) (g.xorigin() + g.rwidth())) {
        if ((int) (g.xorigin() + g.rwidth()) <= dx)
            return;
        w = g.xorigin() + g.rwidth() - dx;
    }
    if (dy + h > (int) (g.yorigin() + g.rheight())) {
        if ((int) (g.yorigin() + g.rheight()) <= dy)
            return;
        h = g.yorigin() + g.rheight() - dy;
    }
    if (w <= 0 || h <= 0)
        return;

    const int src_x = int(dx - g.xorigin());
    const int src_y = int(dy - g.yorigin());

    //MSG(("composite ++ %d %d %d %d | %d %d", x, y, w, h, dx, dy));
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    gdk_pixbuf_xlib_get_from_drawable(pixbuf,
                                      g.drawable(),
                                      xapp->colormap(),
                                      xapp->visual(),
                                      src_x, src_y, 0, 0, w, h);
    gdk_pixbuf_composite(fPixbuf, pixbuf,
                         0, 0, w, h,
                         -x, -y, 1.0, 1.0,
                         GDK_INTERP_BILINEAR, 255);
    gdk_pixbuf_xlib_render_to_drawable(pixbuf, g.drawable(), g.handleX(),
                                             0, 0, src_x, src_y, w, h,
//                                             GDK_PIXBUF_ALPHA_BILEVEL, 128,
                                             XLIB_RGB_DITHER_NONE, 0, 0);
    g_object_unref(G_OBJECT(pixbuf));
}


void image_init() {
#if (GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 36 && GLIB_MICRO_VERSION <= 0)
    g_type_init();
#endif
    xlib_rgb_init(xapp->display(), ScreenOfDisplay(xapp->display(), xapp->screen()));
    gdk_pixbuf_xlib_init(xapp->display(), xapp->screen());
}

#endif

// vim: set sw=4 ts=4 et:
