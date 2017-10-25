#ifndef __YCURSOR_H
#define __YCURSOR_H

class YCursorLoader : public refcounted {
public:
    virtual Cursor load(upath path, unsigned int fallback) = 0;
};

class YCursor {
public:

    ~YCursor();

    explicit YCursor(Cursor cursor = None, bool own = false)
        : fCursor(cursor), fOwned(own)
    {
    }

    YCursor(YCursor const & other): fCursor(other.fCursor), fOwned(false)
    {
    }

    YCursor& operator= (Cursor cursor) {
        if (fCursor != cursor) {
            unload();
            fCursor = cursor;
        }
        fOwned = true;
        return *this;
    }

    YCursor& operator= (YCursor const & other) {
        if (this != &other) {
            unload();
            fCursor = other.fCursor;
            fOwned = false;
        }
        return *this;
    }

    static YCursorLoader* newLoader();

    Cursor handle() const { return fCursor; }

    operator Cursor() const { return fCursor; }

private:
    Cursor fCursor;
    bool fOwned;

    void unload();
};

#endif

// vim: set sw=4 ts=4 et:
