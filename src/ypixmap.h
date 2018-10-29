#ifndef __YPIXMAP_H
#define __YPIXMAP_H

#include "ref.h"
#include "ylib.h"
#include "upath.h"

class YImage;

class YPixmap: public virtual refcounted {
public:
    static ref<YPixmap> create(unsigned w, unsigned h, unsigned depth, bool mask = false);
    static ref<YPixmap> load(upath filename);
//    static ref<YPixmap> scale(ref<YPixmap> source, int width, int height);
    static ref<YPixmap> createFromImage(ref<YImage> image, unsigned depth);
    static ref<YPixmap> createFromPixmapAndMask(Pixmap pixmap,
                                                Pixmap mask,
                                                unsigned w, unsigned h);
    static ref<YPixmap> createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                      unsigned width, unsigned height,
                                                      unsigned nw, unsigned nh);

    void replicate(bool horiz, bool copyMask);

    Pixmap pixmap() const { return fPixmap; }
    Pixmap mask() const { return fMask; }
    unsigned width() const { return fWidth; }
    unsigned height() const { return fHeight; }
    unsigned depth() const { return fDepth; }
    ref<YImage> image();
    Pixmap pixmap32();
    ref<YPixmap> scale(unsigned w, unsigned h);
    ref<YPixmap> subimage(unsigned x, unsigned y, unsigned w, unsigned h);

private:
    YPixmap(Pixmap pixmap, Pixmap mask,
            unsigned width, unsigned height,
            unsigned depth, ref<YImage> image):
        fWidth(width),
        fHeight(height),
        fDepth(depth),
        fPixmap(pixmap),
        fMask(mask),
        fImage(image),
        fPixmap32()
    {
    }
    virtual ~YPixmap();

    friend class YImage;

private:
    unsigned fWidth;
    unsigned fHeight;
    unsigned fDepth;

    Pixmap fPixmap;
    Pixmap fMask;
    ref<YImage> fImage;
    ref<YPixmap> fPixmap32;
};

#endif

// vim: set sw=4 ts=4 et:
