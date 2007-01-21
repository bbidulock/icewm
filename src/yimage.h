#ifndef __YIMAGE_H
#define __YIMAGE_H

#include "ref.h"
#include "ypaint.h"

class YPixmap;
class Graphics;

class YImage: public refcounted {
public:
    static ref<YImage> create(int width, int height);
    static ref<YImage> load(upath filename);
    static ref<YImage> createFromPixmap(ref<YPixmap> image);
    static ref<YImage> createFromPixmapAndMask(Pixmap pix, Pixmap mask,
                                               int width, int height);
    static ref<YImage> createFromPixmapAndMaskScaled(Pixmap pix, Pixmap mask,
                                                     int width, int height,
                                                     int nw, int nh);
    static ref<YImage> createFromIconProperty(long *pixels,
                                              int width, int height);

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    virtual bool valid() const = 0;

    virtual ref<YPixmap> renderToPixmap() = 0;
    virtual ref<YImage> scale(int width, int height) = 0;
    virtual void draw(Graphics &g, int dx, int dy) = 0;
    virtual void draw(Graphics &g, int x, int y, int w, int h, int dx, int dy) = 0;
    virtual void composite(Graphics &g, int x, int y, int w, int h, int dx, int dy) = 0;
protected:
    YImage(int width, int height) { fWidth = width; fHeight = height; }
    virtual ~YImage() {};

    ref<YPixmap> createPixmap(Pixmap pixmap, Pixmap mask, int w, int h);

private:
    int fWidth;
    int fHeight;
};

#endif
