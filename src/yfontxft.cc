#include "config.h"

#ifdef CONFIG_XFREETYPE

#include "ystring.h"
#include "ypaint.h"
#include "ypointer.h"
#include "yxapp.h"
#include "yfontname.h"
#include "yfontbase.h"
#include "intl.h"
#include <stdio.h>
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#ifdef CONFIG_I18N
#include <langinfo.h>
#else
#define nl_langinfo(X) ""
#endif

#ifdef CONFIG_FRIBIDI
        // remove deprecated warnings for now...
        #include <fribidi/fribidi-config.h>
        #if FRIBIDI_USE_GLIB+0
                #include <glib.h>
                #undef G_GNUC_DEPRECATED
                #define G_GNUC_DEPRECATED
        #endif
        #include <fribidi/fribidi.h>
#endif

/******************************************************************************/

class YXftFont : public YFontBase {
public:
#ifdef CONFIG_I18N
    typedef class YUnicodeString string_t;
    typedef XftChar32 char_t;
    #define XftDrawString XftDrawString32
    #define XftTextExtents XftTextExtents32
#else
    typedef class YLocaleString string_t;
    typedef XftChar8 char_t;
    #define XftTextExtents XftTextExtents8
    #define XftDrawString XftDrawString8
#endif

    YXftFont(mstring name, bool xlfd);
    virtual ~YXftFont();

    bool valid() const override { return 0 < fFontCount; }
    int descent() const override { return fDescent; }
    int ascent() const override { return fAscent; }
    int textWidth(mstring s) const override;
    int textWidth(char const * str, int len) const override;

    int textWidth(string_t const & str) const;
    void drawGlyphs(class Graphics & graphics, int x, int y,
                            char const * str, int len) override;

    bool supports(unsigned utf32char) override;

private:
    struct TextPart {
        XftFont * font;
        size_t length;
        unsigned width;
    };

    TextPart * partitions(char_t * str, size_t len, size_t nparts = 0) const;
    void drawString(Graphics& g, XftFont* font, int x, int y,
                    char_t* str, size_t len);
    void textExtents(XftFont* font, char_t* str, size_t len,
                     XGlyphInfo& extends) const {
        XftTextExtents(xapp->display(), font, str, len, &extends);
    }

    int fFontCount, fAscent, fDescent;
    XftFont** fFonts;
};

void YXftFont::drawString(Graphics &g, XftFont* font, int x, int y,
                          char_t* str, size_t len)
{
#ifdef CONFIG_FRIBIDI
    const size_t bufsize = 256;
    char_t buf[bufsize];
    char_t* vis_str = buf;
    asmart<char_t> big;

    if (len >= bufsize) {
        big = new char_t[len+1];
        if (big == nullptr)
            return;
        vis_str = big;
    }

    FriBidiCharType pbase_dir = FRIBIDI_TYPE_N;

    if (fribidi_log2vis(str, len, &pbase_dir, //input
                        vis_str, // output
                        nullptr, nullptr, nullptr // stats
                        ))
    {
        str = vis_str;
    }
#endif

    XftDrawString(g.handleXft(), g.color().xftColor(), font,
                  x - g.xorigin(),
                  y - g.yorigin(),
                  str, len);
}

/******************************************************************************/

YXftFont::YXftFont(mstring name, bool use_xlfd):
    fFontCount(1 + name.count(',')),
    fAscent(0),
    fDescent(0),
    fFonts(new XftFont* [fFontCount])
{
    int count = 0;
    for (mstring s(name), r; s.splitall(',', &s, &r); s = r) {
        mstring fname = s.trim();
        if (fname.isEmpty() || count >= fFontCount)
            continue;

        XftFont* font;
        if (use_xlfd) {
            font = XftFontOpenXlfd(xapp->display(), xapp->screen(), fname);
        } else {
            font = XftFontOpenName(xapp->display(), xapp->screen(), fname);
        }

        if (font) {
            fAscent = max(fAscent, font->ascent);
            fDescent = max(fDescent, font->descent);
            fFonts[count++] = font;
        } else {
            warn(_("Could not load font \"%s\"."), fname.c_str());
            --fFontCount;
        }
    }

    if (0 == fFontCount || 0 == count) {
        msg("xft: fallback from '%s'", name.c_str());
        XftFont *sans =
            XftFontOpen(xapp->display(), xapp->screen(),
                        XFT_FAMILY, XftTypeString, "sans-serif",
                        XFT_PIXEL_SIZE, XftTypeInteger, 12,
                        NULL);
        if (sans) {
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
    for (int n = 0; n < fFontCount; ++n) {
        // this leaks memory when xapp is destroyed before fonts
        if (xapp != nullptr)
            XftFontClose(xapp->display(), fFonts[n]);
    }
    delete[] fFonts;
}

int YXftFont::textWidth(mstring s) const {
    return textWidth(s.c_str(), s.length());
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
            drawString(graphics, p->font,
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

bool YXftFont::supports(unsigned utf32char) {
    if (utf32char >= 255 && strcmp("UTF-8", nl_langinfo(CODESET)))
        return false;

    // be conservative, only report when all font candidates can do it
    for (int i = 0; i < fFontCount; ++i) {
        if (!XftCharExists(xapp->display(), fFonts[i], utf32char))
            return false;
    }
    return true;
}

YXftFont::TextPart * YXftFont::partitions(char_t * str, size_t len,
                                          size_t nparts) const
{
    XGlyphInfo extends;
    XftFont ** lFont(fFonts + fFontCount);
    XftFont ** font(nullptr);
    char_t * c(str);

    if (fFonts == nullptr || fFontCount == 0)
        return nullptr;

    for (char_t * endptr(str + len); c < endptr; ++c) {
        XftFont ** probe(fFonts);

        while (probe < lFont && !XftGlyphExists(xapp->display(), *probe, *c))
            ++probe;

        if (probe != font) {
            if (nullptr != font) {
                TextPart *parts = partitions(c, len - (c - str), nparts + 1);
                parts[nparts].length = (c - str);

                if (font < lFont) {
                    textExtents(*font, str, (c - str), extends);
                    parts[nparts].font = *font;
                    parts[nparts].width = extends.xOff;
                } else {
                    parts[nparts].font = nullptr;
                    parts[nparts].width = 0;
                    MSG(("glyph not found: %d", *(c - 1)));
                }

                return parts;
            } else
                font = probe;
        }
    }

    TextPart *parts = new TextPart[nparts + 2];
    parts[nparts + 1].font =  nullptr;
    parts[nparts + 1].width = 0;
    parts[nparts + 1].length = 0;
    parts[nparts].length = (c - str);

    if (nullptr != font && font < lFont) {
        textExtents(*font, str, (c - str), extends);
        parts[nparts].font = *font;
        parts[nparts].width = extends.xOff;
    } else {
        parts[nparts].font = nullptr;
        parts[nparts].width = 0;
    }

    return parts;
}

YFontBase* getXftFontXlfd(const char* name) {
    YXftFont* font = new YXftFont(name, true);
    if (font && !font->valid()) {
        delete font; font = nullptr;
    }
    return font;
}

YFontBase* getXftFont(const char* name) {
    YXftFont* font = new YXftFont(name, false);
    if (font && !font->valid()) {
        delete font; font = nullptr;
    }
    return font;
}

YFontBase* getXftDefault(const char* name) {
    return getXftFont("10x20");
}

#endif // CONFIG_XFREETYPE

// vim: set sw=4 ts=4 et:
