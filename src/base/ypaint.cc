/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#pragma implementation
#include "config.h"

#include "base.h"
#include "yxlib.h"
#include "yconfig.h"
#include "ycstring.h"
#include "yrect.h"

#include "yapp.h"
#include "ywindow.h"
#include "sysdep.h"
#include "default.h"
#include "ypaint.h"
//#include "prefs.h"
#ifdef CONFIG_I18N
static bool multiByte = true;
#endif

#include <string.h>

#ifdef CONFIG_XFREETYPE
#include <X11/Xft/Xft.h>
#define INIT_XFREETYPE(Member, Value) , Member(Value)
#else
#define INIT_XFREETYPE(Member, Value)
#endif

extern Colormap defaultColormap;

YColor::YColor(unsigned short red, unsigned short green, unsigned short blue) {
    fDarker = fBrighter = 0;
    fRed = red;
    fGreen = green;
    fBlue = blue;
    fPixel = 0xFFFFFFFF;
#ifdef CONFIG_XFREETYPE
    xftColor = 0;
#endif

    //alloc();
}

YColor::YColor(const char *clr) {
    XColor color;
    fDarker = fBrighter = 0;

    XParseColor(app->display(),
                defaultColormap,
                clr ? clr : "rgb:00/00/00",
                &color);
    fRed = color.red;
    fGreen = color.green;
    fBlue = color.blue;
    fPixel = 0xFFFFFFFF;
#ifdef CONFIG_XFREETYPE
    xftColor = 0;
#endif
    //alloc();
}

YColor::~YColor() {
#if 0 // display can be 0
    if (NULL != xftColor) {
        XftColorFree (app->display(),
                      DefaultVisual(app->display(), DefaultScreen (app->display())),
                      DefaultColormap(app->display(), DefaultScreen (app->display())),
                      xftColor);
	delete xftColor;
    }
#endif
}

void YColor::alloc() {
    XColor color;

    color.red = fRed;
    color.green = fGreen;
    color.blue = fBlue;
    color.flags = DoRed | DoGreen | DoBlue;

    if (XAllocColor(app->display(),
                    defaultColormap,
                    &color) == 0)
    {
        int j, ncells;
        double d = 65536 * 65536 * 65536 * 24;
        XColor clr;
        unsigned long pix;
        long d_red, d_green, d_blue;
        double long u_red, u_green, u_blue;

        pix = 0xFFFFFFFF;
        ncells = DisplayCells(app->display(), DefaultScreen(app->display()));
        for (j = 0; j < ncells; j++) {
            clr.pixel = j;
            XQueryColor(app->display(), defaultColormap, &clr);

            d_red   = color.red   - clr.red;
            d_green = color.green - clr.green;
            d_blue  = color.blue  - clr.blue;

            u_red   = 3UL * (d_red   * d_red);
            u_green = 4UL * (d_green * d_green);
            u_blue  = 2UL * (d_blue  * d_blue);

            double d1 = u_red + u_blue + u_green;

            if (pix == 0xFFFFFFFF || d1 < d) {
                pix = j;
                d = d1;
            }
        }
        if (pix != 0xFFFFFFFF) {
            clr.pixel = pix;
            XQueryColor(app->display(), defaultColormap, &clr);
            /*DBG(("color=%04X:%04X:%04X, match=%04X:%04X:%04X\n",
                   color.red, color.blue, color.green,
                   clr.red, clr.blue, clr.green));*/
            color = clr;
        }
        if (XAllocColor(app->display(), defaultColormap, &color) == 0) {
            if (color.red + color.green + color.blue >= 32768)
                color.pixel = WhitePixel(app->display(),
                                         DefaultScreen(app->display()));
            else
                color.pixel = BlackPixel(app->display(),
                                         DefaultScreen(app->display()));
        }
    }
    fRed = color.red;
    fGreen = color.green;
    fBlue = color.blue;
    fPixel = color.pixel;
}

void YColor::allocXft() {
    if (0 == xftColor) {
	xftColor = new XftColor;

	XRenderColor color;
	color.red = fRed;
	color.green = fGreen;
	color.blue = fBlue;
	color.alpha = 0xffff;

	XftColorAllocValue(app->display(),
                           DefaultVisual(app->display(), DefaultScreen (app->display())),
                           DefaultColormap(app->display(), DefaultScreen (app->display())),
                           &color, xftColor);
    }
}

YColor *YColor::darker() { // !!! fix
    if (fDarker == 0) {
        unsigned short red, blue, green;

        red = ((unsigned int)fRed) * 2 / 3;
        blue = ((unsigned int)fBlue) * 2 / 3;
        green = ((unsigned int)fGreen) * 2 / 3;
        fDarker = new YColor(red, green, blue);
    }
    return fDarker;
}

YColor *YColor::brighter() { // !!! fix
    if (fBrighter == 0) {
        unsigned short red, blue, green;

#define maxx(x) (((x) <= 0xFFFF) ? (x) : 0xFFFF)

        red = maxx(((unsigned int)fRed) * 4 / 3);
        blue = maxx(((unsigned int)fBlue) * 4 / 3);
        green = maxx(((unsigned int)fGreen) * 4 / 3);

        fBrighter = new YColor(red, green, blue);
    }
    return fBrighter;
}

#ifdef CONFIG_XFREETYPE
class YXftFont {
public:
#if 0
#ifdef CONFIG_I18N
    typedef class YUnicodeString string_t;
    typedef XftChar32 char_t;
#else
    typedef class YLocaleString string_t;
    typedef XftChar8 char_t;
#endif
#endif

    YXftFont(char const * name);
    virtual ~YXftFont();

    virtual operator bool() const { return (fFontCount > 0); }
    virtual unsigned descent() const { return fDescent; }
    virtual unsigned ascent() const { return fAscent; }
    //virtual unsigned textWidth(char const * str, int len) const;

    virtual unsigned textWidth(const char *str, int len) const; // const?
    virtual void drawGlyphs(class Graphics & graphics, int x, int y, 
    			    char const * str, int len);

    int height() { return fDescent + fAscent; }
private:
    struct TextPart {
	XftFont * font;
	size_t length;
	unsigned width;
    };

    TextPart * partitions(const char *str, size_t len, size_t nparts = 0) const;

    unsigned fFontCount, fAscent, fDescent;
    XftFont ** fFonts;
};
#endif

#ifdef CONFIG_XFREETYPE

char const * strnxt(const char * str, const char * delim) {
    str+= strcspn(str, delim);
    str+= strspn(str, delim);
    return str;

}

unsigned strTokens(const char * str, const char * delim) {
    unsigned count = 0;

    if (str)
	while (*str) { 
	    str = strnxt(str, delim);
	    ++count;
        }

    return count;	     
}

char *newstr(char const *str, char const *delim) {
    return (str != NULL ? __newstr(str, strcspn(str, delim)) : NULL);
}

template <class T>
inline T min(T a, T b) {
    return (a < b ? a : b);
}

template <class T>
inline T max(T a, T b) {
    return (a > b ? a : b);
}

YXftFont::YXftFont(const char *name):
    fFontCount(strTokens(name, ",")), fAscent(0), fDescent(0)
{
    XftFont ** fptr(fFonts = new XftFont* [fFontCount]);

    for (char const *s(name); '\0' != *s; s = strnxt(s, ",")) {
	XftFont *& font(*fptr);

	char * xlfd(newstr(s + strspn(s, " \t\r\n"), ","));
	char * endptr(xlfd + strlen(xlfd) - 1);
	while (endptr > xlfd && strchr(" \t\r\n", *endptr)) --endptr;
	endptr[1] = '\0';

        font = XftFontOpenXlfd(app->display(),
                               DefaultScreen(app->display()),
                               xlfd);

	if (NULL != font) {
	    fAscent = max(fAscent, (unsigned) max(0, font->ascent));
	    fDescent = max(fDescent, (unsigned) max(0, font->descent));
	    ++fptr;
	} else {
	    warn("Could not load font \"%s\".", xlfd);
	    --fFontCount;
	}

	delete[] xlfd;
    }

    if (0 == fFontCount) {
        XftFont * sans(
            XftFontOpen(
                app->display(),
                DefaultScreen(app->display()),
                XFT_FAMILY, XftTypeString, "sans",
                XFT_WEIGHT, XftTypeInteger, XFT_WEIGHT_MEDIUM,
                XFT_SLANT, XftTypeInteger, XFT_SLANT_ROMAN,
                XFT_PIXEL_SIZE, XftTypeInteger, 10, 0));

        if (NULL != sans) {
	    delete[] fFonts;

	    fFontCount = 1;
	    fFonts = new XftFont* [fFontCount];
	    fFonts[0] = sans;

	    fAscent = sans->ascent;
	    fDescent = sans->descent;
	} else
	    warn("Loading of fallback font \"%s\" failed.", "sans");
    }
}

YXftFont::~YXftFont() {
    for (unsigned n(0); n < fFontCount; ++n)
	XftFontClose(app->display(), fFonts[n]);

    delete[] fFonts;
}

unsigned YXftFont::textWidth(const char *str, int len) const {
    // !!! fix
    //const char *str = text;// = (char *)text->c_str();
    //size_t len(text->length());

    TextPart * partitions(partitions(str, len));
    unsigned width(0);

    for (TextPart * p(partitions); p && p->length; ++p)
        width += p->width;

    delete[] partitions;
    return width;
}

//unsigned YXftFont::textWidth(char const * str, int len) const {
//    return textWidth(string_t(str, len));
//}

void YXftFont::drawGlyphs(Graphics & graphics, int x, int y,
                          char const * str, int len)
{
    CStaticStr xtext(str, len);
    if (0 == ((const CStr *)xtext)->length()) return;

    //int const y0(y - ascent());
    //int const gcFn = GXcopy; //(graphics.function());

    const char * xstr = (const char *)(((const CStr *)xtext)->c_str());
    size_t xlen = ((const CStr *)xtext)->length();

    TextPart * partitions(partitions(xstr, xlen));
    unsigned int w = 0;
    //unsigned const h(height());

    for (TextPart * p(partitions); p && p->length; ++p)
        w += p->width;

#if 0
    //YWindowAttributes attributes(graphics.drawable());
    //GraphicsCanvas canvas(w, h, attributes.depth());
    //XftGraphics textarea(canvas, attributes.visual(), attributes.colormap());

    switch (gcFn) {
	case GXxor:
	    textarea.drawRect(*YColor::black, 0, 0, w, h);
	    break;

	case GXcopy:
	    canvas.copyDrawable(graphics.drawable(), x, y0, w, h, 0, 0);
	    break;
    }
#endif

    int xpos(0);
    for (TextPart * p(partitions); p && p->length; ++p) {
        if (p->font) {
#if 0
            textarea.drawString(*graphics.color(), p->font,
                                xpos, ascent(), xstr, p->length);
#endif

            XftDrawString8(graphics.xftDraw,
                           (XftColor *)graphics.color->getXftColor(),
                           p->font,
                           x + xpos, y, (XftChar8 *)xstr, p->length);
        }

	xstr+= p->length;
	xpos+= p->width;
    }

    delete[] partitions;

#if 0
    graphics.copyDrawable(canvas.drawable(), 0, 0, w, h, x, y0);
#endif
}

YXftFont::TextPart * YXftFont::partitions(const char * str, size_t len,
					  size_t nparts = 0) const {
    XGlyphInfo extends;
    XftFont ** lFont(fFonts + fFontCount);
    XftFont ** font(NULL);
    const char *c = str;

    for (const char * endptr(str + len); c < endptr; ++c) {
	XftFont ** probe(fFonts);

	while (probe < lFont && !XftGlyphExists(app->display(), *probe, *c))
	    ++probe;

	if (probe != font) {
	    if (NULL != font) {
		TextPart * parts(partitions(c, len - (c - str), nparts + 1));
		parts[nparts].length = (c - str);

                if (font < lFont) {

                    XftTextExtents8(app->display(),
                                   *font,
                                   (XftChar8 *)str, (c - str),
                                   &extends);

		    //XftGraphics::textExtents(*font, str, (c - str), extends);
		    parts[nparts].font = *font;
		    parts[nparts].width = extends.xOff;
		} else {
		    parts[nparts].font = NULL;
		    parts[nparts].width = 0;
		}

		return parts;
	    } else
		font = probe;
	}
    }

    TextPart * parts = new TextPart[nparts + 2];
    parts[nparts + 1].font =  NULL;
    parts[nparts + 1].width = 0;
    parts[nparts + 1].length = 0;
    parts[nparts].length = (c - str);

    if (NULL != font && font < lFont) {
        XftTextExtents8(app->display(),
                        *font,
                        (XftChar8 *)str, (c - str),
                        &extends);
        //XftGraphics::textExtents(*font, str, (c - str), extends);
	parts[nparts].font = *font;
	parts[nparts].width = extends.xOff;
    } else {
	parts[nparts].font = NULL;
	parts[nparts].width = 0;
    }

    return parts;
}

#endif // CONFIG_XFREETYPE


YFont *YFont::getFont(const char *name) {
    YFont *f = new YFont(name);

    if (f) {
        if (
#ifdef CONFIG_XFREETYPE
            f->xftFont == 0
#else
#ifdef CONFIG_I18N
            multiByte && f->font_set == 0 ||
            !multiByte &&
#else
            f->afont == 0
#endif
#endif
           )
        {
            delete f;
            return 0;
        }
    }
    return f;
}

YFont::YFont(const char *name) {
#ifdef CONFIG_XFREETYPE
    xftFont = new YXftFont(name);
    if (xftFont) {
        fontAscent = xftFont->ascent();
        fontDescent = xftFont->descent();
    }
#else
#ifdef CONFIG_I18N
    if (multiByte) {
        char **missing, *def_str;
        int missing_num;
        char *p;

        fontAscent = fontDescent = 0;
        if ((p = new char[strlen(name) + 3]) == 0) {
            font_set = XCreateFontSet(app->display(), name, &missing, &missing_num, &def_str);
        } else {
            sprintf(p, "%s,*", name);
            font_set = XCreateFontSet(app->display(), p, &missing, &missing_num, &def_str);
            delete [] p;
        }
        if (font_set == 0) {
            fprintf(stderr, "Could not load fontset '%s'.\n", name);
            font_set = XCreateFontSet(app->display(), "*fixed*", &missing, &missing_num, &def_str);
            if (font_set == 0)
                fprintf(stderr, "Fallback to '*fixed*' failed.\n");
        }
        if (font_set) {
            if (missing_num) {
                int i;
                fprintf(stderr, "missing fontset in loading %s\n", name);
                for (i = 0; i < missing_num; i++) fprintf(stderr, "%s\n", missing[i]);
                XFreeStringList(missing);
            }
            XFontSetExtents *extents = XExtentsOfFontSet(font_set);
            if (extents) {
                fontAscent = -extents->max_logical_extent.y;
                fontDescent = extents->max_logical_extent.height - fontAscent;
            }
        }
    }
    {
        afont = XLoadQueryFont(app->display(), name);
        if (afont == 0)  {
            warn("Could not load font '%s'.", name);
            afont = XLoadQueryFont(app->display(), "fixed");
            if (afont == 0)
                warn("Fallback to 'fixed' failed.");
        }

        fontAscent = afont ? afont->max_bounds.ascent : 0;
        fontDescent = afont ? afont->max_bounds.descent : 0;
    }
#endif
#endif
}

YFont::~YFont() {
    if (app == 0 || app->display() == 0)
        return;
#ifdef CONFIG_XFREETYPE
#else
#ifdef CONFIG_I18N
    if (font_set) XFreeFontSet(app->display(), font_set);
#endif
    if (afont) XFreeFont(app->display(), afont);
#endif
}

int YFont::textWidth(const CStr *str) const {
    if (str && str->c_str())
        return __textWidth(str->c_str());
    else
        return 0;
}

int YFont::__textWidth(const char *str) const {
    if (str)
        return __textWidth(str, strlen(str));
    else
        return 0;
}

int YFont::__textWidth(const char *str, int len) const {
#ifdef CONFIG_XFREETYPE
    if (xftFont == 0)
        return 0;
    else
        return xftFont->textWidth(str, len);
#else
#ifdef CONFIG_I18N
    if (multiByte) {
        return font_set ? XmbTextEscapement(font_set, str, len) : 0;
    } else
#endif
    {
        return afont ? XTextWidth(afont, str, len) : 0;
    }
#endif
}

Graphics::Graphics(YWindow *window) {
    XGCValues gcv;

    display = app->display();
    drawable = window->handle();

    gcv.graphics_exposures = False;
    gc = XCreateGC(display, drawable,
                   GCGraphicsExposures, &gcv);
    xftDraw = XftDrawCreate(
        app->display(),
        drawable,
        DefaultVisual(app->display(), DefaultScreen (app->display())),
        DefaultColormap(app->display(), DefaultScreen (app->display())));
}

Graphics::Graphics(YPixmap *pixmap) {
    XGCValues gcv;

    display = app->display();
    drawable = pixmap->pixmap();

    gcv.graphics_exposures = False;
    gc = XCreateGC(display, drawable, GCGraphicsExposures, &gcv);
    xftDraw = XftDrawCreate(
        app->display(),
        drawable,
        DefaultVisual(app->display(), DefaultScreen (app->display())),
        DefaultColormap(app->display(), DefaultScreen (app->display())));
}

Graphics::~Graphics() {
    XftDrawDestroy(xftDraw);
    XFreeGC(display, gc);
}

void Graphics::copyArea(int x, int y, int width, int height, int dx, int dy) {
    XCopyArea(display, drawable, drawable, gc,
              x, y, width, height, dx, dy);
}

void Graphics::copyPixmap(YPixmap *pixmap, int x, int y, int width, int height, int dx, int dy) {
    XCopyArea(display, pixmap->pixmap(), drawable, gc,
              x, y, width, height, dx, dy);
}

void Graphics::drawPoint(int x, int y) {
    XDrawPoint(display, drawable, gc, x, y);
}

void Graphics::drawLine(int x1, int y1, int x2, int y2) {
    XDrawLine(display, drawable, gc, x1, y1, x2, y2);
}

void Graphics::drawRect(int x, int y, int width, int height) {
    XDrawRectangle(display, drawable, gc, x, y, width, height);
}

void Graphics::drawRect(const YRect &er) {
    XDrawRectangle(display, drawable, gc, er.x(), er.y(), er.width(), er.height());
}

void Graphics::drawArc(int x, int y, int width, int height, int a1, int a2) {
    XDrawArc(display, drawable, gc, x, y, width, height, a1, a2);
}

void Graphics::__drawChars(const char *data, int offset, int len, int x, int y) {
#ifdef CONFIG_XFREETYPE
    if (font && font->xftFont)
        font->xftFont->drawGlyphs(*this, x, y, data + offset, len);

#else
#ifdef CONFIG_I18N
    if (multiByte) {
        if (font && font->font_set)
            XmbDrawString(display, drawable, font->font_set, gc,
                          x, y, data + offset, len);
    } else
#endif
    {
        XDrawString(display, drawable, gc, x, y, data + offset, len);
    }
#endif
}

void Graphics::__drawCharsEllipsis(const char *str, int len, int x, int y, int maxWidth) {
    int w = font ? font->__textWidth(str, len) : 0;

    if (font == 0 || w <= maxWidth) {
        __drawChars(str, 0, len, x, y);
    } else {
        int l = 0;
        int maxW = maxWidth - font->__textWidth("...", 3);
        int w = 0;
        int wc;

        if (maxW < 0) {
            l = len;
        } else {

            while (w < maxW && l < len) {
                int nc = 1;
#ifdef CONFIG_I18N
                if (multiByte) {
                    nc = mblen(str + l, len - l);
                    wc = font->__textWidth(str + l, nc);
                } else
#endif
                {
                    wc = font->__textWidth(str + l, 1);
                }
                if (w + wc < maxW) {
                    l += nc;
                    w += wc;
                } else
                    break;
            }
        }
        __drawChars(str, 0, l, x, y);
        if (l < len)
            __drawChars("...", 0, 3, x + w, y);
    }
}

void Graphics::__drawCharUnderline(int x, int y, const char *str, int charPos) {
    int left = font ? font->__textWidth(str, charPos) : 0;
    int right = font ? font->__textWidth(str, charPos + 1) - 1 : 0;

    drawLine(x + left, y + 2, x + right, y + 2);
}

void Graphics::fillRect(int x, int y, int width, int height) {
    XFillRectangle(display, drawable, gc,
                   x, y, width, height);
}

void Graphics::fillRect(const YRect &er) {
    XFillRectangle(display, drawable, gc,
                   er.x(), er.y(), er.width(), er.height());
}

void Graphics::fillPolygon(XPoint *points, int n, bool relative) {
    XFillPolygon(display, drawable, gc, points, n, Complex,
                 relative ? CoordModePrevious : CoordModeOrigin);
}

void Graphics::fillArc(int x, int y, int width, int height, int a1, int a2) {
    XFillArc(display, drawable, gc, x, y, width, height, a1, a2);
}

void Graphics::setColor(YColor *aColor) {
    color = aColor;
    XSetForeground(display, gc, color->pixel());
}

void Graphics::setColor(YColorPrefProperty *aColor) {
    setColor(aColor->getColor());
}

void Graphics::setColor(YColorPrefProperty &aColor) {
    setColor(aColor.getColor());
}

void Graphics::setFont(YFont *aFont) {
#ifdef CONFIG_XFREETYPE
    font = aFont;
#else
    font = aFont;
#ifdef CONFIG_I18N
    if (!multiByte)
#endif
        if (font && font->afont) XSetFont(display, gc, font->afont->fid);
#endif
}

void Graphics::setFont(YFontPrefProperty *aFont) {
    setFont(aFont->getFont());
}

void Graphics::setFont(YFontPrefProperty &aFont) {
    setFont(aFont.getFont());
}

void Graphics::setPenStyle(PenStyle penStyle) {
    XGCValues gcv;

    if (penStyle == psDotted) {
        char c = 1;
        gcv.line_style = LineOnOffDash;
        XSetDashes(display, gc, 0, &c, 1);
    } else {
        gcv.line_style = LineSolid;
    }

    XChangeGC(display, gc, GCLineStyle, &gcv);
}

void Graphics::drawPixmap(YPixmap *pix, int x, int y) {
    if (pix->mask())
        drawClippedPixmap(pix->pixmap(),
                          pix->mask(),
                          0, 0, pix->width(), pix->height(), x, y);
    else
        XCopyArea(display, pix->pixmap(), drawable, gc,
                  0, 0, pix->width(), pix->height(), x, y);
}

void Graphics::drawClippedPixmap(XPixmap pix, XPixmap clip,
                                 int x, int y, int w, int h, int toX, int toY)
{
    static GC clipPixmapGC = None;
    XGCValues gcv;

    if (clipPixmapGC == None) {
        gcv.graphics_exposures = False;
        clipPixmapGC = XCreateGC(app->display(), desktop->handle(),
                                 GCGraphicsExposures,
                                 &gcv);
    }

    gcv.clip_mask = clip;
    gcv.clip_x_origin = toX;
    gcv.clip_y_origin = toY;
    XChangeGC(display, clipPixmapGC,
              GCClipMask | GCClipXOrigin | GCClipYOrigin, &gcv);
    XCopyArea(display, pix, drawable, clipPixmapGC,
              x, y, w, h, toX, toY);
    gcv.clip_mask = None;
    XChangeGC(display, clipPixmapGC, GCClipMask, &gcv);
}

void Graphics::draw3DRect(int x, int y, int w, int h, bool raised) {
    YColor *back = getColor();
    YColor *bright = back->brighter();
    YColor *dark = back->darker();
    YColor *t = raised ? bright : dark;
    YColor *b = raised ? dark : bright;

    setColor(t);
    drawLine(x, y, x + w, y);
    drawLine(x, y, x, y + h);
    setColor(b);
    drawLine(x, y + h, x + w, y + h);
    drawLine(x + w, y, x + w, y + h);
    setColor(back);
    drawPoint(x + w, y);
    drawPoint(x, y + h);
}

void Graphics::drawBorderW(int x, int y, int w, int h, bool raised) {
    YColor *back = getColor();
    YColor *bright = back->brighter();
    YColor *dark = back->darker();

    if (raised) {
        setColor(bright);
        drawLine(x, y, x + w - 1, y);
        drawLine(x, y, x, y + h - 1);
        setColor(YColor::black);
        drawLine(x, y + h, x + w, y + h);
        drawLine(x + w, y, x + w, y + h);
        setColor(dark);
        drawLine(x + 1, y + h - 1, x + w - 1, y + h - 1);
        drawLine(x + w - 1, y + 1, x + w - 1, y + h - 1);
    } else {
        setColor(bright);
        drawLine(x + 1, y + h, x + w, y + h);
        drawLine(x + w, y + 1, x + w, y + h);
        setColor(YColor::black);
        drawLine(x, y, x + w, y);
        drawLine(x, y, x, y + h);
        setColor(dark);
        drawLine(x + 1, y + 1, x + w - 1, y + 1);
        drawLine(x + 1, y + 1, x + 1, y + h - 1);
    }
    setColor(back);
}

// doesn't move... needs two pixels on all sides for up and down
// position.
void Graphics::drawBorderM(int x, int y, int w, int h, bool raised) {
    YColor *back = getColor();
    YColor *bright = back->brighter();
    YColor *dark = back->darker();

    if (raised) {
        setColor(bright);
        drawLine(x + 1, y + 1, x + w, y + 1);
        drawLine(x + 1, y + 1, x + 1, y + h);
        drawLine(x + 1, y + h, x + w + 1, y + h);
        drawLine(x + w, y + 1, x + w, y + h);

        setColor(dark);
        drawLine(x, y, x + w - 1, y);
        drawLine(x, y, x, y + h);
        drawLine(x, y + h - 1, x + w - 1, y + h - 1);
        drawLine(x + w - 1, y, x + w - 1, y + h - 1);

        ///  how to get the color of the taskbar??
        setColor(back);

        drawPoint(x, y + h);
        drawPoint(x + w, y);
    } else {
        setColor(dark);
        drawLine(x, y, x + w - 1, y);
        drawLine(x, y, x, y + h - 1);

        setColor(bright);
        drawLine(x + 1, y + h, x + w, y + h);
        drawLine(x + w, y + 1, x + w, y + h);

        setColor(back);
        drawRect(x + 1, y + 1, x + w - 2, y + h - 2);
        //drawLine(x + 1, y + 1, x + w - 1, y + 1);
        //drawLine(x + 1, y + 1, x + 1, y + h - 1);
        //drawLine(x, y + h - 1, x + w - 1, y + h - 1);
        //drawLine(x + w - 1, y, x + w - 1, y + h - 1);

        ///  how to get the color of the taskbar??
        drawPoint(x, y + h);
        drawPoint(x + w, y);
    }
}

void Graphics::drawBorderG(int x, int y, int w, int h, bool raised) {
    YColor *back = getColor();
    YColor *bright = back->brighter();
    YColor *dark = back->darker();

    if (raised) {
        setColor(bright);
        drawLine(x, y, x + w - 1, y);
        drawLine(x, y, x, y + h - 1);
        setColor(dark);
        drawLine(x, y + h, x + w, y + h);
        drawLine(x + w, y, x + w, y + h);
        setColor(YColor::black);
        drawLine(x + 1, y + h - 1, x + w - 1, y + h - 1);
        drawLine(x + w - 1, y + 1, x + w - 1, y + h - 1);
    } else {
        setColor(bright);
        drawLine(x + 1, y + h, x + w, y + h);
        drawLine(x + w, y + 1, x + w, y + h);
        setColor(YColor::black);
        drawLine(x, y, x + w, y);
        drawLine(x, y, x, y + h);
        setColor(dark);
        drawLine(x + 1, y + 1, x + w - 1, y + 1);
        drawLine(x + 1, y + 1, x + 1, y + h - 1);
    }
    setColor(back);
}

void Graphics::drawCenteredPixmap(int x, int y, int w, int h, YPixmap *pixmap) {
    int r = x + w;
    int b = y + h;
    int pw = pixmap->width();
    int ph = pixmap->height();

    drawOutline(x, y, r, b, pw, ph);
    drawPixmap(pixmap, x + w / 2 - pw / 2, y + h / 2 - ph / 2);
}

void Graphics::drawOutline(int l, int t, int r, int b, int iw, int ih) {
    if (l + iw >= r && t + ih >= b)
        return;

    int li = (l + r) / 2 - iw / 2;
    int ri = (l + r) / 2 + iw / 2;
    int ti = (t + b) / 2 - ih / 2;
    int bi = (t + b) / 2 + ih / 2;

    if (l < li)
        fillRect(l, t, li - l, b - t);

    if (ri < r)
        fillRect(ri, t, r - ri, b - t);

    if (t < ti)
        if (li < ri)
            fillRect(li, t, ri - li, ti - t);

    if (bi < b)
        if (li < ri)
            fillRect(li, bi, ri - li, b - bi);
}

void Graphics::repHorz(YPixmap *pixmap, int x, int y, int w) {
    int pw = pixmap->width();
    int ph = pixmap->height();

    while (w > 0) {
        XCopyArea(display, pixmap->pixmap(), drawable, gc,
                  0, 0, (w > pw ? pw : w), ph, x, y);
        x += pw;
        w -= pw;
    }
}

void Graphics::repVert(YPixmap *pixmap, int x, int y, int h) {
    int pw = pixmap->width();
    int ph = pixmap->height();

    while (h > 0) {
        XCopyArea(display, pixmap->pixmap(), drawable, gc,
                  0, 0, pw, (h > ph ? ph : h), x, y);
        y += ph;
        h -= ph;
    }
}

void Graphics::fillPixmap(YPixmap *pixmap, int x, int y, int w, int h) {
    int pw = pixmap->width();
    int ph = pixmap->height();
    int xx, yy, hh, ww;

    xx = x;
    ww = w;
    while (ww > 0) {
        yy = y;
        hh = h;
        while (hh > 0) {
            XCopyArea(display, pixmap->pixmap(), drawable, gc,
                      0, 0, (ww > pw ? pw : ww), (hh > ph ? ph : hh), xx, yy);

            yy += ph;
            hh -= ph;
        }
        xx += pw;
        ww -= pw;
    }
}

void Graphics::drawArrow(int direction, int style, int x, int y, int size) {
    XPoint points[3];

    switch (direction) {
    case 0:
        points[0].x = x;
        points[0].y = y;
        points[1].x = x + ((style == 0) ? size / 2 : size);
        points[1].y = y + size / 2;
        points[2].x = x;
        points[2].y = y + size;
        break;
    default:
        return;
    }
    switch (style) {
    case 0:
        fillPolygon(points, 3, false);
        break;
    case 1:
    case -1:
        {
            YColor *back = getColor();
            YColor *c1 = (style == 1) ? YColor::black /*back->darker()*/ : back->brighter();
            YColor *c2 = (style == 1) ? back->brighter() : YColor::black; // back->darker();

            setColor(c1);
            drawLine(points[0].x, points[0].y, points[1].x, points[1].y);
            drawLine(points[0].x, points[0].y, points[2].x, points[2].y);
            setColor(c2);
            drawLine(points[2].x, points[2].y, points[1].x, points[1].y);
        }
        break;
    }
}

void Graphics::drawText(const YRect &rect, const CStr *text, int flags, int underlinePos) {
    if (font == 0)
        return;
    int x = 0;
    int y = 0;

    int vert = flags & DrawText_Vertical;
    int horz = flags & DrawText_Horizontal;

    if (horz == DrawText_HCenter)
        x = (rect.width() - font->textWidth(text) + 1) / 2;
    else if (horz == DrawText_HRight)
        x = rect.width() - font->textWidth(text);
    else // left
        x = 0;

    if (vert == DrawText_VCenter)
        y = (rect.height() - font->height() + 1) / 2 + font->ascent();
    else if (vert == DrawText_VBottom)
        y = (rect.height() - font->descent());
    else
        y = font->ascent();

    if (x < 0)
        x = 0;
    if (y < font->ascent())
        y = font->ascent();

    __drawChars(text->c_str(), 0, text->length(), rect.left() + x, rect.top() + y);

    if (underlinePos != -1) {
        int left = font->__textWidth(text->c_str(), underlinePos);
        int right = font->__textWidth(text->c_str(), underlinePos + 1) - 1;

        drawLine(x + left, y + 2, x + right, y + 2);
    }
}
