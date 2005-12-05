#ifndef __YPIXMAP_H
#define __YPIXMAP_H

#include "ref.h"
#include "ylib.h"
#include "upath.h"

class YImage;

class YPixmap: public virtual refcounted {
public:
    static ref<YPixmap> create(int w, int h, bool mask = false);
    static ref<YPixmap> load(upath filename);
//    static ref<YPixmap> scale(ref<YPixmap> source, int width, int height);
    static ref<YPixmap> createFromImage(ref<YImage> image);
    static ref<YPixmap> createFromPixmapAndMask(Pixmap pixmap,
                                                Pixmap mask,
                                                int w, int h);
#if 1
    static ref<YPixmap> createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                      int width, int height,
                                                      int nw, int nh);
#endif

#if 1
    void replicate(bool horiz, bool copyMask);
#endif

#if 1
    Pixmap pixmap() const { return fPixmap; }
    Pixmap mask() const { return fMask; }
#endif
    int width() const { return fWidth; }
    int height() const { return fHeight; }
    ref<YPixmap> scale(int w, int h);

protected:
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h) {
        fPixmap = pixmap;
        fMask = mask;
        fWidth = w;
        fHeight = h;
    }
    virtual ~YPixmap();

    friend class YImage;

private:
    int fWidth;
    int fHeight;
#if 1
    Pixmap fPixmap;
    Pixmap fMask;
#endif
};

#endif
