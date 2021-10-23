#include "config.h"

#include "ypaint.h"
#include "yprefs.h"
#include "yfontcache.h"

#include <string.h>
#include "ytrace.h"

#ifdef CONFIG_XFREETYPE
extern YFontBase* getXftFont(const char* name);
extern YFontBase* getXftFontXlfd(const char* name);
extern YFontBase* getXftDefault(const char* name);
#endif
#ifdef CONFIG_COREFONTS
extern YFontBase* getCoreFont(const char* name);
extern YFontBase* getCoreDefault(const char* name);
#endif

YFontCache fontCache;

void YFont::loadFont(fontloader loader, const char* name) {
    base = fontCache.lookup(name);
    if (base == nullptr) {
        base = loader(name);
        if (base) {
            YTraceFont trace(name);
            fontCache.store(name, base);
        }
    }
}

YFont YFont::operator=(YFontName& name) {
    base = nullptr;

    for (bool tf : { true, false }) {
#ifdef CONFIG_XFREETYPE
        if (tf == fontPreferFreetype) {
            if (base == nullptr && name.haveXft()) {
                loadFont(getXftFont, name.xftName());
            }
            if (base == nullptr && name.haveCore()) {
                loadFont(getXftFontXlfd, name.coreName());
            }
        }
#endif
#ifdef CONFIG_COREFONTS
        if (tf) {
            if (base == nullptr && name.haveCore()) {
                loadFont(getCoreFont, name.coreName());
            }
        }
#endif
    }
#ifdef CONFIG_XFREETYPE
    if (base == nullptr) {
        loadFont(getXftDefault,
                 name.haveXft() ? name.xftName() :
                 name.haveCore() ? name.coreName() : "");
    }
#endif
#ifdef CONFIG_COREFONTS
    if (base == nullptr) {
        loadFont(getCoreDefault, name.haveCore() ? name.coreName() : "");
    }
#endif

#ifndef CONFIG_XFREETYPE
#ifndef CONFIG_COREFONTS
    if (ONCE)
        warn("Neither XFT fonts nor X11 core fonts are configured!");
#endif
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
