#ifndef __YPAINT_H
#define __YPAINT_H

#include <X11/Xlib.h>

#define ICON_SMALL	16   // small:	16x16 //!!! fix this
#define ICON_LARGE	32   // large:	32x32
#define ICON_HUGE	48   // huge:	48x48

class YWindow;

#ifndef __YIMP_XUTIL__
#ifdef SHAPE
struct XTextProperty;
#endif
#endif

#ifdef SHAPE
struct XShapeEvent;
#endif

enum Direction {
    Up, Left, Down, Right
};

class YColor {
public:
    YColor(unsigned short red, unsigned short green, unsigned short blue);
    YColor(unsigned long pixel);
    YColor(const char *clr);

    void alloc();
    unsigned long pixel() const { return fPixel; }

    YColor *darker();
    YColor *brighter();

    static YColor *black;
    static YColor *white;

private:
    unsigned long fPixel;
    unsigned short fRed;
    unsigned short fGreen;
    unsigned short fBlue;
    YColor *fDarker; //!!! remove this (needs color caching...)
    YColor *fBrighter; //!!! removethis
};

struct YDimension {
    YDimension(unsigned w, unsigned h): w(w), h(h) {}
    unsigned w, h;
};

class YFont {
public:
    static YFont *getFont(const char *name);
    ~YFont();

    unsigned height() const { return fontAscent + fontDescent; }
    unsigned descent() const { return fontDescent; }
    unsigned ascent() const { return fontAscent; }

    unsigned textWidth(const char *str) const;
    unsigned textWidth(const char *str, int len) const;

    unsigned multilineTabPos(const char *str) const;
    YDimension multilineAlloc(const char *str) const;

    static char const * getNameElement(const char *pattern, unsigned element,
				       char *buffer, unsigned size);
private:
#ifdef I18N
    XFontSet font_set;
#endif
    XFontStruct *afont;
    int fontAscent, fontDescent;

    YFont(const char *name);

#ifdef I18N
    static XFontSet getFontSetWithGuess(const char *, char ***, int *, char **);
#endif

    void alloc();

    friend class Graphics;//!!!fix
};

class YPixmap {
public:
    static Pixmap createPixmap(int w, int h);
    static Pixmap createMask(int w, int h);

    YPixmap(const char *fileName);
    YPixmap(const char *fileName, int w, int h);
    YPixmap(int w, int h, bool mask = false);
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h);
#ifdef CONFIG_IMLIB
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h, int wScaled, int hScaled);
#endif
    ~YPixmap();

    Pixmap pixmap() const { return fPixmap; }
    Pixmap mask() const { return fMask; }
    unsigned int width() const { return fWidth; }
    unsigned int height() const { return fHeight; }
    
    void replicate(bool horiz, bool copyMask);

private:
    Pixmap fPixmap;
    Pixmap fMask;
    unsigned int fWidth, fHeight;
    bool fOwned;
};

class YIcon {
public:
    YIcon(const char *fileName);
    YIcon(YPixmap *small, YPixmap *large, YPixmap *huge);
    ~YIcon();

    YPixmap *huge();
    YPixmap *large();
    YPixmap *small();

    const char *iconName() const { return fPath; }
    YIcon *next() const { return fNext; }
private:
    char *fPath;
    YPixmap *fSmall;
    YPixmap *fLarge;
    YPixmap *fHuge;
    YIcon *fNext;
    bool loadedS;
    bool loadedL;
    bool loadedH;

    bool findIcon(char *base, char **fullPath, int size);
    bool findIcon(char **fullPath, int size);
    YPixmap *loadIcon(int size);
};

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

class Graphics {
public:
    Graphics(YWindow *window, unsigned long vmask, XGCValues * gcv);
    Graphics(YWindow *window);
    Graphics(YPixmap *pixmap);
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
    void copyPixmap(const YPixmap *p, const int x, const int y,
		    const int w, const int h, const int dx, const int dy) {
	if (p) copyDrawable(p->pixmap(), x, y, w, h, dx, dy);
    }
#ifdef CONFIG_ANTIALIASING
    void copyPixbuf(class YPixbuf & pixbuf, const int x, const int y,
		    const int w, const int h, const int dx, const int dy);
#endif

    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawLines(XPoint *points, int n, int mode);
    void drawRect(int x, int y, int width, int height);
    void drawArc(int x, int y, int width, int height, int a1, int a2);
    void drawArrow(Direction direction, int x, int y, int size, bool pressed = false);

    void drawChars(const char *data, int offset, int len, int x, int y);
    void drawCharUnderline(int x, int y, const char *str, int charPos);
    void drawCharsEllipsis(const char *data, int len, int x, int y, int maxWidth);
    void drawCharsMultiline(const char *str, int x, int y);

    void drawPixmap(YPixmap const * pix, int const x, int const y);
    void drawMask(YPixmap const * pix, int const x, int const y);
    void drawClippedPixmap(Pixmap pix, Pixmap clip,
                           int x, int y, int w, int h, int toX, int toY);
    void fillRect(int x, int y, int width, int height);
    void fillPolygon(XPoint * points, int const n, int const shape,
		    int const mode);
    void fillArc(int x, int y, int width, int height, int a1, int a2);
    void setColor(YColor *aColor);
    void setFont(YFont const *aFont);
    void setPenStyle(bool dotLine = false); ///!!!hack
    void setFunction(int function = GXcopy);
    
    void setClipRectangles(int x, int y, XRectangle rectangles[], int n = 1,
    			   int ordering = Unsorted);
    void setClipMask(Pixmap pixmap = None);

    void draw3DRect(int x, int y, int w, int h, bool raised);
    void drawBorderW(int x, int y, int w, int h, bool raised);
    void drawBorderM(int x, int y, int w, int h, bool raised);
    void drawBorderG(int x, int y, int w, int h, bool raised);
    void drawCenteredPixmap(int x, int y, int w, int h, YPixmap *pixmap);
    void drawOutline(int l, int t, int r, int b, int iw, int ih);
    void repHorz(Drawable drawable, int pw, int ph, int x, int y, int w);
    void repVert(Drawable drawable, int pw, int ph, int x, int y, int h);
    void fillPixmap(YPixmap const * pixmap, int x, int y, int w, int h,
    		    int sx = 0, int sy = 0);

    void drawSurface(YSurface const & surface, int x, int y, int w, int h) {
        drawSurface(surface, x, y, w, h, 0, 0, w, h);
    }
    void drawSurface(YSurface const & surface, int x, int y, int w, int h,
		     int const sx, int const sy, const int sw, const int sh);

#ifdef CONFIG_GRADIENTS
    void drawGradient(const class YPixbuf & pixbuf,
		      int const x, int const y, const int w, const int h) {
	drawGradient(pixbuf, x, y, w, h, 0, 0, w, h);
    }
    void drawGradient(const class YPixbuf & pixbuf,
		      int const x, int const y, const int w, const int h,
		      int const gx, int const gy, const int gw, const int gh);
#endif

    void repHorz(YPixmap const * p, int x, int y, int w) {
	if (p) repHorz(p->pixmap(), p->width(), p->height(), x, y, w);
    }
    void repVert(YPixmap const * p, int x, int y, int h) {
	if (p) repVert(p->pixmap(), p->width(), p->height(), x, y, h);
    }

    YColor *getColor() const { return color; }
    YFont const *getFont() const { return font; }
    GC handle() const { return gc; }
private:
    Display *display;
    Drawable drawable;
    GC gc;

    YColor *color;
    YFont const *font;
};

extern Colormap defaultColormap; //!!!???

YIcon *getIcon(const char *name);
void freeIcons();

#endif
