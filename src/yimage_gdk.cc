#include "config.h"

#ifdef CONFIG_GDK_PIXBUF_XLIB

#include "yimage.h"
#include "yxapp.h"
#include <stdlib.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>

#define ATH 10  /* alpha threshold */

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
    virtual ref<YPixmap> renderToPixmap(unsigned depth, bool premult);
    virtual ref<YImage> scale(unsigned width, unsigned height);
    virtual void draw(Graphics &g, int dx, int dy);
    virtual void draw(Graphics &g, int x, int y,
                       unsigned w, unsigned h, int dx, int dy);
    virtual void composite(Graphics &g, int x, int y,
                            unsigned w, unsigned h, int dx, int dy);
    virtual unsigned depth() const;
    virtual bool hasAlpha() const;
    virtual bool valid() const { return fPixbuf != nullptr; }
    virtual ref<YImage> subimage(int x, int y, unsigned w, unsigned h);
    virtual void save(upath filename);

private:
    GdkPixbuf *fPixbuf;
};

bool YImage::supportsDepth(unsigned depth) {
    return depth == unsigned(xlib_rgb_get_depth());
}

bool YImageGDK::hasAlpha() const {
    return gdk_pixbuf_get_has_alpha(fPixbuf);
}

unsigned YImageGDK::depth() const {
    return 8 * gdk_pixbuf_get_n_channels(fPixbuf);
}

ref<YImage> YImage::load(upath filename) {
    ref<YImage> image;
    GError *gerror = nullptr;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename.string(), &gerror);

    if (pixbuf != nullptr) {
        image.init(new YImageGDK(gdk_pixbuf_get_width(pixbuf),
                                 gdk_pixbuf_get_height(pixbuf),
                                 pixbuf));
        return image;
    }

    // support themes with indirect XPM images, like OnyX:
    const int lim = 64;
    for (int k = 9; --k > 0 && inrange(int(filename.fileSize()), 5, lim); ) {
        fileptr fp(filename.fopen("r"));
        if (fp == nullptr)
            break;

        char buf[lim];
        if (fgets(buf, lim, fp) == nullptr)
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

void YImageGDK::save(upath filename) {
    mstring handle(filename.replaceExtension(".png"));
    GError *gerror = nullptr;
    gdk_pixbuf_save(fPixbuf, handle, "png", &gerror, (void *) nullptr);
    if (gerror) {
        msg("Cannot save YImageGDK %s: %s", handle.c_str(), gerror->message);
        g_error_free(gerror);
    }
}

ref<YImage> YImageGDK::scale(unsigned w, unsigned h) {
    if (w == width() && h == height())
        return ref<YImage>(this);

    ref<YImage> image;
    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_scale_simple(fPixbuf,
                                     w, h,
                                     GDK_INTERP_BILINEAR);
    if (pixbuf != nullptr) {
        image.init(new YImageGDK(w, h, pixbuf));
    }

    return image;
}

ref<YImage> YImageGDK::subimage(int x, int y, unsigned w, unsigned h) {
    PRECONDITION(w <= width() && unsigned(x) <= width() - w);
    PRECONDITION(h <= height() && unsigned(y) <= height() - h);

    GdkPixbuf* pixbuf = gdk_pixbuf_new(
                        gdk_pixbuf_get_colorspace(fPixbuf),
                        gdk_pixbuf_get_has_alpha(fPixbuf),
                        gdk_pixbuf_get_bits_per_sample(fPixbuf),
                        w, h);
    gdk_pixbuf_copy_area(fPixbuf, x, y, int(w), int(h), pixbuf, 0, 0);
    return ref<YImage>(new YImageGDK(w, h, pixbuf));
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
    if (image != null && (nw != width || nh != height))
        image = image->scale(nw, nh);
    return image;
}

ref<YPixmap> YImageGDK::renderToPixmap(unsigned depth, bool premult) {
    Pixmap pixmap = None, mask = None;

    if (depth == 0) {
        depth = xapp->depth();
    }
    if (depth == 32 || depth == 24) {
        int width = int(this->width());
        int height = int(this->height());
        Visual* visual = xapp->visualForDepth(depth);
        XImage* image = XCreateImage(xapp->display(), visual, depth,
                                     ZPixmap, 0, nullptr, width, height, 8, 0);
        if (image)
            image->data = (char *) calloc(image->bytes_per_line * height, 1);
        XImage* imask = XCreateImage(xapp->display(), visual, 1,
                                     XYPixmap, 0, nullptr, width, height, 8, 0);
        if (imask)
            imask->data = (char *) calloc(imask->bytes_per_line * height, 1);

        if (image && image->data && imask && imask->data) {
            const bool alpha = gdk_pixbuf_get_has_alpha(fPixbuf);
            const int nchans = gdk_pixbuf_get_n_channels(fPixbuf);
            const int stride = gdk_pixbuf_get_rowstride(fPixbuf);
            const guchar* pixels = gdk_pixbuf_get_pixels(fPixbuf);

            for (int row = 0; row < height; row++) {
                const guchar* rowpix = pixels + row * stride;
                for (int col = 0; col < width; col++, rowpix += nchans) {
                    guchar red = rowpix[0];
                    guchar grn = rowpix[1];
                    guchar blu = rowpix[2];
                    guchar alp = alpha
                               ? (rowpix[3] >= ATH ? rowpix[3] : 0x00)
                               : 0xFF;
                    if (premult) {
                        red = (red * (alp + 1)) >> 8;
                        grn = (grn * (alp + 1)) >> 8;
                        blu = (blu * (alp + 1)) >> 8;
                    }
                    XPutPixel(image, col, row,
                              (red << 16) |
                              (grn << 8) |
                              (blu << 0) |
                              (alp << 24));
                    bool bit = (alp >= ATH);
                    XPutPixel(imask, col, row, bit);
                }
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
    else if (depth == unsigned(xlib_rgb_get_depth())) {
        gdk_pixbuf_xlib_render_pixmap_and_mask(fPixbuf, &pixmap, &mask, ATH);
        if (pixmap == None)
            return null;
    }

    return createPixmap(pixmap, mask, width(), height(), depth);
}

ref<YPixmap> YImage::createPixmap(Pixmap pixmap, Pixmap mask,
                                  unsigned w, unsigned h, unsigned depth) {
    ref<YPixmap> n;

    if (pixmap)
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
                                             ATH,
                                             XLIB_RGB_DITHER_NORMAL, 0, 0);
#endif
}

void YImageGDK::draw(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy) {
#if 1
    composite(g, x, y, w, h, dx, dy);
#else
    if (x < 0 || x + w > width() || y < 0 || y + h > height())
        return;
    gdk_pixbuf_xlib_render_to_drawable_alpha(fPixbuf, g.drawable(), //g.handleX(),
                                             x, y, dx, dy, w, h,
                                             GDK_PIXBUF_ALPHA_BILEVEL,
                                             ATH,
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
    Visual* visual = xapp->visualForDepth(g.rdepth());
    Colormap cmap = xapp->colormapForVisual(visual);
    gdk_pixbuf_xlib_get_from_drawable(pixbuf,
                                      g.drawable(),
                                      cmap,
                                      visual,
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
#if !GLIB_CHECK_VERSION(2,36,0)
    g_type_init();
#endif

    int depth = xapp->alpha() ? 32 : int(xapp->depth());

    gdk_pixbuf_xlib_init_with_depth(xapp->display(), xapp->screen(), depth);
}

#endif

// vim: set sw=4 ts=4 et:
