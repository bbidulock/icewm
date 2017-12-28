#include "config.h"

#include "ypaint.h"
#include "yxapp.h"
#include "ywindow.h"
#include "default.h"

#include <string.h>

extern ref<YFont> getXftFont(ustring name, bool antialias);
extern ref<YFont> getXftFontXlfd(ustring name, bool antialias);
extern ref<YFont> getCoreFont(const char*);

ref<YFont> YFont::getFont(ustring name, ustring xftFont, bool antialias) {
#if defined(CONFIG_XFREETYPE) && defined(CONFIG_COREFONTS)
    ref<YFont> ret;
    if (fontPreferFreetype) {
        if (xftFont != null && xftFont.length() > 0) ret = getXftFont(xftFont, antialias);
        if (ret != null) return ret;
        ret = getXftFontXlfd(name, antialias);
        if (ret != null) return ret;
    }
    return getCoreFont(cstring(name));
#elif defined(CONFIG_XFREETYPE)
    if (xftFont != null && xftFont.length() > 0) return getXftFont(xftFont, antialias);
    return getXftFontXlfd(name, antialias);
#elif defined(CONFIG_COREFONTS)
    return getCoreFont(cstring(name));
#else
    (void) antialias;
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
    const unsigned tabPos(multilineTabPos(str));
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

// vim: set sw=4 ts=4 et:
