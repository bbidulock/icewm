#ifndef __YPIXMAP_H
#define __YPIXMAP_H

#include "ref.h"
#include "ylib.h"

class YPixbuf;

class YPixmap: public virtual refcounted {
public:
    static ref<YPixmap> create(int w, int h, bool mask = false);
    static ref<YPixmap> load(const char *filename);
//    YPixmap(YPixmap const &pixmap);
#ifdef CONFIG_ANTIALIASING
    YPixmap(YPixbuf & pixbuf);
#endif

    YPixmap(char const * fileName);
#if 0
    YPixmap(char const * fileName, int w, int h);
#endif
    YPixmap(int w, int h, bool mask = false);
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h);
#ifdef CONFIG_IMLIB
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h, int wScaled, int hScaled);
    void scaleImage(Pixmap pixmap, Pixmap mask, int x, int y, int w, int h, int nw, int nh);
#endif
    ~YPixmap();

private:
    YPixmap(const ref<YPixmap> &pixmap, int newWidth, int newHeight);
public:
    ref<YPixmap> scale(int width, int height);
    static ref<YPixmap> scale(ref<YPixmap> source, int width, int height);

    Pixmap pixmap() const { return fPixmap; }
    Pixmap mask() const { return fMask; }
    int width() const { return fWidth; }
    int height() const { return fHeight; }
    
    bool valid() const { return (fPixmap != None); }

    void replicate(bool horiz, bool copyMask);

    static Pixmap createPixmap(int w, int h);
    static Pixmap createPixmap(int w, int h, int depth);
    static Pixmap createMask(int w, int h);

private:
    int fWidth;
    int fHeight;
    Pixmap fPixmap;
    Pixmap fMask;
    bool fOwned;
};

#endif
