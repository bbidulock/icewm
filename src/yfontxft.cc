#include "config.h"

#ifdef CONFIG_XFREETYPE

#include "ystring.h"
#include "ypaint.h"
#include "yxapp.h"
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

    YXftFont(char const * name, bool xlfd);
    virtual ~YXftFont();

    virtual bool valid() const { return (fFontCount > 0); }
    virtual int descent() const { return fDescent; }
    virtual int ascent() const { return fAscent; }
    virtual int textWidth(const ustring &s) const;
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

#if 0
    void drawRect(Graphics &g, XftColor * color, int x, int y, unsigned w, unsigned h) {
        XftDrawRect(fDraw, color, x - xOrigin, y - yOrigin, w, h);
    }
#endif

    static void drawString(Graphics &g, XftFont * font, int x, int y,
                           char_t * str, size_t len)
    {
        XftColor *c = *g.color();
        XftDrawString(g.handleXft(), c, font,
                      x - g.xorigin(),
                      y - g.yorigin(),
                      str, len);
    }

    static void textExtents(XftFont * font, char_t * str, size_t len,
                            XGlyphInfo & extends) {
        XftTextExtents(xapp->display (), font, str, len, &extends);
    }

       //    XftDraw * handle() const { return fDraw; }
};

/******************************************************************************/

YXftFont::YXftFont(const char *name, bool use_xlfd):
    fFontCount(strtoken(name, ",")), fAscent(0), fDescent(0)
{
    XftFont ** fptr(fFonts = new XftFont* [fFontCount]);

    for (char const *s(name); '\0' != *s; s = strnxt(s, ",")) {
        XftFont *& font(*fptr);

        char * fname(newstr(s + strspn(s, " \t\r\n"), ","));
        char * endptr(fname + strlen(fname) - 1);
        while (endptr > fname && strchr(" \t\r\n", *endptr)) --endptr;
        endptr[1] = '\0';

        if (use_xlfd)
            font = XftFontOpenXlfd(xapp->display(), xapp->screen(), fname);
        else
            font = XftFontOpenName(xapp->display(), xapp->screen(), fname);

        if (NULL != font) {
            fAscent = max(fAscent, (unsigned) max(0, font->ascent));
            fDescent = max(fDescent, (unsigned) max(0, font->descent));
            ++fptr;
        } else {
            warn(_("Could not load font \"%s\"."), fname);
            --fFontCount;
        }
        delete[] fname;
    }

    if (0 == fFontCount) {
        msg("xft: fallback from '%s'", name);
        XftFont *sans =
            XftFontOpen(xapp->display(), xapp->screen(),
                        XFT_FAMILY, XftTypeString, "sans-serif",
                        XFT_PIXEL_SIZE, XftTypeInteger, 12,
                        0);

        if (NULL != sans) {
            delete[] fFonts;

            fFontCount = 1;
            fFonts = new XftFont* [fFontCount];
            fFonts[0] = sans;

            fAscent = sans->ascent;
            fDescent = sans->descent;
        } else
            warn(_("Loading of fallback font \"%s\" failed."), "sans-serif");
    }
}

YXftFont::~YXftFont() {
    for (unsigned n = 0; n < fFontCount; ++n) {
        if (xapp != 0)
            XftFontClose(xapp->display(), fFonts[n]);
    }
    delete[] fFonts;
}

int YXftFont::textWidth(const ustring &s) const {
    cstring cs(s);
    return textWidth(cs.c_str(), cs.c_str_len());
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
///    unsigned w(0);
///    unsigned const h(height());

///    for (TextPart *p = parts; p && p->length; ++p) w+= p->width;

///    YPixmap *pixmap = new YPixmap(w, h);
///    Graphics canvas(*pixmap, 0, 0);
//    XftGraphics textarea(graphics, xapp->visual(), xapp->colormap());

    switch (gcFn) {
        case GXxor:
///         textarea.drawRect(*YColor::black, 0, 0, w, h);
            break;

        case GXcopy:
///            canvas.copyDrawable(graphics.drawable(),
///                                x - graphics.xorigin(), y0 - graphics.yorigin(), w, h, 0, 0);
            break;
    }


    int xpos(0);
    for (TextPart *p = parts; p && p->length; ++p) {
        if (p->font) {
            XftGraphics::drawString(graphics, p->font,
                                    xpos + x, ascent() + y0,
                                    xstr, p->length);
        }

        xstr += p->length;
        xpos += p->width;
    }

    delete[] parts;

///    graphics.copyDrawable(canvas.drawable(), 0, 0, w, h, x, y0);
///    delete pixmap;
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

        while (probe < lFont && !XftGlyphExists(xapp->display(), *probe, *c))
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

ref<YFont> getXftFontXlfd(const char *name) {
    ref<YFont> font(new YXftFont(name, true));
    if (font == null || !font->valid()) {
        msg("failed to load font '%s', trying fallback", name);
        font.init(new YXftFont("sans-serif:size=12", false));
        if (font == null || !font->valid())
            msg("Could not load fallback Xft font.");
    }
    return font;
}

ref<YFont> getXftFont(const char *name) {
    ref<YFont>font(new YXftFont(name, false));
    if (font == null || !font->valid()) {
        msg("failed to load font '%s', trying fallback", name);
        font.init(new YXftFont("sans-serif:size=12", false));
        if (font == null || !font->valid())
            msg("Could not load fallback Xft font.");
    }
    return font;
}

#endif // CONFIG_XFREETYPE
