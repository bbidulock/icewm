#ifndef YIMAGE_H
#define YIMAGE_H

#if defined(CONFIG_LIBRSVG) || defined(CONFIG_NANOSVG)
#define ICE_SUPPORT_SVG 1
#endif

#include "ref.h"
#include "ypaint.h"

class YPixmap;
class Graphics;

class YImage: public refcounted {
public:
    static ref<YImage> load(upath filename);
#if ICE_SUPPORT_SVG
    static ref<YImage> loadsvg(upath filename);
#endif
    static ref<YImage> createFromPixmap(ref<YPixmap> image);
    static ref<YImage> createFromPixmapAndMask(Pixmap pix, Pixmap mask,
                                               unsigned width, unsigned height);
    static ref<YImage> createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                     unsigned width, unsigned height,
                                                     unsigned nw, unsigned nh);
    static ref<YImage> createFromIconProperty(long *pixels,
                                              unsigned width, unsigned height);
    static bool supportsDepth(unsigned depth);
    static const char* renderName();

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
    virtual void copy(Graphics& g, int x, int y) { draw(g, x, y); }

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
