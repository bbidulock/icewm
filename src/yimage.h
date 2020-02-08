#ifndef __YIMAGE_H
#define __YIMAGE_H

#include "ref.h"
#include "ypaint.h"

class YPixmap;
class Graphics;

class YImage: public refcounted {
public:
    static ref<YImage> load(upath filename);
    static ref<YImage> createFromPixmap(ref<YPixmap> image);
    static ref<YImage> createFromPixmapAndMask(Pixmap pix, Pixmap mask,
                                               unsigned width, unsigned height);
    static ref<YImage> createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                     unsigned width, unsigned height,
                                                     unsigned nw, unsigned nh);
    static ref<YImage> createFromIconProperty(long *pixels,
                                              unsigned width, unsigned height);
    static bool supportsDepth(unsigned depth);

    unsigned width() const { return fWidth; }
    unsigned height() const { return fHeight; }
    virtual unsigned depth() const = 0;
    virtual bool hasAlpha() const = 0;
    virtual bool valid() const = 0;

    virtual ref<YPixmap> renderToPixmap(unsigned depth, bool premult = false) = 0;
    virtual ref<YImage> scale(unsigned width, unsigned height) = 0;
    virtual void draw(Graphics &g, int dx, int dy) = 0;
    virtual void draw(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy) = 0;
    virtual void composite(Graphics &g, int x, int y, unsigned w, unsigned h, int dx, int dy) = 0;
    virtual ref<YImage> subimage(int x, int y, unsigned w, unsigned h) = 0;
    virtual void save(upath filename) = 0;

protected:
    YImage(unsigned width, unsigned height) { fWidth = width; fHeight = height; }
    virtual ~YImage() {}

    ref<YPixmap> createPixmap(Pixmap pixmap, Pixmap mask, unsigned w, unsigned h, unsigned depth);

private:
    unsigned fWidth;
    unsigned fHeight;
};

#endif

// vim: set sw=4 ts=4 et:
