#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <signal.h>

#include "intl.h"

#ifdef IMLIB
#include <Imlib.h>

static ImlibData *hImlib = 0;
#else
#include <X11/xpm.h>
#endif

#include "base.h"
#include "WinMgr.h"

char *displayName = 0;
Display *display = 0;
Window root = 0;
Colormap defaultColormap;

Atom _XA_WIN_WORKSPACE;

long activeWorkspace = WinWorkspaceInvalid;

bool getWorkspace() {
    long w;
    Atom r_type;
    int r_format;
    unsigned long nitems, lbytes;
    unsigned char *prop;

    if (XGetWindowProperty(display, root,
                           _XA_WIN_WORKSPACE,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &nitems, &lbytes, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && nitems == 1) {
            w = ((long *)prop)[0];
            //printf("%ld\n", w);
            if (w != activeWorkspace) {
                activeWorkspace = w;
                //printf("active=%ld\n", activeWorkspace);
                XFree(prop);
                return true;
            }
        }
        XFree(prop);
    }
    return false;
}

long bgCount;
Pixmap defbg, bg[64] = { 0 };

Pixmap loadPixmap(const char *fileName) {
    Pixmap pixmap = 0;
#ifdef IMLIB
    if(!hImlib) hImlib=Imlib_init(display);

    ImlibImage *im = Imlib_load_image(hImlib, (char *)fileName);
    if(im) {
        Imlib_render(hImlib, im, im->rgb_width, im->rgb_height);
        pixmap = (Pixmap)Imlib_move_image(hImlib, im);
        Imlib_destroy_image(hImlib, im);
    } else {
        fprintf(stderr, _("Warning: loading image %s failed\n"), fileName);
    }
#else
    XpmAttributes xpmAttributes;
    Pixmap fake;
    int rc;

    xpmAttributes.colormap  = defaultColormap;
    xpmAttributes.closeness = 65535;
    xpmAttributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

    rc = XpmReadFileToPixmap(display, root,
                             (char *)fileName,
                             &pixmap, &fake,
                             &xpmAttributes);

    if (rc == 0) {
        if (fake != None)
            XFreePixmap(display, fake);
    } else
        fprintf(stderr, _("Warning: load pixmap %s failed with rc=%d\n"), fileName, rc);
#endif
    return pixmap;
}

void updateBg(long workspace) {
    Pixmap pixmap = defbg;

    if (workspace < bgCount && bg[workspace])
        pixmap = bg[workspace];

    if (pixmap != None) {
        XSetWindowBackgroundPixmap(display, root, pixmap);
        XClearWindow(display, root);
    }
}

int main(int argc, char **argv) {

#ifdef ENABLE_NLS
    bindtextdomain("icewm", LOCALEDIR);
    textdomain("icewm");
#endif

    if (argc <= 1 ||
        strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0)
    {
        fprintf(stderr,
                _("Usage: icewmbg pixmap1 pixmap2 ...\n\n"
                "Changes desktop background on workspace switches.\n"
                "The first pixmap is used as a default one.\n"));
        exit(1);
    }
    if (!(display = XOpenDisplay(displayName))) {
        fprintf(stderr, _("Can't open display: %s"), displayName);
        exit(1);
    }

    root = RootWindow(display, DefaultScreen(display));
    defaultColormap = DefaultColormap(display, DefaultScreen(display));
    _XA_WIN_WORKSPACE = XInternAtom(display, XA_WIN_WORKSPACE, False);

    XSelectInput(display, root, PropertyChangeMask);

//  if (getWorkspace())
//      updateBg(activeWorkspace);

    // could be optimized
    bgCount = argc - 1;
    for (int ws = 1; ws < argc; ws++) {
        bg[ws - 1] = loadPixmap(argv[ws]);
        if (!defbg)
            defbg = bg[ws - 1];
    }

//     Figment: moved here ...
    if (getWorkspace())
        updateBg(activeWorkspace);


    while (1) {
        XEvent xev;

        XNextEvent(display, &xev);

        switch (xev.type) {
        case PropertyNotify:
            if (xev.xproperty.window == root &&
                xev.xproperty.atom == _XA_WIN_WORKSPACE)
            {
                if (getWorkspace())
                    updateBg(activeWorkspace);
            }
            break;
        }
    }
}
