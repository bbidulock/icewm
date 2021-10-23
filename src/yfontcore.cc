#include "config.h"

#ifdef CONFIG_COREFONTS

#include "intl.h"
#include "yxapp.h"

#include "yprefs.h"
#include <string.h>
#ifdef CONFIG_I18N
#include <locale.h>
#endif

class YCoreFont : public YFontBase {
public:
    YCoreFont(char const * name);
    virtual ~YCoreFont();

    virtual bool valid() const { return fFont != nullptr; }
    virtual int descent() const { return fFont->max_bounds.descent; }
    virtual int ascent() const { return fFont->max_bounds.ascent; }
    virtual int textWidth(mstring s) const;
    virtual int textWidth(char const * str, int len) const;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            char const * str, int len);

private:
    XFontStruct * fFont;
};

#ifdef CONFIG_I18N
class YFontSet : public YFontBase {
public:
    YFontSet(char const * name);
    virtual ~YFontSet();

    virtual bool valid() const { return fFontSet != nullptr; }
    virtual int descent() const { return fDescent; }
    virtual int ascent() const { return fAscent; }
    virtual int textWidth(mstring s) const;
    virtual int textWidth(char const * str, int len) const;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            char const * str, int len);

private:
    static XFontSet getFontSetWithGuess(char const * pattern, char *** missing,
                                        int * nMissing, char ** defString);

    XFontSet fFontSet;
    int fAscent, fDescent;
};

static char *getNameElement(const char *pattern, unsigned const element) {
    unsigned h(0);
    const char *p(pattern);

    while (*p && (*p != '-' || element != ++h)) ++p;
    return (element == h ? newstr(p + 1, "-") : newstr("*"));
}
#endif

/******************************************************************************/

YCoreFont::YCoreFont(char const * name) {
    fFont = XLoadQueryFont(xapp->display(), name);
    if (fFont == nullptr) {
        if (testOnce(name, __LINE__))
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

int YCoreFont::textWidth(mstring s) const {
    return textWidth(s.c_str(), s.length());
}

int YCoreFont::textWidth(const char *str, int len) const {
    return XTextWidth(fFont, str, len);
}

void YCoreFont::drawGlyphs(Graphics & graphics, int x, int y,
                           char const * str, int len) {
    XSetFont(xapp->display(), graphics.handleX(), fFont->fid);
    XDrawString(xapp->display(), graphics.drawable(), graphics.handleX(),
                x - graphics.xorigin(), y - graphics.yorigin(), str, len);
}

/******************************************************************************/

#ifdef CONFIG_I18N

YFontSet::YFontSet(char const * name):
    fFontSet(None), fAscent(0), fDescent(0)
{
    int nMissing = 0;
    char **missing = 0, *defString = 0;

    fFontSet = getFontSetWithGuess(name, &missing, &nMissing, &defString);

    if (fFontSet == nullptr) {
        if (testOnce(name, __LINE__))
            warn(_("Could not load fontset \"%s\"."), name);
        if (nMissing) XFreeStringList(missing);
    }

    if (fFontSet) {
        if (nMissing && testOnce(name, __LINE__)) {
            warn(_("Missing codesets for fontset \"%s\":"), name);
            for (int n(0); n < nMissing; ++n)
                warn("  %s\n", missing[n]);

            XFreeStringList(missing);
        }

        XFontSetExtents * extents(XExtentsOfFontSet(fFontSet));

        if (extents) {
            fAscent = -extents->max_logical_extent.y;
            fDescent = extents->max_logical_extent.height - fAscent;
        }
    }
}

YFontSet::~YFontSet() {
    if (fFontSet) {
        if (xapp)
            XFreeFontSet(xapp->display(), fFontSet);
        fFontSet = nullptr;
    }
}

int YFontSet::textWidth(mstring s) const {
    return textWidth(s.c_str(), s.length());
}

int YFontSet::textWidth(const char *str, int len) const {
    return XmbTextEscapement(fFontSet, str, len);
}

void YFontSet::drawGlyphs(Graphics & graphics, int x, int y,
                          char const * str, int len) {
    XmbDrawString(xapp->display(), graphics.drawable(),
                  fFontSet, graphics.handleX(),
                  x - graphics.xorigin(), y - graphics.yorigin(), str, len);
}

XFontSet YFontSet::getFontSetWithGuess(char const * pattern, char *** missing,
                                       int * nMissing, char ** defString) {
    XFontSet fontset(XCreateFontSet(xapp->display(), pattern,
                                    missing, nMissing, defString));

    if (fontset && !*nMissing) // --------------- got an exact match ---
        return fontset;

    if (*nMissing) XFreeStringList(*missing);

    if (fontset == nullptr) { // --- get a fallback fontset for pattern analyis ---
        fontset = XCreateFontSet(xapp->display(), pattern,
                                 missing, nMissing, defString);
    }

    if (fontset) { // ----------------------------- get default XLFD ---
        char ** fontnames;
        XFontStruct ** fontstructs;
        XFontsOfFontSet(fontset, &fontstructs, &fontnames);
        pattern = *fontnames;
    }

    char *weight(getNameElement(pattern, 3));
    char *slant(getNameElement(pattern, 4));
    char *pxlsz(getNameElement(pattern, 7));

    // --- build fuzzy font pattern for better matching for various charsets ---
    if (!strcmp(weight, "*")) { delete[] weight; weight = newstr("medium"); }
    if (!strcmp(slant,  "*")) { delete[] slant; slant = newstr("r"); }

    pattern = cstrJoin(pattern, ","
                      "-*-*-", weight, "-", slant, "-*-*-", pxlsz, "-*-*-*-*-*-*-*,"
                      "-*-*-*-*-*-*-", pxlsz, "-*-*-*-*-*-*-*,*", NULL);

    if (fontset) XFreeFontSet(xapp->display(), fontset);

    delete[] pxlsz;
    delete[] slant;
    delete[] weight;

    MSG(("trying fuzzy fontset pattern: \"%s\"", pattern));

    fontset = XCreateFontSet(xapp->display(), pattern,
                             missing, nMissing, defString);
    delete[] pattern;
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
        msg("failed to load fontset '%s'", name);
        delete font;
    }
#endif

    if ((font = new YCoreFont(name)) != nullptr) {
        MSG(("CoreFont: %s", name));
        if (font->valid())
            return font;
        delete font;
    }
    msg("failed to load font '%s'", name);
    return nullptr;
}

YFontBase* getCoreDefault(const char* name) {
    return getCoreFont("10x20");
}

#endif

// vim: set sw=4 ts=4 et:
