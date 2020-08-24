#ifndef __YPAINT_H
#define __YPAINT_H

#include "ypixmap.h"
#include "yimage.h"
#include "mstring.h"

#ifdef CONFIG_SHAPE
#include <X11/extensions/shape.h>
#endif

#ifdef CONFIG_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#include "ycolor.h"

class YWindow;

enum YDirection {
    Up, Left, Down, Right
};

/******************************************************************************/
/******************************************************************************/

enum Opaqueness {
    NilOpacity = 0,
    MinOpacity = 1,
    MaxOpacity = 100,
};

inline bool validOpacity(int opacity) {
    return MinOpacity <= opacity && opacity <= MaxOpacity;
}

inline unsigned opacityAlpha(int opacity) {
    return (unsigned(opacity) * 255U) / 100U;
}

/******************************************************************************/
/******************************************************************************/

struct YDimension {
    YDimension(unsigned w, unsigned h): w(w), h(h) {}
    unsigned w, h;

    bool operator==(const YDimension& d) const { return w == d.w && h == d.h; }
    bool operator!=(const YDimension& d) const { return w != d.w || h != d.h; }
};

/******************************************************************************/
/******************************************************************************/

class YFont: public virtual refcounted {
public:
    static ref<YFont> getFont(mstring name, mstring xftFont, bool antialias = true);

    virtual ~YFont() {}

    virtual bool valid() const = 0;
    virtual int height() const { return ascent() + descent(); }
    virtual int descent() const = 0;
    virtual int ascent() const = 0;
    virtual int textWidth(mstring s) const = 0;
    virtual int textWidth(char const * str, int len) const = 0;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            char const * str, int len) = 0;

    int textWidth(char const * str) const;
    int multilineTabPos(char const * str) const;
    YDimension multilineAlloc(char const * str) const;
};

/******************************************************************************/
/******************************************************************************/

struct YSurface {
    YSurface(YColor color, ref<YPixmap> pixmap,
             ref<YImage> gradient):
    color(color), pixmap(pixmap), gradient(gradient) {}

    YColor color;
    ref<YPixmap> pixmap;
    ref<YImage> gradient;
};

class Graphics {
public:
    Graphics(YWindow & window, unsigned long vmask, XGCValues * gcv);
    Graphics(YWindow & window);
    Graphics(ref<YPixmap> pixmap, int x_org, int y_org);
    Graphics(Drawable drawable, unsigned w, unsigned h, unsigned depth, unsigned long vmask, XGCValues * gcv);
    Graphics(Drawable drawable, unsigned w, unsigned h, unsigned depth);
    ~Graphics();

    void clear();
    void clearArea(int x, int y, unsigned w, unsigned h);
    void copyArea(const int x, const int y, const unsigned width, const unsigned height,
                  const int dx, const int dy);
    void copyDrawable(const Drawable d, const int x, const int y,
                      const unsigned w, const unsigned h, const int dx, const int dy);
    void copyImage(ref<YImage> image, int x, int y);
    void copyPixmap(ref<YPixmap> p, int dx, int dy);
    void copyPixmap(ref<YPixmap> p, const int x, const int y,
                     const unsigned w, const unsigned h, const int dx, const int dy);

    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawLines(XPoint * points, int n, int mode = CoordModeOrigin);
    void drawSegments(XSegment * segments, int n);
    void drawRect(int x, int y, unsigned width, unsigned height);
    void drawRects(XRectangle * rects, unsigned n);
    void drawArc(int x, int y, unsigned width, unsigned height, int a1, int a2);
    void drawArrow(YDirection direction, int x, int y, unsigned size, bool pressed = false);

    void drawChars(mstring s, int x, int y);
    void drawChars(char const * data, int offset, int len, int x, int y);
    void drawCharUnderline(int x, int y, char const * str, int charPos);

    void drawString(int x, int y, char const * str);
    void drawStringEllipsis(int x, int y, char const * str, int maxWidth);
    void drawStringMultiline(int x, int y, char const * str);

    void drawPixmap(ref<YPixmap> pix, int const x, int const y);
    void drawPixmap(ref<YPixmap> pix, int const x, int const y, unsigned w, unsigned h, int dx, int dy);
    void drawImage(ref<YImage> pix, int const x, int const y);
    void drawImage(ref<YImage> pix, int const x, int const y, unsigned w, unsigned h, int dx, int dy);
    void compositeImage(ref<YImage> pix, int const x, int const y, unsigned w, unsigned h, int dx, int dy);
    void drawMask(ref<YPixmap> pix, int const x, int const y);
    void drawClippedPixmap(Pixmap pix, Pixmap clip,
                           int x, int y, unsigned w, unsigned h, int toX, int toY);
    void fillRect(int x, int y, unsigned width, unsigned height);
    void fillRects(XRectangle * rects, int n);
    void fillPolygon(XPoint * points, int const n, int const shape,
                     int const mode);
    void fillArc(int x, int y, unsigned width, unsigned height, int a1, int a2);
    void setColor(YColor aColor);
    void setColorPixel(unsigned long pixel);
    void setFont(ref<YFont> aFont);
    void setThinLines() { setLineWidth(0); }
    void setWideLines(unsigned width = 1) { setLineWidth(width >= 1 ? width : 1); }
    void setLineWidth(unsigned width);
    void setPenStyle(bool dotLine = false); ///!!!hack
    void setFunction(int function = GXcopy);
    unsigned long getColorPixel() const;

    void draw3DRect(int x, int y, unsigned w, unsigned h, bool raised);
    void drawBorderW(int x, int y, unsigned w, unsigned h, bool raised);
    void drawBorderM(int x, int y, unsigned w, unsigned h, bool raised);
    void drawBorderG(int x, int y, unsigned w, unsigned h, bool raised);
    void drawCenteredPixmap(int x, int y, unsigned w, unsigned h, ref<YPixmap> pixmap);
    void drawOutline(int l, int t, int r, int b, unsigned iw, unsigned ih);
    void repHorz(Drawable drawable, unsigned pw, unsigned ph, int x, int y, unsigned w);
    void repVert(Drawable drawable, unsigned pw, unsigned ph, int x, int y, unsigned h);
    void fillPixmap(ref<YPixmap> pixmap, int x, int y, unsigned w, unsigned h,
                    int sx = 0, int sy = 0);

    void drawSurface(YSurface const & surface, int x, int y, unsigned w, unsigned h,
                     int const sx, int const sy, const unsigned sw, const unsigned sh);
    void drawSurface(YSurface const & surface, int x, int y, unsigned w, unsigned h) {
        drawSurface(surface, x, y, w, h, 0, 0, w, h);
    }

    void drawGradient(ref<YImage> gradient,
                      int const x, int const y, const unsigned w, const unsigned h,
                      int const gx, int const gy, const unsigned gw, const unsigned gh);
    void drawGradient(ref<YImage> gradient,
                      int const x, int const y, const unsigned w, const unsigned h) {
        drawGradient(gradient, x, y, w, h, 0, 0, w, h);
    }

    void repHorz(ref<YPixmap> p, int x, int y, unsigned w);
    void repVert(ref<YPixmap> p, int x, int y, unsigned h);

    Drawable drawable() const { return fDrawable; }
    GC handleX() const { return gc; }
#ifdef CONFIG_XFREETYPE
    struct _XftDraw* handleXft();
#endif

    YColor   color() const { return fColor; }
    ref<YFont> font() const { return fFont; }
    int function() const;
    int xorigin() const { return xOrigin; }
    int yorigin() const { return yOrigin; }
    unsigned rwidth() const { return rWidth; }
    unsigned rheight() const { return rHeight; }
    unsigned rdepth() const { return rDepth; }
    Picture picture();

    void setClipRectangles(XRectangle *rect, int count);
    void setClipMask(Pixmap mask = None);
    void resetClip();
    void maxOpacity();

private:
    Drawable fDrawable;
    GC gc;
#ifdef CONFIG_XFREETYPE
    struct _XftDraw* fXftDraw;
#endif

    YColor   fColor;
    ref<YFont> fFont;
    Picture fPicture;
    int xOrigin, yOrigin;
    unsigned rWidth, rHeight, rDepth;

    Graphics(Graphics const&) = delete;
    Graphics& operator=(Graphics const&) = delete;
};

/******************************************************************************/
/******************************************************************************/

class GraphicsBuffer {
public:
    GraphicsBuffer(YWindow* ywindow, bool clipping = false) :
        fWindow(ywindow),
        fClipping(clipping),
        fNesting(0),
        fPixmap(None),
        fDim(0, 0)
    {
    }
    ~GraphicsBuffer();
    void paint(const class YRect& rect);
    void paint();
    void release();

    YWindow* window() const { return fWindow; }
    int nesting() const { return fNesting; }
    bool operator!() const { return !fPixmap; }

private:
    YWindow* fWindow;
    bool fClipping;
    int fNesting;
    Pixmap fPixmap;
    YDimension fDim;

    Pixmap pixmap();
    void paint(Pixmap p, const class YRect& rect);
};

#endif

// vim: set sw=4 ts=4 et:
