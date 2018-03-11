/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ycolor.h"
#include "yxapp.h"

#ifdef CONFIG_XFREETYPE
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#define INIT_XFREETYPE(Member, Value) , Member(Value)
#else
#define INIT_XFREETYPE(Member, Value)
#endif

#include "intl.h"

YColorName YColor::black("rgb:00/00/00");
YColorName YColor::white("rgb:FF/FF/FF");

static inline Display* display()  { return xapp->display(); }
static inline Colormap colormap() { return xapp->colormap(); }
static inline Visual*  visual()   { return xapp->visual(); }

class YPixel {
public:
    YPixel(unsigned long pix, unsigned long col) :
        fPixel(pix), fColor(col), fBright(), fDarken()
        INIT_XFREETYPE(fXftColor, 0) { }

    ~YPixel();

    unsigned long pixel() const { return fPixel; }
    unsigned long color() const { return fColor; }
    YColor brighter();
    YColor darker();

    static unsigned long
    color(unsigned short r, unsigned short g, unsigned short b)
    {
        if (sizeof(unsigned long) < 8)
            return (r & 0xFFE0) << 16 | (g & 0xFFE0) <<  5 | ((b >> 6) & 0x3FF);
        else
            return (unsigned long) r << 32 | (unsigned long) g << 16 | b;
    }
    unsigned short red() const {
        if (sizeof(unsigned long) < 8)
            return (fColor >> 16 & 0xFFE0);
        else
            return (fColor >> 32 & 0xFFFF);
    }
    unsigned short green() const {
        if (sizeof(unsigned long) < 8)
            return (fColor >>  5 & 0xFFE0);
        else
            return (fColor >> 16 & 0xFFFF);
    }
    unsigned short blue() const {
        if (sizeof(unsigned long) < 8)
            return (fColor <<  6 & 0xFFC0);
        else
            return (fColor & 0xFFFF);
    }

private:
    unsigned long fPixel;
    unsigned long fColor;
    YColor fBright;
    YColor fDarken;

#ifdef CONFIG_XFREETYPE
    XftColor* fXftColor;
    XftColor* allocXft();
public:
    XftColor* xftColor() { return fXftColor ? fXftColor : allocXft(); }
#endif
};

static class YPixelCache {
public:
    YPixel* black() {
        if (pixels.getCount() == 0 || pixels[0]->pixel() != 0) {
            pixels.insert(0, new YPixel(xapp ? xapp->black() : 0, 0));
        }
        return pixels[0];
    }

    YPixel* get(unsigned short r, unsigned short g, unsigned short b) {
        unsigned long color = YPixel::color(r, g, b);
        int lo = 0, hi = pixels.getCount();

        while (lo < hi) {
            const int pv = (lo + hi) / 2;
            YPixel* pivot = pixels[pv];

            if (color < pivot->color()) {
                lo = pv + 1;
            }
            else if (pivot->color() < color) {
                hi = pv;
            }
            else {
                return pivot;
            }
        }
        unsigned long allocated = alloc(r, g, b);
        if (allocated == 0)
            return black();
        else {
            YPixel* pixel = new YPixel(allocated, color);
            pixels.insert(lo, pixel);
            return pixel;
        }
    }

private:
    unsigned long alloc(unsigned short r, unsigned short g, unsigned short b);

    YObjectArray<YPixel> pixels;

} cache;

YPixel::~YPixel() {
#ifdef CONFIG_XFREETYPE
    if (fXftColor) {
        if (xapp) {
            XftColorFree(display(), visual(), colormap(), fXftColor);
        }
        delete fXftColor;
    }
#endif
}

void YColor::alloc(const char* name) {
    if (name && name[0]) {
        XColor color;
        if (XParseColor(display(), colormap(), name, &color)) {
            fPixel = cache.get(color.red, color.green, color.blue);
            return;
        }

        if (testOnce(name, __LINE__))
            msg(_("Could not parse color \"%s\""), name);

        if (strncmp(name, "rgb:rgb:", 8) == 0)
            return alloc(name + 4);

        mstring str(name);
        if (str.match("^[0-9a-f]{6}$", "i") != null)
            return alloc(cstring("#" + str));
        if (str.match("^#[0-9a-f]{5}$", "i") != null)
            return alloc(cstring(str + &name[5]));

        mstring rgb(str.match("rgb:[0-9a-f]{2}/[0-9a-f]{2}/[0-9a-f]{2}", "i"));
        if (rgb != null)
            return alloc(cstring(rgb));
        rgb = str.match("[0-9a-f]{2}/[0-9a-f]{2}/[0-9a-f]{2}", "i");
        if (rgb != null)
            return alloc(cstring("rgb:" + rgb));
    }
    fPixel = cache.black();
}

#ifdef CONFIG_XFREETYPE
XftColor* YPixel::allocXft() {
    fXftColor = new XftColor;
    XRenderColor color = { red(), green(), blue(), 0xffff };
    XftColorAllocValue(display(), visual(), colormap(), &color, fXftColor);
    return fXftColor;
}

XftColor* YColor::xftColor() {
    return fPixel->xftColor();
}
#endif

unsigned long
YPixelCache::alloc(unsigned short r, unsigned short g, unsigned short b)
{
    XColor color = { 0, r, g, b, (DoRed | DoGreen | DoBlue), 0 };
    Visual *visual = ::visual();

    if (visual->c_class == TrueColor) {
        int padding, unused;
        int depth = visual->bits_per_rgb;

        int red_shift = lowbit(visual->red_mask);
        int red_prec = highbit(visual->red_mask) - red_shift + 1;
        int green_shift = lowbit(visual->green_mask);
        int green_prec = highbit(visual->green_mask) - green_shift + 1;
        int blue_shift = lowbit(visual->blue_mask);
        int blue_prec = highbit(visual->blue_mask) - blue_shift + 1;

        /* Shifting by >= width-of-type isn't defined in C */
        if (depth >= 32)
            padding = 0;
        else
            padding = ((~(unsigned int)0)) << depth;

        unused = ~ (visual->red_mask | visual->green_mask | visual->blue_mask | padding);

        color.pixel = (unused +
                       ((color.red >> (16 - red_prec)) << red_shift) +
                       ((color.green >> (16 - green_prec)) << green_shift) +
                       ((color.blue >> (16 - blue_prec)) << blue_shift));

    }
    else if (Success == XAllocColor(display(), colormap(), &color)) {
        int j, ncells;
        double d = 65536. * 65536. * 24;
        XColor clr;
        unsigned long pix;
        long d_red, d_green, d_blue;
        double u_red, u_green, u_blue;

        pix = 0xFFFFFFFF;
        ncells = DisplayCells(display(), xapp->screen());
        for (j = 0; j < ncells; j++) {
            clr.pixel = j;
            XQueryColor(display(), colormap(), &clr);

            d_red   = color.red   - clr.red;
            d_green = color.green - clr.green;
            d_blue  = color.blue  - clr.blue;

            u_red   = 3UL * (d_red   * d_red);
            u_green = 4UL * (d_green * d_green);
            u_blue  = 2UL * (d_blue  * d_blue);

            double d1 = u_red + u_blue + u_green;

            if (pix == 0xFFFFFFFF || d1 < d) {
                pix = j;
                d = d1;
            }
        }
        if (pix != 0xFFFFFFFF) {
            clr.pixel = pix;
            XQueryColor(display(), colormap(), &clr);
            /*DBG(("color=%04X:%04X:%04X, match=%04X:%04X:%04X\n",
                   color.red, color.blue, color.green,
                   clr.red, clr.blue, clr.green));*/
            color = clr;
        }
        if (XAllocColor(display(), colormap(), &color) == 0) {
            if (color.red + color.green + color.blue >= 32768)
                color.pixel = xapp->white();
            else
                color.pixel = xapp->black();
        }
    }
    return color.pixel;
}

inline unsigned short darken(unsigned short color) {
    return color * 2 / 3;
}

inline unsigned short bright(unsigned short color) {
    return min(color * 4 / 3, 0xFFFF);
}

YColor YPixel::darker() {
    if (fDarken == false)
        fDarken = YColor( cache.get(darken(red()),
                                    darken(green()),
                                    darken(blue())));
    return fDarken;
}

YColor YPixel::brighter() {
    if (fBright == false)
        fBright = YColor( cache.get(bright(red()),
                                    bright(green()),
                                    bright(blue())));
    return fBright;
}

unsigned long YColor::pixel() {
    return fPixel->pixel();
}

YColor YColor::darker() {
    return fPixel->darker();
}

YColor YColor::brighter() {
    return fPixel->brighter();
}

bool YColor::operator==(YColor& c) {
    return fPixel && c.fPixel && fPixel->pixel() == c.fPixel->pixel();
}

bool YColor::operator!=(YColor& c) {
    return !(*this == c);
}

// vim: set sw=4 ts=4 et:
