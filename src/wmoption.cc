/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "wmoption.h"
#include "wmframe.h"
#include "ytrace.h"
#include "ascii.h"
#include "intl.h"

lazy<WindowOptions> defOptions;
lazy<WindowOptions> hintOptions;
unsigned WindowOptions::allOptions;
unsigned WindowOptions::serial;

WindowOption::WindowOption(mstring n_class_instance):
    w_class_instance(n_class_instance),
    keyboard(nullptr), icon(nullptr),
    functions(0), function_mask(0),
    decors(0), decor_mask(0),
    options(0), option_mask(0),
    workspace(WinWorkspaceInvalid),
    layer(WinLayerInvalid),
    tray(WinTrayInvalid),
    order(0), opacity(0),
    gflags(0), gx(0), gy(0), gw(0), gh(0),
    frame(0), serial(WindowOptions::serial)
{
}

WindowOption::~WindowOption()
{
    if (keyboard)
        free(keyboard);
    if (icon)
        free(icon);
}

void WindowOption::combine(const WindowOption& n) {
    if (nonempty(n.icon) && isEmpty(icon))
        icon = strdup(n.icon);
    if (nonempty(n.keyboard) && isEmpty(keyboard))
        keyboard = strdup(n.keyboard);
    if (n.function_mask) {
        functions |= n.functions & ~function_mask;
        function_mask |= n.function_mask;
    }
    if (n.decor_mask) {
        decors |= n.decors & ~decor_mask;
        decor_mask |= n.decor_mask;
    }
    if (n.option_mask) {
        options |= n.options & ~option_mask;
        option_mask |= n.option_mask;
    }
    if (n.workspace >= 0 && workspace == WinWorkspaceInvalid)
        workspace = n.workspace;
    if (n.layer >= 0 && layer == WinLayerInvalid)
        layer = n.layer;
    if (n.tray >= 0 && tray == WinTrayInvalid)
        tray = n.tray;
    if (n.order && order == 0)
        order = n.order;
    if (n.opacity > 0 && opacity == 0)
        opacity = n.opacity;
    if ((n.gflags & XValue) && !(gflags & XValue)) {
        gx = n.gx;
        gflags |= XValue;
        if (n.gflags & XNegative)
            gflags |= XNegative;
        else
            gflags &= ~XNegative;
    }
    if ((n.gflags & YValue) && !(gflags & YValue)) {
        gy = n.gy;
        gflags |= YValue;
        if (n.gflags & YNegative)
            gflags |= YNegative;
        else
            gflags &= ~YNegative;
    }
    if ((n.gflags & WidthValue) && !(gflags & WidthValue)) {
        gw = n.gw;
        gflags |= WidthValue;
    }
    if ((n.gflags & HeightValue) && !(gflags & HeightValue)) {
        gh = n.gh;
        gflags |= HeightValue;
    }
    if (frame == 0 && n.frame)
        frame = n.frame;
    if (serial < n.serial)
        serial = n.serial;
}

bool WindowOption::outdated() const {
    return serial < WindowOptions::serial;
}

bool WindowOptions::findOption(mstring a_class_instance, int *index) {
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

WindowOption* WindowOptions::getOption(mstring a_class_instance) {
    int where;
    if (findOption(a_class_instance, &where) == false) {
        fWinOptions.insert(where, new WindowOption(a_class_instance));
    }
    return fWinOptions[where];
}

void WindowOptions::setWinOption(mstring n_class_instance,
                                 const char *opt, const char *arg)
{
    WindowOption *op = getOption(n_class_instance);

    if (strcmp(opt, "icon") == 0) {
        if (op->icon)
            free(op->icon);
        op->icon = strdup(arg);
    } else if (strcmp(opt, "keyboard") == 0) {
        if (op->keyboard)
            free(op->keyboard);
        op->keyboard = strdup(arg);
    } else if (strcmp(opt, "workspace") == 0) {
        int workspace = atoi(arg);
        op->workspace = max(workspace, int(WinWorkspaceInvalid));
    } else if (strcmp(opt, "order") == 0) {
        op->order = atoi(arg);
    } else if (strcmp(opt, "opacity") == 0) {
        int opaq = atoi(arg);
        if (inrange(opaq, 0, 100))
            op->opacity = opaq;
    } else if (strcmp(opt, "frame") == 0) {
        op->frame = unsigned(strhash(arg));
    } else if (strcmp(opt, "geometry") == 0) {
        op->gx = 0;
        op->gy = 0;
        op->gw = 0;
        op->gh = 0;
        op->gflags = XParseGeometry(arg, &op->gx, &op->gy, &op->gw, &op->gh);
    } else if (strcmp(opt, "layer") == 0) {
        op->layer = WinLayerInvalid;
        if (ASCII::isDigit(arg[0])) {
            char* end = nullptr;
            long num = strtol(arg, &end, 10);
            if (end && arg < end && 0 == *end && validLayer(num))
                op->layer = int(num);
        }
        else if (ASCII::isUpper(arg[0])) {
            static const struct {
                const char name[12];
                int layer;
            } layers[] = {
                { "Desktop",    WinLayerDesktop },
                { "Below",      WinLayerBelow },
                { "Normal",     WinLayerNormal },
                { "Above",      WinLayerOnTop },
                { "OnTop",      WinLayerOnTop },
                { "Dock",       WinLayerDock },
                { "AboveDock",  WinLayerAboveDock },
                { "Menu",       WinLayerMenu },
                { "Fullscreen", WinLayerFullscreen },
                { "AboveAll",   WinLayerAboveAll },
            };
            for (int i = 0; i < int ACOUNT(layers); ++i)
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
            { "ignoreActivationMessages", YFrameWindow::foIgnoreActivationMessages },
            { "ignoreNoFocusHint",        YFrameWindow::foIgnoreNoFocusHint },
            { "ignoreOverrideRedirect",   YFrameWindow::foIgnoreOverrideRedirect },
            { "ignorePagerPreview",       YFrameWindow::foIgnorePagerPreview },
            { "ignorePositionHint",       YFrameWindow::foIgnorePosition },
            { "ignoreQuickSwitch",        YFrameWindow::foIgnoreQSwitch },
            { "ignoreTaskBar",            YFrameWindow::foIgnoreTaskBar },
            { "ignoreUrgentHint",         YFrameWindow::foIgnoreUrgent },
            { "ignoreWinList",            YFrameWindow::foIgnoreWinList },
            { "noFocusOnAppRaise",        YFrameWindow::foNoFocusOnAppRaise },
            { "noFocusOnMap",             YFrameWindow::foNoFocusOnMap },
            { "noIgnoreTaskBar",          YFrameWindow::foNoIgnoreTaskBar },
            { "nonICCCMconfigureRequest", 0 },
            { "startClose",               YFrameWindow::foClose },
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
            tlog(_("Unknown window option: %s"), opt);
        }
        else {
            unsigned bit = options[least].flag;
            bool setting = 0 != atoi(arg);
            unsigned set = setting ? bit : 0;
            unsigned clr = setting ? 0 : bit;

            if (*opt == 'f' && ASCII::isUpper(opt[1])) {
                op->functions = ((op->functions | set) & ~clr);
                op->function_mask |= bit;
            }
            else if (*opt == 'd' && ASCII::isUpper(opt[1])) {
                op->decors = ((op->decors | set) & ~clr);
                op->decor_mask |= bit;
            }
            else {
                op->options = ((op->options | set) & ~clr);
                op->option_mask |= bit;
                allOptions |= set;
            }
        }
    }
}

void WindowOptions::mergeWindowOption(WindowOption &cm,
                                      mstring a_class_instance,
                                      bool remove)
{
    int lo;
    if (findOption(a_class_instance, &lo)) {
        cm.combine(*fWinOptions[lo]);
        if (remove)
            fWinOptions.remove(lo);
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
        dot = nullptr;
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

        if (*p == 0 || dot == nullptr || end == word || *end != ':') {
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
        *dest = '\0';

        mstring class_instance(word, dest - word);

        *end = 0;
        opt = 1 + dot;
        end++;

        p = end;
        while (ASCII::isSpaceOrTab(*p))
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
        YTraceConfig trace(optFile.string());
        auto buf(optFile.loadText());
        if (buf) {
            defOptions = null;
            WindowOptions::serial += 1;
            parseWinOptions(buf, optFile.string());
        }
    }
}

// vim: set sw=4 ts=4 et:
