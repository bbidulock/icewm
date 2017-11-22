#include "config.h"

#ifdef CONFIG_GDK_PIXBUF_XLIB

#include "yimage.h"
#include "yxapp.h"

extern "C" {
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
}

class YImageGDK: public YImage {
public:
    YImageGDK(unsigned width, unsigned height, GdkPixbuf *pixbuf): YImage(width, height) {
        fPixbuf = pixbuf;
    }
    virtual ~YImageGDK() {
        g_object_unref(G_OBJECT(fPixbuf));
    }
    virtual ref<YPixmap> renderToPixmap();
    virtual ref<YImage> scale(unsigned width, unsigned height);
    virtual void draw(Graphics &g, int dx, int dy);
    virtual void draw(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy);
    virtual void composite(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy);
    virtual bool valid() const { return fPixbuf != 0; }
private:
    GdkPixbuf *fPixbuf;
};

ref<YImage> YImage::create(unsigned width, unsigned height) {
    ref<YImage> image;
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
    if (pixbuf != NULL) {
        image.init(new YImageGDK(width, height, pixbuf));
    }
    return image;
}

ref<YImage> YImage::load(upath filename) {
    ref<YImage> image;
    GError *gerror = 0;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename.string(), &gerror);

    if (pixbuf != NULL) {
        image.init(new YImageGDK(gdk_pixbuf_get_width(pixbuf),
                                 gdk_pixbuf_get_height(pixbuf),
                                 pixbuf));
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

ref<YPixmap> YImageGDK::renderToPixmap() {
    Pixmap pixmap = None, mask = None;
    gdk_pixbuf_xlib_render_pixmap_and_mask(fPixbuf, &pixmap, &mask, 128);

    return createPixmap(pixmap, mask,
                        gdk_pixbuf_get_width(fPixbuf),
                        gdk_pixbuf_get_height(fPixbuf),
                        xapp->depth());
}

ref<YPixmap> YImage::createPixmap(Pixmap pixmap, Pixmap mask, unsigned w, unsigned h, unsigned depth) {
    ref<YPixmap> n;

    n.init(new YPixmap(pixmap, mask, w, h, depth));
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

void YImageGDK::composite(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy) {

    //MSG(("composite -- %d %d %d %d | %d %d", x, y, w, h, dx, dy));
    if (g.xorigin() > dx) {
        if ((int) w <= g.xorigin() - dx)
            return;
        w -= g.xorigin() - dx;
        x += g.xorigin() - dx;
        dx = g.xorigin();
    }
    if (g.yorigin() > dy) {
        if ((int) h <= g.xorigin() - dx)
            return;
        h -= g.yorigin() - dy;
        y += g.yorigin() - dy;
        dy = g.yorigin();
    }
    if ((int) (dx + w) > (int) (g.xorigin() + g.rwidth())) {
        if ((int) (g.xorigin() + g.rwidth()) <= dx)
            return;
        w = g.xorigin() + g.rwidth() - dx;
    }
    if ((int) (dy + h) > (int) (g.yorigin() + g.rheight())) {
        if ((int) (g.yorigin() + g.rheight()) <= dy)
            return;
        h = g.yorigin() + g.rheight() - dy;
    }
    if (w <= 0 || h <= 0)
        return;

    //MSG(("composite ++ %d %d %d %d | %d %d", x, y, w, h, dx, dy));
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    gdk_pixbuf_xlib_get_from_drawable(pixbuf,
                                      g.drawable(),
                                      xapp->colormap(),
                                      xapp->visual(),
                                      dx - g.xorigin(), dy - g.yorigin(), 0, 0, w, h);
    gdk_pixbuf_composite(fPixbuf, pixbuf,
                         0, 0, w, h,
                         -x, -y, 1.0, 1.0,
                         GDK_INTERP_BILINEAR, 255);
    gdk_pixbuf_xlib_render_to_drawable(pixbuf, g.drawable(), g.handleX(),
                                             0, 0, dx - g.xorigin(), dy - g.yorigin(), w, h,
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
