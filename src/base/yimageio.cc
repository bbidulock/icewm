/*
 * IceWM
 *
 * Copyright (C) 1997-2000 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ypaint.h"
#include "yapp.h"
#include "sysdep.h"
#include "prefs.h"
//#include "wmprog.h" // !!! remove this
#include "debug.h"

#ifdef XPM
#include <X11/xpm.h>
#endif

#ifdef IMLIB
#include <Imlib.h>
static ImlibData *hImlib = 0;
#endif

YPixmap *YApplication::loadPixmap(const char *fileName) {
    Pixmap fPixmap;
    Pixmap fMask;

#if defined(IMLIB)
    if(!hImlib) hImlib=Imlib_init(app->display());

    ImlibImage *im = Imlib_load_image(hImlib, (char *)REDIR_ROOT(fileName));
    if(im) {
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
#elif defined(XPM)
    XpmAttributes xpmAttributes;
    int rc;

    xpmAttributes.colormap  = defaultColormap;
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    rc = XpmReadFileToPixmap(app->display(),
                             desktop->handle(),
                             (char *)REDIR_ROOT(fileName), // !!!
                             &fPixmap, &fMask,
                             &xpmAttributes);

    if (rc == 0)
        return new YPixmap(fPixmap, fMask, xpmAttributes.width, xpmAttributes.height, true);
    else {
        warn("Warning: load pixmap %s failed with rc=%d\n", fileName, rc);
        return 0;
    }
#else
    return 0;
#endif
}

#ifdef IMLIB
/* Load pixmap at specified size */
YPixmap *YApplication::loadPixmap(const char *fileName, int w, int h) {

    if(!hImlib) hImlib = Imlib_init(app->display());

    fOwned = true;
    fWidth = w;
    fHeight = h;

    ImlibImage *im = Imlib_load_image(hImlib, (char *)REDIR_ROOT(fileName));
    if(im) {
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
