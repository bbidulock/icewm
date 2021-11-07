#include "config.h"

#ifdef CONFIG_XFREETYPE

#include "ystring.h"
#include "ypaint.h"
#include "ypointer.h"
#include "yxapp.h"
#include "yfontname.h"
#include "yfontbase.h"
#include "ybidi.h"
#include "intl.h"
#include <stdio.h>
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#ifdef CONFIG_I18N
#include <langinfo.h>
#else
#define nl_langinfo(X) ""
#endif

/******************************************************************************/

class YXftFont : public YFontBase {
public:
    typedef YString<wchar_t> string_t;

    YXftFont(mstring name, bool xlfd);
    YXftFont(XftFont* font);
    virtual ~YXftFont();

    bool valid() const override { return 0 < fFontCount; }
    int descent() const override { return fDescent; }
    int ascent() const override { return fAscent; }
    int textWidth(char const * str, int len) const override;

    int textWidth(string_t& str) const;
    void drawGlyphs(class Graphics & graphics, int x, int y,
                    const char* str, int len, int limit = 0) override;

    bool supports(unsigned utf32char) const override;

private:
    struct TextPart {
        XftFont * font;
        size_t length;
        unsigned width;
    };

    TextPart * partitions(wchar_t* str, size_t len, size_t nparts = 0) const;
    void drawString(Graphics& g, XftFont* font, int x, int y,
                    wchar_t* str, size_t len);
    void textExtents(XftFont* font, wchar_t* str, size_t len,
                     XGlyphInfo& extends) const {
        switch (true) {
            case sizeof(wchar_t) == sizeof(FcChar32):
                XftTextExtents32(xapp->display(), font,
                                 (FcChar32 *) str, len, &extends);
            case false: ;
        }
    }

    int fFontCount, fAscent, fDescent;
    XftFont** fFonts;
};

void YXftFont::drawString(Graphics &g, XftFont* font, int x, int y,
                          wchar_t* str, size_t len)
{
    YBidi bidi(str, len);
    switch (true) {
        case sizeof(wchar_t) == sizeof(FcChar32):
            XftDrawString32(g.handleXft(), g.color().xftColor(), font,
                            x - g.xorigin(), y - g.yorigin(),
                            (FcChar32 *) bidi.string(), bidi.length());
        case false: ;
    }
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
        }
    }
    if ((fFontCount = count) == 0) {
        delete[] fFonts; fFonts = nullptr;
    }
}

YXftFont::YXftFont(XftFont* font) :
    fFontCount(1),
    fAscent(font->ascent),
    fDescent(font->descent),
    fFonts(new XftFont* [1])
{
    *fFonts = font;
}

YXftFont::~YXftFont() {
    if (xapp) {
        for (int i = fFontCount; 0 < i--; )
            XftFontClose(xapp->display(), fFonts[i]);
    }
    delete[] fFonts;
}

int YXftFont::textWidth(string_t& str) const {
    YBidi bidi(str.data(), str.length());
    TextPart* parts = partitions(bidi.string(), bidi.length());
    unsigned width(0);

    for (TextPart* p = parts; p && p->length; ++p) {
        width += p->width;
    }

    delete[] parts;
    return width;
}

int YXftFont::textWidth(const char* str, int len) const {
    int width;
    if (0 < len) {
#ifndef CONFIG_I18N
        const size_t size = len + 1;
        wchar_t text[size];
        int count = 0;
        for (int i = 0; i < len; ) {
            int k = mbtowc(&text[count], str + i, size_t(len - i));
            if (k < 1) {
                i++;
            } else {
                i += k;
                count++;
            }
        }
        text[count] = 0;
        string_t string(text, count);
#else
        YUnicodeString string(str, len);
#endif
        width = textWidth(string);
    } else {
        width = 0;
    }
    return width;
}

void YXftFont::drawGlyphs(Graphics & graphics, int x, int y,
                          const char* str, int len, int limit) {
    if (0 < len) {
#ifndef CONFIG_I18N
        const size_t size = len + 1;
        wchar_t text[size];
        int count = 0;
        for (int i = 0; i < len; ) {
            int k = mbtowc(&text[count], str + i, size_t(len - i));
            if (k < 1) {
                i++;
            } else {
                i += k;
                count++;
            }
        }
        text[count] = 0;
        wchar_t* xstr(text);
        size_t xlen(count);
#else
        YUnicodeString string(str, len);
        wchar_t* xstr(string.data());
        size_t xlen(string.length());
#endif

        TextPart *parts = partitions(xstr, xlen);
        int xpos = x;
        for (TextPart *p = parts; p && p->length; ++p) {
            if (p->font) {
                drawString(graphics, p->font,
                           xpos, y,
                           xstr, p->length);
            }
            xstr += p->length;
            xpos += p->width;
        }

        delete[] parts;
    }
}

bool YXftFont::supports(unsigned utf32char) const {
    if (utf32char >= 255 && strcmp("UTF-8", nl_langinfo(CODESET)))
        return false;

    // be conservative, only report when all font candidates can do it
    for (int i = 0; i < fFontCount; ++i) {
        if (!XftCharExists(xapp->display(), fFonts[i], utf32char))
            return false;
    }
    return true;
}

YXftFont::TextPart* YXftFont::partitions(wchar_t* str, size_t len,
                                         size_t nparts) const
{
    XftFont ** lFont(fFonts + fFontCount);
    XftFont ** font(nullptr);
    wchar_t * c(str);

    for (wchar_t* endptr(str + len); c < endptr; ++c) {
        XftFont ** probe(fFonts);

        while (probe < lFont && !XftCharExists(xapp->display(), *probe, *c))
            ++probe;

        if (probe != font) {
            if (nullptr != font) {
                TextPart *parts = partitions(c, len - (c - str), nparts + 1);
                parts[nparts].length = (c - str);

                if (font < lFont) {
                    XGlyphInfo extends;
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
        XGlyphInfo extends;
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
    XftFont* font =
        XftFontOpen(xapp->display(), xapp->screen(),
                    XFT_FAMILY, XftTypeString, "sans-serif",
                    XFT_PIXEL_SIZE, XftTypeInteger, 12,
                    nullptr);
    return font ? new YXftFont(font) : nullptr;
}

#endif // CONFIG_XFREETYPE

// vim: set sw=4 ts=4 et:
