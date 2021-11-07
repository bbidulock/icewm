#ifndef YFONTBASE_H
#define YFONTBASE_H

#include <string.h>

class YDimension;
class mstring;

class YFontBase {
public:
    virtual ~YFontBase() {}

    virtual bool valid() const = 0;
    virtual int height() const { return ascent() + descent(); }
    virtual int descent() const = 0;
    virtual int ascent() const = 0;
    virtual int textWidth(char const* str, int len) const = 0;
    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            const char* str, int len, int limit = 0) = 0;
    virtual bool supports(unsigned ucs4char) const { return ucs4char <= 255; }

    int textWidth(mstring& str) const { return textWidth(str, str.length()); }
    int textWidth(char const* st) const { return textWidth(st, strlen(st)); }
    int multilineTabPos(char const * str) const;
    YDimension multilineAlloc(char const * str) const;
    const char* ellipsis() const;
};

#endif
