/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"

#ifndef NO_WINDOW_OPTIONS
#include "yfull.h"
#include "wmoption.h"
#include "wmframe.h"

#include "WinMgr.h"
#include "base.h"
#include "sysdep.h"
#include <stdlib.h>

#include "intl.h"

#if 0
static int strnullcmp(const char *a, const char *b) {
    return a ? (b ? strcmp(a, b) : 1) : (b ? -1 : 0);
}
#endif

upath winOptFile;
WindowOptions *defOptions = 0;
WindowOptions *hintOptions = 0;

WindowOption::WindowOption(ustring n_class_instance):
    w_class_instance(n_class_instance),
    icon(0),
    functions(0), function_mask(0),
    decors(0), decor_mask(0),
    options(0), option_mask(0),
    workspace(WinWorkspaceInvalid),
    layer(WinLayerInvalid),
#ifdef CONFIG_TRAY
    tray(WinTrayInvalid),
#endif
    gflags(0), gx(0), gy(0), gw(0), gh(0) 
{
}

WindowOption::~WindowOption() {
    ////delete[] name; name = 0;
    ////delete[] icon; icon = 0;
}

static int wo_cmp(ustring a_class_instance,
                  const WindowOption *pivot)
{
    int cmp = a_class_instance.compareTo(pivot->w_class_instance);
    return cmp;
}

WindowOption *WindowOptions::getWindowOption(ustring a_class_instance,
                                             bool create, bool remove)
{
    int lo = 0, hi = fWinOptions.getCount();

    while (lo < hi) {
        const int pv = (lo + hi) / 2;
	const WindowOption *pivot = fWinOptions[pv];

        int cmp = wo_cmp(a_class_instance,
                         pivot);
        if (cmp > 0) {
            lo = pv + 1;
            continue;
        } else if (cmp < 0) {
            hi = pv;
            continue;
        }

        if (remove) {
            static WindowOption result = *pivot;
            fWinOptions.remove(pv);
            return &result;
        }

        return fWinOptions.getItem(pv);
    }

    if (!create) return 0;

    WindowOption *newopt = new WindowOption(a_class_instance);

    MSG(("inserting window option %p at position %d", newopt, lo));
    fWinOptions.insert(lo, newopt);

#ifdef DEBUG
    for (int i = 0; i < fWinOptions.getCount(); ++i)
    	MSG(("> %d: %p", i, fWinOptions[i]));
#endif

    return newopt;
}

void WindowOptions::setWinOption(ustring n_class_instance,
                                 const char *opt, const char *arg)
{
    WindowOption *op = getWindowOption(n_class_instance, true);

    //msg("%s-%s-%s", class_instance, opt, arg);

    if (strcmp(opt, "icon") == 0) {
        op->icon = newstr(arg);
    } else if (strcmp(opt, "workspace") == 0) {
        op->workspace = atoi(arg);
    } else if (strcmp(opt, "geometry") == 0) {
        int rx, ry;
        unsigned int rw, rh;

        op->gx = 0;
        op->gy = 0;
        op->gw = 0;
        op->gh = 0;

        //msg("parsing %s", arg);
        if ((op->gflags = XParseGeometry(arg, &rx, &ry, &rw, &rh)) != 0) {
            if (op->gflags & XNegative)
                rx = - rx;
            if (op->gflags & YNegative)
                ry = - ry;
            op->gx = rx;
            op->gy = ry;
            op->gw = rw;
            op->gh = rh;
            //msg("parsed %d %d %d %d %X", rx, ry, rw, rh, op->gflags);
        }
    } else if (strcmp(opt, "layer") == 0) {
        char *endptr;
        long l = strtol(arg, &endptr, 10);

        op->layer = WinLayerInvalid;

        if (arg[0] && !endptr[0])
            op->layer = l;
        else {
            struct {
                const char *name;
                int layer;
            } layers[] = {
                { "Desktop", WinLayerDesktop }, //
                { "Below", WinLayerBelow }, //
                { "Normal", WinLayerNormal }, //
                { "OnTop", WinLayerOnTop }, //
                { "Dock", WinLayerDock }, //
                { "AboveDock", WinLayerAboveDock }, //
                { "Menu", WinLayerMenu }
            };
            for (unsigned int i = 0; i < ACOUNT(layers); i++)
                if (strcmp(layers[i].name, arg) == 0)
                    op->layer = layers[i].layer;
        }
#ifdef CONFIG_TRAY
    } else if (strcmp(opt, "tray") == 0) {
        char *endptr;
        long const t(strtol(arg, &endptr, 10));

        op->tray = WinTrayInvalid;

        if (arg[0] && !endptr[0])
            op->tray = t;
        else {
            struct {
                const char *name;
                int tray;
            } tray_ops[] = {
                { "Ignore", WinTrayIgnore },
                { "Minimized", WinTrayMinimized },
                { "Exclusive", WinTrayExclusive },
                { "1", WinTrayExclusive }
            };
            for (unsigned int i = 0; i < ACOUNT(tray_ops); i++)
                if (strcmp(tray_ops[i].name, arg) == 0)
                    op->tray = tray_ops[i].tray;
        }
#endif
    } else {
        static struct {
            int what;
            const char *name;
            unsigned long flag;
        } options[] = {
            { 0, "fMove", YFrameWindow::ffMove }, //
            { 0, "fResize", YFrameWindow::ffResize }, //
            { 0, "fClose", YFrameWindow::ffClose }, //
            { 0, "fMinimize", YFrameWindow::ffMinimize }, //
            { 0, "fMaximize", YFrameWindow::ffMaximize }, //
            { 0, "fHide", YFrameWindow::ffHide }, //
            { 0, "fRollup", YFrameWindow::ffRollup }, //
            { 1, "dTitleBar", YFrameWindow::fdTitleBar }, //
            { 1, "dSysMenu", YFrameWindow::fdSysMenu }, //
            { 1, "dBorder", YFrameWindow::fdBorder }, //
            { 1, "dResize", YFrameWindow::fdResize }, //
            { 1, "dClose", YFrameWindow::fdClose }, //
            { 1, "dMinimize", YFrameWindow::fdMinimize }, //
            { 1, "dMaximize", YFrameWindow::fdMaximize }, //
            { 1, "dHide", YFrameWindow::fdHide },
            { 1, "dRollup", YFrameWindow::fdRollup },
            { 1, "dDepth", YFrameWindow::fdDepth },
            { 2, "allWorkspaces", YFrameWindow::foAllWorkspaces }, //
            { 2, "ignoreTaskBar", YFrameWindow::foIgnoreTaskBar }, //
            { 2, "noIgnoreTaskBar", YFrameWindow::foNoIgnoreTaskBar }, //
            { 2, "ignoreWinList", YFrameWindow::foIgnoreWinList }, //
            { 2, "ignoreQuickSwitch", YFrameWindow::foIgnoreQSwitch }, //
            { 2, "fullKeys", YFrameWindow::foFullKeys }, //
            { 2, "noFocusOnAppRaise", YFrameWindow::foNoFocusOnAppRaise }, //
            { 2, "ignoreNoFocusHint", YFrameWindow::foIgnoreNoFocusHint }, //
            { 2, "ignorePositionHint", YFrameWindow::foIgnorePosition }, //
            { 2, "doNotCover", YFrameWindow::foDoNotCover }, //
            { 2, "doNotFocus", YFrameWindow::foDoNotFocus }, //
            { 2, "startFullscreen", YFrameWindow::foFullscreen },
            { 2, "startMinimized", YFrameWindow::foMinimized }, //
            { 2, "startMaximized", YFrameWindow::foMaximizedVert | YFrameWindow::foMaximizedHorz }, //
            { 2, "startMaximizedVert", YFrameWindow::foMaximizedVert }, //
            { 2, "startMaximizedHorz", YFrameWindow::foMaximizedHorz }, //
            { 2, "nonICCCMconfigureRequest", YFrameWindow::foNonICCCMConfigureRequest },
            { 2, "forcedClose", YFrameWindow::foForcedClose },
            { 2, "noFocusOnMap", YFrameWindow::foNoFocusOnMap },
            { 2, "appTakesFocus", YFrameWindow::foAppTakesFocus }
        };

        for (unsigned int a = 0; a < ACOUNT(options); a++) {
            unsigned long *what, *what_mask;

            if (options[a].what == 0) {
                what = &op->functions;
                what_mask  = &op->function_mask;
            } else if (options[a].what == 1) {
                what = &op->decors;
                what_mask = &op->decor_mask;
            } else if (options[a].what == 2) {
                what = &op->options;
                what_mask = &op->option_mask;
            } else {
                msg(_("Error in window option: %s"), opt);
                break;
            }

            if (strcmp(opt, options[a].name) == 0) {
                if (*what == 2 && options[a].flag == YFrameWindow::foIgnoreWinList)
                    DEPRECATE("ignoreWinlist windowoption");
                if (*what == 2 && options[a].flag == YFrameWindow::foIgnoreQSwitch)
                    DEPRECATE("ignoreQuickSwitch windowoption");
                if (atoi(arg) != 0)
                    *what = (*what) | options[a].flag;
                else
                    *what = (*what) & ~options[a].flag;
                *what_mask =  (*what_mask) | options[a].flag;
                return ;
            }
        }
        msg(_("Unknown window option: %s"), opt);
    }
}

void WindowOptions::mergeWindowOption(WindowOption &cm,
                                      ustring a_class_instance,
                                      bool remove)
{
    WindowOption *wo = getWindowOption(a_class_instance, false, remove);
    if (wo)
        combineOptions(cm, *wo);
}

void WindowOptions::combineOptions(WindowOption &cm, WindowOption &n) {
    if (!cm.icon && n.icon) cm.icon = n.icon;
    cm.functions |= n.functions & ~cm.function_mask;
    cm.function_mask |= n.function_mask;
    cm.decors |= n.decors & ~cm.decor_mask;
    cm.decor_mask |= n.decor_mask;
    cm.options |= n.options & ~cm.option_mask;
    cm.option_mask |= n.option_mask;
    if (n.workspace != (long)WinWorkspaceInvalid)
        cm.workspace = n.workspace;
    if (n.layer != (long)WinLayerInvalid)
        cm.layer = n.layer;
#ifdef CONFIG_TRAY
    if (n.tray != (long)WinTrayInvalid)
        cm.tray = n.tray;
#endif
    if ((n.gflags & XValue) && !(cm.gflags & XValue)) {
        cm.gx = n.gx;
        cm.gflags |= XValue;
        if (n.gflags & XNegative)
            cm.gflags |= XNegative;
    }
    if ((n.gflags & YValue) && !(cm.gflags & YValue)) {
        cm.gy = n.gy;
        cm.gflags |= YValue;
        if (n.gflags & YNegative)
            cm.gflags |= YNegative;
    }
    if ((n.gflags & WidthValue) && !(cm.gflags & WidthValue)) {
        cm.gw = n.gw;
        cm.gflags |= WidthValue;
    }
    if ((n.gflags & HeightValue) && !(cm.gflags & HeightValue)) {
        cm.gh = n.gh;
        cm.gflags |= HeightValue;
    }
}

char *parseWinOptions(char *data) {
    char *p = data;
    char *w, *e, *c;
    char *class_instance;

    char *opt;

    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n')
            p++;
        if (*p == '#') {
            while (*p && *p != '\n') {
                if (*p == '\\' && p[1] != 0)
                    p++;
                p++;
            }
            continue;
        }
        w = p;
        c = 0;
        while (*p && *p != ':') {
            if (*p == '\\' && p[1] != 0)
                p++;
            else if (*p == '.')
                c = p;
            p++;
        }
        e = p;

        if (e == w || *p == 0)
            break;
        if (c == 0) {
            msg(_("Syntax error in window options"));
            break;
        }

        if (c - w + 1 == 0)
            class_instance = 0;
        else {
            char *d = w, *p = w;
            while (p < c) {
                if (*p == '\\' && p + 1 < c)
                    p++;
                *d++ = *p++;
            }

/// TODO #warning "separate handling of class and instance, the current way is a hack"
            class_instance = newstr(w, d - w);
            if (class_instance == 0)
                goto nomem;
            MSG(("class_instance: (%s)", class_instance));
        }

        *e = 0;
        c++;
        opt = c;
        e++;

        p = e;
        while (*p == ' ' || *p == '\t')
            p++;

        w = p;
        while (*p && (*p != '\n' && *p != ' ' && *p != '\t'))
            p++;

        if (*p != 0) {
            *p = 0;
            defOptions->setWinOption(class_instance, opt, w);
            delete[] class_instance;
        } else {
            defOptions->setWinOption(class_instance, opt, w);
            delete class_instance;
            break;
        }
        p++;
    }
    return p;
nomem:
    msg(_("Out of memory for window options"));
    return 0;
}

void loadWinOptions(upath optFile) {
    if (optFile == null)
        return ;

    int fd = open(cstring(optFile.path()).c_str(), O_RDONLY | O_TEXT);

    if (fd == -1)
        return ;

    struct stat sb;

    if (fstat(fd, &sb) == -1)
        return ;

    int len = sb.st_size;

    char *buf = new char[len + 1];
    if (buf == 0)
        return ;

    if ((len = read(fd, buf, len)) < 0) {
        delete[] buf;
        return;
    }

    buf[len] = 0;
    close(fd);

    parseWinOptions(buf);

    delete[] buf;
}
#endif
