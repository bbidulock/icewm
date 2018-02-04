#include "config.h"

#include "ypixmap.h"
#include "yxapp.h"

static Pixmap createPixmap(int w, int h, int depth) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, depth);
}

#if 0
static Pixmap createPixmap(int w, int h) {
    return createPixmap(w, h, xapp->depth());
}
#endif

static Pixmap createMask(int w, int h) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, 1);
}

void YPixmap::replicate(bool horiz, bool copyMask) {
    if (pixmap() == None || (fMask == None && copyMask))
        return;

    int dim(horiz ? width() : height());
    if (dim >= 128) return;
    dim = 128 + dim - 128 % dim;

    Pixmap nPixmap(horiz ? createPixmap(dim, height(), depth())
                         : createPixmap(width(), dim, depth()));
    Pixmap nMask(copyMask ? (horiz ? createMask(dim, height())
                                   : createMask(width(), dim)) : None);

    if (horiz)
        Graphics(nPixmap, dim, height(), depth()).repHorz(fPixmap, width(), height(), 0, 0, dim);
    else
        Graphics(nPixmap, width(), dim, depth()).repVert(fPixmap, width(), height(), 0, 0, dim);

    if (nMask != None) {
        if (horiz)
            Graphics(nMask, dim, height(), depth()).repHorz(fMask, width(), height(), 0, 0, dim);
        else
            Graphics(nMask, width(), dim, depth()).repVert(fMask, width(), height(), 0, 0, dim);
    }

    if (fPixmap != None)
        XFreePixmap(xapp->display(), fPixmap);
    if (fMask != None)
        XFreePixmap(xapp->display(), fMask);

    fPixmap = nPixmap;
    fMask = nMask;

    (horiz ? fWidth : fHeight) = dim;
}

YPixmap::~YPixmap() {
    if (fPixmap != None) {
        if (xapp != 0)
            XFreePixmap(xapp->display(), fPixmap);
        fPixmap = 0;
    }
    if (fMask != None) {
        if (xapp != 0)
            XFreePixmap(xapp->display(), fMask);
        fMask = 0;
    }
}

ref<YImage> YPixmap::image() {
    if (fImage == null) {
        fImage = YImage::createFromPixmap(ref<YPixmap>(this));
    }
    return fImage;
}

Pixmap YPixmap::pixmap32() {
    if (fPixmap32 == null && image() != null) {
        fPixmap32 = fImage->renderToPixmap(32);
    }
    return fPixmap32 != null ? fPixmap32->pixmap() : None;
}

ref<YPixmap> YPixmap::scale(unsigned const w, unsigned const h) {
    ref<YPixmap> pixmap;
    pixmap.init(this);
    ref<YImage> image = YImage::createFromPixmap(pixmap);
    if (image != null) {
        image = image->scale(w, h);
        if (image != null)
            pixmap = YPixmap::createFromImage(image, depth());
    }
    return pixmap;
}

ref<YPixmap> YPixmap::create(unsigned w, unsigned h, unsigned depth, bool useMask) {
    ref<YPixmap> n;

    Pixmap pixmap = createPixmap(w, h, depth);
    Pixmap mask = useMask ? createMask(w, h) : None;
    if (pixmap != None && (!useMask || mask != None)) {
        n.init(new YPixmap(pixmap, mask, w, h, depth, null));
    }
    return n;
}

ref<YPixmap> YPixmap::createFromImage(ref<YImage> image, unsigned depth) {
    return image->renderToPixmap(depth);
}

ref<YPixmap> YPixmap::createFromPixmapAndMask(Pixmap /*pixmap*/,
                                              Pixmap /*mask*/,
                                              unsigned /*w*/,
                                              unsigned /*h*/)
{
    die(2, "YPixmap::createFromPixmapAndMask");
    return null;
}

ref<YPixmap> YPixmap::createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                    unsigned width, unsigned height,
                                                    unsigned nw, unsigned nh)
{
    if (pix != None) {
        ref<YImage> image =
            YImage::createFromPixmapAndMaskScaled(pix, mask,
                                                  width, height, nw, nh);
        if (image != null) {
            ref<YPixmap> pixmap =
                YPixmap::createFromImage(image, xapp->depth());
            return pixmap;
        }
    }
    return null;
}

ref<YPixmap> YPixmap::load(upath filename) {
    ref<YImage> image = YImage::load(filename);
    ref<YPixmap> pixmap;
    if (image != null) {
        pixmap = YPixmap::createFromImage(image, xapp->depth());
    }
    return pixmap;
}

// vim: set sw=4 ts=4 et:
