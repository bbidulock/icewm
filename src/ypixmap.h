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
    static ref<YPixmap> createFromImage(ref<YImage> image, unsigned width);
    static ref<YPixmap> createFromPixmapAndMask(Pixmap pixmap,
                                                Pixmap mask,
                                                unsigned w, unsigned h);
#if 1
    static ref<YPixmap> createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                      unsigned width, unsigned height,
                                                      unsigned nw, unsigned nh);
#endif

#if 1
    void replicate(bool horiz, bool copyMask);
#endif

#if 1
    Pixmap pixmap() const { return fPixmap; }
    Pixmap mask() const { return fMask; }
#endif
    unsigned width() const { return fWidth; }
    unsigned height() const { return fHeight; }
    unsigned depth() const { return fDepth; }
    ref<YPixmap> scale(unsigned w, unsigned h);

protected:
    YPixmap(Pixmap pixmap, Pixmap mask, unsigned w, unsigned h, unsigned depth) {
        fPixmap = pixmap;
        fMask = mask;
        fWidth = w;
        fHeight = h;
        fDepth = depth;
    }
    virtual ~YPixmap();

    friend class YImage;

private:
    unsigned fWidth;
    unsigned fHeight;
    unsigned fDepth;
#if 1
    Pixmap fPixmap;
    Pixmap fMask;
#endif
};

#endif

// vim: set sw=4 ts=4 et:
