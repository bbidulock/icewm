#include "config.h"

#include "upath.h"
#include "ykey.h"
#include "yconfig.h"
#include "ypaint.h"
#include "yprefs.h"
#include "ypaths.h"
#include "sysdep.h"
#include "binascii.h"
#include "yapp.h"
#include "intl.h"
#include "ascii.h"

upath findPath(ustring path, int mode, upath name, bool /*path_relative*/) {
#ifdef __EMX__
    if (mode & X_OK)
        name = name.addExtension(".exe");
#endif
    if (name.isAbsolute()) { // check for root in XFreeOS/2
        if (name.fileExists() && name.access(mode) == 0)
            return name;
    } else {
        if (path == null)
            return null;

        ustring s(null), r(null);
        for (s = path; s.splitall(PATHSEP, &s, &r); s = r) {
            upath prog = upath(s).relative(name);
            if (prog.access(mode) == 0)
                return prog;

#if 0
            if (!(mode & X_OK) &&
                !prog.path().endsWith(".xpm") &&
                !prog.path().endsWith(".png"))
            {
                upath prog_png = prog.addExtension(".png");
                if (prog_png.access(mode) == 0)
                    return prog_png;
            }
#endif
        }
    }
    return null;
}

#if !defined(NO_CONFIGURE) || !defined(NO_CONFIGURE_MENUS)

char *YConfig::getArgument(char **dest, char *p, bool comma) {
    while (*p && (*p == ' ' || *p == '\t'))
        p++;

    char *buf = 0;
    char *argStart = p;
    for (int loopTwice = 2; loopTwice > 0; --loopTwice) {
        int k = 0;
        for (p = argStart; *p; p = *p ? 1 + p : p) {
            if (*p == '\'') {
                while (*++p && *p != '\'') {
                    if (buf) buf[k] = *p;
                    ++k;
                }
            }
            else if (*p == '"') {
                while (*++p && *p != '"') {
                    if (*p == '\\' && p[1] == '"')
                        ++p;
                    if (buf) buf[k] = *p;
                    ++k;
                }
            }
            else if (*p == '\\' && p[1] && p[1] != '\n' && p[1] != '\r') {
                // add any char protected by backslash and move forward
                // exception: line ending (unwanted, may do bad things).
                // OTOH, if the two last checks are disable, it will cause a
                // side effect (multiline argument parsing with \n after \).
                ++p;
                if (buf) buf[k] = *p;
                ++k;
            }
            else if (ASCII::isWhiteSpace(*p) || (*p == ',' && comma))
                break;
            else {
                if (buf) buf[k] = *p;
                ++k;
            }
        }
        if (buf == 0) {
            buf = new char[k + 1];
            if (buf == 0) {
                return 0;
            }
        } else {
            buf[k] = '\0';
            *dest = buf;
        }
    }
    return p;
}

#endif

#ifndef NO_CONFIGURE

// FIXME: P1 - parse keys later, not when loading
bool YConfig::parseKey(const char *arg, KeySym *key, unsigned int *mod) {
    const char *orig_arg = arg;
    static const struct {
        const char key[7];
        unsigned char flag;
    } mods[] = {
        { "Alt+",   kfAlt   },
        { "AltGr+", kfAltGr },
        { "Ctrl+",  kfCtrl  },
        { "Hyper+", kfHyper },
        { "Meta+",  kfMeta  },
        { "Shift+", kfShift },
        { "Super+", kfSuper },
    };
    *mod = 0;
    for (int k = 0; k < (int) ACOUNT(mods); ++k) {
        for (int i = 0; arg[i] == mods[k].key[i]; ++i) {
            if (arg[i] == '+') {
                *mod |= mods[k].flag;
                arg += i + 1;
                k = -1;
                break;
            }
        }
    }

    if (modSuperIsCtrlAlt && (*mod & kfSuper)) {
        *mod &= ~kfSuper;
        *mod |= kfAlt | kfCtrl;
    }

    if (*arg == 0)
        *key = NoSymbol;
    else if (strcmp(arg, "Esc") == 0)
        *key = XK_Escape;
    else if (strcmp(arg, "Enter") == 0)
        *key = XK_Return;
    else if (strcmp(arg, "Space") == 0)
        *key = ' ';
    else if (strcmp(arg, "BackSp") == 0)
        *key = XK_BackSpace;
    else if (strcmp(arg, "Del") == 0)
        *key = XK_Delete;
    else if (ASCII::isUpper(arg[0]) && arg[1] == 0) {
        char s[2];
        s[0] = ASCII::toLower(arg[0]);
        s[1] = 0;
        *key = XStringToKeysym(s);
    } else {
        *key = XStringToKeysym(arg);
    }

    if (*key == NoSymbol && *arg) {
        msg(_("Unknown key name %s in %s"), arg, orig_arg);
        return false;
    }
    return true;
}

static char *setOption(cfoption *options, char *name, char *arg, bool append, char *rest) {
    unsigned int a;

    MSG(("SET %s := %s ;", name, arg));

    for (a = 0; options[a].type != cfoption::CF_NONE; a++) {
        if (strcmp(name, options[a].name) != 0)
            continue;
        if (options[a].notify) {
            options[a].notify(name, arg, append);
            return rest;
        }

        switch (options[a].type) {
        case cfoption::CF_BOOL:
            if (options[a].v.bool_value) {
                if ((arg[0] == '1' || arg[0] == '0') && arg[1] == 0) {
                    *(options[a].v.bool_value) = (arg[0] == '1') ? true : false;
                } else {
                    msg(_("Bad argument: %s for %s"), arg, name);
                    return rest;
                }
                return rest;
            }
            break;
        case cfoption::CF_INT:
            if (options[a].v.i.int_value) {
                int const v(atoi(arg));

                if (v >= options[a].v.i.min && v <= options[a].v.i.max)
                    *(options[a].v.i.int_value) = v;
                else {
                    msg(_("Bad argument: %s for %s"), arg, name);
                    return rest;
                }
                return rest;
            }
            break;
        case cfoption::CF_STR:
            if (options[a].v.s.string_value) {
                if (!options[a].v.s.initial)
                    delete[] (char *)*options[a].v.s.string_value;
                *options[a].v.s.string_value = newstr(arg);
                options[a].v.s.initial = false;
                return rest;
            }
            break;
#ifndef NO_KEYBIND
        case cfoption::CF_KEY:
            if (options[a].v.k.key_value) {
                WMKey *wk = options[a].v.k.key_value;

                if (YConfig::parseKey(arg, &wk->key, &wk->mod)) {
                    if (!wk->initial)
                        delete[] (char *)wk->name;
                    wk->name = newstr(arg);
                    wk->initial = false;
                }
                return rest;
            }
            break;
#endif
        case cfoption::CF_NONE:
            break;
        }
    }
#if 0
    msg(_("Bad option: %s"), name);
#endif
    ///!!! check
    return rest;
}

// Parse one option name at 'str' and its argument(s).
// The name is a string without spaces up to '='.
// Option is a quoted string or characters up to next space.
static char *parseOption(cfoption *options, char *str) {
    char name[64];
    char *p = str;
    size_t len = 0;
    bool append = false;

    while (*p && *p != '=' && ASCII::isWhiteSpace(*p) == false)
        p++;
    len = (size_t)(p - str);

    while (*p != '\n' && ASCII::isWhiteSpace(*p))
        p++;
    if (*p != '=' || len >= sizeof name) {
        // ignore this line.
        for (; *p && *p != '\n'; ++p)
            if (*p == '\\' && p[1])
                p++;
        return p;
    }

    memcpy(name, str, len);
    name[len] = 0;

    while (*++p) {
        char *argument = 0;
        p = YConfig::getArgument(&argument, p, true);
        if (p == 0)
            break;

        p = setOption(options, name, argument, append, p);

        delete[] argument;

        append = true;

        if (p == 0)
            return 0;

        while (*p && (*p == ' ' || *p == '\t'))
            p++;

        if (*p != ',')
            break;
    }

    return p;
}

void YConfig::parseConfiguration(cfoption *options, char *data) {
    for (char *p = data; p && *p; ) {
        while (ASCII::isWhiteSpace(*p) || (*p == '\\' && p[1] == '\n'))
            p++;

        if (*p == '#') {
            while (*++p && *p != '\n')
                if (*p == '\\' && p[1])
                    p++;
        } else if (*p)
            p = parseOption(options, p);
    }
}

bool YConfig::loadConfigFile(cfoption *options, upath fileName) {
    char* buf = load_text_file(cstring(fileName));
    if (buf) {
        parseConfiguration(options, buf);
        delete[] buf;
    }
    return buf != 0;
}

void YConfig::freeConfig(cfoption *options) {
    for (unsigned int a = 0; options[a].type != cfoption::CF_NONE; a++) {
        if (!options[a].v.s.initial) {
            if (options[a].v.s.string_value) {
                delete[] (char *)*options[a].v.s.string_value;
                *options[a].v.s.string_value = 0;
            }
        }
    }
}

bool YConfig::findLoadConfigFile(IApp *app, cfoption *options, upath name) {
    upath conf = app->findConfigFile(name);
    return conf.nonempty() && YConfig::loadConfigFile(options, conf);
}

#endif
