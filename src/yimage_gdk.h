#ifndef YIMAGE_GDK_H
#define YIMAGE_GDK_H

#ifdef CONFIG_GDK_PIXBUF_XLIB

#include "yimage.h"
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>

#define ATH 10  /* alpha threshold */

class YImageGDK: public YImage {
public:
    YImageGDK(unsigned width, unsigned height, GdkPixbuf *pixbuf):
        YImage(width, height)
    {
        fPixbuf = pixbuf;
    }
    virtual ~YImageGDK() {
        g_object_unref(G_OBJECT(fPixbuf));
    }
    virtual ref<YPixmap> renderToPixmap(unsigned depth, bool premult);
    virtual ref<YImage> scale(unsigned width, unsigned height);
    virtual void draw(Graphics &g, int dx, int dy);
    virtual void draw(Graphics &g, int x, int y,
                       unsigned w, unsigned h, int dx, int dy);
    virtual void composite(Graphics &g, int x, int y,
                            unsigned w, unsigned h, int dx, int dy);
    virtual unsigned depth() const;
    virtual bool hasAlpha() const;
    virtual bool valid() const { return fPixbuf != nullptr; }
    virtual ref<YImage> subimage(int x, int y, unsigned w, unsigned h);
    virtual void save(upath filename);

private:
    GdkPixbuf *fPixbuf;
};

#endif

#endif

// vim: set sw=4 ts=4 et:
