#ifndef YCURSOR_H
#define YCURSOR_H

#include <X11/X.h>

class YCursor {
public:
    void init(const char* p, unsigned g) { cursor = 0; glyph = g; path = p; }
    operator Cursor() { return cursor ? Cursor(cursor) : load(); }
    void discard();
    ~YCursor() { discard(); }

private:
    static Cursor load(const char* path);

    unsigned cursor;
    unsigned glyph;
    const char* path;
    Cursor load();
};

#endif

// vim: set sw=4 ts=4 et:
