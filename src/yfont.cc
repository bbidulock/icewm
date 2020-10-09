#include "config.h"

#include "ypaint.h"
#include "yxapp.h"
#include "ywindow.h"
#include "default.h"
#include "yprefs.h"

#include <string.h>

ref<YFont> getXftFont(mslice name, bool antialias, bool xlfd);
ref<YFont> getCoreFont(const char*);

ref<YFont> YFont::getFont(mslice name,
        mslice xftFont, bool antialias) {

    ref<YFont> ret;

#if defined(CONFIG_XFREETYPE) && defined(CONFIG_COREFONTS)
    if (fontPreferFreetype) {
        if (xftFont.nonempty())
            ret = getXftFont(xftFont, antialias, false);
        if (ret == null)
            ret = getXftFont(name, antialias, true);
    }
    if (ret == null)
        ret = getCoreFont(mstring(name));

#elif defined(CONFIG_XFREETYPE)
    if (xftFont.nonempty())
        ret = getXftFont(xftFont, antialias, false);
    if (ret == null)
        ret = getXftFont(name, antialias, true);

#elif defined(CONFIG_COREFONTS)
    ret = getCoreFont(mstring(name));

#else
    (void) antialias;
    if (ONCE)
        warn("Neither XFT fonts nor X11 core fonts are configured!");

#endif

    return ret;
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

// vim: set sw=4 ts=4 et:
