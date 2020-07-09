/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ycolor.h"
#include "yxapp.h"
#include "ascii.h"

#ifdef CONFIG_XFREETYPE
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#define INIT_XFREETYPE(Member, Value) , Member(Value)
#else
#define INIT_XFREETYPE(Member, Value)
#endif

#include "intl.h"
#include <stdlib.h> // for strtol

YColorName YColor::black("rgb:00/00/00");
YColorName YColor::white("rgb:FF/FF/FF");

static inline Display* display()  { return xapp->display(); }
static inline Colormap colormap() { return xapp->colormap(); }
static inline Visual*  visual()   { return xapp->visual(); }
static inline unsigned vdepth()   { return xapp->depth(); }

typedef unsigned short Word;

class YPixel {
public:
    YPixel(unsigned long pix, unsigned long col) :
        fPixel(pix), fColor(col), fBright(), fDarken()
        INIT_XFREETYPE(fXftColor, nullptr) { }

    ~YPixel();

    unsigned long pixel() const { return fPixel; }
    unsigned long color() const { return fColor; }
    YColor brighter();
    YColor darker();

    static unsigned long
    color(Word r, Word g, Word b, Word a)
    {
        if (sizeof(unsigned long) < 8)
            return (r & 0xFF00) << 16 | (g & 0xFF00) << 8 | ((b >> 8) & 0xFF)
                 | ((a & 0xFF) << 24);
        else
            return (unsigned long) r << 32 | (unsigned long) g << 16 | b
                 | (unsigned long) (a & 0xFF) << 48;
    }
    unsigned short red() const {
        if (sizeof(unsigned long) < 8)
            return (fColor >> 16 & 0xFF00);
        else
            return (fColor >> 32 & 0xFFFF);
    }
    unsigned short green() const {
        if (sizeof(unsigned long) < 8)
            return (fColor >>  8 & 0xFF00);
        else
            return (fColor >> 16 & 0xFFFF);
    }
    unsigned short blue() const {
        if (sizeof(unsigned long) < 8)
            return (fColor <<  8 & 0xFF00);
        else
            return (fColor & 0xFFFF);
    }
    unsigned short alpha() const {
        if (sizeof(unsigned long) < 8)
            return (fColor >> 24 & 0xFF);
        else
            return (fColor >> 48 & 0xFF);
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
        if (pixels.isEmpty() || pixels[0]->pixel() != 0) {
            pixels.insert(0, new YPixel(xapp ? xapp->black() : 0, 0));
        }
        return pixels[0];
    }

    YPixel* get(Word r, Word g, Word b, Word a = 0) {
        if (a == 0 && xapp->alpha() && validOpacity(fOpacity)) {
            a = opacityAlpha(fOpacity);
        }
        unsigned long color = YPixel::color(r, g, b, a);
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
        unsigned long allocated = alloc(r, g, b, a);
        if (allocated == 0)
            return black();
        else {
            YPixel* pixel = new YPixel(allocated, color);
            pixels.insert(lo, pixel);
            return pixel;
        }
    }

    void setOpacity(int opacity) {
        fOpacity = opacity;
    }

    int getOpacity() {
        return fOpacity;
    }

    YPixelCache() :
        fOpacity(0)
    {
    }

private:
    unsigned long alloc(Word r, Word g, Word b, Word a);

    int fOpacity;
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

YColor::YColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    : fPixel(nullptr)
{
    int previous = cache.getOpacity();
    cache.setOpacity(a ? (a * MaxOpacity / 255) : MaxOpacity);
    fPixel = cache.get(r << 8 | r, g << 8 | g, b << 8 | b);
    cache.setOpacity(previous);
}

void YColor::alloc(const char* name, int opacity) {
    if (name && name[0]) {
        if (*name == '[') {
            long opaq(strtol(++name, (char **) &name, 10));
            if (inrange<long>(opaq, MinOpacity, MaxOpacity)) {
                opacity = int(opaq);
            }
            name += (*name == ']');
        }
        cache.setOpacity(opacity);
        alloc(name);
        cache.setOpacity(0);
    }
}

void YColor::alloc(const char* name) {
    if (name && 0 == strncmp(name, "rgba:", 5)) {
        unsigned short color[4] = { 0, 0, 0, 0 };
        bool valid = false;
        int k = 0, n = 12;
        for (const char* str = name + 5; -4 <= n; ++str, n -= 4) {
            const char c = *str;
            const int hex = ASCII::hexDigit(c);
            if (0 <= hex) {
                if (0 <= n) {
                    color[k] |= hex << n;
                }
            }
            else {
                while (0 <= n && n <= 8) {
                    color[k] |= (color[k] >> (12 - n));
                    n = max(-4, n - (12 - n));
                }
                if (c != '/' || n != -4 || ++k == 4) {
                    valid = (c == 0 && k == 3 && n == -4);
                    break;
                }
                n = 16;
            }
        }
        if (valid) {
            int previous = cache.getOpacity();
            if (previous == 0) {
                cache.setOpacity(MaxOpacity * color[3] / 0xFFFF);
            }
            fPixel = cache.get(color[0], color[1], color[2]);
            cache.setOpacity(previous);
            return;
        }
        else if (testOnce(name, __LINE__)) {
            msg(_("Could not parse color \"%s\""), name);
        }
    }
    else if (nonempty(name)) {
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
            return alloc("#" + str);
        if (str.match("^#[0-9a-f]{5}$", "i") != null)
            return alloc(str + &name[5]);

        mstring rgb(str.match("rgb:[0-9a-f]{2}/[0-9a-f]{2}/[0-9a-f]{2}", "i"));
        if (rgb != null)
            return alloc(rgb);
        rgb = str.match("[0-9a-f]{2}/[0-9a-f]{2}/[0-9a-f]{2}", "i");
        if (rgb != null)
            return alloc("rgb:" + rgb);
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
YPixelCache::alloc(Word r, Word g, Word b, Word a)
{
    XColor color = { 0, r, g, b, (DoRed | DoGreen | DoBlue), 0 };
    Visual *visual = ::visual();

    if (visual->c_class == TrueColor) {
        unsigned depth = vdepth(), high, unused = 0;

        int red_shift = int(lowbit(visual->red_mask));
        int red_prec = int(highbit(visual->red_mask)) - red_shift + 1;
        int green_shift = int(lowbit(visual->green_mask));
        int green_prec = int(highbit(visual->green_mask)) - green_shift + 1;
        int blue_shift = int(lowbit(visual->blue_mask));
        int blue_prec = int(highbit(visual->blue_mask)) - blue_shift + 1;

        high = 1
             + highbit(visual->red_mask | visual->green_mask | visual->blue_mask);
        if (high < depth) {
            if (a) {
                unused = a;
            }
            else if (validOpacity(fOpacity)) {
                unused = (((1U << (depth - high)) - 1U) * fOpacity) / MaxOpacity;
            }
            else {
                unused = ((1U << (depth - high)) - 1U);
            }
            unused <<= high;
        }

        color.pixel = (unused +
                       ((color.red >> (16 - red_prec)) << red_shift) +
                       ((color.green >> (16 - green_prec)) << green_shift) +
                       ((color.blue >> (16 - blue_prec)) << blue_shift));

    }
    else if (Success == XAllocColor(display(), colormap(), &color)) {
        unsigned j, ncells;
        double d = 65536. * 65536. * 24;
        XColor clr;
        unsigned long pix;
        unsigned long d_red, d_green, d_blue;
        double u_red, u_green, u_blue;

        pix = 0xFFFFFFFF;
        ncells = unsigned(DisplayCells(display(), xapp->screen()));
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
                                    darken(blue()),
                                    alpha()));
    return fDarken;
}

YColor YPixel::brighter() {
    if (fBright == false)
        fBright = YColor( cache.get(bright(red()),
                                    bright(green()),
                                    bright(blue()),
                                    alpha()));
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

unsigned char YColor::red()   { return fPixel->red()   >> 8; }
unsigned char YColor::green() { return fPixel->green() >> 8; }
unsigned char YColor::blue()  { return fPixel->blue()  >> 8; }
unsigned char YColor::alpha() { return fPixel->alpha() >> 8; }

// vim: set sw=4 ts=4 et:
