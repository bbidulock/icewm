/*
 * IceWM
 *
 * Copyright (C) 1997-2000 Marko Macek
 */
#include "config.h"
#include "yxlib.h"
#include "base.h"
#include "yapp.h"
#include "ywindow.h"
#include "sysdep.h"
#include "prefs.h"
#include "ypaint.h"

#define CONFIG_XPM
#undef CONFIG_IMLIB
#undef CONFIG_GDKPIXBUF

#ifdef CONFIG_XPM
#include <X11/xpm.h>
#endif

#ifdef CONFIG_IMLIB
#include <Imlib.h>
static ImlibData *hImlib = 0;
#endif

#ifdef CONFIG_GDKPIXBUF
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf-xlib.h>
static bool gpbInit = false;
#endif

extern Colormap defaultColormap;

YPixmap *YApplication::loadPixmap(const char *fileName) {
    Pixmap fPixmap;
    Pixmap fMask;

#if defined(CONFIG_IMLIB)
    if (!hImlib) hImlib = Imlib_init(app->display());

    ImlibImage *im = Imlib_load_image(hImlib, (char *)REDIR_ROOT(fileName));
    if (im) {
        int fWidth = im->rgb_width;
        int fHeight = im->rgb_height;
        Imlib_render(hImlib, im, fWidth, fHeight);
        fPixmap = (Pixmap)Imlib_move_image(hImlib, im);
        fMask = (Pixmap)Imlib_move_mask(hImlib, im);
        Imlib_destroy_image(hImlib, im);
        return new YPixmap(fPixmap, fMask, im->rgb_width, im->rgb_height, true);
    } else {
        warn("Warning: loading image %s failed\n", fileName);
        return 0;
    }
#elif defined(CONFIG_XPM)
//#warning "dynamically allocate XpmAttributes"
    XpmAttributes xpmAttributes;
    int rc;

    xpmAttributes.colormap  = defaultColormap;
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize | XpmReturnPixels | XpmColormap | XpmCloseness;

    rc = XpmReadFileToPixmap(app->display(),
                             desktop->handle(),
                             (char *)REDIR_ROOT(fileName), // !!!
                             &fPixmap, &fMask,
                             &xpmAttributes);

    if (rc == 0) {
        YPixmap *p = new YPixmap(fPixmap, fMask, xpmAttributes.width, xpmAttributes.height, true);
        XpmFreeAttributes(&xpmAttributes);
        return p;
    } else {
        warn("Warning: load pixmap %s failed with rc=%d\n", fileName, rc);
        return 0;
    }
#elif defined(CONFIG_GDKPIXBUF)
    if (!gpbInit) {
        gdk_pixbuf_xlib_init(app->display(), 0);
        gpbInit = true;
    }
    GdkPixbuf *img = gdk_pixbuf_new_from_file(fileName);
    if (img == 0) {
        warn("Warning: load pixmap %s failed with rc=%d\n", fileName, rc);
        return 0;
    }
    int w = gdk_pixbuf_get_width(img);
    int w = gdk_pixbuf_get_width(heigth);
    YPixmap *pix = new YPixmap(w, h);
    Graphics &g = desktop->getGraphics();
    gdk_pixbuf_xlib_render_to_drawable(img, pix->handle(), g.handle(), 
                                       0, 0, 0, 0, w, h, 
                                       XLIB_RGB_DITHER_NORMAL, 0, 0);
    return pix;
#else
    return 0;
#endif
}

#ifdef CONFIG_IMLIB
/* Load pixmap at specified size */
YPixmap *YApplication::loadPixmap(const char *fileName, int w, int h) {

    if (!hImlib) hImlib = Imlib_init(app->display());

    fOwned = true;
    fWidth = w;
    fHeight = h;

    ImlibImage *im = Imlib_load_image(hImlib, (char *)REDIR_ROOT(fileName));
    if (im) {
        Imlib_render(hImlib, im, fWidth, fHeight);
        fPixmap = (Pixmap) Imlib_move_image(hImlib, im);
        fMask = (Pixmap) Imlib_move_mask(hImlib, im);
        Imlib_destroy_image(hImlib, im);
        return new YPixmap(fPixmap, fMask, w, h, true);
    } else {
        warn("Warning: loading image %s failed\n", fileName);
        return 0;
    }
}
#endif
