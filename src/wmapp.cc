/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
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

char const * YApplication::Name = "icewm";

int initializing = 1;
int rebootOrShutdown = 0;

YWMApp *wmapp = 0;
YWindowManager *manager = 0;

char *keysFile = 0;

Atom XA_IcewmWinOptHint(None);
Atom XA_ICEWM_FONT_PATH(None);

Atom _XA_XROOTPMAP_ID(None);
Atom _XA_XROOTCOLOR_PIXEL(None);

YCursor YWMApp::sizeRightPointer;
YCursor YWMApp::sizeTopRightPointer;
YCursor YWMApp::sizeTopPointer;
YCursor YWMApp::sizeTopLeftPointer;
YCursor YWMApp::sizeLeftPointer;
YCursor YWMApp::sizeBottomLeftPointer;
YCursor YWMApp::sizeBottomPointer;
YCursor YWMApp::sizeBottomRightPointer;

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
    Atom win_proto[] = {
	_XA_WIN_WORKSPACE,
	_XA_WIN_WORKSPACE_COUNT,
	_XA_WIN_WORKSPACE_NAMES,
	_XA_WIN_ICONS,
	_XA_WIN_WORKAREA,
	
	_XA_WIN_STATE,
	_XA_WIN_HINTS,
	_XA_WIN_LAYER,
	_XA_WIN_SUPPORTING_WM_CHECK,
	_XA_WIN_CLIENT_LIST
    };

    XChangeProperty(app->display(), manager->handle(),
                    _XA_WIN_PROTOCOLS, XA_ATOM, 32, PropModeReplace,
		    (unsigned char *)win_proto, ACOUNT(win_proto));

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
		    
    if (supportSemitransparency) {
        XDeleteProperty(app->display(),
			manager->handle(),
			_XA_XROOTPMAP_ID);
        XDeleteProperty(app->display(),
			manager->handle(),
			_XA_XROOTCOLOR_PIXEL);
    }
}

#ifdef CONFIG_WM_SESSION
static void initResourceManager(pid_t pid) {
    if (access(PROC_WM_SESSION, W_OK) == 0) {
	FILE *wmProc(fopen(PROC_WM_SESSION, "w"));

	if (wmProc != NULL) {
	    MSG(("registering pid %d in \""PROC_WM_SESSION"\"", pid));
	    fprintf(wmProc, "%d\n", getpid());
	    fclose(wmProc);
	} else
	    warn(PROC_WM_SESSION": %s", strerror(errno));
    }
}

void resetResourceManager() {
    initResourceManager(MAX_PID);
}
#endif

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
    XA_ICEWM_FONT_PATH = XInternAtom(app->display(), "ICEWM_FONT_PATH", False);
    _XA_XROOTPMAP_ID = XInternAtom(app->display(), "_XROOTPMAP_ID", False);
    _XA_XROOTCOLOR_PIXEL = XInternAtom(app->display(), "_XROOTCOLOR_PIXEL", False);
}

static void initFontPath() {
#ifndef LITE
    if (themeName) { // =================== find the current theme directory ===
	char themeSubdir[PATH_MAX];
	strncpy(themeSubdir, themeName, sizeof(themeSubdir) - 1);
	themeSubdir[sizeof(themeSubdir) - 1] = '\0';

	char * strfn(strrchr(themeSubdir, '/'));
	if (strfn) *strfn = '\0';

	// ================================ is there a file named fonts.dir? ===  
	char * fontsdir;

	if (*themeName == '/')
	    fontsdir = strJoin(themeSubdir, "/fonts.dir", NULL);
	else {
	    strfn = strJoin("themes/", themeSubdir, "/fonts.dir", NULL);
	    fontsdir = (app->findConfigFile(strfn));
	    delete[] strfn;
	}

	if (fontsdir) { // =========================== build a new font path ===
	    strfn = strrchr(fontsdir, '/');
	    if (strfn) *strfn = '\0';

	    int ndirs; // ------------------- retrieve the old X's font path ---
	    char ** fontPath(XGetFontPath(app->display(), &ndirs));

	    char ** newFontPath = new char *[ndirs + 1];
	    newFontPath[0] = fontsdir;

	    if (fontPath)
		memcpy(newFontPath + 1, fontPath, ndirs * sizeof (char *));
	    else
		warn(_("Unable to get current font path."));

#ifdef DEBUG
	    for (int n = 0; n < ndirs + 1; ++n)
		MSG(("Font path element %d: %s", n, newFontPath[n]));
#endif

	    char * icewmFontPath; // ---------- find death icewm's font path ---
	    Atom r_type; int r_format;
	    unsigned long count, bytes_remain;

	    if (XGetWindowProperty(app->display(),
				   manager->handle(),
				   XA_ICEWM_FONT_PATH,
				   0, PATH_MAX, False, XA_STRING,
				   &r_type, &r_format,
				   &count, &bytes_remain, 
				   (unsigned char **) &icewmFontPath) ==
				   Success && icewmFontPath) {
		if (r_type == XA_STRING && r_format == 8) {
		    for (int n(ndirs); n > 0; --n) // ---- remove death paths ---
			if (!strcmp(icewmFontPath, newFontPath[n])) {
			    if (n != ndirs)
				memmove(newFontPath + n, newFontPath + n + 1,
					(ndirs - n) * sizeof(char *));
			    --ndirs;
			}
		} else
		    warn(_("Unexpected format of ICEWM_FONT_PATH property"));

		XFree(icewmFontPath);
	    }

#ifdef DEBUG
	    for (int n = 0; n < ndirs + 1; ++n)
		MSG(("Font path element %d: %s", n, newFontPath[n]));
#endif
	    // ----------------------------------------- set the new font path ---
	    XChangeProperty(app->display(), manager->handle(),
			    XA_ICEWM_FONT_PATH, XA_STRING, 8, PropModeReplace,
			    (unsigned char *) fontsdir, strlen(fontsdir));
	    XSetFontPath(app->display(), newFontPath, ndirs + 1);

	    if (fontPath) XFreeFontPath(fontPath);
	    delete[] fontsdir;
	    delete[] newFontPath;
	}
    }
#endif    
}

static void initPointers() {
    YWMApp::sizeRightPointer.	   load("sizeR.xpm",  XC_right_side);
    YWMApp::sizeTopRightPointer.   load("sizeTR.xpm", XC_top_right_corner);
    YWMApp::sizeTopPointer.	   load("sizeT.xpm",  XC_top_side);
    YWMApp::sizeTopLeftPointer.	   load("sizeTL.xpm", XC_top_left_corner);
    YWMApp::sizeLeftPointer.	   load("sizeL.xpm",  XC_left_side);
    YWMApp::sizeBottomLeftPointer. load("sizeBL.xpm", XC_bottom_left_corner);
    YWMApp::sizeBottomPointer.	   load("sizeB.xpm",  XC_bottom_side);
    YWMApp::sizeBottomRightPointer.load("sizeBR.xpm", XC_bottom_right_corner);
}

static void initPixmaps() {
    YResourcePaths paths("", true);

#ifdef CONFIG_LOOK_PIXMAP
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
           closePixmap[0] = paths.loadPixmap(0, "closeI.xpm");
           depthPixmap[0] = paths.loadPixmap(0, "depthI.xpm");
        maximizePixmap[0] = paths.loadPixmap(0, "maximizeI.xpm");
        minimizePixmap[0] = paths.loadPixmap(0, "minimizeI.xpm");
         restorePixmap[0] = paths.loadPixmap(0, "restoreI.xpm");
            hidePixmap[0] = paths.loadPixmap(0, "hideI.xpm");
          rollupPixmap[0] = paths.loadPixmap(0, "rollupI.xpm");
        rolldownPixmap[0] = paths.loadPixmap(0, "rolldownI.xpm");
           closePixmap[1] = paths.loadPixmap(0, "closeA.xpm");
           depthPixmap[1] = paths.loadPixmap(0, "depthA.xpm");
        maximizePixmap[1] = paths.loadPixmap(0, "maximizeA.xpm");
        minimizePixmap[1] = paths.loadPixmap(0, "minimizeA.xpm");
         restorePixmap[1] = paths.loadPixmap(0, "restoreA.xpm");
            hidePixmap[1] = paths.loadPixmap(0, "hideA.xpm");
          rollupPixmap[1] = paths.loadPixmap(0, "rollupA.xpm");
        rolldownPixmap[1] = paths.loadPixmap(0, "rolldownA.xpm");

        frameTL[0][0] = paths.loadPixmap(0, "frameITL.xpm");
        frameT [0][0] = paths.loadPixmap(0, "frameIT.xpm");
        frameTR[0][0] = paths.loadPixmap(0, "frameITR.xpm");
        frameL [0][0] = paths.loadPixmap(0, "frameIL.xpm");
        frameR [0][0] = paths.loadPixmap(0, "frameIR.xpm");
        frameBL[0][0] = paths.loadPixmap(0, "frameIBL.xpm");
        frameB [0][0] = paths.loadPixmap(0, "frameIB.xpm");
        frameBR[0][0] = paths.loadPixmap(0, "frameIBR.xpm");
        frameTL[0][1] = paths.loadPixmap(0, "frameATL.xpm");
        frameT [0][1] = paths.loadPixmap(0, "frameAT.xpm");
        frameTR[0][1] = paths.loadPixmap(0, "frameATR.xpm");
        frameL [0][1] = paths.loadPixmap(0, "frameAL.xpm");
        frameR [0][1] = paths.loadPixmap(0, "frameAR.xpm");
        frameBL[0][1] = paths.loadPixmap(0, "frameABL.xpm");
        frameB [0][1] = paths.loadPixmap(0, "frameAB.xpm");
        frameBR[0][1] = paths.loadPixmap(0, "frameABR.xpm");

        frameTL[1][0] = paths.loadPixmap(0, "dframeITL.xpm");
        frameT [1][0] = paths.loadPixmap(0, "dframeIT.xpm");
        frameTR[1][0] = paths.loadPixmap(0, "dframeITR.xpm");
        frameL [1][0] = paths.loadPixmap(0, "dframeIL.xpm");
        frameR [1][0] = paths.loadPixmap(0, "dframeIR.xpm");
        frameBL[1][0] = paths.loadPixmap(0, "dframeIBL.xpm");
        frameB [1][0] = paths.loadPixmap(0, "dframeIB.xpm");
        frameBR[1][0] = paths.loadPixmap(0, "dframeIBR.xpm");
        frameTL[1][1] = paths.loadPixmap(0, "dframeATL.xpm");
        frameT [1][1] = paths.loadPixmap(0, "dframeAT.xpm");
        frameTR[1][1] = paths.loadPixmap(0, "dframeATR.xpm");
        frameL [1][1] = paths.loadPixmap(0, "dframeAL.xpm");
        frameR [1][1] = paths.loadPixmap(0, "dframeAR.xpm");
        frameBL[1][1] = paths.loadPixmap(0, "dframeABL.xpm");
        frameB [1][1] = paths.loadPixmap(0, "dframeAB.xpm");
        frameBR[1][1] = paths.loadPixmap(0, "dframeABR.xpm");


        titleJ[0] = paths.loadPixmap(0, "titleIJ.xpm");
        titleL[0] = paths.loadPixmap(0, "titleIL.xpm");
        titleS[0] = paths.loadPixmap(0, "titleIS.xpm");
        titleP[0] = paths.loadPixmap(0, "titleIP.xpm");
        titleT[0] = paths.loadPixmap(0, "titleIT.xpm");
        titleM[0] = paths.loadPixmap(0, "titleIM.xpm");
        titleB[0] = paths.loadPixmap(0, "titleIB.xpm");
        titleR[0] = paths.loadPixmap(0, "titleIR.xpm");
        titleQ[0] = paths.loadPixmap(0, "titleIQ.xpm");
        titleJ[1] = paths.loadPixmap(0, "titleAJ.xpm");
        titleL[1] = paths.loadPixmap(0, "titleAL.xpm");
        titleS[1] = paths.loadPixmap(0, "titleAS.xpm");
        titleP[1] = paths.loadPixmap(0, "titleAP.xpm");
        titleT[1] = paths.loadPixmap(0, "titleAT.xpm");
        titleM[1] = paths.loadPixmap(0, "titleAM.xpm");
        titleB[1] = paths.loadPixmap(0, "titleAB.xpm");
        titleR[1] = paths.loadPixmap(0, "titleAR.xpm");
        titleQ[1] = paths.loadPixmap(0, "titleAQ.xpm");

#ifdef CONFIG_SHAPED_DECORATION
	bool const copyMask(true);
#else
	bool const copyMask(false);
#endif

        for (int a = 0; a <= 1; a++) {
            for (int b = 0; b <= 1; b++) {
                frameT[a][b]->replicate(true, copyMask);
                frameB[a][b]->replicate(true, copyMask);
                frameL[a][b]->replicate(false, copyMask);
                frameR[a][b]->replicate(false, copyMask);
            }
            titleS[a]->replicate(true, copyMask);
            titleT[a]->replicate(true, copyMask);
            titleB[a]->replicate(true, copyMask);
        }

        menuButton[0] =	paths.loadPixmap(0, "menuButtonI.xpm");
        menuButton[1] =	paths.loadPixmap(0, "menuButtonA.xpm");
    } else
#endif
    {
           depthPixmap[0] = paths.loadPixmap(0, "depth.xpm");
           closePixmap[0] = paths.loadPixmap(0, "close.xpm");
        maximizePixmap[0] = paths.loadPixmap(0, "maximize.xpm");
        minimizePixmap[0] = paths.loadPixmap(0, "minimize.xpm");
         restorePixmap[0] = paths.loadPixmap(0, "restore.xpm");
            hidePixmap[0] = paths.loadPixmap(0, "hide.xpm");
          rollupPixmap[0] = paths.loadPixmap(0, "rollup.xpm");
        rolldownPixmap[0] = paths.loadPixmap(0, "rolldown.xpm");
    }

      menubackPixmap = paths.loadPixmap(0, "menubg.xpm");
       menuselPixmap = paths.loadPixmap(0, "menusel.xpm");
       menusepPixmap = paths.loadPixmap(0, "menusep.xpm");
    switchbackPixmap = paths.loadPixmap(0, "switchbg.xpm");
        logoutPixmap = paths.loadPixmap(0, "logoutbg.xpm");

#ifdef CONFIG_TASKBAR
    if (!showTaskBar) {
        taskbuttonPixmap =
	    paths.loadPixmap("taskbar/", "taskbuttonbg.xpm");
        taskbuttonactivePixmap =
	    paths.loadPixmap("taskbar/", "taskbuttonactive.xpm");
    }
#endif


    YColor *c = DesktopBackgroundColor && DesktopBackgroundColor[0]
	      ? new YColor(DesktopBackgroundColor) : YColor::black;
    unsigned long bPixel = c->pixel();
    Pixmap bgPixmap = None;
    bool handleBackground = false;

    if (DesktopBackgroundPixmap && DesktopBackgroundPixmap[0]) {
        YPixmap *bg = 0;
        if (DesktopBackgroundPixmap[0] == '/') {
            if (access(DesktopBackgroundPixmap, R_OK) == 0) {
                bg = new YPixmap(DesktopBackgroundPixmap); // should be a separate program to reduce memory waste
            }
        } else
            bg = paths.loadPixmap(0, DesktopBackgroundPixmap);

        if (bg) {
            if (centerBackground) {
                YPixmap *back = new YPixmap(desktop->width(), desktop->height());;
                Graphics g(back);;

                g.setColor(c);
                g.fillRect(0, 0, desktop->width(), desktop->height());
                g.drawPixmap(bg, (desktop->width() -  bg->width()) / 2,
				(desktop->height() - bg->height()) / 2);
                delete bg;
                bg = back;
            }
	    
            XSetWindowBackgroundPixmap (app->display(), desktop->handle(),
	    				bgPixmap = bg->pixmap());
	    handleBackground = true;
        }
    } else if (DesktopBackgroundColor && DesktopBackgroundColor[0]) {
        XSetWindowBackground(app->display(), desktop->handle(), bPixel);
	handleBackground = true;
    }

    if (handleBackground && supportSemitransparency && 
	_XA_XROOTPMAP_ID && _XA_XROOTCOLOR_PIXEL) {
	XChangeProperty(app->display(), desktop->handle(),
			_XA_XROOTPMAP_ID, XA_PIXMAP, 32,
			PropModeReplace, (unsigned char*) &bgPixmap, 1);
	XChangeProperty(app->display(), desktop->handle(),
			_XA_XROOTCOLOR_PIXEL, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char*) &bPixel, 1);
        XClearWindow(app->display(), desktop->handle());
    }
}

static void initMenus() {
#ifdef CONFIG_WINMENU
    windowListMenu = new WindowListMenu();
    windowListMenu->setShared(true); // !!!
    windowListMenu->setActionListener(wmapp);
#endif

    if (showLogoutMenu) {
	logoutMenu = new YMenu();
	PRECONDITION(logoutMenu != 0);
	
	logoutMenu->setShared(true); /// !!! get rid of this (refcount objects)
	logoutMenu->addItem(_("_Logout"), -2, "", actionLogout)->setChecked(true);
	logoutMenu->addItem(_("_Cancel logout"), -2, "", actionCancelLogout)->setEnabled(false);
	logoutMenu->addSeparator();
	
#ifndef NO_CONFIGURE_MENUS
    {
        const char ** args = new const char*[4];
        args[0] = newstr(ICEWMEXE);
        args[1] = configArg ? newstr("-c") : 0;
        args[2] = configArg;
        args[3] = 0;
        DProgram *re_icewm(DProgram::newProgram
	    (_("Restart _Icewm"), 0, true, 0, ICEWMEXE, args)); //!!!
        if (re_icewm)
            logoutMenu->add(new DObjectMenuItem(re_icewm));
    }
    {
        DProgram *re_xterm
	    (DProgram::newProgram(_("Restart _Xterm"), 0, true, 0, "xterm", 0));
        if (re_xterm)
            logoutMenu->add(new DObjectMenuItem(re_xterm));
    }
#endif
    }

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
#ifndef CONFIG_PDA		    
    windowMenu->addItem(_("_Hide"),     -2, KEY_NAME(gKeyWinHide), actionHide);
#endif    
    windowMenu->addItem(_("Roll_up"),   -2, KEY_NAME(gKeyWinRollup), actionRollup);
    windowMenu->addSeparator();
    windowMenu->addItem(_("R_aise"),    -2, KEY_NAME(gKeyWinRaise), actionRaise);
    windowMenu->addItem(_("_Lower"),    -2, KEY_NAME(gKeyWinLower), actionLower);
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
    manager->activateWorkspace(ws);
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

        sprintf(number, "%d", xev->request_code);
        XGetErrorDatabaseText(display,
                              "XRequest",
                              number, "",
                              req, sizeof(req));
        if (!req[0])
            sprintf(req, "[request_code=%d]", xev->request_code);

        if (XGetErrorText(display,
                          xev->error_code,
                          message, sizeof(message)) !=
			  Success);
	    *message = '\0';

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

void runRestart(const char *str, const char **args) {
    XSync(app->display(), False);
    ///!!! problem with repeated SIGHUP for restart...
    app->resetSignals();

    if (str) {
        if (args) {
            execvp(str, (char * const *) args);
        } else {
            execlp(str, str, 0);
        }
    } else {
        const char *c = configArg ? "-c" : NULL;
        execlp(ICEWMEXE, ICEWMEXE, c, configArg, 0);
    }

    app->alert();
    warn(_("Could not restart: %s\n%s not in $PATH?"), 
	   strerror(errno), str ? str : ICEWMEXE );
}

void YWMApp::restartClient(const char *str, const char **args) {
    phase = phaseRestart;
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geRestart);
#endif
#ifdef CONFIG_WM_SESSION
    resetResourceManager();
#endif
    manager->unmanageClients();
    unregisterProtocols();

    runRestart(str, args);

    /* somehow exec failed, try to recover */
    phase = phaseStartup;
    registerProtocols();
    manager->manageClients();
}

void YWMApp::runOnce(const char *resource, const char *str, const char **args) {
    Window win(manager->findWindow(resource));

    if (win) {
	YFrameWindow * frame(manager->findFrame(win));
	if (frame) frame->activate();
	else XMapRaised(app->display(), win);
    } else
	runProgram(str, args);
}

void YWMApp::runCommandOnce(const char *resource, const char *cmdline) {
    char const * argv[] = { "/bin/sh", "-c", cmdline, NULL };

    if (resource)
	runOnce(resource, argv[0], argv);
    else
	runProgram(argv[0], argv);
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
#ifdef CONFIG_WM_SESSION
	resetResourceManager();
#endif
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
                manager->activateWorkspace(w);
                return ;
            }
        }
    }
}

YWMApp::YWMApp(int *argc, char ***argv, const char *displayName): 
    YApplication(argc, argv, displayName) {
    wmapp = this;

#ifndef LITE
    if (autoDetectGnome) {
        if (detectGNOME()) {
#ifdef CONFIG_TASKBAR
            showTaskBar = false;
#endif
            useRootButtons = 0;
            DesktopBackgroundColor = 0;
            DesktopBackgroundPixmap = 0;
            // !!! more to come, probably
        }
    }
#endif

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
    initFontPath();
    initIconSize();
    initPixmaps();
    initMenus();

    if (scrollBarWidth == 0)
	switch(wmLook) {
	    case lookWarp4:
		scrollBarWidth = 14;
		break;

	    case lookMotif:
	    case lookGtk:
		scrollBarWidth = 15;
		break;

	    case lookNice:
	    case lookWin95:
	    case lookWarp3:
	    case lookPixmap:
		scrollBarWidth = 16;
		break;

	    case lookMetal:
		scrollBarWidth = 17;
		break;
		
	    case lookMAX:
		break;
	}

    if (scrollBarHeight == 0)
	switch(wmLook) {
	    case lookWarp4:
		scrollBarHeight = 20;
		break;

	    case lookMotif:
	    case lookGtk:
		scrollBarHeight = scrollBarWidth;
		break;

	    case lookNice:
	    case lookWin95:
	    case lookWarp3:
	    case lookPixmap:
		scrollBarHeight = scrollBarWidth;
		break;

	    case lookMetal:
		scrollBarHeight = scrollBarWidth;
		break;
		
	    case lookMAX:
		break;
	}

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
    delete menuselPixmap;
    delete menusepPixmap;
    delete switchbackPixmap;
    delete logoutPixmap;

#ifdef CONFIG_TASKBAR
    if (!showTaskBar) {
        delete taskbuttonactivePixmap;
        delete taskbuttonminimizedPixmap;
    }
#endif

    //!!!XFreeGC(display(), outlineGC); lazy init in movesize.cc
    //!!!XFreeGC(display(), clipPixmapGC); in ypaint.cc

    XFlush(display());
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

#ifdef CONFIG_WM_SESSION
    case SIGUSR1:
	manager->removeLRUProcess();
	break;
#endif

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

        if (m1 == 0 && app->WinMask)
            if (k1 == app->Win_L && k2 == app->Win_L) {
                manager->popupStartMenu();
            }
#ifdef CONFIG_WINLIST
            else if (k1 == app->Win_R && k2 == app->Win_R) {
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
    if (loc == NULL || !strcmp(loc, "C") || !strcmp(loc, "POSIX") ||
        !XSupportsLocale())
        multiByte = false;
    else
        multiByte = true;
#endif
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCDIR);
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
                fputs("icewm " VERSION ", Copyright 1997-2001 Marko Macek\n",
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

        if (themeName)
	    if (*themeName == '/')
                loadConfiguration(themeName);
	    else {
		char *theme(strJoin("themes/", themeName, NULL));
		char *themePath(app->findConfigFile(theme));

		if (themePath) loadConfiguration(themePath);

		delete[] themePath;
		delete[] theme;
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
#ifdef CONFIG_WM_SESSION
    app.catchSignal(SIGUSR1);

    initResourceManager(getpid());
#endif
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
    phase = phaseShutdown;
#ifdef CONFIG_GUIEVENTS
    app.signalGuiEvent(geShutdown);
#endif
#ifdef CONFIG_WM_SESSION
    resetResourceManager();
#endif
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
    
    if (logoutMenu) {
	logoutMenu->disableCommand(actionLogout);
	logoutMenu->enableCommand(actionCancelLogout);
    }
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
    
    if (logoutMenu) {
	logoutMenu->enableCommand(actionLogout);
	logoutMenu->disableCommand(actionCancelLogout);
    }
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
