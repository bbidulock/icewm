/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "atasks.h"
#include "wmapp.h"
#include "wmaction.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "wmprog.h"
#include "wmswitch.h"
#include "wmstatus.h"
#include "wmabout.h"
#include "wmdialog.h"
#include "wmconfig.h"
#include "wmwinmenu.h"
#include "wmwinlist.h"
#include "wmtaskbar.h"
#include "wmclient.h"
#include "ymenuitem.h"
#include "wmsession.h"
#include "gnomeapps.h"
#include "browse.h"
#include "objmenu.h"
#include "themes.h"
#include "sysdep.h"
#include "prefs.h"
#include <stdio.h>
#ifdef I18N
#include <X11/Xlocale.h>
#endif

#include "intl.h"

int initializing = 1;
int rebootOrShutdown = 0;

YWMApp *wmapp = 0;
YWindowManager *manager = 0;

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
#ifdef CONFIG_WINMENU
YMenu *windowListMenu = 0;
#endif
#ifdef CONFIG_WINLIST
YMenu *windowListPopup = 0;
YMenu *windowListAllPopup = 0;
#endif
YMenu *logoutMenu = 0;

char *configArg = 0;

PhaseType phase = phaseStartup;

static void registerProtocols() {
    Atom win_proto[10];
    unsigned int i = 0;

    win_proto[i++] = _XA_WIN_WORKSPACE;
    win_proto[i++] = _XA_WIN_WORKSPACE_COUNT;
    win_proto[i++] = _XA_WIN_WORKSPACE_NAMES;
    win_proto[i++] = _XA_WIN_ICONS;
    win_proto[i++] = _XA_WIN_WORKAREA;

    win_proto[i++] = _XA_WIN_STATE;
    win_proto[i++] = _XA_WIN_HINTS;
    win_proto[i++] = _XA_WIN_LAYER;
    win_proto[i++] = _XA_WIN_SUPPORTING_WM_CHECK;
    win_proto[i++] = _XA_WIN_CLIENT_LIST;

    XChangeProperty(app->display(), manager->handle(),
                    _XA_WIN_PROTOCOLS, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)win_proto, i);


    YWindow *checkWindow = new YWindow();
    Window xid = checkWindow->handle();

    XChangeProperty(app->display(), checkWindow->handle(),
                    _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    XChangeProperty(app->display(), manager->handle(),
                    _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    unsigned long ac[2] = { 1, 1 };
    unsigned long ca[2] = { 0, 0 };

    XChangeProperty(app->display(), manager->handle(),
                    _XA_WIN_AREA_COUNT, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&ac, 2);
    XChangeProperty(app->display(), manager->handle(),
                    _XA_WIN_AREA, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&ca, 2);
}

static void unregisterProtocols() {
    XDeleteProperty(app->display(),
                    manager->handle(),
                    _XA_WIN_PROTOCOLS);
}
static void initIconSize() {
    XIconSize *is;

    is = XAllocIconSize();
    if (is) {
        is->min_width = 16;
        is->min_height = 16;
        is->max_width = 32;
        is->max_height = 32;
        is->width_inc = 16;
        is->height_inc = 16;
        XSetIconSizes(app->display(), manager->handle(), is, 1);
        XFree(is);
    }
}


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

#ifdef CONFIG_LOOK_PIXMAP
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
        loadPixmap(tpaths, 0, "closeI.xpm", &closePixmap[0]);
        loadPixmap(tpaths, 0, "depthI.xpm", &depthPixmap[0]);
        loadPixmap(tpaths, 0, "maximizeI.xpm", &maximizePixmap[0]);
        loadPixmap(tpaths, 0, "minimizeI.xpm", &minimizePixmap[0]);
        loadPixmap(tpaths, 0, "restoreI.xpm", &restorePixmap[0]);
        loadPixmap(tpaths, 0, "hideI.xpm", &hidePixmap[0]);
        loadPixmap(tpaths, 0, "rollupI.xpm", &rollupPixmap[0]);
        loadPixmap(tpaths, 0, "rolldownI.xpm", &rolldownPixmap[0]);
        loadPixmap(tpaths, 0, "closeA.xpm", &closePixmap[1]);
        loadPixmap(tpaths, 0, "depthA.xpm", &depthPixmap[1]);
        loadPixmap(tpaths, 0, "maximizeA.xpm", &maximizePixmap[1]);
        loadPixmap(tpaths, 0, "minimizeA.xpm", &minimizePixmap[1]);
        loadPixmap(tpaths, 0, "restoreA.xpm", &restorePixmap[1]);
        loadPixmap(tpaths, 0, "hideA.xpm", &hidePixmap[1]);
        loadPixmap(tpaths, 0, "rollupA.xpm", &rollupPixmap[1]);
        loadPixmap(tpaths, 0, "rolldownA.xpm", &rolldownPixmap[1]);
        loadPixmap(tpaths, 0, "frameITL.xpm", &frameTL[0][0]);
        loadPixmap(tpaths, 0, "frameIT.xpm",  &frameT[0][0]);
        loadPixmap(tpaths, 0, "frameITR.xpm", &frameTR[0][0]);
        loadPixmap(tpaths, 0, "frameIL.xpm",  &frameL[0][0]);
        loadPixmap(tpaths, 0, "frameIR.xpm",  &frameR[0][0]);
        loadPixmap(tpaths, 0, "frameIBL.xpm", &frameBL[0][0]);
        loadPixmap(tpaths, 0, "frameIB.xpm",  &frameB[0][0]);
        loadPixmap(tpaths, 0, "frameIBR.xpm", &frameBR[0][0]);
        loadPixmap(tpaths, 0, "frameATL.xpm", &frameTL[0][1]);
        loadPixmap(tpaths, 0, "frameAT.xpm",  &frameT[0][1]);
        loadPixmap(tpaths, 0, "frameATR.xpm", &frameTR[0][1]);
        loadPixmap(tpaths, 0, "frameAL.xpm",  &frameL[0][1]);
        loadPixmap(tpaths, 0, "frameAR.xpm",  &frameR[0][1]);
        loadPixmap(tpaths, 0, "frameABL.xpm", &frameBL[0][1]);
        loadPixmap(tpaths, 0, "frameAB.xpm",  &frameB[0][1]);
        loadPixmap(tpaths, 0, "frameABR.xpm", &frameBR[0][1]);

        loadPixmap(tpaths, 0, "dframeITL.xpm", &frameTL[1][0]);
        loadPixmap(tpaths, 0, "dframeIT.xpm",  &frameT[1][0]);
        loadPixmap(tpaths, 0, "dframeITR.xpm", &frameTR[1][0]);
        loadPixmap(tpaths, 0, "dframeIL.xpm",  &frameL[1][0]);
        loadPixmap(tpaths, 0, "dframeIR.xpm",  &frameR[1][0]);
        loadPixmap(tpaths, 0, "dframeIBL.xpm", &frameBL[1][0]);
        loadPixmap(tpaths, 0, "dframeIB.xpm",  &frameB[1][0]);
        loadPixmap(tpaths, 0, "dframeIBR.xpm", &frameBR[1][0]);
        loadPixmap(tpaths, 0, "dframeATL.xpm", &frameTL[1][1]);
        loadPixmap(tpaths, 0, "dframeAT.xpm",  &frameT[1][1]);
        loadPixmap(tpaths, 0, "dframeATR.xpm", &frameTR[1][1]);
        loadPixmap(tpaths, 0, "dframeAL.xpm",  &frameL[1][1]);
        loadPixmap(tpaths, 0, "dframeAR.xpm",  &frameR[1][1]);
        loadPixmap(tpaths, 0, "dframeABL.xpm", &frameBL[1][1]);
        loadPixmap(tpaths, 0, "dframeAB.xpm",  &frameB[1][1]);
        loadPixmap(tpaths, 0, "dframeABR.xpm", &frameBR[1][1]);


        loadPixmap(tpaths, 0, "titleIL.xpm", &titleL[0]);
        loadPixmap(tpaths, 0, "titleIS.xpm", &titleS[0]);
        loadPixmap(tpaths, 0, "titleIP.xpm", &titleP[0]);
        loadPixmap(tpaths, 0, "titleIT.xpm", &titleT[0]);
        loadPixmap(tpaths, 0, "titleIM.xpm", &titleM[0]);
        loadPixmap(tpaths, 0, "titleIB.xpm", &titleB[0]);
        loadPixmap(tpaths, 0, "titleIR.xpm", &titleR[0]);
        loadPixmap(tpaths, 0, "titleAL.xpm", &titleL[1]);
        loadPixmap(tpaths, 0, "titleAS.xpm", &titleS[1]);
        loadPixmap(tpaths, 0, "titleAP.xpm", &titleP[1]);
        loadPixmap(tpaths, 0, "titleAT.xpm", &titleT[1]);
        loadPixmap(tpaths, 0, "titleAM.xpm", &titleM[1]);
        loadPixmap(tpaths, 0, "titleAB.xpm", &titleB[1]);
        loadPixmap(tpaths, 0, "titleAR.xpm", &titleR[1]);

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

        loadPixmap(tpaths, 0, "menuButtonI.xpm", &menuButton[0]);
        loadPixmap(tpaths, 0, "menuButtonA.xpm", &menuButton[1]);
    } else
#endif
    {
        loadPixmap(tpaths, 0, "depth.xpm", &depthPixmap[0]);
        loadPixmap(tpaths, 0, "close.xpm", &closePixmap[0]);
        loadPixmap(tpaths, 0, "maximize.xpm", &maximizePixmap[0]);
        loadPixmap(tpaths, 0, "minimize.xpm", &minimizePixmap[0]);
        loadPixmap(tpaths, 0, "restore.xpm", &restorePixmap[0]);
        loadPixmap(tpaths, 0, "hide.xpm", &hidePixmap[0]);
        loadPixmap(tpaths, 0, "rollup.xpm", &rollupPixmap[0]);
        loadPixmap(tpaths, 0, "rolldown.xpm", &rolldownPixmap[0]);
    }
    loadPixmap(tpaths, 0, "menubg.xpm", &menubackPixmap);
    loadPixmap(tpaths, 0, "switchbg.xpm", &switchbackPixmap);
    loadPixmap(tpaths, 0, "logoutbg.xpm", &logoutPixmap);
    
    if (!showTaskBar) {
        loadPixmap(tpaths, "taskbar/", 
                  "taskbuttonbg.xpm", &taskbuttonPixmap);
        loadPixmap(tpaths, "taskbar/", 
                  "taskbuttonactive.xpm", &taskbuttonactivePixmap);
    }

    if (DesktopBackgroundPixmap && DesktopBackgroundPixmap[0]) {
        YPixmap *bg = 0;
        if (DesktopBackgroundPixmap[0] == '/') {
            if (access(DesktopBackgroundPixmap, R_OK) == 0) {
                bg = new YPixmap(DesktopBackgroundPixmap); // should be a separate program to reduce memory waste
            }
        } else
            loadPixmap(tpaths, 0, DesktopBackgroundPixmap, &bg);

        if (bg) {
            if (centerBackground) {
                YPixmap *back = new YPixmap(desktop->width(), desktop->height());;
                Graphics g(back);;
                YColor *c = 0;
                if (DesktopBackgroundColor && DesktopBackgroundColor[0])
                    c = new YColor(DesktopBackgroundColor);
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
    } else if (DesktopBackgroundColor && DesktopBackgroundColor[0]) {
        YColor *c = new YColor(DesktopBackgroundColor); //!!! leaks
        XSetWindowBackground(app->display(), desktop->handle(), c->pixel());
        XClearWindow(app->display(), desktop->handle());
    }

}

static void initMenus() {
#ifdef CONFIG_WINMENU
    windowListMenu = new WindowListMenu();
    windowListMenu->setShared(true); // !!!
    windowListMenu->setActionListener(wmapp);
#endif

    logoutMenu = new YMenu();
    PRECONDITION(logoutMenu != 0);
    logoutMenu->setShared(true); /// !!! get rid of this (refcount objects)
    logoutMenu->addItem(_("_Logout"), -2, "", actionLogout)->setChecked(true);
    logoutMenu->addItem(_("_Cancel logout"), -2, "", actionCancelLogout)->setEnabled(false);
    logoutMenu->addSeparator();
#ifndef NO_CONFIGURE_MENUS
    {
        const char *c = configArg ? "-c" : 0;
        char **args = (char **)MALLOC(4 * sizeof(char*));
        args[0] = newstr(ICEWMEXE);
        args[1] = (char *)c; //!!!
        args[2] = configArg;
        args[3] = 0;
        DProgram *re_icewm = DProgram::newProgram(_("Restart _Icewm"), 0, true, ICEWMEXE, args); //!!!
        if (re_icewm)
            logoutMenu->add(new DObjectMenuItem(re_icewm));
    }
    {
        DProgram *re_xterm = DProgram::newProgram(_("Restart _Xterm"), 0, true, "xterm", 0);
        if (re_xterm)
            logoutMenu->add(new DObjectMenuItem(re_xterm));
    }
#endif

    windowMenu = new YMenu();
    assert(windowMenu != 0);
    windowMenu->setShared(true);

    layerMenu = new YMenu();
    assert(layerMenu != 0);
    layerMenu->setShared(true);

    layerMenu->addItem(_("_Menu"),       -2, 0, layerActionSet[WinLayerMenu]);
    layerMenu->addItem(_("_Above Dock"), -2, 0, layerActionSet[WinLayerAboveDock]);
    layerMenu->addItem(_("_Dock"),       -2, 0, layerActionSet[WinLayerDock]);
    layerMenu->addItem(_("_OnTop"),      -2, 0, layerActionSet[WinLayerOnTop]);
    layerMenu->addItem(_("_Normal"),     -2, 0, layerActionSet[WinLayerNormal]);
    layerMenu->addItem(_("_Below"),      -2, 0, layerActionSet[WinLayerBelow]);
    layerMenu->addItem(_("D_esktop"),    -2, 0, layerActionSet[WinLayerDesktop]);

    moveMenu = new YMenu();
    assert(moveMenu != 0);
    moveMenu->setShared(true);
    for (int w = 0; w < workspaceCount; w++) {
        char s[128];
        sprintf(s, "%lu. %s", (unsigned long)(w + 1), workspaceNames[w]);
        moveMenu->addItem(s, 0, 0, workspaceActionMoveTo[w]);
    }

    windowMenu->addItem(_("_Restore"),  -2, KEY_NAME(gKeyWinRestore), actionRestore);
    windowMenu->addItem(_("_Move"),     -2, KEY_NAME(gKeyWinMove), actionMove);
    windowMenu->addItem(_("_Size"),     -2, KEY_NAME(gKeyWinSize), actionSize);
    windowMenu->addItem(_("Mi_nimize"), -2, KEY_NAME(gKeyWinMinimize), actionMinimize);
    windowMenu->addItem(_("Ma_ximize"), -2, KEY_NAME(gKeyWinMaximize), actionMaximize);
    windowMenu->addItem(_("_Hide"),     -2, KEY_NAME(gKeyWinHide), actionHide);
    windowMenu->addItem(_("Roll_up"),   -2, KEY_NAME(gKeyWinRollup), actionRollup);
    windowMenu->addSeparator();
    windowMenu->addItem(_("R_aise"),	-2, KEY_NAME(gKeyWinRaise), actionRaise);
    windowMenu->addItem(_("_Lower"),	-2, KEY_NAME(gKeyWinLower), actionLower);
    windowMenu->addSubmenu(_("La_yer"), -2, layerMenu);
    if (workspaceCount > 1) {
        windowMenu->addSeparator();
        windowMenu->addSubmenu(_("Move _To"), -2, moveMenu);
        windowMenu->addItem(_("Occupy _All"), -2, KEY_NAME(gKeyWinOccupyAll), actionOccupyAllOrCurrent);
    }
    windowMenu->addSeparator();
    windowMenu->addItem(_("_Close"),    -2, KEY_NAME(gKeyWinClose), actionClose);
#ifdef CONFIG_WINLIST
    windowMenu->addSeparator();
    windowMenu->addItem(_("_Window list"), -2, actionWindowList, windowListMenu);
#endif

#ifndef NO_CONFIGURE_MENUS
    rootMenu = new StartMenu("menu");
    rootMenu->setActionListener(wmapp);
#endif
}

void initWorkspaces() {
    XTextProperty names;

    if (XStringListToTextProperty(workspaceNames, workspaceCount, &names)) {
        XSetTextProperty(app->display(),
                         manager->handle(),
                         &names, _XA_WIN_WORKSPACE_NAMES);
        XFree(names.value);
    }

    XChangeProperty(app->display(), manager->handle(),
                    _XA_WIN_WORKSPACE_COUNT, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&workspaceCount, 1);

    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;
    long ws = 0;

    if (XGetWindowProperty(app->display(),
                           manager->handle(),
                           _XA_WIN_WORKSPACE,
                           0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
            long n = *(long *)prop;

            if (n < workspaceCount)
                ws = n;
        }
        XFree(prop);
    }
    manager->activateWorkspace(ws, false);
}

int handler(Display *display, XErrorEvent *xev) {

    if (initializing &&
        xev->request_code == X_ChangeWindowAttributes &&
        xev->error_code == BadAccess)
    {
        msg(_("Another window manager already running, exiting..."));
        exit(1);
    }

    DBG {
        char message[80], req[80], number[80];

        XGetErrorText(display,
                      xev->error_code,
                      message, sizeof(message));
        sprintf(number, "%d", xev->request_code);

        XGetErrorDatabaseText(display,
                              "XRequest",
                              number, "",
                              req, sizeof(req));
        if (!req[0])
            sprintf(req, "[request_code=%d]", xev->request_code);

        warn(_("X error %s(0x%lX): %s"), req, xev->resourceid, message);
    }
    return 0;
}

#ifdef DEBUG
void dumpZorder(const char *oper, YFrameWindow *w, YFrameWindow *a) {
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
    ///!!! problem with repeated SIGHUP for restart...
    app->resetSignals();
    if (str) {
        if (args) {
            execvp(str, args);
        } else {
            execlp(str, str, 0);
        }
    } else {
        const char *c = configArg ? "-c" : NULL;
        execlp(ICEWMEXE, ICEWMEXE, c, configArg, 0);
    }

    XBell(app->display(), 100);
    warn(_("Could not restart %s, not on $PATH?"), str ? str : ICEWMEXE );
}

void YWMApp::restartClient(const char *str, char **args) {
    phase = phaseRestart;
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geRestart);
#endif
    manager->unmanageClients();
    unregisterProtocols();

    runRestart(str, args);

    /* somehow exec failed, try to recover */
    phase = phaseStartup;
    registerProtocols();
    manager->manageClients();
}

void YWMApp::actionPerformed(YAction *action, unsigned int /*modifiers*/) {
    if (action == actionLogout) {
        if (!confirmLogout)
            logout();
        else {
#ifndef LITE
            if (fLogoutMsgBox == 0) {
                YMsgBox *msgbox = new YMsgBox(YMsgBox::mbOK|YMsgBox::mbCancel);
                fLogoutMsgBox = msgbox;
                msgbox->setTitle(_("Confirm Logout"));
                msgbox->setText(_("Logout will close all active applications.\nProceed?"));
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
    } else if (action == actionExit) {
        phase = phaseShutdown;
        manager->unmanageClients();
        unregisterProtocols();
        exit(0);
    } else if (action == actionRefresh) {
        static YWindow *w = 0;
        if (w == 0) w = new YWindow();
        if (w) {
            w->setGeometry(0, 0, desktop->width(), desktop->height());
            w->raise();
            w->show();
            w->hide();
        }
#ifndef LITE
    } else if (action == actionAbout) {
        if (aboutDlg)
            aboutDlg->showFocused();
#endif
    } else if (action == actionTileVertical ||
               action == actionTileHorizontal)
    {
        YFrameWindow **w = 0;
        int count = 0;

        manager->getWindowsToArrange(&w, &count);
        if (w) {
            manager->tileWindows(w, count,
                                 (action == actionTileVertical) ? true : false);
            delete [] w;
        }
    } else if (action == actionArrange) {
        YFrameWindow **w = 0;
        int count = 0;
        manager->getWindowsToArrange(&w, &count);
        if (w) {
            manager->smartPlace(w, count);
            delete [] w;
        }
    } else if (action == actionHideAll || action == actionMinimizeAll) {
        YFrameWindow **w = 0;
        int count = 0;
        manager->getWindowsToArrange(&w, &count);
        if (w) {
            manager->setWindows(w, count, action);
            delete [] w;
        }
    } else if (action == actionCascade) {
        YFrameWindow **w = 0;
        int count = 0;
        manager->getWindowsToArrange(&w, &count);
        if (w) {
            manager->cascadePlace(w, count);
            delete [] w;
        }
    } else if (action == actionUndoArrange) {
        manager->undoArrange();
#ifdef CONFIG_WINLIST
    } else if (action == actionWindowList) {
        if (windowList)
            windowList->showFocused(-1, -1);
#endif
    } else {
        for (int w = 0; w < workspaceCount; w++) {
            if (workspaceActionActivate[w] == action) {
                manager->activateWorkspace(w, workspaceStatusIfExplicit);
                return ;
            }
        }
    }
}

YWMApp::YWMApp(int *argc, char ***argv, const char *displayName): YApplication(argc, argv, displayName) {
    wmapp = this;

    if (keysFile)
        loadMenus(keysFile, 0);

    XSetErrorHandler(handler);

    fLogoutMsgBox = 0;

    delete desktop; desktop = 0;

    desktop = manager = fWindowManager =
        new YWindowManager(0, RootWindow(display(),
                                         DefaultScreen(display())));
    PRECONDITION(desktop != 0);

    registerProtocols();

    initAtoms();
    initActions();
    initPointers();
    initIconSize();
    initPixmaps();
    initMenus();

#ifndef LITE
    statusMoveSize = new MoveSizeStatus(manager);
    statusWorkspace = new WorkspaceStatus(manager);
#endif
#ifndef LITE
    if (quickSwitch)
        switchWindow = new SwitchWindow(manager);
#endif
#ifdef CONFIG_TASKBAR
    if (showTaskBar) {
        taskBar = new TaskBar(manager);
        if (taskBar) 
            taskBar->showBar(true);
    } else {
        taskBar = 0;
    }
#endif
#ifdef CONFIG_WINLIST
    windowList = new WindowList(manager);
#endif
    //windowList->show();
#ifndef LITE
    ctrlAltDelete = new CtrlAltDelete(manager);
#endif
#ifndef LITE
    aboutDlg = new AboutDlg();
#endif

    manager->grabKeys();

    manager->setupRootProxy();

    manager->updateWorkArea();

    initializing = 0;
}

YWMApp::~YWMApp() {
    if (fLogoutMsgBox) {
        manager->unmanageClient(fLogoutMsgBox->handle());
        fLogoutMsgBox = 0;
    }
#ifndef LITE
    delete ctrlAltDelete; ctrlAltDelete = 0;
#endif
#ifdef CONFIG_TASKBAR
    delete taskBar; taskBar = 0;
#endif
    //delete windowList; windowList = 0;
#ifndef LITE
    delete switchWindow; switchWindow = 0;
#endif
#ifndef LITE
    delete statusMoveSize; statusMoveSize = 0;
    delete statusWorkspace; statusWorkspace = 0;
#endif

#ifndef NO_CONFIGURE_MENUS
    delete rootMenu; rootMenu = 0;
#endif
#ifdef CONFIG_WINLIST
    delete windowListPopup; windowListPopup = 0;
#endif
    delete windowMenu; windowMenu = 0;

    // shared menus last
    delete moveMenu; moveMenu = 0;
#ifdef CONFIG_WINMENU
    delete windowListMenu; windowListMenu = 0;
#endif

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
    delete logoutPixmap;

    if (!showTaskBar) {
        delete taskbuttonactivePixmap;
        delete taskbuttonminimizedPixmap;
    }

    //!!!XFreeGC(display(), outlineGC); lazy init in movesize.cc
    //!!!XFreeGC(display(), clipPixmapGC); in ypaint.cc
}

void YWMApp::handleSignal(int sig) {
    switch (sig) {
        case SIGINT:
	case SIGTERM:
	    actionPerformed(actionExit, 0);
	    break;

	case SIGQUIT:
	    actionPerformed(actionLogout, 0);
	    break;

	case SIGHUP:
	    restartClient(0, 0);
	    break;

	default:
	    YApplication::handleSignal(sig);
	    break;
    }
}

void YWMApp::handleIdle() {
#ifdef CONFIG_TASKBAR
    if (taskBar && taskBar->taskPane())
        taskBar->taskPane()->relayoutNow();
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

void YWMApp::afterWindowEvent(XEvent &xev) {
    static XEvent lastKeyEvent = { 0 };

    if (xev.type == KeyRelease && lastKeyEvent.type == KeyPress) {
        KeySym k1 = XKeycodeToKeysym(app->display(), xev.xkey.keycode, 0);
        unsigned int m1 = KEY_MODMASK(lastKeyEvent.xkey.state);
        KeySym k2 = XKeycodeToKeysym(app->display(), lastKeyEvent.xkey.keycode, 0);

        if (m1 == 0 && win95keys && app->MetaMask)
            if (k1 == XK_Meta_L && k2 == XK_Meta_L) {
                manager->popupStartMenu();
            }
#ifdef CONFIG_WINLIST
            else if (k1 == XK_Meta_R && k2 == XK_Meta_R) {
                if (windowList)
                    windowList->showFocused(-1, -1);
            }
#endif
    }

    if (xev.type == KeyPress ||
        xev.type == KeyRelease ||
        xev.type == ButtonPress ||
        xev.type == ButtonRelease)
        lastKeyEvent = xev;
}

int main(int argc, char **argv) {
#ifndef NO_CONFIGURE
    char *configFile = 0;
#endif
    char *overrideTheme = 0;
#ifdef I18N
    char *loc = setlocale(LC_ALL, "");
    if (loc == NULL || !strcmp(loc, "C") || !strcmp(loc, "POSIX"))
	multiByte = false;
    else
	multiByte = true;
#endif
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
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
                configArg = newstr(configFile);
            } else if (strcmp(argv[i], "-t") == 0)
                overrideTheme = argv[++i];
            else if (strcmp(argv[i], "-n") == 0)
                configurationLoaded = 1;
            else if (strcmp(argv[i], "-v") == 0) {
                fputs("icewm " VERSION ", Copyright 1997-1999 Marko Macek\n",
		      stderr);
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

#ifndef LITE
    if (autoDetectGnome) {
        if (getenv("SESSION_MANAGER") != NULL) { // !!! for now, fix!
            showTaskBar = false;
            useRootButtons = 0;
            DesktopBackgroundColor = 0;
            DesktopBackgroundPixmap = 0;
            // !!! more to come, probably
        }
    }
#endif

    if (workspaceCount == 0)
        addWorkspace(" 0 ");

#ifndef NO_WINDOW_OPTIONS
    if (winOptFile == 0)
        winOptFile = app->findConfigFile("winoptions");
#endif

    if (keysFile == 0)
        keysFile = app->findConfigFile("keys");

    phase = phaseStartup;
    YWMApp app(&argc, &argv);

    app.catchSignal(SIGINT);
    app.catchSignal(SIGTERM);
    app.catchSignal(SIGQUIT);
    app.catchSignal(SIGHUP);
    app.catchSignal(SIGCHLD);

    initWorkspaces();

#ifndef NO_WINDOW_OPTIONS
    defOptions = new WindowOptions();
    hintOptions = new WindowOptions();
    if (winOptFile)
        loadWinOptions(winOptFile);
    delete winOptFile; winOptFile = 0;
#endif

#ifdef SM
    if (app.haveSessionManager())
        loadWindowInfo();
#endif

#ifdef CONFIG_GUIEVENTS
    app.signalGuiEvent(geStartup);
#endif
    manager->manageClients();

    int rc = app.mainLoop();
#ifdef CONFIG_GUIEVENTS
    app.signalGuiEvent(geShutdown);
#endif
    phase = phaseShutdown;
    manager->unmanageClients();
    unregisterProtocols();
#ifndef LITE
    freeIcons();
#endif
#ifndef NO_CONFIGURE
    freeConfig();
#endif
#ifndef NO_WINDOW_OPTIONS
    delete defOptions; defOptions = 0;
    delete hintOptions; hintOptions = 0;
#endif
    return rc;
}

void YWMApp::logout() {
    if (logoutCommand && logoutCommand[0]) {
        runCommand(logoutCommand);
#ifdef SM
    } else if (haveSessionManager()) {
        smRequestShutdown();
#endif
    } else {
        manager->wmCloseSession();
        // should we always do this??
        manager->exitAfterLastClient(true);
    }
    logoutMenu->disableCommand(actionLogout);
    logoutMenu->enableCommand(actionCancelLogout);
}

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
        manager->exitAfterLastClient(false);
    }
    logoutMenu->enableCommand(actionLogout);
    logoutMenu->disableCommand(actionCancelLogout);
}

void YWMApp::handleMsgBox(YMsgBox *msgbox, int operation) {
    if (msgbox == fLogoutMsgBox && fLogoutMsgBox) {
        if (fLogoutMsgBox) {
            manager->unmanageClient(fLogoutMsgBox->handle());
            fLogoutMsgBox = 0;
        }
        if (operation == YMsgBox::mbOK) {
            logout();
        }
    }
}
