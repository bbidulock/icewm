#ifndef YFONTBASE_H
#define YFONTBASE_H

class YDimension;
class mstring;

class YFontBase {
public:
    virtual ~YFontBase() {}

    virtual bool valid() const = 0;
    virtual int height() const { return ascent() + descent(); }
    virtual int descent() const = 0;
    virtual int ascent() const = 0;
    virtual int textWidth(mstring s) const = 0;
    virtual int textWidth(char const * str, int len) const = 0;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y,
                            char const * str, int len) = 0;
    virtual bool supports(unsigned ucs4char) { return ucs4char <= 255; }

    int textWidth(char const * str) const;
    int multilineTabPos(char const * str) const;
    YDimension multilineAlloc(char const * str) const;
};

#endif
