#include "config.h"

#ifdef CONFIG_IMLIB

#include "yimage.h"
#include "yxapp.h"

void image_init() {
}

class YImageImlib: public YImage {
public:
    YImageImlib(int width, int height, XImage *ximage, XImage *xmask): YImage(width, height) {
        fXImage = ximage;
        fXMask = xmask;
    }
    virtual ~YImageImlib() {
        if (fXImage != 0) {
            XDestroyImage(fXImage);
            fXImage = 0;
        }
        if (fXMask != 0) {
            XDestroyImage(fXMask);
            fXMask = 0;
        }
    }
    virtual ref<YPixmap> renderToPixmap();
    virtual ref<YImage> scale(int width, int height);
    virtual void draw(Graphics &g, int x, int y);
private:
    XImage *fXImage;
    XImage *fXMask;
};

ref<YImage> YImage::load(upath filename) {
    ref<YImage> image;
#if 0
    XImage *ximage = 0;
    XImage *xmask = 0;

    XpmAttributes xpmAttributes;
    xpmAttributes.colormap  = xapp->colormap();
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    int rc = XpmReadFileToImage(xapp->display(), (char *)filename,
                                &ximage, &xmask,
                                &xpmAttributes);

    if (rc == XpmSuccess) {
        image.init(new YImageImlib(xpmAttributes.width,
                                 xpmAttributes.height,
                                 ximage, xmask));
        XpmFreeAttributes(&xpmAttributes);
    }
#endif
    return image;
}

ref<YImage> YImageImlib::scale(int w, int h) {
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
#if 0
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
        image.init(new YImageImlib(width, height,
                                 ximage, xmask));
    }
#endif
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

ref<YPixmap> YImageImlib::renderToPixmap() {
    ref<YPixmap> pixmap = YPixmap::create(width(), height(), true);

#if 0
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
#endif
    return pixmap;
}

ref<YPixmap> YImage::createPixmap(Pixmap pixmap, Pixmap mask, int w, int h) {
    ref<YPixmap> n;

    n.init(new YPixmap(pixmap, mask, w, h));
    return n;
}

void YImageImlib::draw(Graphics &g, int x, int y) {
}

#endif
