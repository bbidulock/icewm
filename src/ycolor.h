#ifndef __YCOLOR_H
#define __YCOLOR_H

class YPixel;
class YColorName;

class YColor {
public:
    YColor() : fPixel(0) { }
    explicit YColor(const char* s) : fPixel(0) { if (s) alloc(s); }
    YColor(const YColor& c) : fPixel(c.fPixel) { }
    YColor& operator=(const YColor& c) { fPixel = c.fPixel; return *this; }

    void alloc(const char* name);
    unsigned long pixel();

    YColor darker();
    YColor brighter();

    bool operator==(YColor& c);
    bool operator!=(YColor& c);
    operator bool() { return fPixel; }
    operator bool() const { return fPixel; }
    void release() { fPixel = 0; }

#ifdef CONFIG_XFREETYPE
    struct _XftColor* xftColor();
#endif

    static YColorName black;
    static YColorName white;

private:
    YColor(YPixel* pixel) : fPixel(pixel) { }


    YPixel* fPixel;

    friend class YColorName;
    friend class YPixel;
};

class YColorName {
public:
    YColorName(const char* clr = 0) : fName(clr), fNamePtr(&fName) { }
    YColorName(const char** clrp) : fName(0), fNamePtr(clrp) { }

    const char* name() const { return *fNamePtr; }
    YColor color() { if (!fColor) alloc(); return fColor; }
    YColor darker() { return color().darker(); }
    YColor brighter() { return color().brighter(); }
    unsigned long pixel() { return color().pixel(); }
    void release() { fColor.release(); }

    operator bool() { return name() && *name(); }
    operator YColor() { return color(); }
    YColor* operator->() { if (!fColor) alloc(); return &fColor; }

    void operator=(const char* clr) { fName = clr; fNamePtr = &fName; release(); }
    void operator=(const char** clrp) { fName = 0; fNamePtr = clrp; release(); }

private:
    void alloc() { fColor.alloc(name()); }

    const char* fName;
    const char** fNamePtr;
    YColor fColor;
};

#endif

// vim: set sw=4 ts=4 et:
