#include "config.h"

#ifdef CONFIG_XFREETYPE

#include "ypaint.h"
#include "yapp.h"
#include "ystring.h"
#include "intl.h"

/******************************************************************************/

class YXftFont : public YFont {
public:
#ifdef CONFIG_I18N
    typedef class YUnicodeString string_t;
    typedef XftChar32 char_t;
#else
    typedef class YLocaleString string_t;
    typedef XftChar8 char_t;
#endif    

    YXftFont(char const * name);
    virtual ~YXftFont();

    virtual operator bool() const { return (fFontCount > 0); }
    virtual int descent() const { return fDescent; }
    virtual int ascent() const { return fAscent; }
    virtual int textWidth(char const * str, int len) const;

    virtual int textWidth(string_t const & str) const;
    virtual void drawGlyphs(class Graphics & graphics, int x, int y, 
                            char const * str, int len);

private:
    struct TextPart {
        XftFont * font;
        size_t length;
        unsigned width;
    };

    TextPart * partitions(char_t * str, size_t len, size_t nparts = 0) const;

    unsigned fFontCount, fAscent, fDescent;
    XftFont ** fFonts;
};

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

YXftFont::YXftFont(const char *name):
    fFontCount(strtoken(name, ",")), fAscent(0), fDescent(0) {
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

#if 0
        if (strstr(xlfd, "koi") != 0) {
            msg("font %s", xlfd);
            for (int c = 0; c < 0x500; c++) {
                if ((c % 64) == 0) {
                    printf("\n%04X ", c);
                }
                int ok = XftGlyphExists(app->display(), font, c);
                printf("%c", ok ? 'X' : '.');
                if ((c % 8) == 7)
                    printf(" ");
            }
            printf("\n");
        }
#endif
	delete[] xlfd;
    }

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
}

YXftFont::~YXftFont() {
    for (unsigned n(0); n < fFontCount; ++n)
	XftFontClose(app->display(), fFonts[n]);

    delete[] fFonts;
}

int YXftFont::textWidth(string_t const & text) const {
    char_t * str((char_t *) text.data());
    size_t len(text.length());

    TextPart *parts = partitions(str, len);
    unsigned width(0);

    for (TextPart * p = parts; p && p->length; ++p) width+= p->width;

    delete[] parts;
    return width;
}

int YXftFont::textWidth(char const * str, int len) const {
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

    TextPart *parts = partitions(xstr, xlen);
    unsigned w(0); unsigned const h(height());

    for (TextPart *p = parts; p && p->length; ++p) w+= p->width;

    YPixmap *pixmap = new YPixmap(w, h);
    Graphics canvas(*pixmap);
    XftGraphics textarea(canvas, app->visual(), app->colormap());

    switch (gcFn) {
	case GXxor:
	    textarea.drawRect(*YColor::black, 0, 0, w, h);
	    break;

	case GXcopy:
	    canvas.copyDrawable(graphics.drawable(), x, y0, w, h, 0, 0);
	    break;
    }

    int xpos(0);
    for (TextPart *p = parts; p && p->length; ++p) {
        if (p->font) textarea.drawString(*graphics.color(), p->font,
                                         xpos, ascent(), xstr, p->length);

	xstr+= p->length;
	xpos+= p->width;
    }

    delete[] parts;

    graphics.copyDrawable(canvas.drawable(), 0, 0, w, h, x, y0);
    delete pixmap;
}

YXftFont::TextPart * YXftFont::partitions(char_t * str, size_t len,
					  size_t nparts) const 
{
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
		TextPart *parts = partitions(c, len - (c - str), nparts + 1);
		parts[nparts].length = (c - str);

		if (font < lFont) {
		    XftGraphics::textExtents(*font, str, (c - str), extends);
		    parts[nparts].font = *font;
		    parts[nparts].width = extends.xOff;
		} else {
		    parts[nparts].font = NULL;
		    parts[nparts].width = 0;
                    warn("glyph not found: %d", *(c - 1));
		}

		return parts;
	    } else
		font = probe;
	}
    }

    TextPart *parts = new TextPart[nparts + 2];
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

YFont *getXftFont(const char *name) {
    YFont *font = new YXftFont(name);
    if (font)
        return font;
    else
        return new YXftFont("sans-serif");
}

#endif // CONFIG_XFREETYPE
