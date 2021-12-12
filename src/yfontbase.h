#ifndef YFONTBASE_H
#define YFONTBASE_H

#include <string.h>

struct YDimension;
class Graphics;
class mstring;

class YFontBase {
public:
    virtual ~YFontBase() {}

    virtual bool valid() const = 0;
    virtual int height() const { return ascent() + descent(); }
    virtual int descent() const = 0;
    virtual int ascent() const = 0;
    virtual int textWidth(wchar_t* str, int len) const = 0;
    virtual int textWidth(const char* str, int len) const = 0;
    virtual void drawGlyphs(Graphics& graphics, int x, int y,
                            const char* str, int len, int limit = 0) = 0;
    virtual void drawGlyphs(Graphics& graphics, int x, int y,
                            wchar_t* str, int len, int limit = 0) = 0;
    virtual bool supports(unsigned ucs4char) const { return ucs4char <= 255; }

    int textWidth(mstring& str) const { return textWidth(str, str.length()); }
    int textWidth(const char* st) const { return textWidth(st, strlen(st)); }
    int multilineTabPos(const char* str) const;
    YDimension multilineAlloc(const char* str) const;
    const char* ellipsis() const;
};

#endif
