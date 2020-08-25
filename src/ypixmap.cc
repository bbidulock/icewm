#include "config.h"

#include "ypixmap.h"
#include "yxapp.h"

static Pixmap createPixmap(unsigned w, unsigned h, unsigned depth) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, depth);
}

static Pixmap createMask(unsigned w, unsigned h) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, 1);
}

void YPixmap::replicate(bool horiz, bool copyMask) {
    if (pixmap() == None || (fMask == None && copyMask))
        return;

    unsigned dim(horiz ? width() : height());
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
    if (fPixmap32 != null)
        fPixmap32 = null;
    if (fPixmap24 != null)
        fPixmap24 = null;

    fPixmap = nPixmap;
    fMask = nMask;

    (horiz ? fWidth : fHeight) = dim;
}

YPixmap::~YPixmap() {
    if (fPixmap != None) {
        if (xapp != nullptr)
            XFreePixmap(xapp->display(), fPixmap);
        fPixmap = 0;
    }
    if (fMask != None) {
        if (xapp != nullptr)
            XFreePixmap(xapp->display(), fMask);
        fMask = 0;
    }
    freePicture();
}

Picture YPixmap::picture() {
    if (fPicture == None) {
        XRenderPictFormat* format = xapp->formatForDepth(fDepth);
        if (format) {
            XRenderPictureAttributes attr;
            unsigned long mask = fMask ? CPClipMask : None;
            attr.clip_mask = fMask;
            attr.component_alpha = (fDepth == 32);
            mask |= CPComponentAlpha;
            fPicture = XRenderCreatePicture(xapp->display(), fPixmap,
                                            format, mask, &attr);
        }
    }
    return fPicture;
}

void YPixmap::freePicture() {
    if (fPicture) {
        XRenderFreePicture(xapp->display(), fPicture);
        fPicture = None;
    }
}

ref<YImage> YPixmap::image() {
    if (fImage == null) {
        fImage = YImage::createFromPixmap(ref<YPixmap>(this));
    }
    return fImage;
}

Pixmap YPixmap::pixmap32() {
    if (fDepth == 32) {
        return pixmap();
    }
    if (fPixmap32 == null && image() != null) {
        fPixmap32 = fImage->renderToPixmap(32);
    }
    return fPixmap32 != null ? fPixmap32->pixmap() : None;
}

Pixmap YPixmap::pixmap24() {
    if (fDepth == 24) {
        return pixmap();
    }
    if (fPixmap24 == null && image() != null) {
        fPixmap24 = fImage->renderToPixmap(24);
    }
    return fPixmap24 != null ? fPixmap24->pixmap() : None;
}

Pixmap YPixmap::pixmap(unsigned depth) {
    if (depth == fDepth)
        return pixmap();
    if (depth == 32)
        return pixmap32();
    if (depth == 24)
        return pixmap24();
    return None;
}

ref<YPixmap> YPixmap::scale(unsigned const w, unsigned const h) {
    if (w == width() && h == height())
        return ref<YPixmap>(this);

    if (image() != null) {
        ref<YImage> scaled(image()->scale(w, h));
        if (scaled != null) {
            return YPixmap::createFromImage(scaled, depth());
        }
    }
    return null;
}

ref<YPixmap> YPixmap::subimage(unsigned x, unsigned y, unsigned w, unsigned h) {
    PRECONDITION(w <= width() && x <= width() - w);
    PRECONDITION(h <= height() && y <= height() - h);

    ref<YPixmap> pixmap(YPixmap::create(w, h, depth()));
    Graphics g(pixmap, 0, 0);
    g.copyPixmap(ref<YPixmap>(this), x, y, w, h, 0, 0);
    return pixmap;
}

ref<YPixmap> YPixmap::create(unsigned w, unsigned h, unsigned depth, bool useMask) {
    ref<YPixmap> n;

    Pixmap pixmap = createPixmap(w, h, depth);
    if (pixmap) {
        Pixmap mask = useMask ? createMask(w, h) : None;
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

unsigned YPixmap::verticalOffset() const {
    unsigned offset = 0;
    if (fMask) {
        XImage* image = XGetImage(xapp->display(), fMask, 0, 0,
                                  fWidth, fHeight, 1UL, XYPixmap);
        if (image) {
            for (; offset < fHeight; ++offset) {
                unsigned k = 0;
                for (; k < fWidth; ++k) {
                    if (XGetPixel(image, k, offset)) {
                        break;
                    }
                }
                if (k < fWidth) {
                    break;
                }
            }
            XDestroyImage(image);
        }
    }
    return offset;
}

// vim: set sw=4 ts=4 et:
