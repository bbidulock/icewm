#include "config.h"

#ifdef CONFIG_GDK_PIXBUF_XLIB

#include "yimage.h"
#include "yxapp.h"

extern "C" {
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
}

class YImageGDK: public YImage {
public:
    YImageGDK(int width, int height, GdkPixbuf *pixbuf): YImage(width, height) {
        fPixbuf = pixbuf;
    }
    virtual ~YImageGDK() {
        gdk_pixbuf_unref(fPixbuf);
    }
    virtual ref<YPixmap> renderToPixmap();
    virtual ref<YImage> scale(int width, int height);
    virtual void draw(Graphics &g, int x, int y);
    virtual bool valid() const { return fPixbuf != 0; }
private:
    GdkPixbuf *fPixbuf;
};

ref<YImage> YImage::create(int width, int height) {
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
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(cstring(filename.path()).c_str(), &gerror);

    if (pixbuf != NULL) {
        image.init(new YImageGDK(gdk_pixbuf_get_width(pixbuf),
                                 gdk_pixbuf_get_height(pixbuf),
                                 pixbuf));
    }
    return image;
}

ref<YImage> YImageGDK::scale(int w, int h) {
    ref<YImage> image;

//    GdkPixbuf *pixbuf =
//        gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
//    if (pixbuf != NULL) {
        GdkPixbuf *pixbuf =
            gdk_pixbuf_scale_simple(fPixbuf,
                                    //pixbuf,
                                    //0, 0,
                                    w, h,
//                         0.0, 0.0, 1.0, 1.0,
                         GDK_INTERP_BILINEAR);

        if (pixbuf)
            image.init(new YImageGDK(w, h, pixbuf));
//    }
    return image;
}

ref<YImage> YImage::createFromPixmap(ref<YPixmap> pixmap) {
    return createFromPixmapAndMask(pixmap->pixmap(),
                                   pixmap->mask(),
                                   pixmap->width(),
                                   pixmap->height());
}

ref<YImage> YImage::createFromPixmapAndMask(Pixmap pixmap, Pixmap mask,
                                            int width, int height)
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
                for (int r = 0; r < height; r++) {
                    for (int c = 0; c < width; c++) {
                        unsigned int pix = XGetPixel(image, c, r);
                        pixels[c * 4 + 3] = pix ? 255 : 0;
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
                                           int width, int height)
{
    ref<YImage> image;
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                       width, height);

    if (!pixbuf)
        return null;

    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            unsigned long pix =
                prop_pixels[c + r * width];
#warning "check if byteorder switching is needed"
            pixels[c * 4 + 2] = pix & 0xFF;
            pixels[c * 4 + 1] = (pix >> 8) & 0xFF;
            pixels[c * 4] = (pix >> 16) & 0xFF;
            pixels[c * 4 + 3] = (pix >> 24) & 0xFF;
        }
        pixels += gdk_pixbuf_get_rowstride(pixbuf);
    }
    image.init(new YImageGDK(width,
                             height,
                             pixbuf));
    return image;
}

ref<YImage> YImage::createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                  int width, int height,
                                                  int nw, int nh)
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
                        gdk_pixbuf_get_height(fPixbuf));
}

ref<YPixmap> YImage::createPixmap(Pixmap pixmap, Pixmap mask, int w, int h) {
    ref<YPixmap> n;

    n.init(new YPixmap(pixmap, mask, w, h));
    return n;
}

void YImageGDK::draw(Graphics &g, int x, int y) {
    gdk_pixbuf_xlib_render_to_drawable_alpha(fPixbuf, g.drawable(), //g.handleX(),
                                             0, 0, x, y, width(), height(),
                                             GDK_PIXBUF_ALPHA_FULL, 128,
                                             XLIB_RGB_DITHER_NORMAL, 0, 0);
}

void image_init() {
    g_type_init();
    xlib_rgb_init(xapp->display(), ScreenOfDisplay(xapp->display(), xapp->screen()));
    gdk_pixbuf_xlib_init(xapp->display(), xapp->screen());
}

#endif
