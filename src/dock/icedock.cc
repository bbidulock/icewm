/*
 * IceWM
 *
 * Copyright (C) 1997,1998,1999,2000 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmprog.h"
#include "wmwinlist.h"
#include "wmtaskbar.h"
#include "wmdialog.h"
#include "ymenuitem.h"
#include "gnomeapps.h"
#include "browse.h"
#include "objmenu.h"
#include "themes.h"
#include "sysdep.h"
#include "default.h"
#include "yapp.h"
#include "wmdesktop.h"

#include <stdio.h>
#if CONFIG_I18N == 1
#include <X11/Xlocale.h>
#endif

#define CFGDEF
#include "default.h"

int main(int argc, char **argv) {
#ifndef NO_CONFIGURE
    char *configFile = 0;
    char *overrideTheme = 0;
#endif
#if CONFIG_I18N == 1
    setlocale(LC_ALL, "");
#endif
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
#ifdef DEBUG
            if (strcmp(argv[i], "-debug") == 0) {
                debug = true;
            } else if (strcmp(argv[i], "-debug_z") == 0) {
                debug_z = true;
            }
#endif
#ifndef NO_CONFIGURE
            if (strcmp(argv[i], "-c") == 0) {
                configFile = newstr(argv[++i]);
            } else if (strcmp(argv[i], "-t") == 0)
                overrideTheme = argv[++i];
///            else if (strcmp(argv[i], "-n") == 0)
///                configurationLoaded = 1;
            else if (strcmp(argv[i], "-v") == 0) {
                fprintf(stderr, "icewm " VERSION ", Copyright 1997-1999 Marko Macek\n");
///                configurationLoaded = 1;
                exit(0);
            }
#endif
        }
    }

    YApplication app("icedock", &argc, &argv);

    delete desktop; desktop = 0;

    DesktopInfo *fDesktopInfo = 0;

    desktop = fDesktopInfo =
        new DesktopInfo(0, RootWindow(app.display(), DefaultScreen(app.display())));

    PRECONDITION(desktop != 0);

    //!!!taskBarDoubleHeight = true;

    TaskBar *taskBar = new TaskBar(fDesktopInfo, 0);
    taskBar->show();

    int rc = app.mainLoop();
    return rc;
}
