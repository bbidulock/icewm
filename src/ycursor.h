#ifndef __YCURSOR_H
#define __YCURSOR_H

#include "config.h"
#include "upath.h"

class YCursor {
public:
#if 0
    YCursor(char const * filename, int hotSpotX, int hotSpotY):
        fOwned(true)
    {
        load(filename, hotSpotX, hotSpotY);
    }
#endif

    ~YCursor();

    YCursor(Cursor const cursor = None):
        fCursor(cursor), fOwned(false) {
    }

    YCursor(YCursor & other): fCursor(other.fCursor), fOwned(true)
    {
        other.fOwned = false;
    }

    YCursor(YCursor const & other): fCursor(other.fCursor), fOwned(false)
    {
    }

    YCursor& operator= (YCursor & other) {
        fCursor = other.fCursor;
        fOwned = true;
        other.fOwned = false;
        return *this;
    }

    YCursor& operator= (YCursor const & other) {
        fCursor = other.fCursor;
        fOwned = false;
        return *this;
    }

    void load(upath name, unsigned int fallback);
    Cursor handle() const { return fCursor; }

private:
    Cursor fCursor;
    bool fOwned;

    void load(upath path);
};

#endif
