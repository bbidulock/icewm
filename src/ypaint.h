#ifndef __YPAINT_H
#define __YPAINT_H

#include "base.h"

#include <X11/Xlib.h>

#ifdef CONFIG_SHAPE //-----------------------------------------------------------------
#include <X11/Xutil.h>
#define __YIMP_XUTIL__

#include <X11/extensions/shape.h>
#endif // CONFIG_SHAPE ---------------------------------------------------------

#ifdef CONFIG_XFREETYPE //------------------------------------------------------
#include <X11/Xft/Xft.h>
#define INIT_XFREETYPE(Member, Value) , Member(Value)
#else
#define INIT_XFREETYPE(Member, Value)
#endif // CONFIG_XFREETYPE -----------------------------------------------------

#ifdef CONFIG_GRADIENTS //------------------------------------------------------
#define TEST_GRADIENT(Cond) (Cond)
#define IF_CONFIG_GRADIENTS(Cond, Stmt) if (Cond) { Stmt; }
#else    
#define TEST_GRADIENT(Cond) true
#define IF_CONFIG_GRADIENTS(Cond, Stmt) if (false) {}
#endif // CONFIG_GRADIENTS -----------------------------------------------------

class YWindow;
class YPixbuf;

#ifdef SHAPE
struct XShapeEvent;
#endif

enum YDirection {
    Up, Left, Down, Right
};

/******************************************************************************/
/******************************************************************************/

class YColor {
public:
    YColor(unsigned short red, unsigned short green, unsigned short blue);
    YColor(unsigned long pixel);
    YColor(char const * clr);

#ifdef CONFIG_XFREETYPE
    ~YColor();

    operator XftColor * ();
    void allocXft();
#endif

    unsigned long pixel() const { return fPixel; }

    YColor * darker();
    YColor * brighter();

    static YColor * black;
    static YColor * white;

private:
    void alloc();

    unsigned long fPixel;
    unsigned short fRed;
    unsigned short fGreen;
    unsigned short fBlue;
    YColor * fDarker; //!!! remove this (needs color caching...)
    YColor * fBrighter; //!!! removethis

#ifdef CONFIG_XFREETYPE
    XftColor * xftColor;
#endif
};

struct YDimension {
    YDimension(unsigned w, unsigned h): w(w), h(h) {}
    unsigned w, h;
};

/******************************************************************************/
/******************************************************************************/

class YFont {
public:
    static YFont * getFont(char const * name, bool antialias = true);
    virtual ~YFont() {}

    virtual bool valid() const = 0;
    virtual int height() const { return ascent() + descent(); }
    virtual int descent() const = 0;
    virtual int ascent() const = 0;
    virtual int textWidth(char const * str, int len) const = 0;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y, 
                            char const * str, int len) = 0;

    int textWidth(char const * str) const;
    int multilineTabPos(char const * str) const;
    YDimension multilineAlloc(char const * str) const;

    static char * getNameElement(char const * pattern, unsigned const element);
};

/******************************************************************************/
/******************************************************************************/

class YPixmap {
public:
    YPixmap(YPixmap const & pixmap);
#ifdef CONFIG_ANTIALIASING
    YPixmap(YPixbuf & pixbuf);
#endif

    YPixmap(char const * fileName);
    YPixmap(char const * fileName, int w, int h);
    YPixmap(int w, int h, bool mask = false);
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h);
#ifdef CONFIG_IMLIB
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h, int wScaled, int hScaled);
#endif
    ~YPixmap();

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
    Pixmap fPixmap;
    Pixmap fMask;
    unsigned int fWidth, fHeight;
    bool fOwned;
};

class YIcon {
public:
    enum Sizes {
        sizeSmall = 16,
        sizeLarge = 32,
        sizeHuge = 48
    };
    
#ifdef CONFIG_ANTIALIASING
    typedef YPixbuf Image;
#else    
    typedef YPixmap Image;
#endif

    YIcon(char const * fileName);
    YIcon(Image * small, Image * large, Image * huge);
    ~YIcon();

    Image * huge();
    Image * large();
    Image * small();

    char const * iconName() const { return fPath; }

    static YIcon *getIcon(const char *name);
    static void freeIcons();
    bool isCached() { return fCached; }
    void setCached(bool cached) { fCached = cached; }

private:
    Image * fSmall;
    Image * fLarge;
    Image * fHuge;

    bool loadedS;
    bool loadedL;
    bool loadedH;

    char * fPath;
    bool fCached;

    char * findIcon(char * base, unsigned size);
    char * findIcon(unsigned size);
    void removeFromCache();
    static int cacheFind(const char *name);
    Image * loadIcon(unsigned size);
};

/******************************************************************************/
/******************************************************************************/

struct YSurface {
#ifdef CONFIG_GRADIENTS
    YSurface(class YColor * color, class YPixmap * pixmap,
             class YPixbuf * gradient):
    color(color), pixmap(pixmap), gradient(gradient) {}
#else
    YSurface(class YColor * color, class YPixmap * pixmap):
    color(color), pixmap(pixmap) {}
#endif

    class YColor * color;
    class YPixmap * pixmap;
#ifdef CONFIG_GRADIENTS
    class YPixbuf * gradient;
#endif
};

/******************************************************************************/
/******************************************************************************/

class Graphics {
public:
    Graphics(YWindow & window, unsigned long vmask, XGCValues * gcv);
    Graphics(YWindow & window);
    Graphics(YPixmap const & pixmap);
    Graphics(Drawable drawable, unsigned long vmask, XGCValues * gcv);
    Graphics(Drawable drawable);
    virtual ~Graphics();

    void copyArea(const int x, const int y, const int width, const int height,
                  const int dx, const int dy);
    void copyDrawable(const Drawable d, const int x, const int y, 
                      const int w, const int h, const int dx, const int dy);
    void copyImage(XImage * im, const int x, const int y, 
                   const int w, const int h, const int dx, const int dy);
    void copyImage(XImage * im, const int x, const int y) {
        copyImage(im, 0, 0, im->width, im->height, x, y);
    }
    void copyPixmap(const YPixmap * p, const int x, const int y,
                    const int w, const int h, const int dx, const int dy) {
        if (p) copyDrawable(p->pixmap(), x, y, w, h, dx, dy);
    }
#ifdef CONFIG_ANTIALIASING
    void copyPixbuf(class YPixbuf & pixbuf, const int x, const int y,
                    const int w, const int h, const int dx, const int dy,
                    bool useAlpha = true);
    void copyAlphaMask(class YPixbuf & pixbuf, const int x, const int y,
                       const int w, const int h, const int dx, const int dy);
#endif

    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawLines(XPoint * points, int n, int mode = CoordModeOrigin);
    void drawSegments(XSegment * segments, int n);
    void drawRect(int x, int y, int width, int height);
    void drawRects(XRectangle * rects, int n);
    void drawArc(int x, int y, int width, int height, int a1, int a2);
    void drawArrow(YDirection direction, int x, int y, int size, bool pressed = false);

    void drawChars(char const * data, int offset, int len, int x, int y);
    void drawCharUnderline(int x, int y, char const * str, int charPos);

    void drawString(int x, int y, char const * str);
    void drawStringEllipsis(int x, int y, char const * str, int maxWidth);
    void drawStringMultiline(int x, int y, char const * str);

#ifdef CONFIG_MOVESIZE_FX
    void drawString90(int x, int y, char const * str);
    void drawString180(int x, int y, char const * str);
    void drawString270(int x, int y, char const * str);
#endif

    void drawImage(YIcon::Image * img, int const x, int const y);
    void drawPixmap(YPixmap const * pix, int const x, int const y);
    void drawMask(YPixmap const * pix, int const x, int const y);
    void drawClippedPixmap(Pixmap pix, Pixmap clip,
                           int x, int y, int w, int h, int toX, int toY);
    void fillRect(int x, int y, int width, int height);
    void fillRects(XRectangle * rects, int n);
    void fillPolygon(XPoint * points, int const n, int const shape,
                     int const mode);
    void fillArc(int x, int y, int width, int height, int a1, int a2);
    void setColor(YColor * aColor);
    void setFont(YFont * aFont);
    void setThinLines(void) { setLineWidth(0); }
    void setWideLines(int width = 1) { setLineWidth(width >= 1 ? width : 1); }
    void setLineWidth(int width);
    void setPenStyle(bool dotLine = false); ///!!!hack
    void setFunction(int function = GXcopy);
    
    void setClipRects(int x, int y, XRectangle rectangles[], int n = 1,
                      int ordering = Unsorted);
    void setClipMask(Pixmap mask = None);
    void setClipOrigin(int x, int y);

    void draw3DRect(int x, int y, int w, int h, bool raised);
    void drawBorderW(int x, int y, int w, int h, bool raised);
    void drawBorderM(int x, int y, int w, int h, bool raised);
    void drawBorderG(int x, int y, int w, int h, bool raised);
    void drawCenteredPixmap(int x, int y, int w, int h, YPixmap * pixmap);
    void drawOutline(int l, int t, int r, int b, int iw, int ih);
    void repHorz(Drawable drawable, int pw, int ph, int x, int y, int w);
    void repVert(Drawable drawable, int pw, int ph, int x, int y, int h);
    void fillPixmap(YPixmap const * pixmap, int x, int y, int w, int h,
                    int sx = 0, int sy = 0);

    void drawSurface(YSurface const & surface, int x, int y, int w, int h,
                     int const sx, int const sy, const int sw, const int sh);
    void drawSurface(YSurface const & surface, int x, int y, int w, int h) {
        drawSurface(surface, x, y, w, h, 0, 0, w, h);
    }

#ifdef CONFIG_GRADIENTS
    void drawGradient(const class YPixbuf & pixbuf,
                      int const x, int const y, const int w, const int h,
                      int const gx, int const gy, const int gw, const int gh);
    void drawGradient(const class YPixbuf & pixbuf,
                      int const x, int const y, const int w, const int h) {
        drawGradient(pixbuf, x, y, w, h, 0, 0, w, h);
    }
#endif

    void repHorz(YPixmap const * p, int x, int y, int w) {
        if (p) repHorz(p->pixmap(), p->width(), p->height(), x, y, w);
    }
    void repVert(YPixmap const * p, int x, int y, int h) {
        if (p) repVert(p->pixmap(), p->width(), p->height(), x, y, h);
    }

    Display * display() const { return fDisplay; }
    int drawable() const { return fDrawable; }
    GC handle() const { return gc; }

    YColor * color() const { return fColor; }
    YFont const * font() const { return fFont; }
    int function() const;

private:
    Display * fDisplay;
    Drawable fDrawable;
    GC gc;

    YColor * fColor;
    YFont * fFont;

#ifdef CONFIG_MOVESIZE_FX
    template <class Rotation> 
        void drawStringRotated(int x, int y, char const * str);
#endif
};

#if 0
class YWindowAttributes {
public:
    YWindowAttributes(Window window);
    
    Window root() const { return attributes.root; }
    int x() const { return attributes.x; }
    int y() const { return attributes.y; }
    unsigned width() const { return attributes.width; }
    unsigned height() const { return attributes.height; }
    unsigned border() const { return attributes.border_width; }
    unsigned depth() const { return attributes.depth; }
    Visual * visual() const { return attributes.visual; }
    Colormap colormap() const { return attributes.colormap; }

private:
    XWindowAttributes attributes;
};
#endif

#endif
