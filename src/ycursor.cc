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

#ifndef LITE

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
    YCursorPixmap(char const *path);
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
    operator bool();
};

#ifdef CONFIG_XPM // ================== use libXpm to load the cursor pixmap ===
YCursorPixmap::YCursorPixmap(char const *path): fValid(false) {
    fAttributes.colormap  = xapp->colormap();
    fAttributes.closeness = 65535;
    fAttributes.valuemask = XpmColormap|XpmCloseness|
                            XpmReturnPixels|XpmSize|XpmHotspot;
    fAttributes.x_hotspot = 0;
    fAttributes.y_hotspot = 0;

    int const rc(XpmReadFileToPixmap(xapp->display(), desktop->handle(),
                                     (char *)REDIR_ROOT(path), // !!!
                                     &fPixmap, &fMask, &fAttributes));

    if (rc != XpmSuccess)
        warn(_("Loading of pixmap \"%s\" failed: %s"),
               path, XpmGetErrorString(rc));
    else if (fAttributes.npixels != 2)
        warn(_("Invalid cursor pixmap: \"%s\" contains too many unique colors"), 
               path);
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
YCursorPixmap::YCursorPixmap(char const *path):
    fHotspotX(0), fHotspotY(0) {
    fImage = Imlib_load_image(hImlib, (char *)REDIR_ROOT(path));

    if (fImage == NULL) {
        warn(_("Loading of pixmap \"%s\" failed"), path);
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

    Pixel fg, bg, *pp((Pixel*) fImage->rgb_data);
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
                               "much unique colors"), path);

                        Imlib_destroy_image(hImlib, fImage);
                        fImage = NULL;
                        return;
                    }
            }
    
    fForeground.red = fg.r << 8; // -- alloc the background/foreground color ---
    fForeground.green = fg.g << 8;
    fForeground.blue = fg.b << 8;
    XAllocColor(xapp->display(), xapp->colormap(), &fForeground);

    fBackground.red = bg.r << 8;
    fBackground.green = bg.g << 8;
    fBackground.blue = bg.b << 8;
    XAllocColor(xapp->display(), xapp->colormap(), &fBackground);

    // ----------------- find the hotspot by reading the xpm header manually ---
    FILE *xpm = fopen((char *)REDIR_ROOT(path), "rb");
    if (xpm == NULL)
        warn(_("BUG? Imlib was able to read \"%s\""), path);

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
                           "was able to parse \"%s\""), path);

                fclose(xpm);
                return;
            }
            default:
                if (c == EOF)
                    warn(_("BUG? Unexpected end of XPM file but Imlib "
                           "was able to parse \"%s\""), path);
                else
                    warn(_("BUG? Unexpected characted but Imlib "
                           "was able to parse \"%s\""), path);

                fclose(xpm);
                return;
        }
    }
}
#endif

YCursorPixmap::~YCursorPixmap() {
    XFreePixmap(xapp->display(), fPixmap);
    XFreePixmap(xapp->display(), fMask);

#ifdef CONFIG_XPM 
    XpmFreeAttributes(&fAttributes);
#endif

#ifdef CONFIG_IMLIB
    Imlib_destroy_image(hImlib, fImage);
#endif
}

#endif

YCursor::~YCursor() {
    if(fOwned && fCursor && app)
        XFreeCursor(xapp->display(), fCursor);
}

#ifndef LITE
void YCursor::load(char const *path) {
    YCursorPixmap pixmap(path);
    
    if (pixmap.isValid()) { // ============ convert coloured pixmap into a bilevel one ===
        Pixmap bilevel(YPixmap::createMask(pixmap.width(), pixmap.height()));

        // -------------------------- figure out which plane we have to copy ---
        unsigned long pmask(1 << (xapp->depth() - 1));

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
}
#endif

#ifndef LITE
void YCursor::load(char const *name, unsigned int fallback) {
#else
void YCursor::load(char const */*name*/, unsigned int fallback) {
#endif
    if(fCursor && fOwned)
        XFreeCursor(xapp->display(), fCursor);

#ifndef LITE
    static char const *cursors = "cursors/";
    YResourcePaths paths(cursors);

    for (YPathElement const * pe(paths); pe->root && fCursor == None; pe++) {
        char *path(pe->joinPath(cursors, name));
        if (isreg(path)) load(path);
        delete path;
    }

    if (fCursor == None)
#endif    
        fCursor = XCreateFontCursor(xapp->display(), fallback);
        
    fOwned = true;
}
