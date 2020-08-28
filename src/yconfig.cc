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
#include "argument.h"

char *YConfig::getArgument(Argument *dest, char *source, bool comma) {
    char *p = source;
    while (ASCII::isSpaceOrTab(*p))
        p++;

    dest->reset();
    for (; *p; p = *p ? 1 + p : p) {
        if (*p == '\'') {
            while (*++p && *p != '\'') {
                *dest += *p;
            }
        }
        else if (*p == '"') {
            while (*++p && *p != '"') {
                if (*p == '\\' && p[1] == '"')
                    ++p;
                *dest += *p;
            }
        }
        else if (*p == '\\' && p[1] && p[1] != '\n' && p[1] != '\r') {
            // add any char protected by backslash and move forward
            // exception: line ending (unwanted, may do bad things).
            // OTOH, if the two last checks are disable, it will cause a
            // side effect (multiline argument parsing with \n after \).
            ++p;
            *dest += *p;
        }
        else if (ASCII::isWhiteSpace(*p) || (*p == ',' && comma))
            break;
        else {
            *dest += *p;
        }
    }
    return p;
}

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
    *key = NoSymbol;
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


static char *setOption(cfoption *options, char *name, const char *arg, bool append, char *rest) {
    unsigned int a;

    MSG(("SET %s := %s ;", name, arg));

    for (a = 0; options[a].type != cfoption::CF_NONE; a++) {
        if (strcmp(name, options[a].name) != 0)
            continue;

        switch (options[a].type) {
        case cfoption::CF_BOOL:
            if (options[a].v.b.bool_value) {
                if ((arg[0] == '1' || arg[0] == '0') && arg[1] == 0) {
                    *(options[a].v.b.bool_value) = (arg[0] == '1');
                } else {
                    msg(_("Bad argument: %s for %s [%d,%d]"), arg, name, 0, 1);
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
                    msg(_("Bad argument: %s for %s [%d,%d]"), arg, name,
                            options[a].v.i.min, options[a].v.i.max);
                    return rest;
                }
                return rest;
            }
            break;
        case cfoption::CF_UINT:
            if (options[a].v.u.uint_value) {
                unsigned const v(strtoul(arg, nullptr, 0));

                if (v >= options[a].v.u.min && v <= options[a].v.u.max)
                    *(options[a].v.u.uint_value) = v;
                else {
                    msg(_("Bad argument: %s for %s [%d,%d]"), arg, name,
                            int(options[a].v.u.min), int(options[a].v.u.max));
                    return rest;
                }
                return rest;
            }
            break;
        case cfoption::CF_STR:
            if (options[a].v.s.string_value) {
                if (!options[a].v.s.initial)
                    delete[] const_cast<char *>(*options[a].v.s.string_value);
                *options[a].v.s.string_value = newstr(arg);
                options[a].v.s.initial = false;
                return rest;
            }
            break;
        case cfoption::CF_KEY:
            if (options[a].v.k.key_value) {
                WMKey *wk = options[a].v.k.key_value;

                if (YConfig::parseKey(arg, &wk->key, &wk->mod)) {
                    if (!wk->initial)
                        delete[] const_cast<char *>(wk->name);
                    wk->name = newstr(arg);
                    wk->initial = false;
                }
                return rest;
            }
            break;
        case cfoption::CF_FUNC:
            options[a].fun()(name, arg, append);
            return rest;
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

    Argument argument;
    for (bool append = false; append == (*p == ',') && *++p; append = true) {
        if (append) {
            while (ASCII::isWhiteSpace(*p) || ASCII::isEscapedLineEnding(p))
                ++p;
        }

        p = YConfig::getArgument(&argument, p, true);
        if (p == nullptr)
            break;

        p = setOption(options, name, argument, append, p);
        if (p == nullptr)
            return nullptr;

        while (ASCII::isSpaceOrTab(*p))
            p++;
    }

    return p;
}

void YConfig::parseConfiguration(cfoption *options, char *data) {
    for (char *p = data; p && *p; ) {
        while (ASCII::isWhiteSpace(*p) || ASCII::isEscapedLineEnding(p))
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
    YTraceConfig trace(fileName.string());
    auto buf = fileName.loadText();
    if (buf)
        parseConfiguration(options, buf);
    return buf;
}

void YConfig::freeConfig(cfoption *options) {
    for (cfoption* o = options; o->type != cfoption::CF_NONE; ++o) {
        if (o->type == cfoption::CF_STR &&
            !o->v.s.initial &&
            *o->v.s.string_value)
        {
            delete[] const_cast<char *>(*o->v.s.string_value);
            *o->v.s.string_value = nullptr;
        }
    }
}

bool YConfig::findLoadConfigFile(IApp *app, cfoption *options, upath name) {
    upath conf = app->findConfigFile(name);
    return conf.nonempty() && YConfig::loadConfigFile(options, conf);
}

bool YConfig::findLoadThemeFile(IApp *app, cfoption *options, upath name) {
    upath conf = app->findConfigFile(name);
    if (conf.isEmpty() || false == conf.fileExists()) {
        if (name.getExtension() != ".theme")
            conf = app->findConfigFile(name + "default.theme");
    }
    return conf.nonempty() && YConfig::loadConfigFile(options, conf);
}

size_t YConfig::cfoptionSize() {
    return sizeof(cfoption);
}

// vim: set sw=4 ts=4 et:
