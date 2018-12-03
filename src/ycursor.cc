/*
 * IceWM - Colored cursor support
 *
 * Copyright (C) 2002 The Authors of IceWM
 *
 * initially by Oleastre
 * C++ style implementation by tbf
 */

#include "config.h"
#include "yfull.h"
#include "default.h"
#include "yxapp.h"
#include "wmprog.h"
#include "ypaths.h"

#include <limits.h>
#include <stdlib.h>

#ifdef CONFIG_XPM
#include "X11/xpm.h"
#endif

#ifdef CONFIG_IMLIB
#include <Imlib.h>
extern ImlibData *hImlib;
#endif

#include "ycursor.h"
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
#endif

#ifdef CONFIG_IMLIB
    bool isValid() { return fImage; }
    unsigned int width() const { return fImage ? fImage->rgb_width : 0; }
    unsigned int height() const { return fImage ? fImage->rgb_height : 0; }
    unsigned int hotspotX() const { return fHotspotX; }
    unsigned int hotspotY() const { return fHotspotY; }
#endif

#ifdef CONFIG_GDK_PIXBUF_XLIB
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
#endif

#ifdef CONFIG_IMLIB
    unsigned int fHotspotX, fHotspotY;
    ImlibImage *fImage;
#endif

#ifdef CONFIG_GDK_PIXBUF_XLIB
    bool fValid;
    unsigned int fHotspotX, fHotspotY;
#endif
    operator bool();
};

#ifdef CONFIG_XPM // ================== use libXpm to load the cursor pixmap ===
YCursorPixmap::YCursorPixmap(upath path): fValid(false) {
    fAttributes.colormap  = xapp->colormap();
    fAttributes.closeness = 65535;
    fAttributes.valuemask = XpmColormap|XpmCloseness|
                            XpmReturnPixels|XpmSize|XpmHotspot;
    fAttributes.x_hotspot = 0;
    fAttributes.y_hotspot = 0;

    int const rc(XpmReadFileToPixmap(xapp->display(), desktop->handle(),
                                     (char *)REDIR_ROOT(cstring(path.path()).c_str()), // !!!
                                     &fPixmap, &fMask, &fAttributes));

    if (rc != XpmSuccess)
        warn(_("Loading of pixmap \"%s\" failed: %s"),
               path.string().c_str(), XpmGetErrorString(rc));
    else if (fAttributes.npixels != 2)
        warn("Invalid cursor pixmap: \"%s\" contains too many unique colors",
               path.string().c_str());
    else {
        fBackground.pixel = fAttributes.pixels[0];
        fForeground.pixel = fAttributes.pixels[1];

        XQueryColor(xapp->display(), xapp->colormap(), &fBackground);
        XQueryColor(xapp->display(), xapp->colormap(), &fForeground);

        fValid = true;
    }
}
#endif

#ifdef CONFIG_IMLIB // ================= use Imlib to load the cursor pixmap ===
YCursorPixmap::YCursorPixmap(upath path):
    fHotspotX(0), fHotspotY(0)
{
    cstring cs(path.path());
    fImage = Imlib_load_image(hImlib, (char *)REDIR_ROOT(cs.c_str()));

    if (fImage == NULL) {
        warn(_("Loading of pixmap \"%s\" failed"), cs.c_str());
        return;
    }

    Imlib_render(hImlib, fImage, fImage->rgb_width, fImage->rgb_height);
    fPixmap = (Pixmap)Imlib_move_image(hImlib, fImage);
    fMask = (Pixmap)Imlib_move_mask(hImlib, fImage);

    struct Pixel { // ----------------- find the background/foreground color ---
        bool operator!= (const Pixel& o) {
            return (r != o.r || g != o.g || b != o.b); }
        bool operator!= (const ImlibColor& o) {
            return (r != o.r || g != o.g || b != o.b); }

        unsigned char r,g,b;
    };

    Pixel fg = { 0xFF, 0xFF, 0xFF }, bg = { 0, 0, 0 }, *pp((Pixel*) fImage->rgb_data);
    unsigned ccnt = 0;

    for (unsigned n = fImage->rgb_width * fImage->rgb_height; n > 0; --n, ++pp)
        if (*pp != fImage->shape_color)
            switch (ccnt) {
                case 0:
                    bg = *pp; ++ccnt;
                    break;

                case 1:
                    if (*pp != bg) { fg = *pp; ++ccnt; }
                    break;

                default:
                    if (*pp != bg && *pp != fg) {
                        warn(_("Invalid cursor pixmap: \"%s\" contains too "
                               "much unique colors"), cs.c_str());

                        Imlib_destroy_image(hImlib, fImage);
                        fImage = NULL;
                        return;
                    }
            }

    fForeground.red = (unsigned short)(fg.r << 8); // -- alloc the background/foreground color ---
    fForeground.green = (unsigned short)(fg.g << 8);
    fForeground.blue = (unsigned short)(fg.b << 8);
    XAllocColor(xapp->display(), xapp->colormap(), &fForeground);

    fBackground.red = (unsigned short)(bg.r << 8);
    fBackground.green = (unsigned short)(bg.g << 8);
    fBackground.blue = (unsigned short)(bg.b << 8);
    XAllocColor(xapp->display(), xapp->colormap(), &fBackground);

    // ----------------- find the hotspot by reading the xpm header manually ---
    FILE *xpm = fopen((char *)REDIR_ROOT(cs.c_str()), "rb");
    if (xpm == NULL)
        warn(_("BUG? Imlib was able to read \"%s\""), cs.c_str());

    else {
        while (fgetc(xpm) != '{'); // ----- that's safe since imlib accepted ---

        for (int c;;) switch (c = fgetc(xpm)) {
            case '/':
                if ((c == fgetc(xpm)) == '/') // ------ eat C++ line comment ---
                    while (fgetc(xpm) != '\n');
                else { // -------------------------------- eat block comment ---
                   int pc; do { pc = c; c = fgetc(xpm); }
                   while (c != '/' && pc != '*');
                }
                break;

            case ' ': case '\t': case '\r': case '\n': // ------- whitespace ---
                break;

            case '"': { // ---------------------------------- the XPM header ---
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
#endif

#ifdef CONFIG_GDK_PIXBUF_XLIB
YCursorPixmap::YCursorPixmap(upath /*path*/):
    fPixmap(None), fMask(None),
    fHotspotX(0), fHotspotY(0)
{
}
#endif

YCursorPixmap::~YCursorPixmap() {
    if (fPixmap != None)
        XFreePixmap(xapp->display(), fPixmap);
    if (fMask != None)
        XFreePixmap(xapp->display(), fMask);

#ifdef CONFIG_XPM
    XpmFreeAttributes(&fAttributes);
#endif

#ifdef CONFIG_IMLIB
    Imlib_destroy_image(hImlib, fImage);
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

    if (pixmap.isValid()) { // ============ convert coloured pixmap into a bilevel one ===
        Pixmap bilevel(createMask(pixmap.width(), pixmap.height()));

        // -------------------------- figure out which plane we have to copy ---
        unsigned long pmask(1UL << (xapp->depth() - 1));

        if (pixmap.foreground().pixel &&
            pixmap.foreground().pixel != pixmap.background().pixel)
            while ((pixmap.foreground().pixel & pmask) ==
                   (pixmap.background().pixel & pmask)) pmask >>= 1;
        else if (pixmap.background().pixel)
            while ((pixmap.background().pixel & pmask) == 0) pmask >>= 1;

        GC gc; XGCValues gcv; // ------ copy one plane by using a bilevel GC ---
        gcv.function = (pixmap.foreground().pixel &&
                       (pixmap.foreground().pixel & pmask))
                     ? GXcopyInverted : GXcopy;
        gc = XCreateGC (xapp->display(), bilevel, GCFunction, &gcv);

        XFillRectangle(xapp->display(), bilevel, gc, 0, 0,
                       pixmap.width(), pixmap.height());

        XCopyPlane(xapp->display(), pixmap.pixmap(), bilevel, gc,
                   0, 0, pixmap.width(), pixmap.height(), 0, 0, pmask);
        XFreeGC(xapp->display(), gc);

        // ==================================== allocate a new pixmap cursor ===
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
