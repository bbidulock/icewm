/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#ifndef NO_WINDOW_OPTIONS
#include "yfull.h"
#include "wmoption.h"
#include "wmframe.h"

#include "WinMgr.h"
#include "sysdep.h"
#include "base.h"

#include "intl.h"

char *winOptFile = 0;
WindowOptions *defOptions = 0;
WindowOptions *hintOptions = 0;

WindowOptions::WindowOptions() {
    winOptionsCount = 0;
    winOptions = 0;
}

WindowOptions::~WindowOptions() {
    int i;

    for (i = 0; i < winOptionsCount; i++) {
        if (winOptions[i].name)
            delete[] winOptions[i].name;
        if (winOptions[i].icon)
            delete[] winOptions[i].icon;
    }
    FREE(winOptions);
    winOptions = 0;
    winOptionsCount = 0;
}

WindowOption *WindowOptions::getWindowOption(const char *name, bool create, bool remove) {
    int L = 0, R = winOptionsCount, M, cmp;
    while (L < R) {
        M = (L + R) / 2;
        if (name == 0 && winOptions[M].name == 0)
            cmp = 0;
        else if (name == 0)
            cmp = -1;
        else if (winOptions[M].name == 0)
            cmp = 1;
        else
            cmp = strcmp(name, winOptions[M].name);
        if (cmp == 0) {
            if (remove) {
                static WindowOption o = winOptions[M];

                winOptionsCount--;
                for (int dummy = M; dummy < winOptionsCount; dummy++)
                    winOptions[dummy] = winOptions[dummy + 1];   /* */
                return &o;
            }

            return winOptions + M;
        } else if (cmp > 0)
            L = M + 1;
        else
            R = M;
    }
    if (!create)
        return 0;

    WindowOption *newOptions =
        (WindowOption *)REALLOC(winOptions,
                                sizeof(winOptions[0]) * (winOptionsCount + 1));
    if (newOptions == 0)
        return 0;
    winOptions = newOptions;

    for (int dummy = winOptionsCount; dummy > L; dummy--)
       winOptions[dummy] = winOptions[dummy - 1];   /* */
    winOptionsCount++;

    /* initialize empty option structure */
    memset(winOptions + L, 0, sizeof(WindowOption));
    winOptions[L].workspace = WinWorkspaceInvalid;
    winOptions[L].layer = WinLayerInvalid;
#ifdef CONFIG_TRAY
    winOptions[L].tray = WinTrayInvalid;
#endif
    winOptions[L].name = newstr(name);

    return winOptions + L;
}

void WindowOptions::setWinOption(const char *class_instance, const char *opt, const char *arg) {
    WindowOption *op = getWindowOption(class_instance, true);

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
                rx = desktop->width() - rx;
            if (op->gflags & YNegative)
                ry = desktop->height() - ry;
            op->gx = rx;
            op->gy = ry;
            op->gw = rw;
            op->gh = rh;
            //msg("parsed %d %d %d %d %d", rx, ry, rw, rh, op->gflags);
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
                { "Exclusive", WinTrayExclusive }
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
            { 2, "ignoreWinList", YFrameWindow::foIgnoreWinList }, //
            { 2, "ignoreQuickSwitch", YFrameWindow::foIgnoreQSwitch }, //
            { 2, "fullKeys", YFrameWindow::foFullKeys }, //
            { 2, "noFocusOnAppRaise", YFrameWindow::foNoFocusOnAppRaise }, //
            { 2, "ignoreNoFocusHint", YFrameWindow::foIgnoreNoFocusHint }, //
            { 2, "ignorePositionHint", YFrameWindow::foIgnorePosition }, //
            { 2, "doNotCover", YFrameWindow::foDoNotCover } //
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

void WindowOptions::combineOptions(WindowOption &cm, WindowOption &n) {
    if (!cm.icon && n.icon) cm.icon = newstr(n.icon);
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
    if (n.gflags & XValue)
        cm.gx = n.gx;
    if (n.gflags & YValue)
        cm.gy = n.gy;
    if (n.gflags & WidthValue)
        cm.gw = n.gw;
    if (n.gflags & HeightValue)
        cm.gw = n.gw;
    cm.gflags |= n.gflags;
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
            if (*p == '.')
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
            class_instance = newstr(w, c - w);
            if (class_instance == 0)
                goto nomem;
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
            delete class_instance;
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

void loadWinOptions(const char *optFile) {
    if (optFile == 0)
        return ;

    int fd = open(optFile, O_RDONLY | O_TEXT);

    if (fd == -1)
        return ;

    struct stat sb;

    if (fstat(fd, &sb) == -1)
        return ;

    int len = sb.st_size;

    char *buf = new char[len + 1];
    if (buf == 0)
        return ;

    if (read(fd, buf, len) != len)
        return;

    buf[sb.st_size] = 0;
    close(fd);

    parseWinOptions(buf);

    delete buf;
}
#endif
