/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "wmapp.h"
#include "wmaction.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "wmswitch.h"
#include "wmstatus.h"
#include "wmconfig.h"
#include "wmclient.h"
#include "ymenuitem.h"
#include "wmsession.h"
#include "yresource.h"
#include "yconfig.h"
#include "sysdep.h"
#include "prefs.h"
#include "bindkey.h"
#include <stdio.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

int initializing = 1;
int rebootOrShutdown = 0;

YWMApp *wmapp = 0;
//YWindowManager *manager = 0;

char *keysFile = 0;

Atom XA_IcewmWinOptHint;

Cursor sizeRightPointer;
Cursor sizeTopRightPointer;
Cursor sizeTopPointer;
Cursor sizeTopLeftPointer;
Cursor sizeLeftPointer;
Cursor sizeBottomLeftPointer;
Cursor sizeBottomPointer;
Cursor sizeBottomRightPointer;

YMenu *windowMenu = 0;
YMenu *moveMenu = 0;
YMenu *layerMenu = 0;

char *configArg = 0;

PhaseType phase = phaseStartup;

YIcon *defaultAppIcon = 0;

static void initAtoms() {
    XA_IcewmWinOptHint = XInternAtom(app->display(), "_ICEWM_WINOPTHINT", False);
}

static void initPointers() {
    sizeRightPointer = XCreateFontCursor(app->display(), XC_right_side);
    sizeTopRightPointer = XCreateFontCursor(app->display(), XC_top_right_corner);
    sizeTopPointer = XCreateFontCursor(app->display(), XC_top_side);
    sizeTopLeftPointer = XCreateFontCursor(app->display(), XC_top_left_corner);
    sizeLeftPointer = XCreateFontCursor(app->display(), XC_left_side);
    sizeBottomLeftPointer = XCreateFontCursor(app->display(), XC_bottom_left_corner);
    sizeBottomPointer = XCreateFontCursor(app->display(), XC_bottom_side);
    sizeBottomRightPointer = XCreateFontCursor(app->display(), XC_bottom_right_corner);
}

void replicatePixmap(YPixmap **pixmap, bool horiz) {
    if (*pixmap && (*pixmap)->pixmap()) {
        YPixmap *newpix;
        Graphics *ng;
        int dim;

        if (horiz)
            dim = (*pixmap)->width();
        else
            dim = (*pixmap)->height();

        while (dim < 128) dim *= 2;

        if (horiz)
            newpix = new YPixmap(dim, (*pixmap)->height());
        else
            newpix = new YPixmap((*pixmap)->width(), dim);

        if (!newpix)
            return ;

        ng = new Graphics(newpix);

        if (horiz)
            ng->repHorz(*pixmap, 0, 0, newpix->width());
        else
            ng->repVert(*pixmap, 0, 0, newpix->height());

        delete ng;
        delete *pixmap;
        *pixmap = newpix;
    }
}


static void initPixmaps() {
#if 0
    static const char *home = getenv("HOME");
    static char themeSubdir[256];

    strcpy(themeSubdir, themeName);
    { char *p = strchr(themeSubdir, '/'); if (p) *p = 0; }

    static const char *themeDir = themeSubdir;

    pathelem tpaths[] = {
        { &home, "/.icewm/themes/", &themeDir },
        { &configDir, "/themes/", &themeDir },
        { &libDir, "/themes/", &themeDir },
        { 0, 0, 0 }
    };

    verifyPaths(tpaths, 0);
#endif
    YResourcePath *rp = app->getResourcePath("");

    if (rp) {
#ifdef CONFIG_LOOK_PIXMAP
        if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
            closePixmap[0] = app->loadResourcePixmap(rp, "closeI.xpm");
            depthPixmap[0] = app->loadResourcePixmap(rp, "depthI.xpm");
            maximizePixmap[0] = app->loadResourcePixmap(rp, "maximizeI.xpm");
            minimizePixmap[0] = app->loadResourcePixmap(rp, "minimizeI.xpm");
            restorePixmap[0] = app->loadResourcePixmap(rp, "restoreI.xpm");
            hidePixmap[0] = app->loadResourcePixmap(rp, "hideI.xpm");
            rollupPixmap[0] = app->loadResourcePixmap(rp, "rollupI.xpm");
            rolldownPixmap[0] = app->loadResourcePixmap(rp, "rolldownI.xpm");
            closePixmap[1] = app->loadResourcePixmap(rp, "closeA.xpm");
            depthPixmap[1] = app->loadResourcePixmap(rp, "depthA.xpm");
            maximizePixmap[1] = app->loadResourcePixmap(rp, "maximizeA.xpm");
            minimizePixmap[1] = app->loadResourcePixmap(rp, "minimizeA.xpm");
            restorePixmap[1] = app->loadResourcePixmap(rp, "restoreA.xpm");
            hidePixmap[1] = app->loadResourcePixmap(rp, "hideA.xpm");
            rollupPixmap[1] = app->loadResourcePixmap(rp, "rollupA.xpm");
            rolldownPixmap[1] = app->loadResourcePixmap(rp, "rolldownA.xpm");

            frameTL[0][0] = app->loadResourcePixmap(rp, "frameITL.xpm");
            frameT[0][0] = app->loadResourcePixmap(rp, "frameIT.xpm");
            frameTR[0][0] = app->loadResourcePixmap(rp, "frameITR.xpm");
            frameL[0][0] = app->loadResourcePixmap(rp, "frameIL.xpm");
            frameR[0][0] = app->loadResourcePixmap(rp, "frameIR.xpm");
            frameBL[0][0] = app->loadResourcePixmap(rp, "frameIBL.xpm");
            frameB[0][0] = app->loadResourcePixmap(rp, "frameIB.xpm");
            frameBR[0][0] = app->loadResourcePixmap(rp, "frameIBR.xpm");
            frameTL[0][1] = app->loadResourcePixmap(rp, "frameATL.xpm");
            frameT[0][1] = app->loadResourcePixmap(rp, "frameAT.xpm");
            frameTR[0][1] = app->loadResourcePixmap(rp, "frameATR.xpm");
            frameL[0][1] = app->loadResourcePixmap(rp, "frameAL.xpm");
            frameR[0][1] = app->loadResourcePixmap(rp, "frameAR.xpm");
            frameBL[0][1] = app->loadResourcePixmap(rp, "frameABL.xpm");
            frameB[0][1] = app->loadResourcePixmap(rp, "frameAB.xpm");
            frameBR[0][1] = app->loadResourcePixmap(rp, "frameABR.xpm");

            frameTL[1][0] = app->loadResourcePixmap(rp, "dframeITL.xpm");
            frameT[1][0]  = app->loadResourcePixmap(rp, "dframeIT.xpm");
            frameTR[1][0] = app->loadResourcePixmap(rp, "dframeITR.xpm");
            frameL[1][0]  = app->loadResourcePixmap(rp, "dframeIL.xpm");
            frameR[1][0]  = app->loadResourcePixmap(rp, "dframeIR.xpm");
            frameBL[1][0] = app->loadResourcePixmap(rp, "dframeIBL.xpm");
            frameB[1][0]  = app->loadResourcePixmap(rp, "dframeIB.xpm");
            frameBR[1][0] = app->loadResourcePixmap(rp, "dframeIBR.xpm");
            frameTL[1][1] = app->loadResourcePixmap(rp, "dframeATL.xpm");
            frameT[1][1]  = app->loadResourcePixmap(rp, "dframeAT.xpm");
            frameTR[1][1] = app->loadResourcePixmap(rp, "dframeATR.xpm");
            frameL[1][1]  = app->loadResourcePixmap(rp, "dframeAL.xpm");
            frameR[1][1]  = app->loadResourcePixmap(rp, "dframeAR.xpm");
            frameBL[1][1] = app->loadResourcePixmap(rp, "dframeABL.xpm");
            frameB[1][1]  = app->loadResourcePixmap(rp, "dframeAB.xpm");
            frameBR[1][1] = app->loadResourcePixmap(rp, "dframeABR.xpm");


            titleL[0] = app->loadResourcePixmap(rp, "titleIL.xpm");
            titleS[0] = app->loadResourcePixmap(rp, "titleIS.xpm");
            titleP[0] = app->loadResourcePixmap(rp, "titleIP.xpm");
            titleT[0] = app->loadResourcePixmap(rp, "titleIT.xpm");
            titleM[0] = app->loadResourcePixmap(rp, "titleIM.xpm");
            titleB[0] = app->loadResourcePixmap(rp, "titleIB.xpm");
            titleR[0] = app->loadResourcePixmap(rp, "titleIR.xpm");
            titleL[1] = app->loadResourcePixmap(rp, "titleAL.xpm");
            titleS[1] = app->loadResourcePixmap(rp, "titleAS.xpm");
            titleP[1] = app->loadResourcePixmap(rp, "titleAP.xpm");
            titleT[1] = app->loadResourcePixmap(rp, "titleAT.xpm");
            titleM[1] = app->loadResourcePixmap(rp, "titleAM.xpm");
            titleB[1] = app->loadResourcePixmap(rp, "titleAB.xpm");
            titleR[1] = app->loadResourcePixmap(rp, "titleAR.xpm");

            for (int a = 0; a <= 1; a++) {
                for (int b = 0; b <= 1; b++) {
                    replicatePixmap(&frameT[a][b], true);
                    replicatePixmap(&frameB[a][b], true);
                    replicatePixmap(&frameL[a][b], false);
                    replicatePixmap(&frameR[a][b], false);
                }
                replicatePixmap(&titleS[a], true);
                replicatePixmap(&titleT[a], true);
                replicatePixmap(&titleB[a], true);
            }

             menuButton[0]= app->loadResourcePixmap(rp, "menuButtonI.xpm");
             menuButton[1]= app->loadResourcePixmap(rp, "menuButtonA.xpm");
        } else
#endif
        {
             depthPixmap[0]= app->loadResourcePixmap(rp, "depth.xpm");
             closePixmap[0]= app->loadResourcePixmap(rp, "close.xpm");
             maximizePixmap[0]= app->loadResourcePixmap(rp, "maximize.xpm");
             minimizePixmap[0]= app->loadResourcePixmap(rp, "minimize.xpm");
             restorePixmap[0]= app->loadResourcePixmap(rp, "restore.xpm");
             hidePixmap[0]= app->loadResourcePixmap(rp, "hide.xpm");
             rollupPixmap[0]= app->loadResourcePixmap(rp, "rollup.xpm");
             rolldownPixmap[0]= app->loadResourcePixmap(rp, "rolldown.xpm");
        }
        menubackPixmap = app->loadResourcePixmap(rp, "menubg.xpm");
        switchbackPixmap = app->loadResourcePixmap(rp, "switchbg.xpm");
    //loadPixmap(tpaths, 0, "logoutbg.xpm", &logoutPixmap);

        // !!! This should go to separate program
        YPref prefDesktopBackgroundPixmap("icewm", "DesktopBackgroundPixmap");
        const char *pvDesktopBackgroundPixmap = prefDesktopBackgroundPixmap.getStr(0);
        YPref prefDesktopBackgroundColor("icewm", "DesktopBackgroundColor");
        const char *pvDesktopBackgroundColor = prefDesktopBackgroundColor.getStr(0);
        YPref prefDesktopBackgroundCenter("icewm", "DesktopBackgroundCenter");
        bool pvDesktopBackgroundCenter  = prefDesktopBackgroundCenter.getBool(false);

        if (pvDesktopBackgroundPixmap && pvDesktopBackgroundPixmap[0]) {
            YPixmap *bg = 0;
            if (pvDesktopBackgroundPixmap[0] == '/') {
                if (access(pvDesktopBackgroundPixmap, R_OK) == 0) {
                    bg = app->loadPixmap(pvDesktopBackgroundPixmap); // should be a separate program to reduce memory waste
                }
            } else
                bg = app->loadResourcePixmap(rp, pvDesktopBackgroundPixmap);

            if (bg) {
                if (pvDesktopBackgroundCenter) {
                    YPixmap *back = new YPixmap(desktop->width(), desktop->height());;
                    Graphics g(back);;
                    YColor *c = 0;
                    if (pvDesktopBackgroundColor && pvDesktopBackgroundColor[0])
                        c = new YColor(pvDesktopBackgroundColor);
                    else
                        c = YColor::black;

                    g.setColor(c);
                    g.fillRect(0, 0, desktop->width(), desktop->height());
                    g.drawPixmap(bg,
                                 (desktop->width() -  bg->width()) / 2,
                                 (desktop->height() - bg->height()) / 2);
                    delete bg;
                    bg = back;
                }
                XSetWindowBackgroundPixmap(app->display(), desktop->handle(), bg->pixmap());
                XClearWindow(app->display(), desktop->handle());
            }
        } else if (pvDesktopBackgroundColor && pvDesktopBackgroundColor[0]) {
            YColor *c = new YColor(pvDesktopBackgroundColor); //!!! leaks
            XSetWindowBackground(app->display(), desktop->handle(), c->pixel());
            XClearWindow(app->display(), desktop->handle());
        }
        delete rp;
    }
}

static void initMenus() {
    windowMenu = new YMenu();
    assert(windowMenu != 0);
    windowMenu->setShared(true);

    layerMenu = new YMenu();
    assert(layerMenu != 0);
    layerMenu->setShared(true);

    layerMenu->addItem("Menu", 0, 0, layerActionSet[WinLayerMenu]);
    layerMenu->addItem("Above Dock", 0, 0, layerActionSet[WinLayerAboveDock]);
    layerMenu->addItem("Dock", 0, 0, layerActionSet[WinLayerDock]);
    layerMenu->addItem("OnTop", 0, 0, layerActionSet[WinLayerOnTop]);
    layerMenu->addItem("Normal", 0, 0, layerActionSet[WinLayerNormal]);
    layerMenu->addItem("Below", 0, 0, layerActionSet[WinLayerBelow]);
    layerMenu->addItem("Desktop", 1, 0, layerActionSet[WinLayerDesktop]);

    moveMenu = new YMenu();
    assert(moveMenu != 0);
    moveMenu->setShared(true);
    for (int w = 0; w < gWorkspaceCount; w++) {
        char s[128];
        sprintf(s, "%lu. %s", (unsigned long)(w + 1), gWorkspaceNames[w]);
        moveMenu->addItem(s, 0, 0, workspaceActionMoveTo[w]);
    }

    windowMenu->addItem("Restore", 0, KEY_NAME(gKeyWinRestore), actionRestore);
    windowMenu->addItem("Move", 0, KEY_NAME(gKeyWinMove), actionMove);
    windowMenu->addItem("Size", 0, KEY_NAME(gKeyWinSize), actionSize);
    windowMenu->addItem("Minimize", 2, KEY_NAME(gKeyWinMinimize), actionMinimize);
    windowMenu->addItem("Maximize", 2, KEY_NAME(gKeyWinMaximize), actionMaximize);
    windowMenu->addItem("Hide", 0, KEY_NAME(gKeyWinHide), actionHide);
    windowMenu->addItem("Rollup", 4, KEY_NAME(gKeyWinRollup), actionRollup);
    windowMenu->addSeparator();
    windowMenu->addItem("Raise", 4, KEY_NAME(gKeyWinRaise), actionRaise);
    windowMenu->addItem("Lower", 0, KEY_NAME(gKeyWinLower), actionLower);
    windowMenu->addSubmenu("Layer", 2, layerMenu);
    if (gWorkspaceCount > 1) {
        windowMenu->addSeparator();
        windowMenu->addSubmenu("Move To", 5, moveMenu);
        windowMenu->addItem("Occupy All", 7, KEY_NAME(gKeyWinOccupyAll), actionOccupyAllOrCurrent);
    }
    windowMenu->addSeparator();
    windowMenu->addItem("Close", 0, KEY_NAME(gKeyWinClose), actionClose);
#if 0
#ifdef CONFIG_WINLIST
    windowMenu->addSeparator();
    windowMenu->addItem("Window list", 0, actionWindowList, windowListMenu);
#endif
#endif
}

int handler(Display *display, XErrorEvent *xev) {

    if (initializing &&
        xev->request_code == X_ChangeWindowAttributes &&
        xev->error_code == BadAccess)
    {
        fprintf(stderr, "Another window manager already running, exiting...\n");
        exit(1);
    }

    DBG {
        char msg[80], req[80], number[80];

        XGetErrorText(display,
                      xev->error_code,
                      msg, sizeof(msg));
        sprintf(number, "%d", xev->request_code);

        XGetErrorDatabaseText(display,
                              "XRequest",
                              number, "",
                              req, sizeof(req));
        if (!req[0])
            sprintf(req, "[request_code=%d]", xev->request_code);

        fprintf(stderr, "icewm: X-error %s(0x%lX): %s\n", req, xev->resourceid, msg);
    }
    return 0;
}

#ifdef DEBUG
void dumpZorder(YWindowManager *manager, const char *oper, YFrameWindow *w, YFrameWindow *a) {
    YFrameWindow *p = manager->top(w->getLayer());
    msg("---- %s ", oper);
    while (p) {
        if (p && p->client())
            msg(" %c %c 0x%lX: %s", (p == w) ? '*' : ' ',  (p == a) ? '#' : ' ', p->client()->handle(), p->client()->windowTitle());
        else
            msg("?? 0x%lX: %s", p->handle());
        p = p->next();
    }
}
#endif

void runRestart(const char *str, char **args) {
    XSync(app->display(), False);
    app->resetSignals();
    if (str) {
        if (args) {
            execvp(str, args);
        } else {
            execlp(str, str, 0);
        }
    } else {
        const char *c = configArg ? "-c" : NULL;
        execlp("icewm"EXEEXT, "icewm"EXEEXT, c, configArg, 0);
    }

    XBell(app->display(), 100);
    fprintf(stderr, "icewm: Could not restart %s, not on $PATH?\n", str ? str : "icewm"EXEEXT );
}

void YWMApp::restartClient(const char *str, char **args) {
    phase = phaseRestart;
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geRestart);
#endif
    fWindowManager->unmanageClients();

    runRestart(str, args);

    /* somehow exec failed, try to recover */
    phase = phaseStartup;
    fWindowManager->manageClients();
}

void YWMApp::actionPerformed(YAction *action, unsigned int /*modifiers*/) {
#if 0
    if (action == actionLogout) {
        if (!confirmLogout)
            logout();
        else {
#ifndef LITE
            if (fLogoutMsgBox == 0) {
                YMsgBox *msgbox = new YMsgBox(YMsgBox::mbOK|YMsgBox::mbCancel);
                fLogoutMsgBox = msgbox;
                msgbox->setTitle("Confirm Logout");
                msgbox->setText("Logout will close all active applications.\nProceed?");
                msgbox->autoSize();
                msgbox->setMsgBoxListener(this);
                msgbox->showFocused();
            }
#endif
        }
    } else if (action == actionCancelLogout) {
        cancelLogout();
    } else if (action == actionRun) {
        runCommand(runDlgCommand);
    } else
#endif
    if (action == actionExit) {
        phase = phaseShutdown;
        fWindowManager->unmanageClients();
        exit(0);
    }
}

YWMApp::YWMApp(int *argc, char ***argv, const char *displayName): YApplication("icewm", argc, argv, displayName) {
    wmapp = this;

    catchSignal(SIGINT);
    catchSignal(SIGTERM);
    catchSignal(SIGQUIT);
    catchSignal(SIGHUP);
    catchSignal(SIGCHLD);

#if 0
    if (keysFile)
        loadMenus(keysFile, 0);
#endif

    XSetErrorHandler(handler);

#if 0
    fLogoutMsgBox = 0;
#endif

    initAtoms();
    initActions();
    initPointers();

    delete desktop; desktop = 0;

    desktop = fWindowManager =
        new YWindowManager(0, RootWindow(display(),
                                         DefaultScreen(display())));
    PRECONDITION(desktop != 0);

    initPixmaps();
    initMenus();

    defaultAppIcon = app->getIcon("app");

    //windowList->show();
#ifndef NO_WINDOW_OPTIONS
    defOptions = new WindowOptions();
    hintOptions = new WindowOptions();
    if (winOptFile)
        loadWinOptions(winOptFile);
    delete winOptFile; winOptFile = 0;
#endif

#ifdef SM
    if (haveSessionManager())
        loadWindowInfo(fWindowManager);
#endif

    initializing = 0;

#ifdef CONFIG_GUIEVENTS
    app.signalGuiEvent(geStartup);
#endif
}

YWMApp::~YWMApp() {
#if 0
    if (fLogoutMsgBox) {
        desktop->unmanageWindow(fLogoutMsgBox);
        fLogoutMsgBox = 0;
    }
#endif

    delete windowMenu; windowMenu = 0;

    // shared menus last
    delete moveMenu; moveMenu = 0;

    delete closePixmap[0];
    delete depthPixmap[0];
    delete minimizePixmap[0];
    delete maximizePixmap[0];
    delete restorePixmap[0];
    delete hidePixmap[0];
    delete rollupPixmap[0];
    delete rolldownPixmap[0];
    delete menubackPixmap;
    delete switchbackPixmap;
    //delete logoutPixmap;

    //!!!XFreeGC(display(), outlineGC); lazy init in movesize.cc
    //!!!XFreeGC(display(), clipPixmapGC); in ypaint.cc
}

void YWMApp::handleSignal(int sig) {
    if (sig == SIGINT || sig == SIGTERM || sig == SIGQUIT) {
        actionPerformed(actionExit, 0);
        return ;
    }
    if (sig == SIGHUP) {
        restartClient(0, 0);
        return ;
    }
    YApplication::handleSignal(sig);
}

void YWMApp::handleIdle() {
#if 0  // !!!
#ifdef CONFIG_TASKBAR
    if (fTaskBar && fTaskBar->taskPane())
        fTaskBar->taskPane()->relayoutNow();
#endif
#endif
}

#ifdef CONFIG_GUIEVENTS
void YWMApp::signalGuiEvent(GUIEvent ge) {
    static Atom GUIEventAtom = None;
    unsigned char num = (unsigned char)ge;

    if (GUIEventAtom == None)
        GUIEventAtom = XInternAtom(app->display(), XA_GUI_EVENT_NAME, False);
    XChangeProperty(app->display(), desktop->handle(),
                    GUIEventAtom, GUIEventAtom, 8, PropModeReplace,
                    &num, 1);
}
#endif

void YWMApp::afterWindowEvent(XEvent & /*xev*/) {
#if 0
    static XEvent lastKeyEvent = { 0 };

    if (xev.type == KeyRelease && lastKeyEvent.type == KeyPress) {
        KeySym k1 = XKeycodeToKeysym(app->display(), xev.xkey.keycode, 0);
        unsigned int m1 = KEY_MODMASK(lastKeyEvent.xkey.state);
        KeySym k2 = XKeycodeToKeysym(app->display(), lastKeyEvent.xkey.keycode, 0);

        if (m1 == 0 && win95keys && app->MetaMask)
            if (k1 == XK_Meta_L && k2 == XK_Meta_L) {
                fWindowManager->popupStartMenu();
            }
            if (k1 == XK_Meta_R && k2 == XK_Meta_R) {
                fWindowManager->showWindowList(-1, -1);
            }
    }

    if (xev.type == KeyPress ||
        xev.type == KeyRelease ||
        xev.type == ButtonPress ||
        xev.type == ButtonRelease)
        lastKeyEvent = xev;
#endif
}

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
                configArg = argv[i];
                configFile = newstr(argv[++i]);
            } else if (strcmp(argv[i], "-t") == 0)
                overrideTheme = argv[++i];
            else if (strcmp(argv[i], "-n") == 0)
                configurationLoaded = 1;
            else if (strcmp(argv[i], "-v") == 0) {
                fprintf(stderr, "icewm " VERSION ", Copyright 1997-1999 Marko Macek\n");
                configurationLoaded = 1;
                exit(0);
            }
#endif
        }
    }
#ifndef NO_CONFIGURE
    if (!configurationLoaded) {
        if (configFile == 0)
            configFile = app->findConfigFile("preferences");
        if (configFile)
            loadConfiguration(configFile);
        delete configFile; configFile = 0;

        if (overrideTheme)
            themeName = newstr(overrideTheme);

        printf("themeName = %s\n", themeName);
        if (themeName != 0) {
            char *theme = strJoin("themes/", themeName, NULL);
            char *themePath = app->findConfigFile(theme);
            printf("themePath=%s\n", themePath);
            if (themePath)
                loadConfiguration(themePath);
            delete theme; theme = 0;
            delete themePath; themePath = 0;
        }
    }
#endif

#ifndef LITE
    if (autoDetectGnome) {
        if (getenv("SESSION_MANAGER") != NULL) { // !!! for now, fix later!
            showTaskBar = false;
            useRootButtons = 0;
#if 0
            //!!!DesktopBackgroundColor = 0;
            //!!!DesktopBackgroundPixmap = 0;
#endif
            // !!! more to come, probably
        }
    }
#endif

    if (gWorkspaceCount == 0)
        addWorkspace(" 0 ");

#ifndef NO_WINDOW_OPTIONS
    if (winOptFile == 0)
        winOptFile = app->findConfigFile("winoptions");
#endif

    if (keysFile == 0)
        keysFile = app->findConfigFile("keys");

    phase = phaseStartup;
    YWMApp app(&argc, &argv);

    app.manageClients();

    int rc = app.mainLoop();
#ifdef CONFIG_GUIEVENTS
    app.signalGuiEvent(geShutdown);
#endif
    phase = phaseShutdown;
    app.unmanageClients();
#ifndef NO_CONFIGURE
    freeConfig();
#endif
#ifndef NO_WINDOW_OPTIONS
    delete defOptions; defOptions = 0;
    delete hintOptions; hintOptions = 0;
#endif
    return rc;
}

#if 0
void YWMApp::logout() {
    if (logoutCommand && logoutCommand[0]) {
        runCommand(logoutCommand);
#ifdef SM
    } else if (haveSessionManager()) {
        smRequestShutdown();
#endif
    } else {
        fWindowManager->wmCloseSession();
        // should we always do this??
        fWindowManager->exitAfterLastClient(true);
    }
    logoutMenu->disableCommand(actionLogout);
    logoutMenu->enableCommand(actionCancelLogout);
}
#endif

#if 0
void YWMApp::cancelLogout() {
    rebootOrShutdown = 0;
    if (logoutCancelCommand && logoutCancelCommand[0]) {
        runCommand(logoutCancelCommand);
#ifdef SM
    } else if (haveSessionManager()) { // !!! this doesn't work
        smCancelShutdown();
#endif
    } else {
        // should we always do this??
        fWindowManager->exitAfterLastClient(false);
    }
    logoutMenu->enableCommand(actionLogout);
    logoutMenu->disableCommand(actionCancelLogout);
}
#endif

#if 0
void YWMApp::handleMsgBox(YMsgBox *msgbox, int operation) {
    if (msgbox == fLogoutMsgBox && fLogoutMsgBox) {
        if (fLogoutMsgBox) {
            desktop->unmanageWindow(fLogoutMsgBox);
            fLogoutMsgBox = 0;
        }
        if (operation == YMsgBox::mbOK) {
            logout();
        }
    }
}
#endif

void YWMApp::manageClients() {
    fWindowManager->manageClients();
}

void YWMApp::unmanageClients() {
    fWindowManager->unmanageClients();
}
