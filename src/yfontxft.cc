#include "config.h"

#ifdef CONFIG_XFREETYPE

#include "ypaint.h"
#include "yprefs.h"
#include "ystring.h"
#include "yxapp.h"
#include "yfontbase.h"
#include "ybidi.h"
#include "intl.h"
#include <stdio.h>
#include <X11/Xft/Xft.h>

/******************************************************************************/

class YXftFont : public YFontBase {
public:
    YXftFont(mstring name, bool xlfd);
    YXftFont(XftFont* font);
    virtual ~YXftFont();

    bool valid() const override { return 0 < fFontCount; }
    int descent() const override { return fDescent; }
    int ascent() const override { return fAscent; }
    int textWidth(char const* str, int len) const override;

    int textWidth(wchar_t* text, int length) const;
    void drawGlyphs(class Graphics& g, int x, int y,
                    const char* str, int len, int limit = 0) override;
    bool supports(unsigned utf32char) const override;

private:
    struct TextPart {
        XftFont* font;
        int length;
        int width;
        TextPart() { }
        TextPart(XftFont* f, int l, int w) :
                 font(f), length(l), width(w) { }
    };
    class TextParts {
        TextPart* parts;
        int length;
    public:
        int extent;

        TextParts(int n): parts(new TextPart[n]), length(n), extent(0) { }
        void discard() { delete[] parts; parts = nullptr; length = 0; }
        TextPart* begin() const { return &parts[0]; }
        TextPart* end() const { return &parts[length]; }
        TextPart& operator[](int index) const { return parts[index]; }
        int size() const { return length; }
    };

    TextParts partitions(wchar_t* str, int len, int nparts = 0) const;

    void drawLimitLeft(Graphics& g, XftFont* font, int x, int y,
                       wchar_t* str, int len, int width, int limit) const;
    void drawLimitRight(Graphics& g, XftFont* font, int x, int y,
                        wchar_t* str, int len, int width, int limit) const;
    void drawString(Graphics& g, XftFont* font, int x, int y,
                    wchar_t* str, int len) const {
        switch (true) {
        case sizeof(wchar_t) == sizeof(FcChar32):
            XftDrawString32(g.handleXft(), g.color().xftColor(), font,
                            x - g.xorigin(), y - g.yorigin(),
                            (FcChar32 *) str, len);
        case false: ;
        }
    }

    void textExtents(XftFont* font, wchar_t* str, int len,
                     XGlyphInfo* extents) const {
        switch (true) {
        case sizeof(wchar_t) == sizeof(FcChar32):
            XftTextExtents32(xapp->display(), font,
                             (FcChar32 *) str, len, extents);
        case false: ;
        }
    }

    wchar_t ellipsis() const {
        const unsigned utf32ellipsis = 0x2026;
        extern bool showEllipsis;
        return showEllipsis && supports(utf32ellipsis) ? utf32ellipsis : None;
    }

    int fFontCount, fAscent, fDescent;
    XftFont** fFonts;
};

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
        if (fname.nonempty() && count < fFontCount) {
            XftFont* font;
            if (use_xlfd) {
                font = XftFontOpenXlfd(xapp->display(), xapp->screen(), fname);
            } else {
                font = XftFontOpenName(xapp->display(), xapp->screen(), fname);
            }
            if (font) {
                fFonts[count++] = font;
            } else {
                warn(_("Could not load font \"%s\"."), fname.c_str());
            }
        }
    }
    if ((fFontCount = count) == 0) {
        delete[] fFonts; fFonts = nullptr;
    } else {
        long numer = 0, accum = 0, decum = 0;
        for (int i = 0; i < count; ++i) {
            XftFont* font = fFonts[i];
            accum += font->ascent * (count - i);
            decum += font->descent * (count - i);
            numer += (count - i);
        }
        fAscent = int((accum + (numer / 2)) / numer);
        fDescent = int((decum + (numer / 2)) / numer);
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

int YXftFont::textWidth(wchar_t* text, int length) const {
    TextParts parts = partitions(text, length);
    parts.discard();
    return parts.extent;
}

int YXftFont::textWidth(const char* str, int len) const {
    int width = 0;
    if (0 < len) {
        YWideString string(str, len);
        YBidi bidi(string.data(), string.length());
        width = textWidth(bidi.string(), int(bidi.length()));
    }
    return width;
}

void YXftFont::drawGlyphs(Graphics& g, int x, int y,
                          const char* str, int len, int limit) {
    if (0 < len && 0 <= limit) {
        YWideString wide(str, len);
        YBidi bidi(wide.data(), wide.length());
        TextParts parts = partitions(bidi.string(), int(bidi.length()));
        if (limit == 0) {
            if (bidi.isRTL() && int(g.rwidth()) < parts.extent) {
                limit = int(g.rwidth());
            }
        } else if (parts.extent <= limit && rightToLeft == false) {
            limit = 0;
        }
        if (limit == 0) {
            wchar_t* xstr = bidi.string();
            int xpos = 0;
            for (TextPart& p : parts) {
                if (p.font) {
                    drawString(g, p.font, x + xpos, y, xstr, p.length);
                }
                xstr += p.length;
                xpos += p.width;
            }
        }
        else {
            bool clipping = (x - g.xorigin() + limit < int(g.rwidth()));
            if (clipping) {
                XRectangle clip = {
                    short(x - g.xorigin()),
                    short(y - ascent() - g.yorigin()),
                    (unsigned short) limit,
                    (unsigned short) g.rheight()
                };
                g.setClipRectangles(&clip, 1);
            }

            if (bidi.isRTL() == false) {
                wchar_t* xstr = bidi.string();
                int xpos = 0;
                for (TextPart& p : parts) {
                    if (p.font && xpos < limit) {
                        if (limit - xpos >= p.width) {
                            drawString(g, p.font, x + xpos, y, xstr, p.length);
                        } else {
                            drawLimitLeft(g, p.font, x + xpos, y, xstr,
                                          p.length, p.width, limit - xpos);
                            break;
                        }
                    }
                    xstr += p.length;
                    xpos += p.width;
                }
            } else {
                wchar_t* xstr = bidi.string() + bidi.length();
                int xpos = limit;
                for (TextPart* q = parts.end(); --q >= parts.begin(); ) {
                    TextPart& p(*q);
                    if (p.font && 0 < xpos) {
                        xstr -= p.length;
                        int left = xpos - p.width;
                        if (left >= 0) {
                            drawString(g, p.font, x + left,
                                       y, xstr, p.length);
                        } else {
                            drawLimitRight(g, p.font, x, y, xstr,
                                           p.length, p.width, xpos);
                            break;
                        }
                        xpos -= p.width;
                    }
                }
            }

            if (clipping) {
                g.resetClip();
            }
        }

        parts.discard();
    }
}

void YXftFont::drawLimitLeft(Graphics& g, XftFont* font, int x, int y,
                             wchar_t* str, int len, int width, int limit) const
{
    wchar_t el = ellipsis();
    int ew = el ? textWidth(&el, 1) : None;
    if (ew) {
        limit -= ew;
    }
    if (0 < limit && 0 < len) {
        int lo = 0, hi = len, pw = width;
        while (lo < hi) {
            int pv = (lo + hi + 1) / 2;
            pw = textWidth(str, pv);
            if (pw <= limit) {
                lo = pv;
            } else {
                hi = pv - 1;
            }
        }
        if (pw > limit && lo > 0)
            lo -= 1;
        if (0 < ew) {
            const int size = lo + 2;
            wchar_t copy[size];
            memcpy(copy, str, lo * sizeof(wchar_t));
            copy[lo] = el;
            copy[lo + 1] = 0;
            drawString(g, font, x, y, copy, lo + 1);
        } else {
            drawString(g, font, x, y, str, lo);
        }
    }
}

void YXftFont::drawLimitRight(Graphics& g, XftFont* font, int x, int y,
                              wchar_t* str, int len, int width, int limit) const
{
    wchar_t el = ellipsis();
    int ew = el ? textWidth(&el, 1) : None;
    if (0 < limit && 0 < len) {
        int lo = 0, hi = len, pw = width + ew;
        while (lo < hi) {
            int pv = (lo + hi + 1) / 2;
            pw = textWidth(str + len - pv, pv) + ew;
            if (pw <= limit) {
                lo = pv;
            } else {
                hi = pv - 1;
            }
        }
        if (pw > limit && lo > 0) {
            lo -= 1;
        }
        if (0 < ew) {
            const int size = lo + 2;
            wchar_t copy[size];
            memcpy(copy + 1, str + len - lo, lo * sizeof(wchar_t));
            copy[0] = el;
            copy[lo + 1] = 0;
            pw = textWidth(copy, lo + 1);
            drawString(g, font, x + limit - pw, y, copy, lo + 1);
        } else {
            pw = textWidth(str + len - lo, lo);
            drawString(g, font, x + limit - pw, y, str + len - lo, lo);
        }
    }
}

bool YXftFont::supports(unsigned utf32char) const {
    if (utf32char >= 255 && YLocale::UTF8() == false)
        return false;

    // be conservative, only report when all font candidates can do it
    for (int i = 0; i < fFontCount; ++i) {
        if (!XftCharExists(xapp->display(), fFonts[i], utf32char))
            return false;
    }
    return true;
}

YXftFont::TextParts YXftFont::partitions(wchar_t* str, int len, int nparts) const
{
    XftFont ** endFont(fFonts + fFontCount);
    XftFont ** font(nullptr);
    int c = 0;

    while (c < len && str[c] == ' ')
        ++c;

    for (; c < len; ++c) {
        XftFont ** probe(fFonts);

        while (probe < endFont
            && !XftCharExists(xapp->display(), *probe, str[c]))
            ++probe;

        if (probe != font) {
            if (font) {
                TextParts parts = partitions(str + c, len - c, nparts + 1);
                if (font < endFont) {
                    XGlyphInfo extents;
                    textExtents(*font, str, c, &extents);
                    parts[nparts] = TextPart(*font, c, extents.xOff);
                    parts.extent += extents.xOff;
                } else {
                    parts[nparts] = TextPart(nullptr, c, 0);
                    MSG(("glyph not found: %d", str[c]));
                }
                return parts;
            } else {
                font = probe;
            }
        }

        if (probe < endFont) {
            while (c + 1 < len && str[c + 1] == ' ' &&
                   XftCharExists(xapp->display(), *probe, str[c + 1])) {
                ++c;
            }
        }
    }

    TextParts parts(nparts + 1);

    if (font && font < endFont) {
        XGlyphInfo extents;
        textExtents(*font, str, c, &extents);
        parts[nparts] = TextPart(*font, c, extents.xOff);
        parts.extent += extents.xOff;
    } else {
        parts[nparts] = TextPart(nullptr, c, 0);
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
