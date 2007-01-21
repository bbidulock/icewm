#include "config.h"

#include "ypaint.h"
#include "yxapp.h"
#include "ywindow.h"

#include <string.h>

#if CONFIG_XFREETYPE == 1
static int haveXft = -1;
#endif

extern ref<YFont> getXftFont(ustring name, bool antialias);
extern ref<YFont> getXftFontXlfd(ustring name, bool antialias);
extern ref<YFont> getCoreFont(ustring name);

#ifdef CONFIG_XFREETYPE
ref<YFont> YFont::getFont(ustring name, ustring xftFont, bool antialias) {
#else
ref<YFont> YFont::getFont(ustring name, ustring xftFont, bool) {
#endif
    ref<YFont> font;

#if CONFIG_XFREETYPE == 1
    if (haveXft == -1) {
        int renderEvents, renderErrors;

        haveXft = (XRenderQueryExtension(xapp->display(), &renderEvents, &renderErrors) &&
                   XftDefaultHasRender(xapp->display())) ? 1 : 0;

        MSG(("RENDER extension: %d", haveXft));
        haveXft = 1;
    }
#endif

#ifdef CONFIG_XFREETYPE
#if CONFIG_XFREETYPE == 1
    if (haveXft)
#endif
    {
        if (xftFont != null && xftFont.length() > 0)
            return getXftFont(xftFont, antialias);
        else
            return getXftFontXlfd(name, antialias);
    }
#endif

#ifdef CONFIG_COREFONTS
    return getCoreFont(name);
#else
    return null;
#endif
}

int YFont::textWidth(char const * str) const {
    return textWidth(str, strlen(str));
}

int YFont::multilineTabPos(const char *str) const {
    int tabPos(0);

    for (const char * end(strchr(str, '\n')); end;
         str = end + 1, end = strchr(str, '\n'))
    {
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
         str = end + 1, end = strchr(str, '\n'))
    {
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

YDimension YFont::multilineAlloc(const ustring &str) const {
    cstring cs(str);
    return multilineAlloc(cs.c_str());
}
