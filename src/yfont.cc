#include "config.h"

#include "ypaint.h"
#include "base.h"
#include "default.h"
#include "yprefs.h"
#include "yfontbase.h"
#include "yfontcache.h"

#include <string.h>
#include "ytrace.h"

extern YFontBase* getXftFont(const char* name);
extern YFontBase* getXftFontXlfd(const char* name);
extern YFontBase* getCoreFont(const char*);

YFontCache fontCache;

YFont YFont::operator=(YFontName& fontName) {
#if defined(CONFIG_XFREETYPE) && defined(CONFIG_COREFONTS)
    if (fontPreferFreetype) {
        if (fontName.haveXft()) {
            base = fontCache.lookup(fontName.xftName());
            if (base == nullptr) {
                base = getXftFont(fontName.xftName());
                if (base) {
                    YTraceFont trace(fontName.xftName());
                    fontCache.store(fontName.xftName(), base);
                }
            }
        }
        if (base == nullptr && fontName.haveCore()) {
            base = fontCache.lookup(fontName.coreName());
            if (base == nullptr) {
                base = getXftFontXlfd(fontName.coreName());
                if (base) {
                    YTraceFont trace(fontName.coreName());
                    fontCache.store(fontName.coreName(), base);
                }
            }
        }
    }
    if (base == nullptr && fontName.haveCore()) {
        base = fontCache.lookup(fontName.coreName());
        if (base == nullptr) {
            base = getCoreFont(fontName.coreName());
            if (base) {
                YTraceFont trace(fontName.coreName());
                fontCache.store(fontName.coreName(), base);
            }
        }
    }

#elif defined(CONFIG_XFREETYPE)
    if (fontName.haveXft()) {
        base = fontCache.lookup(fontName.xftName());
        if (base == nullptr) {
            base = getXftFont(fontName.xftName());
            if (base) {
                YTraceFont trace(fontName.xftName());
                fontCache.store(fontName.xftName(), base);
            }
        }
    }
    if (base == nullptr && fontName.haveCore()) {
        base = fontCache.lookup(fontName.coreName());
        if (base == nullptr) {
            base = getXftFontXlfd(fontName.coreName());
            if (base) {
                YTraceFont trace(fontName.coreName());
                fontCache.store(fontName.coreName(), base);
            }
        }
    }

#elif defined(CONFIG_COREFONTS)
    if (fontName.haveCore()) {
        base = fontCache.lookup(fontName.coreName());
        if (base == nullptr) {
            base = getCoreFont(fontName.coreName());
            if (base) {
                YTraceFont trace(fontName.coreName());
                fontCache.store(fontName.coreName(), base);
            }
        }
    }

#else
    if (ONCE)
        warn("Neither XFT fonts nor X11 core fonts are configured!");

#endif

    return *this;
}

int YFontBase::textWidth(char const * str) const {
    return textWidth(str, strlen(str));
}

int YFontBase::multilineTabPos(const char *str) const {
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

YDimension YFontBase::multilineAlloc(const char *str) const {
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

void clearFontCache() {
    fontCache.clear();
}

// vim: set sw=4 ts=4 et:
