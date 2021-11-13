#include "config.h"

#ifdef CONFIG_COREFONTS

#include "intl.h"
#include "yxapp.h"
#include "yprefs.h"
#include "ystring.h"
#include "ylocale.h"
#include "ybidi.h"

class YCoreFont : public YFontBase {
public:
    YCoreFont(const char* name);
    virtual ~YCoreFont();

    virtual bool valid() const override { return fFont != nullptr; }
    virtual int descent() const override { return fDescent; }
    virtual int ascent() const override { return fAscent; }
    virtual int textWidth(const char* str, int len) const override;
    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            const char* str, int len, int limit = 0) override;

private:
    XFontStruct * fFont;
    int fAscent, fDescent;
};

#ifdef CONFIG_I18N
class YFontSet : public YFontBase {
public:
    YFontSet(const char* name);
    virtual ~YFontSet();

    virtual bool valid() const override { return fFontSet != nullptr; }
    virtual int descent() const override { return fDescent; }
    virtual int ascent() const override { return fAscent; }
    virtual int textWidth(const char* str, int len) const override;
    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            const char* str, int len, int limit = 0) override;

private:
    static XFontSet guessFontSet(const char* pattern, char*** missing,
                                       int* nMissing, char** defString);
    static char* getNameElement(const char* pat, int index, const char* def);
    static int countChar(const char* str, char ch);
    int textWidth(const YBidi& string) const {
        return XwcTextEscapement(fFontSet, string.string(), string.length());
    }
    int textWidth(wchar_t* string, int num_wchars) const {
        return XwcTextEscapement(fFontSet, string, num_wchars);
    }
    void draw(Graphics& g, int x, int y, wchar_t* text, int count) const {
        XwcDrawString(xapp->display(), g.drawable(), fFontSet, g.handleX(),
                      x - g.xorigin(), y - g.yorigin(), text, count);
    }
    void drawLimitRight(Graphics& g, int x, int y, wchar_t* text, int count, int width, int limit) const;
    void drawLimitLeft(Graphics& g, int x, int y, wchar_t* text, int count, int width, int limit) const;

    XFontSet fFontSet;
    int fAscent, fDescent;
};

char* YFontSet::getNameElement(const char* pat, int index, const char* def) {
    int i = 0, n = 0;
    while (pat[i] && (pat[i] != '-' || ++n < index))
        ++i;

    char* name = nullptr;
    if (n == index && pat[i] == '-') {
        int k = ++i;
        while (pat[k] && pat[k] != '-')
            ++k;
        if (strncmp(pat + i, "*", k - i)) {
            name = newstr(pat + i, k - i);
        }
    }
    if (name == nullptr) {
        name = newstr(def);
    }
    return name;
}

int YFontSet::countChar(const char* str, char ch) {
    int count = 0;
    for (; *str; str++) {
        if (*str == ch)
            count++;
    }
    return count;
}
#endif

/******************************************************************************/

YCoreFont::YCoreFont(char const* name)
    : fFont(nullptr)
    , fAscent(0)
    , fDescent(0)
{
    if (strchr(name, ',')) {
        char* buffer = newstr(name);
        char* save = nullptr;
        for (char* str = strtok_r(buffer, ",", &save);
             str; str = strtok_r(nullptr, ",", &save))
        {
            fFont = XLoadQueryFont(xapp->display(), str);
            if (fFont)
                break;
        }
        delete[] buffer;
    } else {
        fFont = XLoadQueryFont(xapp->display(), name);
    }
    if (fFont) {
        fAscent = fFont->ascent;
        fDescent = fFont->descent;
    }
    else if (testOnce(name, __LINE__)) {
        warn(_("Could not load font \"%s\"."), name);
    }
}

YCoreFont::~YCoreFont() {
    if (fFont) {
        if (xapp)
            XFreeFont(xapp->display(), fFont);
        fFont = nullptr;
    }
}

int YCoreFont::textWidth(const char *str, int len) const {
    return XTextWidth(fFont, str, len);
}

void YCoreFont::drawGlyphs(Graphics& g, int x, int y,
                           const char* str, int len, int limit) {
    XSetFont(xapp->display(), g.handleX(), fFont->fid);

    int tw;
    if (limit == 0 || (tw = textWidth(str, len)) <= limit) {
        XDrawString(xapp->display(), g.drawable(), g.handleX(),
                    x - g.xorigin(), y - g.yorigin(), str, len);
    }
    else {
        const char* el = showEllipsis ? ellipsis() : "";
        int ew = showEllipsis ? textWidth(el, 3) : 0;
        limit -= ew;
        if (limit > 0) {
            int lo = 0, hi = len, pw = 0;
            while (lo < hi) {
                int pv = (lo + hi + 1) / 2;
                pw = textWidth(str, pv);
                if (pw <= limit) {
                    lo = pv;
                } else {
                    hi = pv - 1;
                }
            }
            if (pw > limit)
                lo -= 1;
            if (showEllipsis) {
                size_t len = lo + 3;
                char* buf = new char[len + 1];
                memcpy(buf, str, lo);
                memcpy(buf + lo, el, 3);
                buf[len] = '\0';
                XDrawString(xapp->display(), g.drawable(), g.handleX(),
                            x - g.xorigin(), y - g.yorigin(), buf, lo + 3);
                delete[] buf;
            } else {
                XDrawString(xapp->display(), g.drawable(), g.handleX(),
                            x - g.xorigin(), y - g.yorigin(), str, lo);
            }
        }
    }
}

/******************************************************************************/

#ifdef CONFIG_I18N

YFontSet::YFontSet(const char* name):
    fFontSet(None), fAscent(0), fDescent(0)
{
    int nMissing = 0;
    char **missing = nullptr, *defString = nullptr;

    fFontSet = guessFontSet(name, &missing, &nMissing, &defString);
    if (fFontSet) {
        YTraceFont trace;
        if (trace.tracing()) {
            char** names = nullptr;
            XFontStruct** fonts = nullptr;
            int count = XFontsOfFontSet(fFontSet, &fonts, &names);
            tlog("fonts loaded for fontset %s:", name);
            for (int i = 0; i < count; ++i) {
                if (fonts[i]) {
                    tlog("%2d: %dx%d+%d %s", i, fonts[i]->max_bounds.width,
                        fonts[i]->ascent, fonts[i]->descent, names[i]);
                }
            }
            if (0 < nMissing && testOnce(name, __LINE__)) {
                const size_t bufsize = 321;
                char buf[bufsize];
                snprintf(buf, sizeof buf,
                         _("Missing codesets for fontset \"%s\":"), name);
                size_t len = strlen(buf);
                for (char* miss : YRange<char*>(missing, nMissing)) {
                    size_t mln = strlen(miss);
                    if (len + mln + 2 < bufsize) {
                        buf[len++] = ' ';
                        strlcpy(buf + len, miss, bufsize - len);
                        len += mln;
                    }
                }
                tlog("%s", buf);
            }
        }

        char** names = nullptr;
        XFontStruct** fonts = nullptr;
        int count = XFontsOfFontSet(fFontSet, &fonts, &names);
        count = count > 8 ? 8 : count > 4 ? count - 1 : count;
        long numer = 0, accum = 0, decum = 0;
        for (int i = 0; i < count; ++i) {
            accum += fonts[i]->ascent * (count - i);
            decum += fonts[i]->descent * (count - i);
            numer += (count - i);
        }
        fAscent = int((accum + (numer / 2)) / numer);
        fDescent = int((decum + (numer / 2)) / numer);
    }
    else if (testOnce(name, __LINE__)) {
        warn(_("Could not load fontset \"%s\"."), name);
    }
    if (0 < nMissing) {
        XFreeStringList(missing);
    }
}

YFontSet::~YFontSet() {
    if (fFontSet) {
        if (xapp)
            XFreeFontSet(xapp->display(), fFontSet);
        fFontSet = nullptr;
    }
}

int YFontSet::textWidth(const char *str, int len) const {
    return XmbTextEscapement(fFontSet, str, len);
}

void YFontSet::drawGlyphs(Graphics& g, int x, int y,
                          const char* str, int len, int limit)
{
    YWideString wide(str, len);
    YBidi bidi(wide.data(), wide.length());
    int tw;
    if (limit == 0 || (tw = textWidth(bidi)) <= limit) {
        draw(g, x, y, bidi.string(), bidi.length());
    }
    else if (bidi.isRTL()) {
        drawLimitRight(g, x, y, bidi.string(), bidi.length(), tw, limit);
    } else {
        drawLimitLeft(g, x, y, bidi.string(), bidi.length(), tw, limit);
    }
}

void YFontSet::drawLimitLeft(Graphics& g, int x, int y, wchar_t* text,
                             int count, int width, int limit) const
{
    const char* el = showEllipsis ? ellipsis() : "";
    int ew = showEllipsis ? textWidth(el, 3) : 0;
    if (ew) {
        limit -= ew;
    }
    if (0 < limit && 0 < count) {
        int lo = 0, hi = count, pw = width;
        while (lo < hi) {
            int pv = (lo + hi + 1) / 2;
            pw = textWidth(text, pv);
            if (pw <= limit) {
                lo = pv;
            } else {
                hi = pv - 1;
            }
        }
        if (pw > limit && lo > 0)
            lo -= 1;
        if (0 < ew) {
            YWideString ellips(el, 3);
            for (int i = 0; i < int(ellips.length()); ++i) {
                if (lo < count) {
                    text[lo++] = ellips[i];
                }
            }
        }
        draw(g, x, y, text, lo);
    }
}

void YFontSet::drawLimitRight(Graphics& g, int x, int y, wchar_t* text,
                              int count, int width, int limit) const
{
    const char* el = showEllipsis ? ellipsis() : "";
    int ew = showEllipsis ? textWidth(el, 3) : 0;
    if (0 < limit && 0 < count) {
        int lo = 0, hi = count, pw = width + ew;
        while (lo < hi) {
            int pv = (lo + hi + 1) / 2;
            pw = textWidth(text + count - pv, pv) + ew;
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
            YWideString ellipsis(el, 3);
            size_t ellen = ellipsis.length();
            size_t size = lo + ellen;
            wchar_t* copy = new wchar_t[size + 1];
            memcpy(copy, ellipsis.data(), ellen * sizeof(wchar_t));
            memcpy(copy + ellen, text + count - lo, lo * sizeof(wchar_t));
            copy[size] = 0;
            pw = textWidth(copy, size);
            draw(g, x + limit - pw, y, copy, size);
            delete[] copy;
        } else {
            pw = textWidth(text + count - lo, lo);
            draw(g, x + limit - pw, y, text + count - lo, lo);
        }
    }
}

XFontSet YFontSet::guessFontSet(const char* pattern, char*** missing,
                                      int* nMissing, char** defString) {
    XFontSet fontset(XCreateFontSet(xapp->display(), pattern,
                                    missing, nMissing, defString));
#ifdef CONFIG_I18N
    if (fontset == nullptr || *nMissing == 0)
        return fontset;

    char** names = nullptr;
    XFontStruct** fonts = nullptr;
    int fontCount = XFontsOfFontSet(fontset, &fonts, &names);
    if (fontCount < 1 || names[0][0] != '-' || countChar(names[0], '-') != 14)
        return fontset;

    if (0 < *nMissing) {
        XFreeStringList(*missing);
    }
    XFreeFontSet(xapp->display(), fontset);

    char* family = getNameElement(names[0], 2, "*");
    char* weight = getNameElement(names[0], 3, "medium");
    char* slant = getNameElement(names[0], 4, "r");
    char* pxlsz = getNameElement(names[0], 7, "12");

    const size_t patlen = strlen(pattern);
    const size_t bufsize = patlen + 128;
    char* const buffer = new char[bufsize];
    snprintf(buffer, bufsize,
             "%s,"
             "-*-%s-%s-%s-*-*-%s-*-*-*-*-*-*-*,"
             , pattern
             , family, weight, slant, pxlsz
             );

    delete[] family;
    delete[] pxlsz;
    delete[] slant;
    delete[] weight;

    MSG(("trying fuzzy fontset pattern: \"%s\"", buffer));

    fontset = XCreateFontSet(xapp->display(), buffer,
                             missing, nMissing, defString);
    delete[] buffer;
#endif
    return fontset;
}

#endif // CONFIG_I18N

YFontBase* getCoreFont(const char *name) {
    YFontBase* font;
#ifdef CONFIG_I18N
    if (multiByte && (font = new YFontSet(name)) != nullptr) {
        MSG(("FontSet: %s", name));
        if (font->valid())
            return font;
        delete font;
    }
#endif
    if ((font = new YCoreFont(name)) != nullptr) {
        MSG(("CoreFont: %s", name));
        if (font->valid())
            return font;
        delete font;
    }
    return nullptr;
}

YFontBase* getCoreDefault(const char* name) {
    return getCoreFont("10x20");
}

#endif

// vim: set sw=4 ts=4 et:
