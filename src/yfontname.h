#ifndef YFONTNAME_H
#define YFONTNAME_H

class YFontName {
public:
    YFontName(const char** c, const char** x) : core(c), xfte(x) { }
    const char* coreName() { return *core; }
    const char* xftName() { return *xfte; }
    bool haveCore() const { return *core && **core; }
    bool haveXft() const { return *xfte && **xfte; }
    bool nonempty() const { return haveCore() || haveXft(); }
private:
    const char** core;
    const char** xfte;
};

#endif
