#include "config.h"

#ifdef CONFIG_COREFONTS

#include "intl.h"
#include "yxapp.h"
#include "yprefs.h"
#ifdef CONFIG_I18N
#include <locale.h>
#endif

class YCoreFont : public YFontBase {
public:
    YCoreFont(char const * name);
    virtual ~YCoreFont();

    virtual bool valid() const override { return fFont != nullptr; }
    virtual int descent() const override { return fDescent; }
    virtual int ascent() const override { return fAscent; }
    virtual int textWidth(char const * str, int len) const override;
    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            const char* str, int len, int limit = 0) override;

private:
    XFontStruct * fFont;
    int fAscent, fDescent;
};

#ifdef CONFIG_I18N
class YFontSet : public YFontBase {
public:
    YFontSet(char const * name);
    virtual ~YFontSet();

    virtual bool valid() const override { return fFontSet != nullptr; }
    virtual int descent() const override { return fDescent; }
    virtual int ascent() const override { return fAscent; }
    virtual int textWidth(char const * str, int len) const override;
    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            const char* str, int len, int limit = 0) override;

private:
    static XFontSet getFontSetWithGuess(char const * pattern, char *** missing,
                                        int * nMissing, char ** defString);
    static char* getNameElement(const char* pat, int index, const char* def);
    static int countChar(const char* str, char ch);
    int textWidth(wchar_t* string, int num_wchars) const {
        return XwcTextEscapement(fFontSet, string, num_wchars);
    }
    void draw(Graphics& g, int x, int y, const char* str, int len) const {
        XmbDrawString(xapp->display(), g.drawable(), fFontSet, g.handleX(),
                      x - g.xorigin(), y - g.yorigin(), str, len);
    }
    void draw(Graphics& g, int x, int y, wchar_t* text, int count) const {
        XwcDrawString(xapp->display(), g.drawable(), fFontSet, g.handleX(),
                      x - g.xorigin(), y - g.yorigin(), text, count);
    }

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

YFontSet::YFontSet(char const * name):
    fFontSet(None), fAscent(0), fDescent(0)
{
    int nMissing = 0;
    char **missing = nullptr, *defString = nullptr;

    fFontSet = getFontSetWithGuess(name, &missing, &nMissing, &defString);
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
    const int maxwide = 987;
    wchar_t wides[maxwide];
    int numwides = 0;
    for (int i = 0; i < len && str[i] && numwides < maxwide; ) {
        int k = mbtowc(&wides[numwides], str + i, size_t(len - i));
        if (k < 1) {
            i++;
        } else {
            i += k;
            numwides++;
        }
    }
    int tw;
    if (limit == 0 || (tw = textWidth(wides, numwides)) <= limit) {
        draw(g, x, y, wides, numwides);
    }
    else {
        const char* el = showEllipsis ? ellipsis() : "";
        int ew = showEllipsis ? textWidth(el, 3) : 0;
        limit -= ew;
        if (limit > 0) {
            int lo = 0, hi = numwides;
            while (lo < hi) {
                int pv = (lo + hi + 1) / 2;
                int pw = textWidth(wides, pv);
                if (pw <= limit) {
                    lo = pv;
                } else {
                    hi = pv - 1;
                }
            }
            if (0 < ew) {
                for (int i = 0; i < 3 && el[i] && lo < maxwide; ) {
                    int k = mbtowc(&wides[lo], el + i, size_t(3 - i));
                    if (k < 1) {
                        break;
                    } else {
                        i += k;
                        lo++;
                    }
                }
            }
            draw(g, x, y, wides, lo);
        }
    }
}

XFontSet YFontSet::getFontSetWithGuess(char const * pattern, char *** missing,
                                       int * nMissing, char ** defString) {
    XFontSet fontset(XCreateFontSet(xapp->display(), pattern,
                                    missing, nMissing, defString));
    if (fontset == nullptr || *nMissing == 0)
        return fontset;

    char** names = nullptr;
    XFontStruct** fonts = nullptr;
    int fontCount = XFontsOfFontSet(fontset, &fonts, &names);
    if (fontCount < 1 || names[0][0] != '-')
        return fontset;
    char* initial = names[0];
    if (countChar(initial, '-') != 14)
        return fontset;

    if (0 < *nMissing) {
        XFreeStringList(*missing);
    }
    XFreeFontSet(xapp->display(), fontset);

    char* weight(getNameElement(pattern, 3, "medium"));
    char* slant(getNameElement(pattern, 4, "r"));
    char* pxlsz(getNameElement(pattern, 7, "12"));

    const size_t patlen = strlen(pattern);
    const size_t bufsize = patlen + 128;
    char* buffer = new char[bufsize];
    snprintf(buffer, bufsize,
             "%s,"
             "-*-*-%s-%s-*-*-%s-*-*-*-*-*-*-*,"
             , pattern
             , weight, slant, pxlsz
             );

    delete[] pxlsz;
    delete[] slant;
    delete[] weight;

    MSG(("trying fuzzy fontset pattern: \"%s\"", buffer));

    fontset = XCreateFontSet(xapp->display(), buffer,
                             missing, nMissing, defString);
    delete[] buffer;
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
