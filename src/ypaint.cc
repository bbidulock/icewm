/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include "ypixbuf.h"
#include "ypaint.h"

#include "yapp.h"
#include "sysdep.h"
#include "ystring.h"
#include "prefs.h"

#include "intl.h"

/******************************************************************************/
/******************************************************************************/

class XftGraphics {
public:
#ifdef CONFIG_I18N
    typedef XftChar32 char_t;

    #define XftDrawString XftDrawString32
    #define XftTextExtents XftTextExtents32
#else    
    typedef XftChar8 char_t;

    #define XftTextExtents XftTextExtents8
    #define XftDrawString XftDrawString8
#endif

    XftGraphics(Graphics const & graphics, Visual * visual, Colormap colormap):
	fDraw(XftDrawCreate(graphics.display(), graphics.drawable(),
			    visual, colormap)) {}
			    
    ~XftGraphics() {
	if (fDraw) XftDrawDestroy(fDraw);
    }

    void drawRect(XftColor * color, int x, int y, unsigned w, unsigned h) {    
	XftDrawRect(fDraw, color, x, y, w, h);
    }

    void drawString(XftColor * color, XftFont * font, int x, int y,
    		    char_t * str, size_t len) {
	XftDrawString(fDraw, color, font, x, y, str, len);
    }

    static void textExtents(XftFont * font, char_t * str, size_t len,
			    XGlyphInfo & extends) {
	XftTextExtents(app->display (), font, str, len, &extends);
    }

    XftDraw * handle() const { return fDraw; }

private:
    XftDraw * fDraw;
};

/******************************************************************************/
/******************************************************************************/

YColor::YColor(unsigned short red, unsigned short green, unsigned short blue):
    fRed(red), fGreen(green), fBlue(blue),
    fDarker(NULL), fBrighter(NULL)
    INIT_XFREETYPE(xftColor, NULL) {
    alloc();
}

YColor::YColor(unsigned long pixel):
    fDarker(NULL), fBrighter(NULL)
    INIT_XFREETYPE(xftColor, NULL) {

    XColor color;
    color.pixel = pixel;
    XQueryColor(app->display(), app->colormap(), &color);

    fRed = color.red;
    fGreen = color.green;
    fBlue = color.blue;

    alloc();
}

YColor::YColor(const char *clr):
    fDarker(NULL), fBrighter(NULL)
    INIT_XFREETYPE(xftColor, NULL) {

    XColor color;
    XParseColor(app->display(), app->colormap(),
                clr ? clr : "rgb:00/00/00", &color);

    fRed = color.red;
    fGreen = color.green;
    fBlue = color.blue;

    alloc();
}

#ifdef CONFIG_XFREETYPE
YColor::~YColor() {
    if (NULL != xftColor) {
	XftColorFree (app->display (), app->visual (),
		      app->colormap(), xftColor);
	delete xftColor;
    }
}

YColor::operator XftColor * () {
    if (NULL == xftColor) {
	xftColor = new XftColor;

	XRenderColor color;
	color.red = fRed;
	color.green = fGreen;
	color.blue = fBlue;
	color.alpha = 0xffff;

	XftColorAllocValue(app->display(), app->visual(),
			   app->colormap(), &color, xftColor);
    }

    return xftColor;
}
#endif

void YColor::alloc() {
    XColor color;

    color.red = fRed;
    color.green = fGreen;
    color.blue = fBlue;
    color.flags = DoRed | DoGreen | DoBlue;

    if (Success == XAllocColor(app->display(), app->colormap(), &color))
    {
        int j, ncells;
        double long d = 65536. * 65536. * 65536. * 24;
        XColor clr;
        unsigned long pix;
        long d_red, d_green, d_blue;
        double long u_red, u_green, u_blue;

        pix = 0xFFFFFFFF;
        ncells = DisplayCells(app->display(), DefaultScreen(app->display()));
        for (j = 0; j < ncells; j++) {
            clr.pixel = j;
            XQueryColor(app->display(), app->colormap(), &clr);

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
            XQueryColor(app->display(), app->colormap(), &clr);
            /*DBG(("color=%04X:%04X:%04X, match=%04X:%04X:%04X\n",
                   color.red, color.blue, color.green,
                   clr.red, clr.blue, clr.green));*/
            color = clr;
        }
        if (XAllocColor(app->display(), app->colormap(), &color) == 0)
            if (color.red + color.green + color.blue >= 32768)
                color.pixel = WhitePixel(app->display(),
                                         DefaultScreen(app->display()));
            else
                color.pixel = BlackPixel(app->display(),
                                         DefaultScreen(app->display()));
    }
    fRed = color.red;
    fGreen = color.green;
    fBlue = color.blue;
    fPixel = color.pixel;
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

        red = min (((unsigned) fRed) * 4 / 3, 0xFFFFU);
        blue = min (((unsigned) fBlue) * 4 / 3, 0xFFFFU);
        green = min (((unsigned) fGreen) * 4 / 3, 0xFFFFU);

        fBrighter = new YColor(red, green, blue);
    }
    return fBrighter;
}

/******************************************************************************/
/******************************************************************************/

YFont * YFont::getFont(char const * name, bool antialias) {
    YFont * font;

#ifdef CONFIG_XFREETYPE
    if (antialias && haveXft && NULL != (font = new YXftFont(name))) {
	if (*font) return font;
	else delete font;
    }
#endif

#ifdef CONFIG_I18N
    if (multiByte && NULL != (font = new YFontSet(name))) {
	if (*font) return font;
	else delete font;
    }
#endif

    if (NULL != (font = new YCoreFont(name))) {
	if (*font) return font;
	else delete font;
    }
    
    return NULL;
}

unsigned YFont::textWidth(char const * str) const {
    return textWidth(str, strlen(str));
}

unsigned YFont::multilineTabPos(const char *str) const {
    unsigned tabPos(0);

    for (const char * end(strchr(str, '\n')); end;
	 str = end + 1, end = strchr(str, '\n')) {
	int const len(end - str);
	const char * tab((const char *) memchr(str, '\t', len));

	if (tab) tabPos = max(tabPos, textWidth(str, tab - str));
    }
    
    const char * tab(strchr(str, '\t'));
    if (tab) tabPos = max(tabPos, textWidth(str, tab - str));
    
    return (tabPos ? tabPos + 3 * textWidth(" ", 1) : 0);
}

YDimension YFont::multilineAlloc(const char *str) const {
    unsigned const tabPos(multilineTabPos(str));
    YDimension alloc(0, ascent());

    for (const char * end(strchr(str, '\n')); end;
	 str = end + 1, end = strchr(str, '\n')) {
	int const len(end - str);
	const char * tab((const char *) memchr(str, '\t', len));

	alloc.w = max(tab ? tabPos + textWidth(tab + 1, end - tab - 1)
			  : textWidth(str, len), alloc.w);
	alloc.h+= height();
    }

    const char * tab(strchr(str, '\t'));
    alloc.w = max(alloc.w, tab ? tabPos + textWidth(tab + 1) : textWidth(str));

    return alloc;
}

char * YFont::getNameElement(const char *pattern, unsigned const element) {
    unsigned h(0);
    const char *p(pattern);
  
    while (*p && (*p != '-' || element != ++h)) ++p;
    return (element == h ? newstr(p + 1, "-") : newstr("*"));
}

/******************************************************************************/

YCoreFont::YCoreFont(char const * name) {
    if (NULL == (fFont = XLoadQueryFont(app->display(), name))) {
	warn(_("Could not load font \"%s\"."), name);

        if (NULL == (fFont = XLoadQueryFont(app->display(), "fixed")))
	    warn(_("Loading of fallback font \"%s\" failed."), "fixed");
    }
}

YCoreFont::~YCoreFont() {
    if (NULL != fFont) XFreeFont(app->display(), fFont);
}

unsigned YCoreFont::textWidth(const char *str, int len) const {
    return XTextWidth(fFont, str, len);
}

void YCoreFont::drawGlyphs(Graphics & graphics, int x, int y, 
    			   char const * str, int len) {
    XSetFont(app->display(), graphics.handle(), fFont->fid);
    XDrawString(app->display(), graphics.drawable(), graphics.handle(),
    		x, y, str, len);
}

/******************************************************************************/

#ifdef CONFIG_I18N

YFontSet::YFontSet(char const * name):
    fFontSet(None), fAscent(0), fDescent(0) {
    int nMissing;
    char **missing, *defString;

    fFontSet = getFontSetWithGuess(name, &missing, &nMissing, &defString);

    if (None == fFontSet) {
	warn(_("Could not load fontset \"%s\"."), name);
	if (nMissing) XFreeStringList(missing);

	fFontSet = XCreateFontSet(app->display(), "fixed",
				  &missing, &nMissing, &defString);

	if (None == fFontSet)
	    warn(_("Loading of fallback font \"%s\" failed."), "fixed");
    }

    if (fFontSet) {
	if (nMissing) {
	    warn(_("Missing codesets for fontset \"%s\":"), name);
	    for (int n(0); n < nMissing; ++n)
		fprintf(stderr, "  %s\n", missing[n]);

	    XFreeStringList(missing);
	}
	
	XFontSetExtents * extents(XExtentsOfFontSet(fFontSet));

	if (NULL != extents) {
	    fAscent = -extents->max_logical_extent.y;
            fDescent = extents->max_logical_extent.height - fAscent;
	}
    }
}

YFontSet::~YFontSet() {
    if (NULL != fFontSet) XFreeFontSet(app->display(), fFontSet);
}

unsigned YFontSet::textWidth(const char *str, int len) const {
    return XmbTextEscapement(fFontSet, str, len);
}

void YFontSet::drawGlyphs(Graphics & graphics, int x, int y, 
    			  char const * str, int len) {
    XmbDrawString(app->display(), graphics.drawable(),
    		  fFontSet, graphics.handle(), x, y, str, len);
}

XFontSet YFontSet::getFontSetWithGuess(char const * pattern, char *** missing,
				       int * nMissing, char ** defString) {
    XFontSet fontset(XCreateFontSet(app->display(), pattern,
   				    missing, nMissing, defString));

    if (None != fontset && !*nMissing) // --------------- got an exact match ---
	return fontset;

    if (*nMissing) XFreeStringList(*missing);

    if (None == fontset) { // --- get a fallback fontset for pattern analyis ---
	char const * locale(setlocale(LC_CTYPE, NULL));
	setlocale(LC_CTYPE, "C"); 

	fontset = XCreateFontSet(app->display(), pattern,
				 missing, nMissing, defString);

	setlocale(LC_CTYPE, locale);
    }

    if (None != fontset) { // ----------------------------- get default XLFD ---
	char ** fontnames;
	XFontStruct ** fontstructs;
	XFontsOfFontSet(fontset, &fontstructs, &fontnames);
	pattern = *fontnames;
    }

    char * weight(getNameElement(pattern, 3));
    char * slant(getNameElement(pattern, 4));
    char * pxlsz(getNameElement(pattern, 7));

    // --- build fuzzy font pattern for better matching for various charsets ---
    if (!strcmp(weight, "*")) { delete[] weight; weight = newstr("medium"); }
    if (!strcmp(slant,  "*")) { delete[] slant; slant = newstr("r"); }

    pattern = strJoin(pattern, ","
	"-*-*-", weight, "-", slant, "-*-*-", pxlsz, "-*-*-*-*-*-*-*,"
	"-*-*-*-*-*-*-", pxlsz, "-*-*-*-*-*-*-*,*", NULL);

    if (fontset) XFreeFontSet(app->display(), fontset);

    delete[] pxlsz;
    delete[] slant;
    delete[] weight;

    MSG(("trying fuzzy fontset pattern: \"%s\"", pattern));

    fontset = XCreateFontSet(app->display(), pattern,
    			     missing, nMissing, defString);
    delete[] pattern;
    return fontset;
}

#endif // CONFIG_I18N

/******************************************************************************/

#ifdef CONFIG_XFREETYPE

YXftFont::YXftFont(const char *name):
    fFontCount(strTokens(name, ",")), fAscent(0), fDescent(0) {
    XftFont ** fptr(fFonts = new XftFont* [fFontCount]);

    for (char const *s(name); '\0' != *s; s = strnxt(s, ",")) {
	XftFont *& font(*fptr);

	char * xlfd(newstr(s + strspn(s, " \t\r\n"), ","));
	char * endptr(xlfd + strlen(xlfd) - 1);
	while (endptr > xlfd && strchr(" \t\r\n", *endptr)) --endptr;
	endptr[1] = '\0';

	font = XftFontOpenXlfd(app->display(), app->screen(), xlfd);

	if (NULL != font) {
	    fAscent = max(fAscent, (unsigned) max(0, font->ascent));
	    fDescent = max(fDescent, (unsigned) max(0, font->descent));
	    ++fptr;
	} else {
	    warn(_("Could not load font \"%s\"."), xlfd);
	    --fFontCount;
	}

	delete[] xlfd;
    }

msg("%s -> %d", name, fFontCount);
    
    if (0 == fFontCount) {
	XftFont * sans(XftFontOpen(app->display(), app->screen(),
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
	    warn(_("Loading of fallback font \"%s\" failed."), "sans");
    }

msg("%s -> %d", name, fFontCount);
}

YXftFont::~YXftFont() {
    for (unsigned n(0); n < fFontCount; ++n)
	XftFontClose(app->display(), fFonts[n]);

    delete[] fFonts;
}

unsigned YXftFont::textWidth(string_t const & text) const {
    char_t * str((char_t *) text.data());
    size_t len(text.length());

    TextPart * partitions(partitions(str, len));
    unsigned width(0);

    for (TextPart * p(partitions); p && p->length; ++p) width+= p->width;

    delete[] partitions;
    return width;
}

unsigned YXftFont::textWidth(char const * str, int len) const {
    return textWidth(string_t(str, len));
}

void YXftFont::drawGlyphs(Graphics & graphics, int x, int y, 
    			  char const * str, int len) {
    string_t xtext(str, len);
    if (0 == xtext.length()) return;

    int const y0(y - ascent());
    int const gcFn(graphics.function());

    char_t * xstr((char_t *) xtext.data());
    size_t xlen(xtext.length());

    TextPart * partitions(partitions(xstr, xlen));
    unsigned w(0); unsigned const h(height());

    for (TextPart * p(partitions); p && p->length; ++p) w+= p->width;

    YWindowAttributes attributes(graphics.drawable());
    GraphicsCanvas canvas(w, h, attributes.depth());
    XftGraphics textarea(canvas, attributes.visual(), attributes.colormap());

    switch (gcFn) {
	case GXxor:
	    textarea.drawRect(*YColor::black, 0, 0, w, h);
	    break;

	case GXcopy:
	    canvas.copyDrawable(graphics.drawable(), x, y0, w, h, 0, 0);
	    break;
    }

    int xpos(0);
    for (TextPart * p(partitions); p && p->length; ++p) {
	textarea.drawString(*graphics.color(), p->font, xpos, ascent(),
			    xstr, p->length);

	xstr+= p->length; 
	xpos+= p->width;
    }
    
    delete[] partitions;

    graphics.copyDrawable(canvas.drawable(), 0, 0, w, h, x, y0);
}

YXftFont::TextPart * YXftFont::partitions(char_t * str, size_t len,
					  size_t nparts = 0) const {
    XGlyphInfo extends;
    XftFont ** lFont(fFonts + fFontCount);
    XftFont ** font(NULL);
    char_t * c(str);

    for (char_t * endptr(str + len); c < endptr; ++c) {
	XftFont ** probe(fFonts);

	while (probe < lFont && !XftGlyphExists(app->display(), *probe, *c))
	    ++probe;

	if (probe != font) {
	    if (NULL != font) {
		TextPart * parts(partitions(c, len - (c - str), nparts + 1));
		parts[nparts].length = (c - str);

		if (font < lFont) {
		    XftGraphics::textExtents(*font, str, (c - str), extends);
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
	XftGraphics::textExtents(*font, str, (c - str), extends);
	parts[nparts].font = *font;
	parts[nparts].width = extends.xOff;
    } else {
	parts[nparts].font = NULL;
	parts[nparts].width = 0;
    }

    return parts;
}
	
#endif // CONFIG_XFREETYPE

/******************************************************************************/
/******************************************************************************/

Graphics::Graphics(YWindow *window, unsigned long vmask, XGCValues * gcv):
    fDisplay(app->display()), fDrawable(window->handle()),
    fColor(NULL), fFont(NULL) {
    gc = XCreateGC(fDisplay, fDrawable, vmask, gcv);
}

Graphics::Graphics(YWindow *window):
    fDisplay(app->display()), fDrawable(window->handle()),
    fColor(NULL), fFont(NULL) {
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(fDisplay, fDrawable, GCGraphicsExposures, &gcv);
}

Graphics::Graphics(YPixmap *pixmap):
    fDisplay(app->display()), fDrawable(pixmap->pixmap()),
    fColor(NULL), fFont(NULL) {
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(fDisplay, fDrawable, GCGraphicsExposures, &gcv);
}

Graphics::Graphics(Drawable drawable, unsigned long vmask, XGCValues * gcv):
    fDisplay(app->display()), fDrawable(drawable),
    fColor(NULL), fFont(NULL) {
    gc = XCreateGC(fDisplay, fDrawable, vmask, gcv);
}

Graphics::Graphics(Drawable drawable):
    fDisplay(app->display()), fDrawable(drawable),
    fColor(NULL), fFont(NULL) {
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(fDisplay, fDrawable, GCGraphicsExposures, &gcv);
}

Graphics::~Graphics() {
    XFreeGC(fDisplay, gc);
}

/******************************************************************************/

void Graphics::copyArea(const int x, const int y,
			const int width, const int height,
			const int dx, const int dy) {
    XCopyArea(fDisplay, fDrawable, fDrawable, gc,
              x, y, width, height, dx, dy);
}

void Graphics::copyDrawable(Drawable const d, const int x, const int y, 
			    const int w, const int h, const int dx, const int dy) {
    XCopyArea(fDisplay, d, fDrawable, gc, x, y, w, h, dx, dy);
}
    
void Graphics::copyImage(XImage * image,
			 const int x, const int y, const int w, const int h,
			 const int dx, const int dy) {
    XPutImage(fDisplay, fDrawable, gc, image, x, y, dx, dy, w, h);
}

#ifdef CONFIG_ANTIALIASING
void Graphics::copyPixbuf(YPixbuf & pixbuf,
			  const int x, const int y, const int w, const int h,
			  const int dx, const int dy) {
    pixbuf.copyToDrawable(fDrawable, gc, x, y, w, h, dx, dy);
}
#endif

/******************************************************************************/

void Graphics::drawPoint(int x, int y) {
    XDrawPoint(fDisplay, fDrawable, gc, x, y);
}

void Graphics::drawLine(int x1, int y1, int x2, int y2) {
    XDrawLine(fDisplay, fDrawable, gc, x1, y1, x2, y2);
}

void Graphics::drawLines(XPoint * points, int n, int mode) {
    XDrawLines(fDisplay, fDrawable, gc, points, n, mode);
}

void Graphics::drawSegments(XSegment * segments, int n) {
    XDrawSegments(fDisplay, fDrawable, gc, segments, n);
}

void Graphics::drawRect(int x, int y, int width, int height) {
    XDrawRectangle(fDisplay, fDrawable, gc, x, y, width, height);
}

void Graphics::drawRects(XRectangle * rects, int n) {
    XDrawRectangles(fDisplay, fDrawable, gc, rects, n);
}

void Graphics::drawArc(int x, int y, int width, int height, int a1, int a2) {
    XDrawArc(fDisplay, fDrawable, gc, x, y, width, height, a1, a2);
}

/******************************************************************************/

void Graphics::drawChars(const char *data, int offset, int len, int x, int y) {
    if (NULL != fFont) fFont->drawGlyphs(*this, x, y, data + offset, len);
}

void Graphics::drawString(int x, int y, char const * str) {
    drawChars(str, 0, strlen(str), x, y);
}

void Graphics::drawStringEllipsis(int x, int y, const char *str, int maxWidth) {
    int const len(strlen(str));
    int const w(fFont ? fFont->textWidth(str, len) : 0);

    if (fFont == 0 || w <= maxWidth) {
        drawChars(str, 0, len, x, y);
    } else {
        int const maxW(maxWidth - fFont->textWidth("...", 3));
        int l(0), w(0);
        int sl(0), sw(0);

	if (multiByte) mblen(NULL, 0);

        if (maxW > 0) {
            while (l < len) {
                int nc, wc;
#ifdef CONFIG_I18N
                if (multiByte) {
                    nc = mblen(str + l, len - l);
                    wc = fFont->textWidth(str + l, nc);
                } else
#endif
                {
		    nc = 1;
                    wc = fFont->textWidth(str + l, 1);
                }

                if (w + wc < maxW) {
		    if (1 == nc && isspace (str[l]))
		    {
			sl+= nc;
			sw+= wc;
		    } else {
			sl = 
			sw = 0;
		    }

		    l+= nc;
                    w+= wc;
                } else
                    break;
            }
        }

	l-= sl;
	w-= sw;

        if (l > 0)
	    drawChars(str, 0, l, x, y);
        if (l < len)
            drawChars("...", 0, 3, x + w, y);
    }
}

void Graphics::drawCharUnderline(int x, int y, const char *str, int charPos) {
    int left = fFont ? fFont->textWidth(str, charPos) : 0;
    int right = fFont ? fFont->textWidth(str, charPos + 1) - 1 : 0;

    drawLine(x + left, y + 2, x + right, y + 2);
}

void Graphics::drawStringMultiline(int x, int y, const char *str) {
    unsigned const tx(x + fFont->multilineTabPos(str));

    for (const char * end(strchr(str, '\n')); end;
	 str = end + 1, end = strchr(str, '\n')) {
	int const len(end - str);
	const char * tab((const char *) memchr(str, '\t', len));

	if (tab) {
	    drawChars(str, 0, tab - str, x, y);
	    drawChars(tab + 1, 0, end - tab - 1, tx, y);
        }
	else
	    drawChars(str, 0, end - str, x, y);
	    
	y+= fFont->height();
    }

    const char * tab(strchr(str, '\t'));

    if (tab) {
	drawChars(str, 0, tab - str, x, y);
	drawChars(tab + 1, 0, strlen(tab + 1), tx, y);
    }
    else
	drawChars(str, 0, strlen(str), x, y);
}

namespace YRotated {
    struct R90 {
	static int xOffset(YFont const * font) { return -font->descent(); }
	static int yOffset(YFont const * /*font*/) { return 0; }

	template <class T> 
	static T width(T const & /*w*/, T const & h) { return h; }
	template <class T> 
	static T height(T const & w, T const & /*h*/) { return w; }
	

	static void rotate(XImage * src, XImage * dst) {
	    for (int sy(src->height - 1), dx(0); sy >= 0; --sy, ++dx)
		for (int sx(src->width - 1), & dy(sx); sx >= 0; --sx)
		    XPutPixel(dst, dx, dy, XGetPixel(src, sx, sy));
	}
    };

    struct R270 {
	static int xOffset(YFont const * font) { return -font->descent(); }
	static int yOffset(YFont const * /*font*/) { return 0; }

	template <class T> 
	static T width(T const & /*w*/, T const & h) { return h; }
	template <class T> 
	static T height(T const & w, T const & /*h*/) { return w; }

	static void rotate(XImage * src, XImage * dst) {
	    for (int sy(src->height - 1), & dx(sy); sy >= 0; --sy)
	        for (int sx(src->width - 1), dy(0); sx >= 0; --sx, ++dy)
		    XPutPixel(dst, dx, dy, XGetPixel(src, sx, sy));
	}
    };
}

template <class Rt>
void Graphics::drawStringRotated(int x, int y, char const * str) {
    int const l(strlen(str));
    int const w(fFont->textWidth(str, l));
    int const h(fFont->ascent() + fFont->descent());
    
    GraphicsCanvas canvas(w, h, 1);
    if (None == canvas.drawable()) {
	warn(_("Resource allocation for rotated string \"%s\" (%dx%d px) "
	       "failed"), str, w, h);
	return;
    }

    canvas.fillRect(0, 0, w, h);	
    canvas.setFont(fFont);
    canvas.setColor(YColor::white);
    canvas.drawChars(str, 0, l, 0, fFont->ascent());

    XImage * horizontal(XGetImage(fDisplay, canvas.drawable(),
				  0, 0, w, h, 1, XYPixmap));
    if (NULL == horizontal) {
	warn(_("Resource allocation for rotated string \"%s\" (%dx%d px) "
	       "failed"), str, w, h);
	return;
    }

    int const bpl(((Rt::width(w, h) >> 3) + 3) & ~3);
    YWindowAttributes attributes(drawable());
    
    XImage * rotated(XCreateImage(fDisplay, attributes.visual(), 1, XYPixmap,
				  0, new char[bpl * Rt::height(w, h)],
				  Rt::width(w, h), Rt::height(w, h), 32, bpl));

    if (NULL == rotated) {
	warn(_("Resource allocation for rotated string \"%s\" (%dx%d px) "
	       "failed"), str, Rt::width(w, h), Rt::height(w, h));
	return;
    }

    Rt::rotate(horizontal, rotated);
    XDestroyImage(horizontal);

    GraphicsCanvas mask(Rt::width(w, h), Rt::height(w, h), 1);
    if (None == mask.drawable()) {
	warn(_("Resource allocation for rotated string \"%s\" (%dx%d px) "
	       "failed"), str, Rt::width(w, h), Rt::height(w, h));
	return;
    }

    mask.copyImage(rotated, 0, 0);
    XDestroyImage(rotated);

    x += Rt::xOffset(fFont);
    y += Rt::yOffset(fFont);

    setClipMask(mask.drawable());
    setClipOrigin(x, y);

    fillRect(x, y, Rt::width(w, h), Rt::height(w, h));

    setClipOrigin(0, 0);
    setClipMask(None);
}

void Graphics::drawString90(int x, int y, char const * str) {
    drawStringRotated<YRotated::R90>(x, y, str);
}
/*
void Graphics::drawString180(int x, int y, char const * str) {
    drawStringRotated<YRotated::R180>(x, y, str);
}
*/
void Graphics::drawString270(int x, int y, char const * str) {
    drawStringRotated<YRotated::R270>(x, y, str);
}

/******************************************************************************/

void Graphics::fillRect(int x, int y, int width, int height) {
    XFillRectangle(fDisplay, fDrawable, gc,
                   x, y, width, height);
}

void Graphics::fillRects(XRectangle * rects, int n) {
    XFillRectangles(fDisplay, fDrawable, gc, rects, n);
}

void Graphics::fillPolygon(XPoint * points, int const n, int const shape,
			  int const mode) {
    XFillPolygon(fDisplay, fDrawable, gc, points, n, shape, mode);
}

void Graphics::fillArc(int x, int y, int width, int height, int a1, int a2) {
    XFillArc(fDisplay, fDrawable, gc, x, y, width, height, a1, a2);
}

/******************************************************************************/

void Graphics::setColor(YColor * aColor) {
    fColor = aColor;
    XSetForeground(fDisplay, gc, fColor->pixel());
}

void Graphics::setFont(YFont * aFont) {
    fFont = aFont;
}

void Graphics::setPenStyle(bool dotLine) {
    XGCValues gcv;

    if (dotLine) {
        char c = 1;
        gcv.line_style = LineOnOffDash;
        XSetDashes(fDisplay, gc, 0, &c, 1);
    } else {
        gcv.line_style = LineSolid;
    }

    XChangeGC(fDisplay, gc, GCLineStyle, &gcv);
}

void Graphics::setFunction(int function) {
    XSetFunction(fDisplay, gc, function);
}

void Graphics::setClipRects(int x, int y, XRectangle rectangles[], int n,
			    int ordering) {
    XSetClipRectangles(fDisplay, gc, x, y, rectangles, n, ordering);
}

void Graphics::setClipMask(Pixmap mask) {
    XSetClipMask(fDisplay, gc, mask);
}

void Graphics::setClipOrigin(int x, int y) {
    XSetClipOrigin(fDisplay, gc, x, y);
}

/******************************************************************************/

void Graphics::drawImage(YIcon::Image * image, int const x, int const y) {
#ifdef CONFIG_ANTIALIASING
    if (YWindow::viewable(fDrawable)) {
	unsigned const w(image->width()), h(image->height());
	YPixbuf bg(fDrawable, None, w, h, x, y);
    	bg.copyArea(*image, 0, 0, w, h, 0, 0);
    	bg.copyToDrawable(fDrawable, gc, 0, 0, w, h, x, y);
    }
#else
    drawPixmap(image, x, y);
#endif
}

void Graphics::drawPixmap(YPixmap const * pix, int const x, int const y) {
    if (pix->mask())
        drawClippedPixmap(pix->pixmap(),
                          pix->mask(),
                          0, 0, pix->width(), pix->height(), x, y);
    else
        XCopyArea(fDisplay, pix->pixmap(), fDrawable, gc,
                  0, 0, pix->width(), pix->height(), x, y);
}

void Graphics::drawMask(YPixmap const * pix, int const x, int const y) {
    if (pix->mask())
        XCopyArea(fDisplay, pix->mask(), fDrawable, gc,
                  0, 0, pix->width(), pix->height(), x, y);
}

void Graphics::drawClippedPixmap(Pixmap pix, Pixmap clip,
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
    XChangeGC(fDisplay, clipPixmapGC,
              GCClipMask|GCClipXOrigin|GCClipYOrigin, &gcv);
    XCopyArea(fDisplay, pix, fDrawable, clipPixmapGC,
              x, y, w, h, toX, toY);
    gcv.clip_mask = None;
    XChangeGC(fDisplay, clipPixmapGC, GCClipMask, &gcv);
}

/******************************************************************************/

void Graphics::draw3DRect(int x, int y, int w, int h, bool raised) {
    YColor *back(color());
    YColor *bright(back->brighter());
    YColor *dark(back->darker());
    YColor *t(raised ? bright : dark);
    YColor *b(raised ? dark : bright);

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
    YColor *back(color());
    YColor *bright(back->brighter());
    YColor *dark(back->darker());

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
    YColor *back(color());
    YColor *bright(back->brighter());
    YColor *dark(back->darker());

    if (raised) {
        setColor(bright);
        drawLine(x + 1, y + 1, x + w, y + 1);
        drawLine(x + 1, y + 1, x + 1, y + h);
        drawLine(x + 1, y + h, x + w, y + h);
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
    YColor *back(color());
    YColor *bright(back->brighter());
    YColor *dark(back->darker());

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
        return ;

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

void Graphics::repHorz(Drawable d, int pw, int ph, int x, int y, int w) {
    while (w > 0) {
        XCopyArea(fDisplay, d, fDrawable, gc, 0, 0, min(w, pw), ph, x, y);
        x += pw;
        w -= pw;
    }
}

void Graphics::repVert(Drawable d, int pw, int ph, int x, int y, int h) {
    while (h > 0) {
        XCopyArea(fDisplay, d, fDrawable, gc, 0, 0, pw, min(h, ph), x, y);
        y += ph;
        h -= ph;
    }
}

void Graphics::fillPixmap(YPixmap const * pixmap, int const x, int const y,
			  int const w, int const h, int px, int py) {
    int const pw(pixmap->width());
    int const ph(pixmap->height());

    px%= pw; const int pww(px ? pw - px : 0);
    py%= ph; const int phh(py ? ph - py : 0);

    if (px) {
	if (py)
            XCopyArea(fDisplay, pixmap->pixmap(), fDrawable, gc,
                      px, py, pww, phh, x, y);

        for (int yy(y + phh), hh(h - phh); hh > 0; yy += ph, hh -= ph)
            XCopyArea(fDisplay, pixmap->pixmap(), fDrawable, gc,
                      px, 0, pww, min(hh, ph), x, yy);
    }

    for (int xx(x + pww), ww(w - pww); ww > 0; xx+= pw, ww-= pw) {
	int const www(min(ww, pw));

	if (py)
            XCopyArea(fDisplay, pixmap->pixmap(), fDrawable, gc,
                      0, py, www, phh, xx, y);

        for (int yy(y + phh), hh(h - phh); hh > 0; yy += ph, hh -= ph)
            XCopyArea(fDisplay, pixmap->pixmap(), fDrawable, gc,
                      0, 0, www, min(hh, ph), xx, yy);
    }
}

void Graphics::drawSurface(YSurface const & surface, int x, int y, int w, int h,
			   int const sx, int const sy, 
#ifdef CONFIG_GRADIENTS    
			   const int sw, const int sh) {
    if (surface.gradient)
	drawGradient(*surface.gradient, x, y, w, h, sx, sy, sw, sh);
    else 
#else
			   const int /*sw*/, const int /*sh*/) {
#endif    
    if (surface.pixmap)
	fillPixmap(surface.pixmap, x, y, w, h, sx, sy);
    else if (surface.color) {
	setColor(surface.color);
	fillRect(x, y, w, h);
    }
}

#ifdef CONFIG_GRADIENTS
void Graphics::drawGradient(const class YPixbuf & pixbuf,
			    int const x, int const y, const int w, const int h,
			    int const gx, int const gy, const int gw, const int gh) {
    YPixbuf(pixbuf, gw, gh).copyToDrawable(fDrawable, gc, gx, gy, w, h, x, y);
}
#endif

/******************************************************************************/

void Graphics::drawArrow(Direction direction, int x, int y, int size, 
			 bool pressed) {
    YColor *nc(color());
    YColor *oca(pressed ? nc->darker() : nc->brighter()),
	   *ica(pressed ? YColor::black : nc),
    	   *ocb(pressed ? wmLook == lookGtk ? nc : nc->brighter()
			: nc->darker()),
	   *icb(pressed ? nc->brighter() : YColor::black);

    XPoint points[3];

    short const am(size / 2);
    short const ah(wmLook == lookGtk ||
		   wmLook == lookMotif ? size : size / 2);
    short const aw(wmLook == lookGtk ||
		   wmLook == lookMotif ? size : size - (size & 1));
		   
    switch (direction) {
	case Up:
	    points[0].x = x;
	    points[0].y = y + ah;
	    points[1].x = x + am;
	    points[1].y = y;
	    points[2].x = x + aw;
	    points[2].y = y + ah;
	    break;

	case Down:
	    points[0].x = x;
	    points[0].y = y;
	    points[1].x = x + am;
	    points[1].y = y + ah;
	    points[2].x = x + aw;
	    points[2].y = y;
	    break;

	case Left:
	    points[0].x = x + ah;
	    points[0].y = y;
	    points[1].x = x;
	    points[1].y = y + am;
	    points[2].x = x + ah;
	    points[2].y = y + aw;
	    break;

	case Right:
	    points[0].x = x;
	    points[0].y = y;
	    points[1].x = x + ah;
	    points[1].y = y + am;
	    points[2].x = x;
	    points[2].y = y + aw;
	    break;

	default:
	    return ;
    }

    short const dx0(direction == Up || direction == Down ? 1 : 0);
    short const dy0(direction == Up || direction == Down ? 0 : 1);
    short const dx1(direction == Up || direction == Left ? dy0 : -dy0);
    short const dy1(direction == Up || direction == Left ? dx0 : -dx0);

// ============================================================= inner bevel ===
    if (wmLook == lookGtk || wmLook == lookMotif) {
	setColor(ocb);
	drawLine(points[2].x - dx0 - (size & 1 ? 0 : dx1),
		 points[2].y - (size & 1 ? 0 : dy1) - dy0,
		 points[1].x + 2 * dx1, points[1].y + 2 * dy1);

	setColor(wmLook == lookMotif ? oca : ica);
	drawLine(points[0].x + dx0 - (size & 1 ? dx1 : 0),
		 points[0].y + dy0 - (size & 1 ? dy1 : 0),
		 points[1].x + (size & 1 ? dx0 : 0)
		 	     + (size & 1 ? dx1 : dx1 + dx1),
		 points[1].y + (size & 1 ? dy0 : 0)
		 	     + (size & 1 ? dy1 : dy1 + dy1));
		 
	if ((direction == Up || direction == Left)) setColor(ocb);
	drawLine(points[0].x + dx0 - dx1, points[0].y + dy0 - dy1,
	    	 points[2].x - dx0 - dx1, points[2].y - dy0 - dy1);
    } else if (wmLook == lookWarp3) {
	drawLine(points[0].x + dx0, points[0].y + dy0,
		 points[1].x + dx0, points[1].y + dy0);
	drawLine(points[2].x + dx0, points[2].y + dy0,
		 points[1].x + dx0, points[1].y + dy0);
    } else
        fillPolygon(points, 3, Convex, CoordModeOrigin);

// ============================================================= outer bevel ===
    if (wmLook == lookMotif || wmLook == lookGtk) {
	setColor(wmLook == lookMotif ? ocb : icb);

	drawLine(points[2].x, points[2].y, points[1].x, points[1].y);

	if (wmLook == lookGtk || wmLook == lookMotif) setColor(oca);
	drawLine(points[0].x, points[0].y, points[1].x, points[1].y);

	if ((direction == Up || direction == Left))
	    setColor(wmLook == lookMotif ? ocb : icb);

    } else
        drawLines(points, 3, CoordModeOrigin);

    if (wmLook != lookWarp3)
	drawLine(points[0].x, points[0].y, points[2].x, points[2].y);

    setColor(nc);
}

int Graphics::function() const {
    XGCValues values;
    XGetGCValues(fDisplay, gc, GCFunction, &values);
    return values.function;
}

/******************************************************************************/
/******************************************************************************/

GraphicsCanvas::GraphicsCanvas(int w, int h):
    Graphics(YPixmap::createPixmap(w, h)) {
}    

GraphicsCanvas::GraphicsCanvas(int w, int h, int depth):
    Graphics(YPixmap::createPixmap(w, h, depth)) {
}

GraphicsCanvas::~GraphicsCanvas() {
    XFreePixmap(display(), drawable());
}
