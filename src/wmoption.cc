/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "wmoption.h"
#include "wmframe.h"
#include "ascii.h"
#include "intl.h"
#include <ctype.h>

lazy<WindowOptions> defOptions;
lazy<WindowOptions> hintOptions;

WindowOption::WindowOption(ustring n_class_instance):
    w_class_instance(n_class_instance),
    icon(0),
    functions(0), function_mask(0),
    decors(0), decor_mask(0),
    options(0), option_mask(0),
    workspace(WinWorkspaceInvalid),
    layer(WinLayerInvalid),
    tray(WinTrayInvalid),
    order(0),
    gflags(0), gx(0), gy(0), gw(0), gh(0)
{
}

bool WindowOptions::findOption(ustring a_class_instance, int *index) {
    int lo = 0, hi = fWinOptions.getCount();

    while (lo < hi) {
        const int pv = (lo + hi) / 2;
        const WindowOption *pivot = fWinOptions[pv];

        int cmp = a_class_instance.compareTo(pivot->w_class_instance);
        if (cmp > 0) {
            lo = pv + 1;
        } else if (cmp < 0) {
            hi = pv;
        } else {
            *index = pv;
            return true;
        }
    }

    *index = lo;
    return false;
}

WindowOption *WindowOptions::getOption(ustring a_class_instance) {
    int lo;
    if (findOption(a_class_instance, &lo))
        return fWinOptions[lo];

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
    WindowOption *op = getOption(n_class_instance);

    // msg("%s . %s : %s", cstring(n_class_instance).c_str(), opt, arg);

    if (strcmp(opt, "icon") == 0) {
        op->icon = newstr(arg);
    } else if (strcmp(opt, "workspace") == 0) {
        int workspace = atoi(arg);
        op->workspace = inrange(workspace, 0, MAXWORKSPACES - 1)
                      ? workspace
                      : WinWorkspaceInvalid;
    } else if (strcmp(opt, "order") == 0) {
        op->order = atoi(arg);
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

        if (arg[0] && !endptr[0]) {
            if (inrange(l, 0L, WinLayerCount - 1L))
                op->layer = (int) l;
        }
        else {
            static const struct {
                const char name[12];
                int layer;
            } layers[] = {
                { "Desktop",    WinLayerDesktop },
                { "Below",      WinLayerBelow },
                { "Normal",     WinLayerNormal },
                { "OnTop",      WinLayerOnTop },
                { "Dock",       WinLayerDock },
                { "AboveDock",  WinLayerAboveDock },
                { "Menu",       WinLayerMenu },
                { "Fullscreen", WinLayerFullscreen },
                { "AboveAll",   WinLayerAboveAll },
            };
            for (unsigned int i = 0; i < ACOUNT(layers); i++)
                if (strcmp(layers[i].name, arg) == 0) {
                    op->layer = layers[i].layer;
                    return;
                }
        }
    } else if (strcmp(opt, "tray") == 0) {
        char *endptr;
        long const t(strtol(arg, &endptr, 10));

        op->tray = WinTrayInvalid;

        if (arg[0] && !endptr[0]) {
            if (inrange(t, 0L, WinTrayOptionCount - 1L))
                op->tray = (int) t;
        }
        else {
            static const struct {
                const char name[12];
                int tray;
            } tray_ops[] = {
                { "Ignore",    WinTrayIgnore },
                { "Minimized", WinTrayMinimized },
                { "Exclusive", WinTrayExclusive },
            };
            for (unsigned int i = 0; i < ACOUNT(tray_ops); i++)
                if (strcmp(tray_ops[i].name, arg) == 0) {
                    op->tray = tray_ops[i].tray;
                    return;
                }
        }
    } else {
        const unsigned foMaximized = YFrameWindow::foMaximizedHorz |
                                     YFrameWindow::foMaximizedVert;
        const unsigned foNonICCCM = YFrameWindow::foNonICCCMConfigureRequest;
        static const struct {
            const char *name;
            unsigned flag;
        } options[] = {
            // Keep sorted on name:
            { "allWorkspaces",            YFrameWindow::foAllWorkspaces },
            { "appTakesFocus",            YFrameWindow::foAppTakesFocus },
            { "dBorder",                  YFrameWindow::fdBorder },
            { "dClose",                   YFrameWindow::fdClose },
            { "dDepth",                   YFrameWindow::fdDepth },
            { "dHide",                    YFrameWindow::fdHide },
            { "dMaximize",                YFrameWindow::fdMaximize },
            { "dMinimize",                YFrameWindow::fdMinimize },
            { "dResize",                  YFrameWindow::fdResize },
            { "dRollup",                  YFrameWindow::fdRollup },
            { "dSysMenu",                 YFrameWindow::fdSysMenu },
            { "dTitleBar",                YFrameWindow::fdTitleBar },
            { "doNotCover",               YFrameWindow::foDoNotCover },
            { "doNotFocus",               YFrameWindow::foDoNotFocus },
            { "fClose",                   YFrameWindow::ffClose },
            { "fHide",                    YFrameWindow::ffHide },
            { "fMaximize",                YFrameWindow::ffMaximize },
            { "fMinimize",                YFrameWindow::ffMinimize },
            { "fMove",                    YFrameWindow::ffMove },
            { "fResize",                  YFrameWindow::ffResize },
            { "fRollup",                  YFrameWindow::ffRollup },
            { "forcedClose",              YFrameWindow::foForcedClose },
            { "fullKeys",                 YFrameWindow::foFullKeys },
            { "ignoreNoFocusHint",        YFrameWindow::foIgnoreNoFocusHint },
            { "ignorePagerPreview",       YFrameWindow::foIgnorePagerPreview },
            { "ignorePositionHint",       YFrameWindow::foIgnorePosition },
            { "ignoreQuickSwitch",        YFrameWindow::foIgnoreQSwitch },
            { "ignoreTaskBar",            YFrameWindow::foIgnoreTaskBar },
            { "ignoreUrgentHint",         YFrameWindow::foIgnoreUrgent },
            { "ignoreWinList",            YFrameWindow::foIgnoreWinList },
            { "noFocusOnAppRaise",        YFrameWindow::foNoFocusOnAppRaise },
            { "noFocusOnMap",             YFrameWindow::foNoFocusOnMap },
            { "noIgnoreTaskBar",          YFrameWindow::foNoIgnoreTaskBar },
            { "nonICCCMconfigureRequest", foNonICCCM },
            { "startFullscreen",          YFrameWindow::foFullscreen },
            { "startMaximized",           foMaximized },
            { "startMaximizedHorz",       YFrameWindow::foMaximizedHorz },
            { "startMaximizedVert",       YFrameWindow::foMaximizedVert },
            { "startMinimized",           YFrameWindow::foMinimized },
        };

        int least = 0;
        int count = int ACOUNT(options);
        while (least < count) {
            int mid = least + (count - least) / 2;
            int cmp = strcmp(opt, options[mid].name);
            if (cmp < 0)
                count = mid;
            else if (0 < cmp)
                least = 1 + mid;
            else {
                least = mid;
                break;
            }
        }
        if (count <= least) {
            msg(_("Unknown window option: %s"), opt);
        }
        else {
            unsigned bit = options[least].flag;
            bool setting = 0 != atoi(arg);
            unsigned set = setting ? bit : 0;
            unsigned clr = setting ? 0 : bit;

            if (*opt == 'f' && isupper((unsigned char) opt[1])) {
                op->functions = ((op->functions | set) & ~clr);
                op->function_mask |= bit;
            }
            else if (*opt == 'd' && isupper((unsigned char) opt[1])) {
                op->decors = ((op->decors | set) & ~clr);
                op->decor_mask |= bit;
            }
            else {
                op->options = ((op->options | set) & ~clr);
                op->option_mask |= bit;
            }
        }
    }
}

void WindowOptions::mergeWindowOption(WindowOption &cm,
                                      ustring a_class_instance,
                                      bool remove)
{
    int lo;
    if (findOption(a_class_instance, &lo)) {
        WindowOption *wo = fWinOptions[lo];
        combineOptions(cm, *wo);
        if (remove)
            fWinOptions.remove(lo);
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
    if (n.workspace != WinWorkspaceInvalid)
        cm.workspace = n.workspace;
    if (n.layer != (long)WinLayerInvalid)
        cm.layer = n.layer;
    if (n.tray != (long)WinTrayInvalid)
        cm.tray = n.tray;
    if (n.order)
        cm.order = n.order;
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

static char *parseWinOptions(char *data, const char* filename) {
    char *p, *word, *end, *dot, *opt;
    int newlines = 0, linenum = 1, numerrors = 0;

    for (p = data; *p; ++p, linenum = 1 + newlines) {
        if (ASCII::isWhiteSpace(*p)) {
            if (*p == '\n')
                ++newlines;
            continue;
        }

        if (*p == '#') {
            while (*++p && *p != '\n') {
                if (*p == '\\' && p[1] != 0)
                    if (*++p == '\n')
                        ++newlines;
            }
            ++newlines;
            continue;
        }

        word = p;
        dot = 0;
        while (*p && *p != ':' && *p != '\n') {
            if (*p == '\\' && p[1] != 0) {
                if (*++p == '\n')
                    ++newlines;
            }
            else if (*p == '.')
                dot = p;
            p++;
        }
        end = p;

        if (*p == 0 || dot == 0 || end == word || *end != ':') {
            msg(_("Syntax error in window options on line %d of %s"),
                    linenum, filename);
            while (*p && *p != '\n')
                ++p;
            if (*p == 0 || 2 < ++numerrors)
                break;
            if (*p == '\n')
                ++newlines;
            continue;
        }

        char *dest = word;
        for (char *scan = word; scan < dot; ++scan) {
            if (*scan == '\\' && scan + 1 < dot)
                ++scan;
            *dest++ = *scan;
        }

        mstring class_instance(word, dest - word);

        *end = 0;
        opt = 1 + dot;
        end++;

        p = end;
        while (*p == ' ' || *p == '\t')
            p++;

        word = p;
        while (*p && false == ASCII::isWhiteSpace(*p))
            p++;
        if (*p == '\n')
            ++newlines;

        bool lastline(*p == '\0');
        *p = '\0';

        defOptions->setWinOption(class_instance, opt, word);

        if (lastline)
            break;
    }
    return p;
}

void loadWinOptions(upath optFile) {
    if (optFile.nonempty()) {
        csmart buf(load_text_file(optFile.string()));
        if (buf)
            parseWinOptions(buf, optFile.string());
    }
}

// vim: set sw=4 ts=4 et:
