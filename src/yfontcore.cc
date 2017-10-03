#include "config.h"

#ifdef CONFIG_COREFONTS

#include "ypaint.h"
#include "sysdep.h"

#include "intl.h"
#include "yxapp.h"

#include "yprefs.h"
#include <string.h>
#ifdef CONFIG_I18N
#include <locale.h>
#endif

class YCoreFont : public YFont {
public:
    YCoreFont(char const * name);
    virtual ~YCoreFont();

    virtual bool valid() const { return (NULL != fFont); }
    virtual int descent() const { return fFont->max_bounds.descent; }
    virtual int ascent() const { return fFont->max_bounds.ascent; }
    virtual int textWidth(const ustring &s) const;
    virtual int textWidth(char const * str, int len) const;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            char const * str, int len);

private:
    XFontStruct * fFont;
};

#ifdef CONFIG_I18N
class YFontSet : public YFont {
public:
    YFontSet(char const * name);
    virtual ~YFontSet();

    virtual bool valid() const { return (None != fFontSet); }
    virtual int descent() const { return fDescent; }
    virtual int ascent() const { return fAscent; }
    int textWidth(const ustring &s) const;
    virtual int textWidth(char const * str, int len) const;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            char const * str, int len);

private:
    static XFontSet getFontSetWithGuess(char const * pattern, char *** missing,
                                        int * nMissing, char ** defString);

    XFontSet fFontSet;
    int fAscent, fDescent;
};
#endif

static char *getNameElement(const char *pattern, unsigned const element) {
    unsigned h(0);
    const char *p(pattern);

    while (*p && (*p != '-' || element != ++h)) ++p;
    return (element == h ? newstr(p + 1, "-") : newstr("*"));
}

/******************************************************************************/

YCoreFont::YCoreFont(char const * name) {
    if (NULL == (fFont = XLoadQueryFont(xapp->display(), name))) {
        warn(_("Could not load font \"%s\"."), name);

        if (NULL == (fFont = XLoadQueryFont(xapp->display(), "fixed")))
            warn(_("Loading of fallback font \"%s\" failed."), "fixed");
    }
}

YCoreFont::~YCoreFont() {
    if (fFont != 0) {
        if (xapp != 0)
            XFreeFont(xapp->display(), fFont);
        fFont = 0;
    }
}

int YCoreFont::textWidth(const ustring &s) const {
    cstring cs(s);
    return textWidth(cs.c_str(), cs.c_str_len());
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
fFontSet(None), fAscent(0), fDescent(0) {
    int nMissing;
    char **missing, *defString;

    fFontSet = getFontSetWithGuess(name, &missing, &nMissing, &defString);

    if (None == fFontSet) {
        warn(_("Could not load fontset \"%s\"."), name);
        if (nMissing) XFreeStringList(missing);

        fFontSet = XCreateFontSet(xapp->display(), "fixed",
                                  &missing, &nMissing, &defString);

        if (None == fFontSet)
            warn(_("Loading of fallback font \"%s\" failed."), "fixed");
    }

    if (fFontSet) {
        if (nMissing) {
            warn(_("Missing codesets for fontset \"%s\":"), name);
            for (int n(0); n < nMissing; ++n)
                warn("  %s\n", missing[n]);

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
    if (NULL != fFontSet) {
        if (xapp != 0)
            XFreeFontSet(xapp->display(), fFontSet);
        fFontSet = 0;
    }
}

int YFontSet::textWidth(const ustring &s) const {
    cstring cs(s);
    return textWidth(cs.c_str(), cs.c_str_len());
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

    if (None != fontset && !*nMissing) // --------------- got an exact match ---
        return fontset;

    if (*nMissing) XFreeStringList(*missing);

    if (None == fontset) { // --- get a fallback fontset for pattern analyis ---
        fontset = XCreateFontSet(xapp->display(), pattern,
                                 missing, nMissing, defString);
    }

    if (None != fontset) { // ----------------------------- get default XLFD ---
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

ref<YFont> getCoreFont(const char *name) {
    ref<YFont> font;
#ifdef CONFIG_I18N
    if (multiByte && font.init(new YFontSet(name)) != null) {
        MSG(("FontSet: %s", name));
        if (font->valid())
            return font;
        font = null;
        msg("failed to load fontset '%s'", name);
    }
#endif

    if (font.init(new YCoreFont(name)) != null) {
        MSG(("CoreFont: %s", name));
        if (font->valid())
            return font;
        font = null;
    }
    msg("failed to load font '%s'", name);
    return null;
}

#endif

// vim: set sw=4 ts=4 et:
