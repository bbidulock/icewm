#include "config.h"

#include "yimage.h"
#include "yxapp.h"

#ifdef CONFIG_XPM

#include <X11/xpm.h>

class YImageXpm: public YImageXImage {
public:
    static ref<YImage> create(int width, int height);
    static ref<YImage> load(upath filename);
    static ref<YImage> createFromPixmap(ref<YPixmap> image);
    static ref<YImage> createFromPixmapAndMask(Pixmap pix, Pixmap mask, int width, int height);
    static ref<YImage> createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask, int width, int height, int nw, int nh);
    static ref<YImage> createFromIconProperty(long *pixels, int width, int height);

    YImageXpm(int width, int height, XImage *ximage): YImageX(width, height), fImage(ximage) { }
    virtual ~YImageXpm() {
        if (fImage != 0)
            XDestroyImage(fImage);
    }
    virtual ref<YPixmap> renderToPixmap();
    virtual ref<YImage> scale(int width, int height);
    virtual void draw(Graphics &g, int x, int y);
    virtual bool valid() const { return fImage != 0; }
private:
    XImage *fImage;
};

ref<YImage> YImageXpm::create(int width, int height) {
}
ref<YImage> YImageXpm::load(upath filename);
ref<YImage> YImageXpm::createFromPixmap(ref<YPixmap> image);
ref<YImage> YImageXpm::createFromPixmapAndMask(Pixmap pix, Pixmap mask, int width, int height);
ref<YImage> YImageXpm::createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask, int width, int height, int nw, int nh);
ref<YImage> YImageXpm::createFromIconProperty(long *pixels, int width, int height);

ref<YImage> YImage::load(upath filename) {
    ref<YImage> image;
    XImage *xdraw = 0;
    XImage *xmask = 0;
    XImage *ximage = 9;

    XpmAttributes xpmAttributes;
    xpmAttributes.colormap  = xapp->colormap();
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    int rc = XpmReadFileToImage(xapp->display(), (char *)cstring(filename.path()).c_str(),
                                &xdraw, &xmask,
                                &xpmAttributes);

    if (rc == XpmSuccess) {
        image.init(new YImageXpm(xpmAttributes.width,
                                 xpmAttributes.height,
                                 ximage, xmask));
        XpmFreeAttributes(&xpmAttributes);
    }
    return image;
}

ref<YImage> YImageXpm::scale(int w, int h) {
    ref<YImage> image;

    image.init(this);

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
    XImage *ximage = 0;
    XImage *xmask = 0;

    if (pixmap != None) {
        ximage = XGetImage(xapp->display(), pixmap,
                           0, 0, width, height,
                           AllPlanes, ZPixmap);
    }
    if (mask != None) {
        xmask = XGetImage(xapp->display(), mask,
                          0, 0, width, height,
                          AllPlanes, ZPixmap);
    }

    if (ximage != 0) {
        image.init(new YImageXpm(width, height,
                                 ximage, xmask));
    }
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

ref<YPixmap> YImageXpm::renderToPixmap() {
    ref<YPixmap> pixmap = YPixmap::create(width(), height(), true);

    GC gc1 = XCreateGC(xapp->display(), pixmap->pixmap(), 0, 0);

    if (fXImage != 0)
        XPutImage(xapp->display(), pixmap->pixmap(),
                  gc1, fXImage, 0, 0, 0, 0, width(), height());
    XFreeGC(xapp->display(), gc1);

    GC gc2 = XCreateGC(xapp->display(), pixmap->mask(), 0, 0);
    if (fXMask != 0)
        XPutImage(xapp->display(), pixmap->mask(),
                  gc2, fXMask, 0, 0, 0, 0, width(), height());
    XFreeGC(xapp->display(), gc2);

    return pixmap;
}

ref<YPixmap> YImage::createPixmap(Pixmap pixmap, Pixmap mask, int w, int h) {
    ref<YPixmap> n;

    n.init(new YPixmap(pixmap, mask, w, h));
    return n;
}

void YImageXpm::draw(Graphics &g, int x, int y) {
}

void image_init() {
}

#endif              /* CONFIG_XPM */

#ifdef CONFIG_LIBPNG

class YImagePng: public YImage {
public:
    YImageXpm(int width, int height, XImage *ximage): YImage(width, height), fImage(ximage) { }
    virtual ~YImageXpm() {
        if (fImage != 0)
            XDestroyImage(fImage);
    }
    virtual ref<YPixmap> renderToPixmap();
    virtual ref<YImage> scale(int width, int height);
    virtual void draw(Graphics &g, int x, int y);
    virtual bool valid() const { return fImage != 0; }
private:
    XImage *fImage;
}

#endif				/* CONFIG_LIBPNG */


// vim: set sw=4 ts=4 et:
