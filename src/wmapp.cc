/*
 * IceWM
 *
 * Copyright (C) 1997-2003 Marko Macek
 */
#include "config.h"
#include "yfull.h"
#include "atasks.h"
#include "atray.h"
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
#include "browse.h"
#include "objmenu.h"
#include "objbutton.h"
#include "aworkspaces.h"
#include "themes.h"
#include "sysdep.h"
#include "prefs.h"
#include "ypixbuf.h"
#include "ylocale.h"
#include <stdio.h>
#include <sys/resource.h>
#include "yrect.h"

#include "intl.h"

char const *ApplicationName("IceWM");
int rebootOrShutdown = 0;
static bool initializing(true);
static bool restart(false);

YWMApp *wmapp(NULL);
YWindowManager *manager(NULL);

char *keysFile(NULL);

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
YCursor YWMApp::scrollLeftPointer;
YCursor YWMApp::scrollRightPointer;
YCursor YWMApp::scrollUpPointer;
YCursor YWMApp::scrollDownPointer;

YMenu *windowMenu(NULL);
YMenu *moveMenu(NULL);
YMenu *layerMenu(NULL);

#ifdef CONFIG_TRAY
YMenu *trayMenu(NULL);
#endif

#ifdef CONFIG_WINMENU
YMenu *windowListMenu(NULL);
#endif

#ifdef CONFIG_WINLIST
YMenu *windowListPopup(NULL);
YMenu *windowListAllPopup(NULL);
#endif

YMenu *logoutMenu(NULL);

char *configArg(NULL);

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
#ifdef CONFIG_TRAY
	_XA_WIN_TRAY,
#endif
	_XA_WIN_SUPPORTING_WM_CHECK,
        _XA_WIN_CLIENT_LIST
#if defined(GNOME1_HINTS) && defined(WMSPEC_HINTS)
        /**/,
#endif
#ifdef WMSPEC_HINTS
        _XA_NET_SUPPORTING_WM_CHECK,
        _XA_NET_SUPPORTED,
        _XA_NET_CLIENT_LIST,
        _XA_NET_CLIENT_LIST_STACKING,
        _XA_NET_NUMBER_OF_DESKTOPS,
        _XA_NET_CURRENT_DESKTOP,
        _XA_NET_WM_DESKTOP,
        _XA_NET_ACTIVE_WINDOW,
        _XA_NET_CLOSE_WINDOW,
        _XA_NET_WM_STRUT,
        _XA_NET_WORKAREA,
        _XA_NET_WM_STATE,
        _XA_NET_WM_STATE_MAXIMIZED_VERT,
        _XA_NET_WM_STATE_MAXIMIZED_HORZ,
        _XA_NET_WM_STATE_SHADED,
        _XA_NET_WM_STATE_FULLSCREEN
#endif
    };
    unsigned int i = sizeof(win_proto) / sizeof(win_proto[0]);

#ifdef GNOME1_HINTS
    XChangeProperty(app->display(), manager->handle(),
                    _XA_WIN_PROTOCOLS, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)win_proto, i);
#endif


#ifdef WMSPEC_HINTS
    XChangeProperty(app->display(), manager->handle(),
                    _XA_NET_SUPPORTED, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)win_proto, i);
#endif

    YWindow *checkWindow = new YWindow();
    Window xid = checkWindow->handle();

    pid_t pid = getpid();
    const char wmname[] = "IceWM "VERSION" ("HOSTOS"/"HOSTCPU")";

#ifdef GNOME1_HINTS
    XChangeProperty(app->display(), checkWindow->handle(),
                    _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    XChangeProperty(app->display(), manager->handle(),
                    _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);
#endif

#ifdef WMSPEC_HINTS
    XChangeProperty(app->display(), checkWindow->handle(),
                    _XA_NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);

    XChangeProperty(app->display(), checkWindow->handle(),
                    _XA_NET_WM_PID, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&pid, 1);

    XChangeProperty(app->display(), checkWindow->handle(),
                    _XA_NET_WM_NAME, XA_STRING, 8,
                    PropModeReplace, (unsigned char *)wmname, sizeof(wmname));

    XChangeProperty(app->display(), manager->handle(),
                    _XA_NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&xid, 1);
#endif

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
	    newFontPath[ndirs] = fontsdir;

	    if (fontPath)
		memcpy(newFontPath, fontPath, ndirs * sizeof (char *));
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
		    for (int n(ndirs - 1); n > 0; --n) // ---- remove death paths ---
			if (!strcmp(icewmFontPath, newFontPath[n])) {
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
    YWMApp::sizeRightPointer.	   load("sizeR.xpm",   XC_right_side);
    YWMApp::sizeTopRightPointer.   load("sizeTR.xpm",  XC_top_right_corner);
    YWMApp::sizeTopPointer.	   load("sizeT.xpm",   XC_top_side);
    YWMApp::sizeTopLeftPointer.	   load("sizeTL.xpm",  XC_top_left_corner);
    YWMApp::sizeLeftPointer.	   load("sizeL.xpm",   XC_left_side);
    YWMApp::sizeBottomLeftPointer. load("sizeBL.xpm",  XC_bottom_left_corner);
    YWMApp::sizeBottomPointer.	   load("sizeB.xpm",   XC_bottom_side);
    YWMApp::sizeBottomRightPointer.load("sizeBR.xpm",  XC_bottom_right_corner);
    YWMApp::scrollLeftPointer.     load("scrollL.xpm", XC_sb_left_arrow);
    YWMApp::scrollRightPointer.    load("scrollR.xpm", XC_sb_right_arrow);
    YWMApp::scrollUpPointer.       load("scrollU.xpm", XC_sb_up_arrow);
    YWMApp::scrollDownPointer.     load("scrollD.xpm", XC_sb_down_arrow);
}

 // should be a separate program to reduce memory waste
static YPixmap * renderBackground(YResourcePaths const & paths,
				  char const * filename, YColor * color) {
    YPixmap *back(NULL);

    if (*filename == '/') {
	if (access(filename, R_OK) == 0)
	    back = new YPixmap(filename);
    } else
	back = paths.loadPixmap(0, filename);

    if (back && centerBackground) {
	YPixmap * cBack = new YPixmap(desktop->width(), desktop->height());
	Graphics g(*cBack);

        g.setColor(color);
        g.fillRect(0, 0, desktop->width(), desktop->height());
        g.drawPixmap(back, (desktop->width() -  back->width()) / 2,
			   (desktop->height() - back->height()) / 2);

        delete back;
        back = cBack;
    }

    return back;
}

#ifdef CONFIG_GRADIENTS
static bool loadGradient(YResourcePaths const & paths,
			 char const * tag, YPixbuf *& pixbuf,
			 char const * name, char const * path = NULL) {
    if (!strcmp(tag, name)) {
	if (pixbuf == NULL)
	    pixbuf = paths.loadPixbuf(path, name, false);
	else
	    warn(_("Multiple references for gradient \"%s\""), name);

	return false;
    }

    return true;
}
#endif

static void initPixmaps() {
    YResourcePaths paths("", true);

#ifdef CONFIG_LOOK_PIXMAP
    if (wmLook == lookPixmap || wmLook == lookMetal || wmLook == lookGtk) {
#ifdef CONFIG_GRADIENTS
	if (gradients) {
	    for (char const * g(gradients + strspn(gradients, " \t\r\n"));
	         *g != '\0'; g = strnxt(g, " \t\r\n")) {
		char const * gradient(newstr(g, " \t\r\n"));

		if (loadGradient(paths, gradient, rgbTitleS[0], "titleIS.xpm") &&
		    loadGradient(paths, gradient, rgbTitleT[0], "titleIT.xpm") &&
		    loadGradient(paths, gradient, rgbTitleB[0], "titleIB.xpm") &&
		    loadGradient(paths, gradient, rgbTitleS[1], "titleAS.xpm") &&
		    loadGradient(paths, gradient, rgbTitleT[1], "titleAT.xpm") &&
		    loadGradient(paths, gradient, rgbTitleB[1], "titleAB.xpm") &&

		    loadGradient(paths, gradient, rgbFrameT[0][0], "frameIT.xpm") &&
		    loadGradient(paths, gradient, rgbFrameL[0][0], "frameIL.xpm") &&
		    loadGradient(paths, gradient, rgbFrameR[0][0], "frameIR.xpm") &&
		    loadGradient(paths, gradient, rgbFrameB[0][0], "frameIB.xpm") &&
		    loadGradient(paths, gradient, rgbFrameT[0][1], "frameAT.xpm") &&
		    loadGradient(paths, gradient, rgbFrameL[0][1], "frameAL.xpm") &&
		    loadGradient(paths, gradient, rgbFrameR[0][1], "frameAR.xpm") &&
		    loadGradient(paths, gradient, rgbFrameB[0][1], "frameAB.xpm") &&

		    loadGradient(paths, gradient, rgbFrameT[1][0], "dframeIT.xpm") &&
		    loadGradient(paths, gradient, rgbFrameL[1][0], "dframeIL.xpm") &&
		    loadGradient(paths, gradient, rgbFrameR[1][0], "dframeIR.xpm") &&
		    loadGradient(paths, gradient, rgbFrameB[1][0], "dframeIB.xpm") &&
		    loadGradient(paths, gradient, rgbFrameT[1][1], "dframeAT.xpm") &&
		    loadGradient(paths, gradient, rgbFrameL[1][1], "dframeAL.xpm") &&
		    loadGradient(paths, gradient, rgbFrameR[1][1], "dframeAR.xpm") &&
		    loadGradient(paths, gradient, rgbFrameB[1][1], "dframeAB.xpm") &&

#ifdef CONFIG_TASKBAR
		    loadGradient(paths, gradient, taskbackPixbuf,
		    		 "taskbarbg.xpm", "taskbar/") &&
		    loadGradient(paths, gradient, taskbuttonPixbuf,
		    		 "taskbuttonbg.xpm", "taskbar/") &&
		    loadGradient(paths, gradient, taskbuttonactivePixbuf,
		    		 "taskbuttonactive.xpm", "taskbar/") &&
		    loadGradient(paths, gradient, taskbuttonminimizedPixbuf,
		    		 "taskbuttonminimized.xpm", "taskbar/") &&
		    loadGradient(paths, gradient, toolbuttonPixbuf,
				 "toolbuttonbg.xpm", "taskbar/") &&
		    loadGradient(paths, gradient, workspacebuttonPixbuf,
				 "workspacebuttonbg.xpm", "taskbar/") &&
		    loadGradient(paths, gradient, workspacebuttonactivePixbuf,
				 "workspacebuttonactive.xpm", "taskbar/") &&
#endif

		    loadGradient(paths, gradient, buttonIPixbuf, "buttonI.xpm") &&
		    loadGradient(paths, gradient, buttonAPixbuf, "buttonA.xpm") &&

		    loadGradient(paths, gradient, logoutPixbuf, "logoutbg.xpm") &&
		    loadGradient(paths, gradient, switchbackPixbuf, "switchbg.xpm") &&
		    loadGradient(paths, gradient, listbackPixbuf, "listbg.xpm") &&
		    loadGradient(paths, gradient, dialogbackPixbuf, "dialogbg.xpm") &&

		    loadGradient(paths, gradient, menubackPixbuf, "menubg.xpm") &&
		    loadGradient(paths, gradient, menuselPixbuf, "menusel.xpm") &&
		    loadGradient(paths, gradient, menusepPixbuf, "menusep.xpm"))
		    warn(_("Unknown gradient name: %s"), gradient);

		delete[] gradient;
	    }

	    delete[] gradients;
	    gradients = NULL;
	}
#endif

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
        frameTR[0][0] = paths.loadPixmap(0, "frameITR.xpm");
        frameBL[0][0] = paths.loadPixmap(0, "frameIBL.xpm");
        frameBR[0][0] = paths.loadPixmap(0, "frameIBR.xpm");
        frameTL[0][1] = paths.loadPixmap(0, "frameATL.xpm");
        frameTR[0][1] = paths.loadPixmap(0, "frameATR.xpm");
        frameBL[0][1] = paths.loadPixmap(0, "frameABL.xpm");
        frameBR[0][1] = paths.loadPixmap(0, "frameABR.xpm");

        frameTL[1][0] = paths.loadPixmap(0, "dframeITL.xpm");
        frameTR[1][0] = paths.loadPixmap(0, "dframeITR.xpm");
        frameBL[1][0] = paths.loadPixmap(0, "dframeIBL.xpm");
        frameBR[1][0] = paths.loadPixmap(0, "dframeIBR.xpm");
        frameTL[1][1] = paths.loadPixmap(0, "dframeATL.xpm");
        frameTR[1][1] = paths.loadPixmap(0, "dframeATR.xpm");
        frameBL[1][1] = paths.loadPixmap(0, "dframeABL.xpm");
        frameBR[1][1] = paths.loadPixmap(0, "dframeABR.xpm");

	if (TEST_GRADIENT(NULL == rgbFrameT[0][0]))
	    frameT[0][0] = paths.loadPixmap(0, "frameIT.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameL[0][0]))
	    frameL[0][0] = paths.loadPixmap(0, "frameIL.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameR[0][0]))
            frameR[0][0] = paths.loadPixmap(0, "frameIR.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameB[0][0]))
            frameB[0][0] = paths.loadPixmap(0, "frameIB.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameT[0][1]))
            frameT[0][1] = paths.loadPixmap(0, "frameAT.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameL[0][1]))
            frameL[0][1] = paths.loadPixmap(0, "frameAL.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameR[0][1]))
            frameR[0][1] = paths.loadPixmap(0, "frameAR.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameB[0][1]))
            frameB[0][1] = paths.loadPixmap(0, "frameAB.xpm");

	if (TEST_GRADIENT(NULL == rgbFrameT[1][0]))
	    frameT[1][0] = paths.loadPixmap(0, "dframeIT.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameL[1][0]))
	    frameL[1][0] = paths.loadPixmap(0, "dframeIL.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameR[1][0]))
            frameR[1][0] = paths.loadPixmap(0, "dframeIR.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameB[1][0]))
            frameB[1][0] = paths.loadPixmap(0, "dframeIB.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameT[1][1]))
            frameT[1][1] = paths.loadPixmap(0, "dframeAT.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameL[1][1]))
            frameL[1][1] = paths.loadPixmap(0, "dframeAL.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameR[1][1]))
            frameR[1][1] = paths.loadPixmap(0, "dframeAR.xpm");
	if (TEST_GRADIENT(NULL == rgbFrameB[1][1]))
            frameB[1][1] = paths.loadPixmap(0, "dframeAB.xpm");

        titleJ[0] = paths.loadPixmap(0, "titleIJ.xpm");
        titleL[0] = paths.loadPixmap(0, "titleIL.xpm");
        titleP[0] = paths.loadPixmap(0, "titleIP.xpm");
        titleM[0] = paths.loadPixmap(0, "titleIM.xpm");
        titleB[0] = paths.loadPixmap(0, "titleIB.xpm");
        titleR[0] = paths.loadPixmap(0, "titleIR.xpm");
        titleQ[0] = paths.loadPixmap(0, "titleIQ.xpm");
        titleJ[1] = paths.loadPixmap(0, "titleAJ.xpm");
        titleL[1] = paths.loadPixmap(0, "titleAL.xpm");
        titleP[1] = paths.loadPixmap(0, "titleAP.xpm");
        titleM[1] = paths.loadPixmap(0, "titleAM.xpm");
        titleR[1] = paths.loadPixmap(0, "titleAR.xpm");
        titleQ[1] = paths.loadPixmap(0, "titleAQ.xpm");

//	if (TEST_GRADIENT(NULL == rgbTitleS[0]))
	    titleS[0] = paths.loadPixmap(0, "titleIS.xpm");
//	if (TEST_GRADIENT(NULL == rgbTitleT[0]))
	    titleT[0] = paths.loadPixmap(0, "titleIT.xpm");
//	if (TEST_GRADIENT(NULL == rgbTitleB[0]))
	    titleB[0] = paths.loadPixmap(0, "titleIB.xpm");
//	if (TEST_GRADIENT(NULL == rgbTitleS[1]))
	    titleS[1] = paths.loadPixmap(0, "titleAS.xpm");
//	if (TEST_GRADIENT(NULL == rgbTitleT[1]))
	    titleT[1] = paths.loadPixmap(0, "titleAT.xpm");
//	if (TEST_GRADIENT(NULL == rgbTitleB[1]))
	    titleB[1] = paths.loadPixmap(0, "titleAB.xpm");
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

    if (TEST_GRADIENT(NULL == logoutPixbuf))
	logoutPixmap = paths.loadPixmap(0, "logoutbg.xpm");
    if (TEST_GRADIENT(NULL == switchbackPixbuf))
	switchbackPixmap = paths.loadPixmap(0, "switchbg.xpm");
    if (TEST_GRADIENT(NULL == menubackPixbuf))
	menubackPixmap = paths.loadPixmap(0, "menubg.xpm");
    if (TEST_GRADIENT(NULL == menuselPixbuf))
	menuselPixmap = paths.loadPixmap(0, "menusel.xpm");
    if (TEST_GRADIENT(NULL == menusepPixbuf))
	menusepPixmap = paths.loadPixmap(0, "menusep.xpm");

#ifndef LITE
    if (TEST_GRADIENT(NULL == listbackPixbuf) &&
        NULL == (listbackPixmap = paths.loadPixmap(0, "listbg.xpm")))
        listbackPixmap = menubackPixmap;
#endif
    if (TEST_GRADIENT(NULL == dialogbackPixbuf) &&
        NULL == (dialogbackPixmap = paths.loadPixmap(0, "dialogbg.xpm")))
        dialogbackPixmap = menubackPixmap;
    if (TEST_GRADIENT(NULL == buttonIPixbuf) &&
        NULL == (buttonIPixmap = paths.loadPixmap(0, "buttonI.xpm")))
        buttonIPixmap = paths.loadPixmap("taskbar/", "taskbuttonbg.xpm");
    if (TEST_GRADIENT(NULL == buttonAPixbuf) &&
        NULL == (buttonAPixmap = paths.loadPixmap(0, "buttonA.xpm")))
        buttonAPixmap = paths.loadPixmap("taskbar/", "taskbuttonactive.xpm");

#ifdef CONFIG_TASKBAR
    if (TEST_GRADIENT(NULL == toolbuttonPixbuf) &&
        NULL == (toolbuttonPixmap =
		 paths.loadPixmap("taskbar/", "toolbuttonbg.xpm")))
	IF_CONFIG_GRADIENTS (buttonIPixbuf,
			     toolbuttonPixbuf = buttonIPixbuf)
			else toolbuttonPixmap = buttonIPixmap;
    if (TEST_GRADIENT(NULL == workspacebuttonPixbuf) &&
        NULL == (workspacebuttonPixmap =
		 paths.loadPixmap("taskbar/", "workspacebuttonbg.xpm")))
	IF_CONFIG_GRADIENTS (buttonIPixbuf,
			     workspacebuttonPixbuf = buttonIPixbuf)
			else workspacebuttonPixmap = buttonIPixmap;
    if (TEST_GRADIENT(NULL == workspacebuttonactivePixbuf) &&
        NULL == (workspacebuttonactivePixmap =
		 paths.loadPixmap("taskbar/", "workspacebuttonactive.xpm")))
	IF_CONFIG_GRADIENTS (buttonAPixbuf,
			     workspacebuttonactivePixbuf = buttonAPixbuf)
			else workspacebuttonactivePixmap = buttonAPixmap;
#endif

    if (logoutPixmap) {
	logoutPixmap->replicate(true, false);
	logoutPixmap->replicate(false, false);
    }
    if (switchbackPixmap) {
	switchbackPixmap->replicate(true, false);
	switchbackPixmap->replicate(false, false);
    }

    if (menubackPixmap) {
	menubackPixmap->replicate(true, false);
	menubackPixmap->replicate(false, false);
    }
    if (menusepPixmap)
	menusepPixmap->replicate(true, false);
    if (menuselPixmap)
	menuselPixmap->replicate(true, false);

#ifndef LITE
    if (listbackPixmap) {
	listbackPixmap->replicate(true, false);
	listbackPixmap->replicate(false, false);
    }
#endif

    if (dialogbackPixmap) {
	dialogbackPixmap->replicate(true, false);
	dialogbackPixmap->replicate(false, false);
    }

    if (buttonIPixmap)
	buttonIPixmap->replicate(true, false);
    if (buttonAPixmap)
	buttonAPixmap->replicate(true, false);

    YColor * bColor((DesktopBackgroundColor && DesktopBackgroundColor[0])
                    ? new YColor(DesktopBackgroundColor)
                    : 0);

    wmapp->bgColor = bColor;

    if (bColor == 0)
        bColor = YColor::black;

    unsigned long const bPixel(bColor->pixel());
    bool handleBackground(false);
    Pixmap bPixmap(None);

    if (DesktopBackgroundPixmap && DesktopBackgroundPixmap[0]) {
        YPixmap * back(renderBackground(paths, DesktopBackgroundPixmap,
					bColor));

        if (back) {
	    bPixmap = back->pixmap();
            XSetWindowBackgroundPixmap(app->display(), desktop->handle(),
	    			       bPixmap);
	    handleBackground = true;
        }
    } else if (DesktopBackgroundColor && DesktopBackgroundColor[0]) {
        XSetWindowBackground(app->display(), desktop->handle(), bPixel);
	handleBackground = true;
    }

    if (handleBackground) {
        if (supportSemitransparency &&
            _XA_XROOTPMAP_ID && _XA_XROOTCOLOR_PIXEL) {
            if (DesktopBackgroundPixmap &&
                DesktopTransparencyPixmap &&
                !strcmp (DesktopBackgroundPixmap,
                         DesktopTransparencyPixmap)) {
                delete[] DesktopTransparencyPixmap;
                DesktopTransparencyPixmap = NULL;
            }

	    YColor * tColor(DesktopTransparencyColor &&
	    		    DesktopTransparencyColor[0]
			  ? new YColor(DesktopTransparencyColor)
			  : bColor);

	    YPixmap * root(DesktopTransparencyPixmap &&
	    		   DesktopTransparencyPixmap[0]
			 ? renderBackground(paths, DesktopTransparencyPixmap,
			 		    tColor) : NULL);

	    unsigned long const tPixel(tColor->pixel());
	    Pixmap const tPixmap(root ? root->pixmap() : bPixmap);

	    XChangeProperty(app->display(), desktop->handle(),
			    _XA_XROOTPMAP_ID, XA_PIXMAP, 32,
			    PropModeReplace, (unsigned char const*)&tPixmap, 1);
	    XChangeProperty(app->display(), desktop->handle(),
			    _XA_XROOTCOLOR_PIXEL, XA_CARDINAL, 32,
			    PropModeReplace, (unsigned char const*)&tPixel, 1);
	}

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
	if (showLogoutSubMenu) {
            logoutMenu->addItem(_("_Logout"), -2, "", actionLogout)->setChecked(true);
            logoutMenu->addItem(_("_Cancel logout"), -2, "", actionCancelLogout)->setEnabled(false);
            logoutMenu->addSeparator();

#ifndef NO_CONFIGURE_MENUS
            YStringArray noargs;

            DProgram *restartIcewm =
                DProgram::newProgram(_("Restart _Icewm"), 0, true, 0, 0, noargs);
            if (restartIcewm)
                logoutMenu->add(new DObjectMenuItem(restartIcewm));

            DProgram *restartXTerm =
                DProgram::newProgram(_("Restart _Xterm"), 0, true, 0, "xterm", noargs);
            if (restartXTerm)
                logoutMenu->add(new DObjectMenuItem(restartXTerm));
#endif
        }
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

#ifdef CONFIG_TRAY
    if (taskBarShowTray) {
	trayMenu = new YMenu();
	trayMenu->setShared(true);

	trayMenu->addItem(_("_No icon"),   -2, 0, trayOptionActionSet[WinTrayIgnore]);
	trayMenu->addItem(_("_Minimized"), -2, 0, trayOptionActionSet[WinTrayMinimized]);
	trayMenu->addItem(_("_Exclusive"), -2, 0, trayOptionActionSet[WinTrayExclusive]);
    }
#endif

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
    if (allowFullscreen)
        windowMenu->addItem(_("_Fullscreen"), -2, KEY_NAME(gKeyWinFullscreen), actionFullscreen);

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

    /// this should probably go away, cause fullscreen will do mostly the same thing
#if DO_NOT_COVER_OLD
    if (!limitByDockLayer)
        windowMenu->addItem(_("Limit _Workarea"), -2, 0, actionDoNotCover);
#endif

#ifdef CONFIG_TRAY
    if (trayMenu) {
    windowMenu->addSeparator();
    windowMenu->addSubmenu(_("Tray _icon"), -2, trayMenu);
    }
#endif

    windowMenu->addSeparator();
    windowMenu->addItem(_("_Close"), -2, KEY_NAME(gKeyWinClose), actionClose);
#ifdef CONFIG_WINLIST
    windowMenu->addSeparator();
    windowMenu->addItem(_("_Window list"), -2, KEY_NAME(gKeySysWindowList), actionWindowList);
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

void runRestart(const char *path, char *const *args) {
    XSync(app->display(), False);
    ///!!! problem with repeated SIGHUP for restart...
    app->resetSignals();

#if 1   /* for now, some debugging code */
        {
            /* close all files */

            int             i, max = 1024;
            struct rlimit   lim;

            if (getrlimit(RLIMIT_NOFILE, &lim) == 0)
                max = lim.rlim_max;

            for (i = 3; i < max; i++) {
                int fl;
                if (fcntl(i, F_GETFD, &fl) == 0) {
                    if (!(fl & FD_CLOEXEC)) {
                        warn("file descriptor still open: %d. "
                             " Check /proc/$icewmpid/fd/%d when running next time. "
                             "Please report a bug (perhaps not an icewm problem)!", i, i);
                    }
                }
                close (i);
            }
        }
#endif

    if (path) {
        if (args) {
            execvp(path, args);
        } else {
            execlp(path, path, 0);
        }
    } else {
        const char *c = configArg ? "-c" : NULL;
        execlp(ICEWMEXE, ICEWMEXE, "--restart", c, configArg, 0);
    }

    app->alert();

    warn(_("Could not restart: %s\nDoes $PATH lead to %s?"),
	   strerror(errno), path ? path : ICEWMEXE);
}

void YWMApp::restartClient(const char *path, char *const *args) {
    phase = phaseRestart;
#ifdef CONFIG_GUIEVENTS
    wmapp->signalGuiEvent(geRestart);
#endif
#ifdef CONFIG_WM_SESSION
    resetResourceManager();
#endif
    manager->wmState = YWindowManager::wmRESTART;
    manager->unmanageClients();
    unregisterProtocols();

    runRestart(path, args);

    /* somehow exec failed, try to recover */
    phase = phaseStartup;
    manager->wmState = YWindowManager::wmSTARTUP;
    registerProtocols();
    manager->manageClients();
}

void YWMApp::runOnce(const char *resource, const char *path, char *const *args) {
    Window win(manager->findWindow(resource));

    if (win) {
	YFrameWindow * frame(manager->findFrame(win));
	if (frame) frame->activate();
	else XMapRaised(app->display(), win);
    } else
	runProgram(path, args);
}

void YWMApp::runCommandOnce(const char *resource, const char *cmdline) {
#warning calling /bin/sh is considered to be bloat
    char const *const argv[] = { "/bin/sh", "-c", cmdline, NULL };

    if (resource)
	runOnce(resource, argv[0], (char *const *) argv);
    else
	runProgram(argv[0], (char *const *) argv);
}

void YWMApp::runSessionScript(PhaseType phase)
{
    const char *scriptname;

    switch (phase) {
    	case phaseStartup:
	    scriptname = "startup";
	    break;

        case phaseShutdown:
	    scriptname = "shutdown";
	    break;

        case phaseRestart:
	    scriptname = "restart";
	    break;

	default:
	    msg("Unexpected program state %d. Please report a bug!", phase);
	    return;
    }

    char *scriptfile = findConfigFile(scriptname, X_OK);

    if (scriptfile) {
        const char *args[] = { scriptfile, 0, 0 };
        const char **arg(args + 1);

        if (hasGNOME()) {
            *arg++ = "--with-gnome";
        }

        MSG(("Running session script: %s", scriptfile));
        runProgram(scriptfile, args);
        delete[] scriptfile;
    }
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
#else
            logout();
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
        manager->wmState = YWindowManager::wmSHUTDOWN;
        manager->unmanageClients();
        unregisterProtocols();
        exit(0);
    } else if (action == actionRefresh) {
        static YWindow *w = 0;
        if (w == 0) w = new YWindow();
        if (w) {
            w->setGeometry(YRect(0, 0,
                                 desktop->width(), desktop->height()));
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
    YSMApplication(argc, argv, displayName)
{
    wmapp = this;
    phase = phaseStartup;

    /// think hard how to make this disappear
#ifndef LITE
    if (autoDetectGnome) {
        if (hasGNOME()) {
#warning "background setting should be done by icewmbg only"
            DesktopBackgroundColor = 0;
            DesktopBackgroundPixmap = 0;
        }
    }
#endif

    if (keysFile)
        loadMenus(keysFile, 0);

    XSetErrorHandler(handler);

    fLogoutMsgBox = 0;
    bgColor = 0;

    initAtoms();
    initActions();
    initPointers();

    delete desktop;

    desktop = manager = fWindowManager =
        new YWindowManager(0, RootWindow(display(),
                                         DefaultScreen(display())));
    PRECONDITION(desktop != 0);

    registerProtocols();

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

    initializing = false;

    runSessionScript(restart ? phaseRestart : phaseStartup);
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

#ifdef CONFIG_GRADIENTS
    delete menubackPixbuf;
    delete menuselPixbuf;
    delete menusepPixbuf;
#endif
    delete bgColor;


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
#ifdef CONFIG_TRAY
    if (taskBar && taskBar->trayPane())
        taskBar->trayPane()->relayoutNow();
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

#ifndef NO_CONFIGURE

static void print_version() {
    puts("IceWM " VERSION ", "
         "Copyright 1997-2003 Marko Macek,  2001 Mathias Hasselmann");

    exit(0);
}

static void print_usage(const char *argv0) {
    const char *usage_client_id =
#ifdef CONFIG_SESSION
             "  --client-id=ID      Client id to use when contacting session manager.\n";
#else
             "";
#endif
    const char *usage_debug =
#ifdef DEBUG
             "\n"
             "  --debug             Print generic debug messages.\n"
             "  --debug-z           Print debug messages regarding window stacking.\n";
#else
             "";
#endif
    printf(_("Usage: %s [OPTIONS]\n"
             "Starts the IceWM window manager.\n"
             "\n"
             "Options:\n"
             "  --display=NAME      NAME of the X server to use.\n"
             "%s"
             "  --sync              Synchronize X11 commands.\n"
             "\n"
             "  -c, --config=FILE   Load preferences from FILE.\n"
             "  -t, --theme=FILE    Load theme from FILE.\n"
             "  -n, --no-configure  Ignore preferences file.\n"
             "\n"
             "  -v, --version       Prints version information and exits.\n"
             "  -h, --help          Prints this usage screen and exits.\n"
             "%s"
             "  --restart           Don't use this: It's an internal flag.\n"
             "\n"
             "Environment variables:\n"
	     "  ICEWM_PRIVCFG=PATH  Directory to use for user private configuration files,\n"
	     "                      \"$HOME/.icewm/\" by default.\n"
             "  DISPLAY=NAME        Name of the X server to use, depends on Xlib by default.\n"
             "  MAIL=URL            Location of your mailbox. If the schema is omitted\n"
             "                      the local \"file\" schema is assumed.\n"
             "\n"
             "Visit http://www.icewm.org/ for report bugs, "
             "support requests, comments...\n"),
             argv0,
             usage_client_id,
             usage_debug);
    exit(0);
}

#endif

int main(int argc, char **argv) {
    YLocale locale;

#ifndef NO_CONFIGURE
    char *configFile(NULL);
    char *overrideTheme(NULL);
#endif

    for (char ** arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
#ifdef DEBUG
            if (IS_LONG_SWITCH("debug"))
                debug = true;
            else if (IS_LONG_SWITCH("debug-z"))
                debug_z = true;
#endif
#ifndef NO_CONFIGURE
            char *value;

            if ((value = GET_LONG_ARGUMENT("config")) != NULL ||
                (value = GET_SHORT_ARGUMENT("c")) != NULL)
                configArg = newstr(configFile = newstr(value));
            else if ((value = GET_LONG_ARGUMENT("theme")) != NULL ||
                     (value = GET_SHORT_ARGUMENT("t")) != NULL)
                overrideTheme = value;
            else if (IS_SWITCH("n", "no-configure"))
                configurationNeeded = false;
            else if (IS_LONG_SWITCH("restart"))
                restart = true;
            else if (IS_SWITCH("v", "version"))
                print_version();
            else if (IS_SWITCH("h", "help"))
                print_usage(basename(argv[0]));
#endif
        }
    }
#ifndef NO_CONFIGURE
    if (configurationNeeded) {
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

    DEPRECATE(warpPointer == true);
    DEPRECATE(focusRootWindow == true);
    DEPRECATE(replayMenuCancelClick == true);
    DEPRECATE(manualPlacement == true);
    DEPRECATE(strongPointerFocus == true);
    DEPRECATE(showPopupsAbovePointer == true);
    DEPRECATE(considerHorizBorder == true);
    DEPRECATE(considerVertBorder == true);
    DEPRECATE(sizeMaximized == true);
    DEPRECATE(dontRotateMenuPointer == false);

    if (workspaceCount == 0)
        addWorkspace(" 0 ");

#ifndef NO_WINDOW_OPTIONS
    if (winOptFile == 0)
        winOptFile = app->findConfigFile("winoptions");
#endif

    if (keysFile == 0)
        keysFile = app->findConfigFile("keys");

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

#ifdef CONFIG_SESSION
    if (app.haveSessionManager())
        loadWindowInfo();
#endif

#ifdef CONFIG_GUIEVENTS
    app.signalGuiEvent(geStartup);
#endif
    manager->manageClients();

    int rc = app.mainLoop();
    app.phase = YWMApp::phaseShutdown;
#ifdef CONFIG_GUIEVENTS
    app.signalGuiEvent(geShutdown);
#endif
#ifdef CONFIG_WM_SESSION
    resetResourceManager();
#endif
    manager->wmState = YWindowManager::wmSHUTDOWN;
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
#ifdef CONFIG_SESSION
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
    rebootOrShutdown = false;
    if (logoutCancelCommand && logoutCancelCommand[0]) {
        runCommand(logoutCancelCommand);
#ifdef CONFIG_SESSION
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
