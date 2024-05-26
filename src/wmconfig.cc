/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "yprefs.h"
#include <errno.h>

#define CFGDEF
#include "wmconfig.h"
#include "bindkey.h"
#include "appnames.h"
#include "default.h"
#include "wmsave.h"
#include "base.h"
#include "yapp.h"
#include "intl.h"

YStringArray configWorkspaces;
MStringArray configKeyboards;

void WMConfig::loadConfiguration(const char* fileName) {
    upath path = YApplication::locateConfigFile(fileName);
    if (path.nonempty()) {
        YConfig::loadConfigFile(icewm_preferences, path,
                                icewm_themable_preferences);
    }
}

bool WMConfig::loadThemeConfiguration() {
    YConfig conf(icewm_themable_preferences);
    if (conf.loadTheme() == false) {
        fail(_("Failed to load theme %s"), themeName);
    }
    if (!conf && strcmp(themeName, CONFIG_DEFAULT_THEME)) {
        themeName = newstr(CONFIG_DEFAULT_THEME);
        if (conf.loadTheme() == false) {
            fail(_("Failed to load theme %s"), themeName);
        }
    }
    if (!conf && strpcmp(themeName, "default", "/")) {
        themeName = newstr("default/default.theme");
        if (conf.loadTheme() == false) {
            fail(_("Failed to load theme %s"), themeName);
        }
    }
    return conf;
}

void WMConfig::freeConfiguration() {
    YConfig::freeConfig(icewm_preferences);
    YConfig::freeConfig(icewm_themable_preferences);
}

void addWorkspace(const char *, const char *value, bool append) {
    if (!append) {
        configWorkspaces.clear();
    }
    configWorkspaces += value;
}

void addKeyboard(const char *, const char *value, bool append) {
    if (!append) {
        configKeyboards.clear();
    }
    if (nonempty(value)) {
        configKeyboards += value;
    }
}

static const struct {
    const char name[8];
    enum WMLook value;
} wmLookNames[] = {
    { "win95",  lookWin95  },
    { "motif",  lookMotif  },
    { "warp3",  lookWarp3  },
    { "warp4",  lookWarp4  },
    { "nice",   lookNice   },
    { "pixmap", lookPixmap },
    { "metal",  lookMetal  },
    { "gtk",    lookGtk    },
    { "flat",   lookFlat   },
};

const char* getLookName(enum WMLook look) {
    for (size_t k = 0; k < ACOUNT(wmLookNames); ++k) {
        if (look == wmLookNames[k].value)
            return wmLookNames[k].name;
    }
    return "";
}

bool getLook(const char *name, const char *arg, enum WMLook *lookPtr) {
    for (size_t k = 0; k < ACOUNT(wmLookNames); ++k) {
        if (0 == strcmp(arg, wmLookNames[k].name)) {
            if (lookPtr) {
                *lookPtr = wmLookNames[k].value;
            }
            return true;
        }
    }
    return false;
}

void setLook(const char *name, const char *arg, bool) {
    enum WMLook look = wmLook;
    if (getLook(name, arg, &look)) {
        wmLook = look;
    } else {
        msg(_("Unknown value '%s' for option '%s'."), arg, name);
    }
}

static upath getDefaultsFilePath(const char* basename) {
    upath prv(YApplication::getPrivConfDir(true));
    if (prv.dirExists()) {
        return prv + basename;
    }
    return null;
}

void WMConfig::setDefault(const char *basename, mstring content) {
    upath confOld(getDefaultsFilePath(basename));
    if (confOld == null) {
        return; // no directory
    }
    upath confNew(confOld.path() + ".new.tmp");

    FILE *fpNew = confNew.fopen("w");
    if (fpNew) {
        fputs(content, fpNew);
        if (content[0] && content[strlen(content)-1] != '\n')
            fputc('\n', fpNew);
    }
    if (fpNew == nullptr || fflush(fpNew) || ferror(fpNew)) {
        fail(_("Unable to write to %s"), confNew.string());
        if (fpNew)
            fclose(fpNew);
        confNew.remove();
        return;
    }

    FILE *fpOld = confOld.fopen("r");
    if (fpOld) {
        const int maxRead = 42;
        const int maxCopy = 10;
        for (int i = 0, k = 0; i < maxRead && k < maxCopy; ++i) {
            char buf[600] = "#", *line = buf;
            if (fgets(1 + buf, sizeof buf - 1, fpOld)) {
                const char* eq = strchr(buf, '=');
                const char* sp = strchr(buf, ' ');
                if (eq && (sp == nullptr || eq < sp)) {
                    while (line[1] == '#')
                        ++line;
                    fputs(line, fpNew);
                    if ('\n' != line[strlen(line)-1])
                        fputc('\n', fpNew);
                    ++k;
                }
            }
            else
                break;
        }
        fclose(fpOld);
    }
    fclose(fpNew);

    if (fpOld != nullptr || confOld.access() == 0) {
        confOld.remove();
    }
    if (confNew.renameAs(confOld)) {
        fail(_("Unable to rename %s to %s"),
                confNew.string(), confOld.string());
        confNew.remove();
    }
}

void WMConfig::setDefaultFocus(long focusMode) {
    mstring header(
            "#\n"
            "# Focus mode (0=custom, 1=click, 2=sloppy"
            ", 3=explicit, 4=strict, 5=quiet)\n"
            "#\n"
            "FocusMode="
            );
    setDefault("focus_mode", header + mstring(focusMode));
}

void WMConfig::setDefaultTheme(mstring themeName) {
    setDefault("theme", "Theme=\"" + themeName + "\"");
}

void WMConfig::print_options(cfoption* options) {
    for (int i = 0; options[i].type != cfoption::CF_NONE; ++i) {
        switch (options[i].type) {
        case cfoption::CF_BOOL:
            printf("%s=%d\n", options[i].name, options[i].boolval());
            break;
        case cfoption::CF_INT:
            printf("%s=%d\n", options[i].name, options[i].intval());
            break;
        case cfoption::CF_UINT:
            printf("%s=%u\n", options[i].name, options[i].uintval());
            break;
        case cfoption::CF_STR:
            printf("%s=\"%s\"\n", options[i].name, Elvis(options[i].str(), ""));
            break;
        case cfoption::CF_KEY:
            printf("%s=\"%s\"\n", options[i].name, options[i].key()->name);
            break;
        case cfoption::CF_FUNC:
            if (0 == strcmp("Look", options[i].name)) {
                printf("%s=%s\n", "Look", getLookName(wmLook));
            }
            break;
        case cfoption::CF_NONE:
            break;
        }
    }
}

void WMConfig::printPrefs(long focus, cfoption* startup) {
    print_options(startup);
    printf("FocusMode=%ld\n", focus);

    print_options(icewm_preferences);
    print_options(icewm_themable_preferences);

    fflush(stdout);
}

class OptionCopy {
public:
    OptionCopy() {
        opts.setCapacity(512);
        keys.setCapacity(128);
    }
    cfoption* copyOptions(cfoption* options);
    static int countOptions(cfoption* options);

private:
    union cfvalue {
        int i; unsigned u; const char* s; bool b;
    };
    YArray<cfvalue> opts;
    YArray<WMKey> keys;
};

int OptionCopy::countOptions(cfoption* options) {
    int i = 0;
    while (options[i].type)
        ++i;
    return i + 1;
}

cfoption* OptionCopy::copyOptions(cfoption* options) {
    const int count = countOptions(options);
    cfoption* copy = new cfoption[count];
    if (copy) {
        memcpy(copy, options, count * sizeof(cfoption));
        for (int i = 0; i < count; ++i) {
            int o = opts.getCount();
            int w = keys.getCount();
            cfvalue val;
            switch (copy[i].type) {
                case cfoption::CF_NONE:
                    break;
                case cfoption::CF_BOOL:
                    val.b = copy[i].boolval();
                    opts.append(val);
                    copy[i].v.b.bool_value = &opts[o].b;
                    break;
                case cfoption::CF_INT:
                    val.i = copy[i].intval();
                    opts.append(val);
                    copy[i].v.i.int_value = &opts[o].i;
                    break;
                case cfoption::CF_UINT:
                    val.u = copy[i].uintval();
                    opts.append(val);
                    copy[i].v.u.uint_value = &opts[o].u;
                    break;
                case cfoption::CF_STR:
                    val.s = copy[i].str();
                    opts.append(val);
                    copy[i].v.s.string_value = &opts[o].s;
                    break;
                case cfoption::CF_KEY:
                    keys.append(*copy[i].key());
                    copy[i].v.k.key_value = &keys[w];
                    break;
                case cfoption::CF_FUNC:
                    break;
            }
        }
    }
    return copy;
}

int WMConfig::rewritePrefs(cfoption* start_preferences, const char* config) {
    upath origin(YApplication::getLibDir() + "/preferences");
    upath source(nonempty(config) ? config :
                 getDefaultsFilePath("preferences"));

    if (source == null) {
        return EXIT_FAILURE;
    }
    if (origin == source) {
        errno = EDEADLK;
        fail("%s", origin.string());
        return EXIT_SUCCESS;
    }
    if (origin.fileExists() == false || origin.isReadable() == false) {
        fail("%s", origin.string());
        return EXIT_FAILURE;
    }
    if (source.fileExists() == false) {
        if (source.copyFrom(origin)) {
            return EXIT_SUCCESS;
        } else {
            fail("%s", source.string());
            return EXIT_FAILURE;
        }
    }
    if (source.isReadable() == false || source.isWritable() == false) {
        fail("%s", source.string());
        return EXIT_FAILURE;
    }

    OptionCopy copy;
    asmart<cfoption> start(copy.copyOptions(start_preferences));
    asmart<cfoption> icewm(copy.copyOptions(icewm_preferences));
    asmart<cfoption> theme(copy.copyOptions(icewm_themable_preferences));
    if (start && icewm && theme) {
        bool load = YConfig::loadConfigFile(start, source, icewm, theme);
        if (load) {
            YArray<int> smods, imods, tmods;
            for (int i = 0; start[i].type; ++i) {
                if (start[i] != start_preferences[i]) {
                    smods += i;
                }
            }
            for (int i = 0; icewm[i].type; ++i) {
                if (icewm[i] != icewm_preferences[i]) {
                    imods += i;
                }
            }
            for (int i = 0; theme[i].type; ++i) {
                if (theme[i] != icewm_themable_preferences[i]) {
                    tmods += i;
                }
            }
            SavePrefs save;
            if (save.loadText(origin)) {
                if (smods.nonempty())
                    save.applyMods(smods, start);
                if (imods.nonempty())
                    save.applyMods(imods, icewm);
                if (tmods.nonempty())
                    save.applyMods(tmods, theme);
                if (save.saveText(source)) {
                    return EXIT_SUCCESS;
                }
            }
        }
    }
    return EXIT_FAILURE;
}

// vim: set sw=4 ts=4 et:
