#ifndef __YPAINT_H
#define __YPAINT_H

#include "base.h"
#include "ref.h"
#include "ypixmap.h"
#include "yimage.h"
#include "mstring.h"
#if 0
#include "ypixbuf.h"
#endif

#include <X11/Xlib.h>

#ifdef CONFIG_SHAPE
#include <X11/Xutil.h>
#define __YIMP_XUTIL__
#include <X11/extensions/shape.h>
#endif

#ifdef CONFIG_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#ifdef CONFIG_XFREETYPE //------------------------------------------------------
#if CONFIG_XFREETYPE >= 2
#include <ft2build.h>
#endif
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
class YIcon;

#ifdef SHAPE
struct XShapeEvent;
#endif

enum YDirection {
    Up, Left, Down, Right
};

class YImage;

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

class YFont: public virtual refcounted {
public:
    static ref<YFont> getFont(ustring name, ustring xftFont, bool antialias = true);

    virtual ~YFont() {}

    virtual bool valid() const = 0;
    virtual int height() const { return ascent() + descent(); }
    virtual int descent() const = 0;
    virtual int ascent() const = 0;
    virtual int textWidth(const ustring &s) const = 0;
    virtual int textWidth(char const * str, int len) const = 0;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y, 
                            char const * str, int len) = 0;

    int textWidth(char const * str) const;
    int multilineTabPos(char const * str) const;
    YDimension multilineAlloc(char const * str) const;
    YDimension multilineAlloc(const ustring &str) const;
};

/******************************************************************************/
/******************************************************************************/

struct YSurface {
#ifdef CONFIG_GRADIENTS
    YSurface(class YColor * color, ref<YPixmap> pixmap,
             ref<YImage> gradient):
    color(color), pixmap(pixmap), gradient(gradient) {}
#else
    YSurface(class YColor * color, ref<YPixmap> pixmap):
    color(color), pixmap(pixmap) {}
#endif

    class YColor * color;
    ref<YPixmap> pixmap;
#ifdef CONFIG_GRADIENTS
    ref<YImage> gradient;
#endif
};

class Graphics {
public:
    Graphics(YWindow & window, unsigned long vmask, XGCValues * gcv);
    Graphics(YWindow & window);
    Graphics(const ref<YPixmap> &pixmap, int x_org, int y_org);
    Graphics(Drawable drawable, int w, int h, unsigned long vmask, XGCValues * gcv);
    Graphics(Drawable drawable, int w, int h);
    virtual ~Graphics();

    void copyArea(const int x, const int y, const int width, const int height,
                  const int dx, const int dy);
    void copyDrawable(const Drawable d, const int x, const int y, 
                      const int w, const int h, const int dx, const int dy);
#if 0
    void copyImage(XImage * im, const int x, const int y, 
                   const int w, const int h, const int dx, const int dy);
    void copyImage(XImage * im, const int x, const int y) {
        copyImage(im, 0, 0, im->width, im->height, x, y);
    }
#endif
    void copyPixmap(const ref<YPixmap> &p, const int x, const int y,
                     const int w, const int h, const int dx, const int dy)
    {
        if (p != null)
            copyDrawable(p->pixmap(), x, y, w, h, dx, dy);
    }
#if 0
#ifdef CONFIG_ANTIALIASING
    void copyPixbuf(class YPixbuf & pixbuf, const int x, const int y,
                    const int w, const int h, const int dx, const int dy,
                    bool useAlpha = true);
    void copyAlphaMask(class YPixbuf & pixbuf, const int x, const int y,
                       const int w, const int h, const int dx, const int dy);
#endif
#endif

    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawLines(XPoint * points, int n, int mode = CoordModeOrigin);
    void drawSegments(XSegment * segments, int n);
    void drawRect(int x, int y, int width, int height);
    void drawRects(XRectangle * rects, int n);
    void drawArc(int x, int y, int width, int height, int a1, int a2);
    void drawArrow(YDirection direction, int x, int y, int size, bool pressed = false);

    void drawChars(const ustring &s, int x, int y);
    void drawChars(char const * data, int offset, int len, int x, int y);
    void drawCharUnderline(int x, int y, char const * str, int charPos);

    void drawCharUnderline(int x, int y, const ustring &str, int charPos);

    void drawString(int x, int y, char const * str);
    void drawStringEllipsis(int x, int y, char const * str, int maxWidth);
    void drawStringEllipsis(int x, int y, const ustring &str, int maxWidth);
    void drawStringMultiline(int x, int y, char const * str);
    void drawStringMultiline(int x, int y, const ustring &str);

    void drawPixmap(ref<YPixmap> pix, int const x, int const y);
    void drawImage(ref<YImage> pix, int const x, int const y);
    void drawImage(ref<YImage> pix, int const x, int const y, int w, int h, int dx, int dy);
    void compositeImage(ref<YImage> pix, int const x, int const y, int w, int h, int dx, int dy);
    void drawMask(ref<YPixmap> pix, int const x, int const y);
    void drawClippedPixmap(Pixmap pix, Pixmap clip,
                           int x, int y, int w, int h, int toX, int toY);
    void fillRect(int x, int y, int width, int height);
    void fillRects(XRectangle * rects, int n);
    void fillPolygon(XPoint * points, int const n, int const shape,
                     int const mode);
    void fillArc(int x, int y, int width, int height, int a1, int a2);
    void setColor(YColor * aColor);
    void setColorPixel(unsigned long pixel);
    void setFont(ref<YFont> aFont);
    void setThinLines(void) { setLineWidth(0); }
    void setWideLines(int width = 1) { setLineWidth(width >= 1 ? width : 1); }
    void setLineWidth(int width);
    void setPenStyle(bool dotLine = false); ///!!!hack
    void setFunction(int function = GXcopy);

    void draw3DRect(int x, int y, int w, int h, bool raised);
    void drawBorderW(int x, int y, int w, int h, bool raised);
    void drawBorderM(int x, int y, int w, int h, bool raised);
    void drawBorderG(int x, int y, int w, int h, bool raised);
    void drawCenteredPixmap(int x, int y, int w, int h, ref<YPixmap> pixmap);
    void drawOutline(int l, int t, int r, int b, int iw, int ih);
    void repHorz(Drawable drawable, int pw, int ph, int x, int y, int w);
    void repVert(Drawable drawable, int pw, int ph, int x, int y, int h);
    void fillPixmap(const ref<YPixmap> &pixmap, int x, int y, int w, int h,
                    int sx = 0, int sy = 0);

    void drawSurface(YSurface const & surface, int x, int y, int w, int h,
                     int const sx, int const sy, const int sw, const int sh);
    void drawSurface(YSurface const & surface, int x, int y, int w, int h) {
        drawSurface(surface, x, y, w, h, 0, 0, w, h);
    }

#ifdef CONFIG_GRADIENTS
    void drawGradient(ref<YImage> gradient,
                      int const x, int const y, const int w, const int h,
                      int const gx, int const gy, const int gw, const int gh);
    void drawGradient(ref<YImage> gradient,
                      int const x, int const y, const int w, const int h) {
        drawGradient(gradient, x, y, w, h, 0, 0, w, h);
    }
#endif

    void repHorz(ref<YPixmap> p, int x, int y, int w) {
        if (p != null)
            repHorz(p->pixmap(), p->width(), p->height(), x, y, w);
    }
    void repVert(ref<YPixmap> p, int x, int y, int h) {
        if (p != null)
            repVert(p->pixmap(), p->width(), p->height(), x, y, h);
    }

    Display * display() const { return fDisplay; }
    int drawable() const { return fDrawable; }
    GC handleX() const { return gc; }
#ifdef CONFIG_XFREETYPE
    XftDraw *handleXft() const { return fDraw; }
#endif

    YColor * color() const { return fColor; }
    ref<YFont> font() const { return fFont; }
    int function() const;
    int xorigin() const { return xOrigin; }
    int yorigin() const { return yOrigin; }
    int rwidth() const { return rWidth; }
    int rheight() const { return rHeight; }

    void setClipRectangles(XRectangle *rect, int count);
    void setClipMask(Pixmap mask = None);
    void resetClip();
private:
    Display * fDisplay;
    Drawable fDrawable;
    GC gc;
#ifdef CONFIG_XFREETYPE
    XftDraw * fDraw;
#endif

    YColor * fColor;
    ref<YFont> fFont;
    int xOrigin, yOrigin;
    int rWidth, rHeight;
};

#endif
