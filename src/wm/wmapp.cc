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
#ifdef CONFIG_I18N
#include <X11/Xlocale.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

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

static char *configArg = 0;

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

static void initMenus() {
    windowMenu = new YMenu();
    PRECONDITION(windowMenu != 0);
    windowMenu->setShared(true);

    layerMenu = new YMenu();
    PRECONDITION(layerMenu != 0);
    layerMenu->setShared(true);

    layerMenu->addItem("Menu", 0, 0, layerActionSet[WinLayerMenu]);
    layerMenu->addItem("Above Dock", 0, 0, layerActionSet[WinLayerAboveDock]);
    layerMenu->addItem("Dock", 0, 0, layerActionSet[WinLayerDock]);
    layerMenu->addItem("OnTop", 0, 0, layerActionSet[WinLayerOnTop]);
    layerMenu->addItem("Normal", 0, 0, layerActionSet[WinLayerNormal]);
    layerMenu->addItem("Below", 0, 0, layerActionSet[WinLayerBelow]);
    layerMenu->addItem("Desktop", 1, 0, layerActionSet[WinLayerDesktop]);

    moveMenu = new YMenu();
    PRECONDITION(moveMenu != 0);
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
}

int handler(Display *display, XErrorEvent *xev) {

    if (initializing &&
        xev->request_code == X_ChangeWindowAttributes &&
        xev->error_code == BadAccess)
    {
        fprintf(stderr, "Another window manager already running, exiting...\n");
        exit(1);
    }

    // DBG
    {
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
        YPref prefConfirmLogout("icewm", "ConfirmLogout");
        bool pvConfirmLogout = prefConfirmLogout.getBool(true);

        if (!pvConfirmLogout)
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
        app->exit(0);
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

    initMenus();

    defaultAppIcon = YIcon::getIcon("app");

    //windowList->show();
#ifndef NO_WINDOW_OPTIONS
    defOptions = new WindowOptions();
    hintOptions = new WindowOptions();
    if (winOptFile)
        loadWinOptions(winOptFile);
    delete winOptFile; winOptFile = 0;
#endif

#ifdef CONFIG_SM
    if (haveSessionManager())
        loadWindowInfo(fWindowManager);
#endif

    initializing = 0;

#ifdef CONFIG_GUIEVENTS
    signalGuiEvent(geStartup);
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

    //!!!delete menubackPixmap;
    //!!!delete logoutPixmap;

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

#ifdef CONFIG_GUIEVENTS // !!! make the event type determination localized to icesound
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

        if (m1 == 0 && win95keys && app->WinMask)
            if (k1 == app->getWinL() && k2 == app->getWinL()) {
                fWindowManager->popupStartMenu();
            }
            if (k1 == app->getWinR() && k2 == app->getWinR()) {
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

char *findConfigFile(const char *name) {
    char *p, *h;

    h = getenv("HOME");
    if (h) {
        p = strJoin(h, "/.icewm/", name, NULL);
        if (access(p, R_OK) == 0)
            return p;
        delete p;
    }
#if 0
    p = strJoin(CONFIGDIR, "/", name, NULL);
    if (access(p, R_OK) == 0)
        return p;
    delete p;

    p = strJoin(REDIR_ROOT(LIBDIR), "/", name, NULL);
    if (access(p, R_OK) == 0)
        return p;
    delete p;
#endif
    return 0;
}

int main(int argc, char **argv) {
#ifndef NO_CONFIGURE
    char *configFile = 0;
    char *overrideTheme = 0;
#endif
#ifdef CONFIG_I18N
    setlocale(LC_ALL, "");
#endif
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
#ifdef DEBUG
            if (strcmp(argv[i], "-debug") == 0) {
                debug = true;
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
#if 0
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
#endif

#if 0
#ifndef LITE
    YPref prefDetectGnome("icewm", "AutoDetectGNOME");
    bool pvDetectGnome = prefDetectGnome.getBool(true);

    if (pvDetectGnome) {
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
#endif

    if (gWorkspaceCount == 0) {
        addWorkspace(" 1 ");
        addWorkspace(" 2 ");
        addWorkspace(" 3 ");
        addWorkspace(" 4 ");
    }

#ifndef NO_WINDOW_OPTIONS
    if (winOptFile == 0)
        winOptFile = findConfigFile("winoptions");
#endif

    if (keysFile == 0)
        keysFile = findConfigFile("keys");

    phase = phaseStartup;
    YWMApp app(&argc, &argv);

    app.manageClients();

    int rc = app.mainLoop();
#ifdef CONFIG_GUIEVENTS
    app.signalGuiEvent(geShutdown);
#endif
    phase = phaseShutdown;
    app.unmanageClients();
#if 0
#ifndef NO_CONFIGURE
    freeConfig();
#endif
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
#ifdef CONFIG_SM
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
#ifdef CONFIG_SM
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
