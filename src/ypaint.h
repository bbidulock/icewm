#ifndef YPAINT_H
#define YPAINT_H

#include "ypixmap.h"
#include "yimage.h"

#ifdef CONFIG_SHAPE
#include <X11/extensions/shape.h>
#endif

#ifdef CONFIG_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#include "ycolor.h"
#include "yfontbase.h"

class mstring;
class YWindow;
class YFontName;

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

class YFont {
public:
    YFont operator=(YFontName& fontName);

    YFont() : base(nullptr) { }
    YFont(YFontName& fontName) : base(nullptr) { operator=(fontName); }

    void operator=(null_ref &) { base = nullptr; }

    bool operator==(null_ref &) const { return base == nullptr; }
    bool operator!=(null_ref &) const { return base != nullptr; }
    operator bool() const { return base != nullptr; }

    YFontBase* operator->() const { return base; }
    YFontBase* operator*() const { return base; }

private:
    operator int() const = delete;
    operator void*() const = delete;

    typedef YFontBase* (*fontloader)(const char* name);
    void loadFont(fontloader loader, const char* name);
    YFontBase* base;
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
    Graphics(YWindow& window, unsigned long vmask, XGCValues* gcv);
    explicit Graphics(YWindow& window);
    explicit Graphics(ref<YPixmap> pixmap);
    Graphics(Drawable drawable, unsigned w, unsigned h, unsigned depth, unsigned long vmask, XGCValues * gcv);
    Graphics(Drawable drawable, unsigned w, unsigned h, unsigned depth);
    ~Graphics();

    void clear();
    void clearArea(int x, int y, unsigned w, unsigned h);
    void copyArea(int x, int y, unsigned width, unsigned height,
                  int dx, int dy);
    void copyDrawable(Drawable d, int x, int y,
                      unsigned w, unsigned h, int dx, int dy);
    void copyImage(ref<YImage> image, int x, int y);
    void copyPixmap(ref<YPixmap> p, int dx, int dy);
    void copyPixmap(ref<YPixmap> p, int x, int y,
                     unsigned w, unsigned h, int dx, int dy);

    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawLines(XPoint * points, int n, int mode = CoordModeOrigin);
    void drawSegments(XSegment * segments, int n);
    void drawRect(int x, int y, unsigned width, unsigned height);
    void drawRects(XRectangle * rects, unsigned n);
    void drawArc(int x, int y, unsigned width, unsigned height, int a1, int a2);
    void drawArrow(YDirection direction, int x, int y, unsigned size, bool pressed = false);

    void drawChars(mstring s, int x, int y);
    void drawChars(const char* data, int offset, int len, int x, int y);
    void drawChars(wchar_t* data, int offset, int len, int x, int y);
    void drawCharUnderline(int x, int y, char const * str, int charPos);

    void drawString(int x, int y, const char* str);
    void drawStringEllipsis(int x, int y, char const * str, int maxWidth);
    void drawStringMultiline(const char* str, int x, int y, unsigned width);

    void drawPixmap(ref<YPixmap> pix, int x, int y);
    void drawPixmap(ref<YPixmap> pix, int x, int y, unsigned w, unsigned h, int dx, int dy);
    void drawImage(ref<YImage> pix, int x, int y);
    void drawImage(ref<YImage> pix, int x, int y, unsigned w, unsigned h, int dx, int dy);
    void compositeImage(ref<YImage> pix, int x, int y, unsigned w, unsigned h, int dx, int dy);
    void drawMask(ref<YPixmap> pix, int x, int y);
    void drawClippedPixmap(Pixmap pix, Pixmap clip,
                           int x, int y, unsigned w, unsigned h, int toX, int toY);
    void fillRect(int x, int y, unsigned width, unsigned height);
    void fillRect(int x, int y, unsigned width, unsigned height, unsigned rounding);
    void fillRects(XRectangle * rects, int n);
    void fillPolygon(XPoint * points, int n, int shape,
                     int mode);
    void fillArc(int x, int y, unsigned width, unsigned height, int a1, int a2);
    void setColor(const YColor& aColor);
    void setColorPixel(unsigned long pixel);
    void setFont(YFont aFont) { fFont = aFont; }
    void setThinLines() { setLineWidth(0); }
    void setWideLines(unsigned width = 1) { setLineWidth(width >= 1 ? width : 1); }
    void setLineWidth(unsigned width);
    void setPenStyle(bool dotLine = false, int cap = CapButt, int join = JoinMiter);
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
                     int sx, int sy, unsigned sw, unsigned sh);
    void drawSurface(YSurface const & surface, int x, int y, unsigned w, unsigned h) {
        drawSurface(surface, x, y, w, h, 0, 0, w, h);
    }

    void drawGradient(ref<YImage> gradient,
                      int x, int y, const unsigned w, const unsigned h,
                      int gx, int gy, unsigned gw, unsigned gh);
    void drawGradient(ref<YImage> gradient,
                      int x, int y, unsigned w, unsigned h) {
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
    YFont font() const { return fFont; }
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
    YFont fFont;
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
    void scroll(int dx, int dy);

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
