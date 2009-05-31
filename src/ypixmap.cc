#include "config.h"

#include "ypixmap.h"
#include "yxapp.h"

#include <stdlib.h>

static Pixmap createPixmap(int w, int h, int depth) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, depth);
}

static Pixmap createPixmap(int w, int h) {
    return createPixmap(w, h, xapp->depth());
}

static Pixmap createMask(int w, int h) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, 1);
}

void YPixmap::replicate(bool horiz, bool copyMask) {
    if (this == NULL || pixmap() == None || (fMask == None && copyMask))
	return;

    int dim(horiz ? width() : height());
    if (dim >= 128) return;
    dim = 128 + dim - 128 % dim;

    Pixmap nPixmap(horiz ? createPixmap(dim, height())
    			 : createPixmap(width(), dim));
    Pixmap nMask(copyMask ? (horiz ? createMask(dim, height())
				   : createMask(width(), dim)) : None);

    if (horiz)
	Graphics(nPixmap, dim, height()).repHorz(fPixmap, width(), height(), 0, 0, dim);
    else
	Graphics(nPixmap, width(), dim).repVert(fPixmap, width(), height(), 0, 0, dim);

    if (nMask != None) {
	if (horiz)
	    Graphics(nMask, dim, height()).repHorz(fMask, width(), height(), 0, 0, dim);
	else
            Graphics(nMask, width(), dim).repVert(fMask, width(), height(), 0, 0, dim);
    }

    if (
#if 1
        true
#else
        fOwned
#endif
       )
    {
        if (fPixmap != None)
            XFreePixmap(xapp->display(), fPixmap);
        if (fMask != None)
            XFreePixmap(xapp->display(), fMask);
    }

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

#if 0
ref<YPixmap> YPixmap::scale(ref<YPixmap> source, int const w, int const h) {
    ref<YPixmap> scaled;
    scaled = source;
    return scaled;
}
#endif

ref<YPixmap> YPixmap::scale(int const w, int const h) {
    ref<YPixmap> pixmap;
    pixmap.init(this);
    ref<YImage> image = YImage::createFromPixmap(pixmap);
    if (image != null) {
        image = image->scale(w, h);
        if (image != null)
            pixmap = YPixmap::createFromImage(image);
    }
    return pixmap;
}

ref<YPixmap> YPixmap::create(int w, int h, bool useMask) {
    ref<YPixmap> n;

    Pixmap pixmap = createPixmap(w, h);
    Pixmap mask = useMask ? createMask(w, h) : None;
    if (pixmap != None && (!useMask || mask != None))

        n.init(new YPixmap(pixmap, mask, w, h));
    return n;
}

ref<YPixmap> YPixmap::createFromImage(ref<YImage> image) {
    return image->renderToPixmap();
}

ref<YPixmap> YPixmap::createFromPixmapAndMask(Pixmap /*pixmap*/,
                                              Pixmap /*mask*/,
                                              int /*w*/,
                                              int /*h*/)
{
    die(2, "YPixmap::createFromPixmapAndMask");
    return null;
}

ref<YPixmap> YPixmap::createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                    int width, int height,
                                                    int nw, int nh)
{
    if (pix != None) {
        ref<YImage> image =
            YImage::createFromPixmapAndMaskScaled(pix, mask,
                                                  width, height, nw, nh);
        if (image != null) {
            ref<YPixmap> pixmap =
                YPixmap::createFromImage(image);
            return pixmap;
        }
    }
    return null;
}

ref<YPixmap> YPixmap::load(upath filename) {
    ref<YImage> image = YImage::load(filename);
    ref<YPixmap> pixmap;
    if (image != null) {
        pixmap = YPixmap::createFromImage(image);
    }
    return pixmap;
}
