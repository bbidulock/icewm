/*
 * IceWM
 *
 * Copyright (C) 1997,1998,1999,2000 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "ymenuitem.h"
#include "sysdep.h"
#include "prefs.h"
#include "yapp.h"

#include "rootmenu.h"

#include <stdio.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

#define NO_KEYBIND
//#include "bindkey.h"
#include "prefs.h"
#define CFGDEF
//#include "bindkey.h"
#include "default.h"


int main(int argc, char **argv) {
#ifndef NO_CONFIGURE
    char *configFile = 0;
    char *overrideTheme = 0;
#endif
#ifdef I18N
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
#if 0
    if (!configurationLoaded) {
        if (configFile == 0)
            configFile = app->findConfigFile("preferences");
        if (configFile)
            loadConfiguration(configFile);
        delete configFile; configFile = 0;

        if (overrideTheme)
            themeName = newstr(overrideTheme);

        if (themeName != 0) {
            char *theme = strJoin("themes/", themeName, NULL);
            char *themePath = app->findConfigFile(theme);
            if (themePath)
                loadConfiguration(themePath);
            delete theme; theme = 0;
            delete themePath; themePath = 0;
        }
    }
#endif

    YApplication app("iceroot", &argc, &argv);

    /*DesktopHandler *desktopHandler =*/ new DesktopHandler();

    int rc = app.mainLoop();
#if 0
    freeConfig();
#endif
    return rc;
}
