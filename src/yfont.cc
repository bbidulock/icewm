#include "config.h"

#include "ypaint.h"
#include "yapp.h"
#include "ywindow.h"

static int haveXft = -1;

extern YFont *getXftFont(const char *name);
extern YFont *getCoreFont(const char *name);

#ifdef CONFIG_XFREETYPE
YFont * YFont::getFont(const char *name, bool antialias) {
#else
YFont * YFont::getFont(const char *name, bool) {
#endif
    YFont * font;

    if (haveXft == -1) {
#if CONFIG_XFREETYPE == 1
        int renderEvents, renderErrors;

        haveXft = (XRenderQueryExtension(display(), &renderEvents, &renderErrors) &&
                   XftDefaultHasRender(display())) ? 1 : 0;

        MSG(("RENDER extension: %d", haveXft));
#else
        haveXft = 1;
#endif
    }

#ifdef CONFIG_XFREETYPE
    if (haveXft) {
        MSG(("XftFont: %s", name));
        return getXftFont(name);
    }
#endif

#ifdef CONFIG_COREFONTS
    return getCoreFont(name);
#else
    return 0;
#endif
}

int YFont::textWidth(char const * str) const {
    return textWidth(str, strlen(str));
}

int YFont::multilineTabPos(const char *str) const {
    int tabPos(0);

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
