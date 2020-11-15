/*
 * IceWM - Colored cursor support
 *
 * Copyright (C) 2002 The Authors of IceWM
 *
 * initially by Oleastre
 * C++ style implementation by tbf
 */

#include "config.h"
#include "yxapp.h"
#include "ycursor.h"
#include "ypaths.h"
#include "ypointer.h"

#ifdef CONFIG_XPM
#include <X11/xpm.h>
#elif defined CONFIG_IMLIB2
#include <Imlib2.h>
typedef Imlib_Image Image;
#else
#error "Need either XPM or Imlib2 for cursors."
#endif

#include "intl.h"

class YCursorPixmap {
public:
    YCursorPixmap(upath path);
    ~YCursorPixmap();

    Pixmap pixmap() const { return fPixmap; }
    Pixmap mask() const { return fMask; }
    const XColor& background() const { return fBackground; }
    const XColor& foreground() const { return fForeground; }

#ifdef CONFIG_XPM

    bool isValid() { return fValid; }
    unsigned int width() const { return fAttributes.width; }
    unsigned int height() const { return fAttributes.height; }
    unsigned int hotspotX() const { return fAttributes.x_hotspot; }
    unsigned int hotspotY() const { return fAttributes.y_hotspot; }

#elif defined CONFIG_IMLIB2

    bool isValid() { return fImage; }
    void release();
    void context() const { imlib_context_set_image(fImage); }
    unsigned int width() const {
        return fImage ? context(), imlib_image_get_width() : 0;
    }
    unsigned int height() const {
        return fImage ? context(), imlib_image_get_height() : 0;
    }
    unsigned int hotspotX() const { return fHotspotX; }
    unsigned int hotspotY() const { return fHotspotY; }

#elif defined CONFIG_GDK_PIXBUF_XLIB

    bool isValid() { return false; }
    unsigned int width() const { return 0; }
    unsigned int height() const { return 0; }
    unsigned int hotspotX() const { return fHotspotX; }
    unsigned int hotspotY() const { return fHotspotY; }

#endif

private:
    Pixmap fPixmap, fMask;
    XColor fForeground, fBackground;

#ifdef CONFIG_XPM

    bool fValid;
    XpmAttributes fAttributes;

#elif defined CONFIG_IMLIB2

    unsigned int fHotspotX, fHotspotY;
    Imlib_Image fImage;

#elif defined CONFIG_GDK_PIXBUF_XLIB

    bool fValid;
    unsigned int fHotspotX, fHotspotY;

#endif
};

#ifdef CONFIG_XPM
//
// === use libXpm to load the cursor pixmap ===
//
YCursorPixmap::YCursorPixmap(upath path): fValid(false) {
    fAttributes.colormap  = xapp->colormap();
    fAttributes.closeness = 65535;
    fAttributes.valuemask = XpmColormap|XpmCloseness|
                            XpmReturnPixels|XpmSize|XpmHotspot;
    fAttributes.x_hotspot = 0;
    fAttributes.y_hotspot = 0;
    fAttributes.depth = 0;
    fAttributes.width = 0;
    fAttributes.height = 0;

    int const rc(XpmReadFileToPixmap(xapp->display(), desktop->handle(),
                                     path.string(),
                                     &fPixmap, &fMask, &fAttributes));

    if (rc != XpmSuccess)
        warn(_("Loading of pixmap \"%s\" failed: %s"),
               path.string(), XpmGetErrorString(rc));
    else if (fAttributes.npixels != 2)
        warn("Invalid cursor pixmap: \"%s\" contains too many unique colors",
               path.string());
    else {
        fBackground.pixel = fAttributes.pixels[0];
        fForeground.pixel = fAttributes.pixels[1];

        XQueryColor(xapp->display(), xapp->colormap(), &fBackground);
        XQueryColor(xapp->display(), xapp->colormap(), &fForeground);

        fValid = true;
    }
}

#elif defined CONFIG_IMLIB2
//
// === use Imlib to load the cursor pixmap ===
//
YCursorPixmap::YCursorPixmap(upath path):
    fHotspotX(0), fHotspotY(0)
{
    mstring cs(path.path());
    fImage = imlib_load_image_immediately_without_cache(cs.c_str());
    if (fImage == nullptr) {
        warn(_("Loading of pixmap \"%s\" failed"), cs.c_str());
        return;
    }
    context();
    imlib_render_pixmaps_for_whole_image(&fPixmap, &fMask);

    unsigned inback = 0;
    unsigned infore = 0;
    DATA32 backgrnd = 0;
    DATA32 foregrnd = 0;
    DATA32* data = imlib_image_get_data_for_reading_only();
    DATA32* stop = data + width() * height();
    bool alpha = imlib_image_has_alpha();
    for (DATA32* p = data; p < stop; ++p) {
        unsigned char a = (unsigned char)((*p >> 24) & 0xFF);
        unsigned char r = (unsigned char)((*p >> 16) & 0xFF);
        unsigned char g = (unsigned char)((*p >> 8) & 0xFF);
        unsigned char b = (unsigned char)(*p & 0xFF);
        unsigned intens = r + g + b;
        if (alpha == 0 || 0 < a) {
            if (inback == 0 || intens < inback) {
                inback = intens;
                backgrnd = *p;
            }
            if (infore == 0 || intens > infore) {
                infore = intens;
                foregrnd = *p;
            }
        }
    }

    fForeground.red = (unsigned short)(((foregrnd >> 16) & 0xFF) << 8);
    fForeground.green = (unsigned short)(((foregrnd >> 8) & 0xFF) << 8);
    fForeground.blue = (unsigned short)((foregrnd & 0xFF) << 8);
    XAllocColor(xapp->display(), xapp->colormap(), &fForeground);

    fBackground.red = (unsigned short)(((backgrnd >> 16) & 0xFF) << 8);
    fBackground.green = (unsigned short)(((backgrnd >> 8) & 0xFF) << 8);
    fBackground.blue = (unsigned short)((backgrnd & 0xFF) << 8);
    XAllocColor(xapp->display(), xapp->colormap(), &fBackground);

    // --- find the hotspot by reading the xpm header manually ---
    FILE* xpm = path.fopen("rb");
    if (xpm == nullptr)
        warn(_("BUG? Imlib was able to read \"%s\""), cs.c_str());
    else {
        while (fgetc(xpm) != '{'); // --- that's safe since imlib accepted ---

        for (int c;;) switch (c = fgetc(xpm)) {
            case '/':
                if ((c = fgetc(xpm)) == '/') // --- eat C++ line comment ---
                    while (fgetc(xpm) != '\n');
                else { // --- eat block comment ---
                   int pc; do { pc = c; c = fgetc(xpm); }
                   while (c != '/' && pc != '*');
                }
                break;

            case ' ': case '\t': case '\r': case '\n': // --- whitespace ---
                break;

            case '"': { // --- the XPM header ---
                unsigned foo; int x, y;
                int tokens = fscanf(xpm, "%u %u %u %u %u %u",
                    &foo, &foo, &foo, &foo, &x, &y);
                if (tokens == 6) {
                    fHotspotX = (x < 0 ? 0 : x);
                    fHotspotY = (y < 0 ? 0 : y);
                } else if (tokens != 4)
                    warn(_("BUG? Malformed XPM header but Imlib "
                           "was able to parse \"%s\""), cs.c_str());

                fclose(xpm);
                return;
            }
            default:
                if (c == EOF)
                    warn(_("BUG? Unexpected end of XPM file but Imlib "
                           "was able to parse \"%s\""), cs.c_str());
                else
                    warn(_("BUG? Unexpected character but Imlib "
                           "was able to parse \"%s\""), cs.c_str());

                fclose(xpm);
                return;
        }
    }
}

#elif defined CONFIG_GDK_PIXBUF_XLIB

YCursorPixmap::YCursorPixmap(upath /*path*/):
    fPixmap(None), fMask(None),
    fHotspotX(0), fHotspotY(0)
{
}
#endif

YCursorPixmap::~YCursorPixmap() {
#ifdef CONFIG_XPM
    if (fPixmap)
        XFreePixmap(xapp->display(), fPixmap);
    if (fMask)
        XFreePixmap(xapp->display(), fMask);
    XpmFreeAttributes(&fAttributes);

#elif defined CONFIG_IMLIB2
    release();
}

void YCursorPixmap::release() {
    if (fImage) {
        context();
        if (fPixmap) {
            imlib_free_pixmap_and_mask(fPixmap);
            fPixmap = fMask = None;
        }
        imlib_free_image();
        fImage = nullptr;
    }
#endif
}

YCursor::~YCursor() {
    unload();
}

class MyCursorLoader : public YCursorLoader {
private:
    ref<YResourcePaths> paths;

    Cursor load(upath path);

public:
    MyCursorLoader()
        : paths(YResourcePaths::subdirs("cursors/"))
    { }

    virtual Cursor load(upath path, unsigned int fallback);
};

static Pixmap createMask(int w, int h) {
    return XCreatePixmap(xapp->display(), desktop->handle(), w, h, 1);
}

Cursor MyCursorLoader::load(upath path) {
    Cursor fCursor = None;
    YCursorPixmap pixmap(path);

    if (pixmap.isValid()) {
        // === convert coloured pixmap into a bilevel one ===
        Pixmap bilevel(createMask(pixmap.width(), pixmap.height()));

        // --- figure out which plane we have to copy ---
        unsigned long pmask(1UL << (xapp->depth() - 1));

        if (pixmap.foreground().pixel &&
            pixmap.foreground().pixel != pixmap.background().pixel)
            while ((pixmap.foreground().pixel & pmask) ==
                   (pixmap.background().pixel & pmask)) pmask >>= 1;
        else if (pixmap.background().pixel)
            while ((pixmap.background().pixel & pmask) == 0) pmask >>= 1;

        GC gc; XGCValues gcv; // --- copy one plane by using a bilevel GC ---
        gcv.function = (pixmap.foreground().pixel &&
                       (pixmap.foreground().pixel & pmask))
                     ? GXcopyInverted : GXcopy;
        gc = XCreateGC (xapp->display(), bilevel, GCFunction, &gcv);

        XFillRectangle(xapp->display(), bilevel, gc, 0, 0,
                       pixmap.width(), pixmap.height());

        XCopyPlane(xapp->display(), pixmap.pixmap(), bilevel, gc,
                   0, 0, pixmap.width(), pixmap.height(), 0, 0, pmask);
        XFreeGC(xapp->display(), gc);

        // === allocate a new pixmap cursor ===
        XColor foreground(pixmap.foreground()),
               background(pixmap.background());

        fCursor = XCreatePixmapCursor(xapp->display(),
                                      bilevel, pixmap.mask(),
                                      &foreground, &background,
                                      pixmap.hotspotX(), pixmap.hotspotY());

        XFreePixmap(xapp->display(), bilevel);
    }
    return fCursor;
}

void YCursor::unload() {
    if (fOwned) {
        fOwned = false;
        if (fCursor) {
            if (xapp) {
                XFreeCursor(xapp->display(), fCursor);
            }
            fCursor = None;
        }
    }
}

YCursorLoader* YCursor::newLoader() {
    return new MyCursorLoader();
}

Cursor MyCursorLoader::load(upath name, unsigned int fallback)
{
    Cursor fCursor = None;

    upath cursors("cursors/");

    for (int i = 0; i < paths->getCount(); i++) {
        upath path = paths->getPath(i) + cursors + name;
        if (path.fileExists()) {
            if ((fCursor = load(path)) != None) {
                /* stop when successful */
                break;
            }
        }
    }

    if (fCursor == None)
        fCursor = XCreateFontCursor(xapp->display(), fallback);

    return fCursor;
}

// vim: set sw=4 ts=4 et:
