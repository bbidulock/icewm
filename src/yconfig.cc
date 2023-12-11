#include "config.h"

#include "upath.h"
#include "ykey.h"
#include "yconfig.h"
#include "yprefs.h"
#include "sysdep.h"
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
    const char *const orig_arg = arg;
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

    *key = parseKeySym(arg);
    if (*key == NoSymbol && *arg) {
        msg(_("Unknown key name %s in %s"), arg, orig_arg);
        return false;
    }
    return true;
}

KeySym YConfig::parseKeySym(const char* arg) {
    if (*arg == 0)
        return NoSymbol;
    if (strcmp(arg, "Esc") == 0)
        return XK_Escape;
    if (strcmp(arg, "Enter") == 0)
        return XK_Return;
    if (strcmp(arg, "Space") == 0)
        return ' ';
    if (strcmp(arg, "BackSp") == 0)
        return XK_BackSpace;
    if (strcmp(arg, "Del") == 0)
        return XK_Delete;
    if (arg[1] == 0) {
        int ch = (*arg & 0xff);
        if (ASCII::isUpper(ch))
            return ASCII::toLower(ch);
        if (inrange(ch, 0xa0, 0xff))
            return ch;
        if (inrange<int>(ch, ' ', '~'))
            return ch;
        return NoSymbol;
    } else {
        int ch = 0;
        if (arg[2] == 0) {
            if ((*arg & 0xe0) == 0xc0 && (arg[1] & 0xc0) == 0x80)
                ch = ((*arg & 0x1f) << 6) | (arg[1] & 0x3f);
        } else if (arg[3] == 0) {
            if ((*arg & 0xf0) == 0xe0 && (arg[1] & 0xc0) == 0x80 &&
                (arg[2] & 0xc0) == 0x80)
                ch = ((*arg & 0xf) << 12) | (arg[1] & 0x3f) << 6 |
                         (arg[2] & 0x3f);
        } else if (arg[4] == 0) {
            if ((*arg & 0xf8) == 0xf0 && (arg[1] & 0xc0) == 0x80 &&
                (arg[2] & 0xc0) == 0x80 && (arg[3] & 0xc0) == 0x80)
                ch = ((*arg & 0x7) << 18) | (arg[1] & 0x3f) << 12 |
                         (arg[2] & 0x3f) << 6 | (arg[3] & 0x3f);
        }
        if (ch) {
            if (inrange(ch, 0xa0, 0xff))
                return ch;
            if (ch < 0x100)
                return NoSymbol;
            return ch | 0x01000000;
        }
    }

    KeySym ks = XStringToKeysym(arg);
    if (ks)
        return ks;

    if (strncmp(arg, "Pointer_Button", 14) == 0) {
        int button = 0;
        if (sscanf(arg + 14, "%d", &button) == 1 && 0 < button)
            return button + XK_Pointer_Button1 - 1;
        return NoSymbol;
    }

    return NoSymbol;
}

void YConfig::setOption(char* arg, bool append, cfoption* opt) {
    MSG(("SET %s := %s ;", opt->name, arg));

    switch (opt->type) {
        case cfoption::CF_BOOL:
            if (opt->v.b.bool_value) {
                if ((arg[0] == '1' || arg[0] == '0') && arg[1] == 0) {
                    *(opt->v.b.bool_value) = (arg[0] == '1');
                } else {
                    msg(_("Bad argument: %s for %s [%d,%d]"), arg, opt->name, 0, 1);
                }
            }
            break;
        case cfoption::CF_INT:
            if (opt->v.i.int_value) {
                int const v(atoi(arg));

                if (v >= opt->v.i.min && v <= opt->v.i.max)
                    *(opt->v.i.int_value) = v;
                else {
                    msg(_("Bad argument: %s for %s [%d,%d]"), arg, opt->name,
                            opt->v.i.min, opt->v.i.max);
                }
            }
            break;
        case cfoption::CF_UINT:
            if (opt->v.u.uint_value) {
                unsigned const v(strtoul(arg, nullptr, 0));

                if (v >= opt->v.u.min && v <= opt->v.u.max)
                    *(opt->v.u.uint_value) = v;
                else {
                    msg(_("Bad argument: %s for %s [%d,%d]"), arg, opt->name,
                            int(opt->v.u.min), int(opt->v.u.max));
                }
            }
            break;
        case cfoption::CF_STR:
            if (opt->v.s.string_value) {
                if (!opt->v.s.initial)
                    delete[] const_cast<char *>(*opt->v.s.string_value);
                *opt->v.s.string_value = newstr(arg);
                opt->v.s.initial = false;
            }
            break;
        case cfoption::CF_KEY:
            if (opt->v.k.key_value) {
                WMKey *wk = opt->v.k.key_value;

                if (YConfig::parseKey(arg, &wk->key, &wk->mod)) {
                    if (!wk->initial)
                        delete[] const_cast<char *>(wk->name);
                    wk->name = newstr(arg);
                    wk->initial = false;
                }
            }
            break;
        case cfoption::CF_FUNC:
            opt->fun()(opt->name, arg, append);
        case cfoption::CF_NONE:
            break;
    }
}

cfoption* YConfig::findOption(char* name, size_t length) {
    if (length && *name) {
        for (cfoption* opt = options; opt->type; ++opt) {
            if (opt->size == 1+length &&
                *opt->name == *name &&
                memcmp(name, opt->name, length) == 0)
            {
                return opt;
            }
        }
    }
    return nullptr;
}

char* YConfig::skipLine(char* p) {
    if (p) {
        // ignore this line.
        p = strchr(p, '\n');
        while (p && (p[-1] == '\r' ? p[-2] == '\\' : p[-1] == '\\')) {
            p = strchr(p + 1, '\n');
        }
    }
    return p;
}

// Parse one option name at 'str' and its argument(s).
// The name is a string without spaces up to '='.
// Option is a quoted string or characters up to next space.
char* YConfig::parseOption(char* str) {
    size_t len = strcspn(str, "= \t\n\r\f\v\\");
    if (len == 0 || len >= 40) {
        return skipLine(str + len);
    }

    char* p = str + len + strspn(str + len, " \t");
    if (*p != '=') {
        return skipLine(p);
    }

    cfoption* found = findOption(str, len);
    if (found == nullptr) {
        return skipLine(p);
    }

    Argument argument;
    for (bool append = false; append == (*p == ',') && *++p; append = true) {
        if (append) {
            while (ASCII::isWhiteSpace(*p) || ASCII::isEscapedLineEnding(p))
                ++p;
        }

        p = YConfig::getArgument(&argument, p, true);
        if (p == nullptr)
            break;

        setOption(argument, append, found);

        while (ASCII::isSpaceOrTab(*p))
            p++;
    }

    return p;
}

void YConfig::parseConfiguration(char *data) {
    for (char *p = data; p && *p; ) {
        if (ASCII::isWhiteSpace(*p)) {
            ++p;
        }
        else if (*p == '\\') {
            ++p;
            if (*p == '\r' && p[1] == '\n')
                ++p;
            if (*p == '\n')
                ++p;
        }
        else if (*p == '#') {
            p = skipLine(p);
        }
        else {
            p = parseOption(p);
        }
    }
}

bool YConfig::loadConfigFile(cfoption* opts, upath fileName,
                             cfoption* more, cfoption* xtra)
{
    YTraceConfig trace(fileName.string());
    auto buf(fileName.loadText());
    if (buf) {
        YConfig(opts).parseConfiguration(buf);
        if (more) {
            YConfig(more).parseConfiguration(buf);
        }
        if (xtra) {
            YConfig(xtra).parseConfiguration(buf);
        }
    }
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

bool YConfig::findLoadConfigFile(cfoption* options, const char* name) {
    upath conf = YApplication::locateConfigFile(name);
    return conf.nonempty() && YConfig::loadConfigFile(options, conf);
}

bool YConfig::findLoadThemeFile(cfoption* options) {
    upath init(themeName);
    upath name(init.isAbsolute() ? init : upath("themes") + init);
    upath conf = YApplication::locateConfigFile(name);
    if (conf.isEmpty() || false == conf.fileExists()) {
        if (name.getExtension() != ".theme")
            conf = YApplication::locateConfigFile(name + "default.theme");
    }
    return conf.nonempty() && YConfig::loadConfigFile(options, conf);
}

size_t YConfig::cfoptionSize() {
    return sizeof(cfoption);
}

YConfig& YConfig::load(const char* file) {
    success = findLoadConfigFile(options, file);
    return *this;
}

YConfig& YConfig::loadTheme() {
    success = findLoadThemeFile(options);
    return *this;
}

YConfig& YConfig::loadOverride() {
    return load("prefoverride");
}

bool cfoption::operator==(const cfoption& r) const {
    if (type == r.type) {
        switch (type) {
            case CF_NONE:
                return true;
            case CF_BOOL:
                return boolval() == r.boolval();
            case CF_INT:
                return intval() == r.intval();
            case CF_UINT:
                return uintval() == r.uintval();
            case CF_STR:
                return str() == r.str() ||
                    (isEmpty(str()) ? isEmpty(r.str()) :
                    (nonempty(r.str()) && 0 == strcmp(str(), r.str())));
            case CF_KEY:
                return key()->operator==(*r.key())
                    && 0 == strcmp(key()->name, r.key()->name);
            case CF_FUNC:
                return false;
        }
    }
    return false;
}

// vim: set sw=4 ts=4 et:
