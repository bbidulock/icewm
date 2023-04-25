#ifndef YIMAGE2_H
#define YIMAGE2_H

#ifdef CONFIG_IMLIB2

#include <Imlib2.h>
#include "yimage.h"

#define ATH 10  /* alpha threshold */

typedef Imlib_Image Image;

class YImage2: public YImage {
public:
    YImage2(unsigned width, unsigned height, Image image):
        YImage(width, height), fImage(image) { ++instances; }
    virtual ~YImage2() {
        context();
        imlib_free_image();
        if (--instances == 0)
            freegcs();
    }
    virtual ref<YPixmap> renderToPixmap(unsigned depth, bool premult);
    virtual ref<YImage> scale(unsigned width, unsigned height);
    virtual void draw(Graphics& g, int dx, int dy);
    virtual void draw(Graphics& g, int x, int y,
                       unsigned w, unsigned h, int dx, int dy);
    virtual void composite(Graphics& g, int x, int y,
                            unsigned w, unsigned h, int dx, int dy);
    virtual unsigned depth() const;
    virtual bool hasAlpha() const;
    virtual bool valid() const { return fImage != nullptr; }
    virtual ref<YImage> subimage(int x, int y, unsigned w, unsigned h);
    virtual void save(upath filename);
    virtual void copy(Graphics& g, int x, int y);

private:
    Image fImage;

    void context(Image image) const { imlib_context_set_image(image); }
    void context() const { imlib_context_set_image(fImage); }
    ref<YImage2> twoHigh(unsigned height);
    ref<YImage2> twoWide(unsigned width);

    static GC gcs[3];
    static GC gc(Drawable draw, unsigned depth);
    static void freegcs();
    static unsigned instances;
};

#endif

#endif

// vim: set sw=4 ts=4 et:
