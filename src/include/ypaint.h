#ifndef __YPAINT_H
#define __YPAINT_H

#include <X11/X.h>

#define ICON_SMALL 16   // small: 16x16 //!!! fix this
#define ICON_LARGE 32   // large: 32x32

class YWindow;

// !!! remove these if possible
#ifndef __YIMP_XLIB__
typedef struct _XDisplay Display;
typedef struct _XGC *GC;
typedef union _XEvent XEvent;
struct XFontStruct;
#ifdef I18N
typedef struct _XOC *XFontSet;
#endif
struct XPoint;
struct XExposeEvent;
struct XGraphicsExposeEvent;
struct XConfigureEvent;
struct XKeyEvent;
struct XButtonEvent;
struct XMotionEvent;
struct XCrossingEvent;
struct XPropertyEvent;
struct XColormapEvent;
struct XFocusChangeEvent;
struct XClientMessageEvent;
struct XMapEvent;
struct XUnmapEvent;
struct XDestroyWindowEvent;
struct XConfigureRequestEvent;
struct XMapRequestEvent;
struct XSelectionEvent;
struct XSelectionClearEvent;
struct XSelectionRequestEvent;
#endif
#ifndef __YIMP_XUTIL__
#ifdef SHAPE
struct XShapeEvent;
struct XTextProperty;
#endif
#endif

class YColor {
public:
    YColor(unsigned short red, unsigned short green, unsigned short blue);
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

class YFont {
public:
    static YFont *getFont(const char *name);
    ~YFont();

    int height() const { return fontAscent + fontDescent; }
    int descent() const { return fontDescent; }
    int ascent() const { return fontAscent; }

    int textWidth(const char *str) const;
    int textWidth(const char *str, int len) const;
private:
#ifdef I18N
    XFontSet font_set;
#endif
    XFontStruct *afont;
    int fontAscent, fontDescent;

    YFont(const char *name);

    void alloc();

    friend class Graphics;//!!!fix
};

class YPixmap {
public:
    YPixmap(int w, int h, bool mask = false);
    YPixmap(Pixmap pixmap, Pixmap mask, int w, int h, bool owned);
    ~YPixmap();

    Pixmap pixmap() const { return fPixmap; }
    Pixmap mask() const { return fMask; }
    unsigned int width() const { return fWidth; }
    unsigned int height() const { return fHeight; }
private:
    Pixmap fPixmap;
    Pixmap fMask;
    unsigned int fWidth, fHeight;
    bool fOwned;
};

class YIcon {
public:
    YIcon(const char *fileName);
    YIcon(YPixmap *small, YPixmap *large);
    ~YIcon();

    YPixmap *large();
    YPixmap *small();

    const char *iconName() const { return fPath; }
    YIcon *next() const { return fNext; }
private:
    char *fPath;
    YPixmap *fSmall;
    YPixmap *fLarge;
    YIcon *fNext;
    bool loadedS;
    bool loadedL;

    bool findIcon(char *base, char **fullPath, int size);
    bool findIcon(char **fullPath, int size);
    YPixmap *loadIcon(int size);
};

class Graphics {
public:
    Graphics(YWindow *window);
    Graphics(YPixmap *pixmap);
    virtual ~Graphics();

    void copyArea(int x, int y, int width, int height, int dx, int dy);
    void copyPixmap(YPixmap *pixmap, int x, int y, int width, int height, int dx, int dy);
    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawRect(int x, int y, int width, int height);
    void drawArc(int x, int y, int width, int height, int a1, int a2);
    void drawChars(const char *data, int offset, int len, int x, int y);
    void drawCharUnderline(int x, int y, const char *str, int charPos);
    void drawPixmap(YPixmap *pix, int x, int y);
    void drawClippedPixmap(Pixmap pix, Pixmap clip,
                           int x, int y, int w, int h, int toX, int toY);
    void fillRect(int x, int y, int width, int height);
    void fillPolygon(XPoint *points, int n, int shape, int mode);
    void fillArc(int x, int y, int width, int height, int a1, int a2);
    void setColor(YColor *aColor);
    void setFont(YFont *aFont);
    void setPenStyle(bool dotLine = false); ///!!!hack

    void draw3DRect(int x, int y, int w, int h, bool raised);
    void drawBorderW(int x, int y, int w, int h, bool raised);
    void drawBorderM(int x, int y, int w, int h, bool raised);
    void drawBorderG(int x, int y, int w, int h, bool raised);
    void drawCenteredPixmap(int x, int y, int w, int h, YPixmap *pixmap);
    void drawOutline(int l, int t, int r, int b, int iw, int ih);
    void repHorz(YPixmap *pixmap, int x, int y, int w);
    void repVert(YPixmap *pixmap, int x, int y, int h);
    void fillPixmap(YPixmap *pixmap, int x, int y, int w, int h);

    void drawArrow(int direction, int style, int x, int y, int size);
    void drawCharsEllipsis(const char *data, int len, int x, int y, int maxWidth);

    YColor *getColor() const { return color; }
    YFont *getFont() const { return font; }
    GC handle() const { return gc; }
private:
    Display *display;
    Drawable drawable;
    GC gc;

    YColor *color;
    YFont *font;
};

extern Colormap defaultColormap; //!!!???

#endif
