/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "ylib.h"

#include "yprefs.h"

#define CFGDEF
#include "wmconfig.h"
#include "bindkey.h"
#include "default.h"

#include "wmoption.h"
#include "wmmgr.h"
#include "yaction.h"
#include "yapp.h"
#include "sysdep.h"

#include "intl.h"

long workspaceCount = 0;
char *workspaceNames[MAXWORKSPACES];
YAction *workspaceActionActivate[MAXWORKSPACES];
YAction *workspaceActionMoveTo[MAXWORKSPACES];

void WMConfig::loadConfiguration(IApp *app, const char *fileName) {
#ifndef NO_CONFIGURE
    YConfig::findLoadConfigFile(app, icewm_preferences, fileName);
    YConfig::findLoadConfigFile(app, icewm_themable_preferences, fileName);
#endif
}

void WMConfig::loadThemeConfiguration(IApp *app, const char *themeName) {
#ifndef NO_CONFIGURE
    bool ok = YConfig::findLoadThemeFile(app,
                icewm_themable_preferences,
                upath("themes").child(themeName));
    if (ok == false)
        fail(_("Failed to load theme %s"), themeName);
#endif
}

void WMConfig::freeConfiguration() {
#ifndef NO_CONFIGURE
    YConfig::freeConfig(icewm_preferences);
    YConfig::freeConfig(icewm_themable_preferences);
#endif
}

void addWorkspace(const char * /*name*/, const char *value, bool append) {
    if (!append) {
        for (int i = 0; i < workspaceCount; i++) {
            delete[] workspaceNames[i];
            delete workspaceActionActivate[i];
            delete workspaceActionMoveTo[i];
        }
        workspaceCount = 0;
    }

    if (workspaceCount >= MAXWORKSPACES)
        return;
    workspaceNames[workspaceCount] = newstr(value);
    workspaceActionActivate[workspaceCount] = new YAction(); // !! fix
    workspaceActionMoveTo[workspaceCount] = new YAction();
    PRECONDITION(workspaceNames[workspaceCount] != NULL);
    workspaceCount++;
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

#ifndef NO_CONFIGURE
void setLook(const char *name, const char *arg, bool) {
    enum WMLook look = wmLook;
    if (getLook(name, arg, &look)) {
        wmLook = look;
    } else {
        msg(_("Unknown value '%s' for option '%s'."), arg, name);
    }
}
#endif

static bool ensureDirectory(const upath& path) {
    if (path.dirExists())
        return true;
    if (path.mkdir() != 0) {
        fail(_("Unable to create directory %s"), path.string().c_str());
    }
    return path.dirExists();
}

static upath getDefaultsFilePath(const pstring& basename) {
    upath xdg(YApplication::getXdgConfDir());
    if (xdg.dirExists()) {
        upath file(xdg + basename);
        if (file.fileExists())
            return file;
    }
    upath prv(YApplication::getPrivConfDir());
    if (ensureDirectory(prv)) {
        return prv + basename;
    }
    ensureDirectory(xdg.parent());
    if (ensureDirectory(xdg)) {
        return xdg + basename;
    }
    return null;
}

int WMConfig::setDefault(const char *basename, const char *content) {
    upath confOld(getDefaultsFilePath(basename));
    if (confOld == null) {
        return -1; // no directory
    }
    upath confNew(confOld.path() + ".new.tmp");

    FILE *fpNew = confNew.fopen("w");
    if (fpNew) {
        fputs(content, fpNew);
        if (content[0] && content[strlen(content)-1] != '\n')
            fputc('\n', fpNew);
    }
    if (fpNew == NULL || fflush(fpNew) || ferror(fpNew)) {
        fail(_("Unable to write to %s"), confNew.string().c_str());
        if (fpNew)
            fclose(fpNew);
        confNew.remove();
        return -1;
    }

    FILE *fpOld = confOld.fopen("r");
    if (fpOld) {
        for (int i = 0; i < 10; ++i) {
            char buf[600] = "#", *line = buf;
            if (fgets(1 + buf, sizeof buf - 1, fpOld)) {
                while (line[1] == '#')
                    ++line;
                fputs(line, fpNew);
                if ('\n' != line[strlen(line)-1])
                    fputc('\n', fpNew);
            }
        }
        fclose(fpOld);
    }
    fclose(fpNew);

    if (fpOld != 0 || confOld.access() == 0) {
        confOld.remove();
    }
    if (confNew.renameAs(confOld)) {
        fail(_("Unable to rename %s to %s"),
                confNew.string().c_str(), confOld.string().c_str());
        confNew.remove();
    }
    return 0;
}

#ifndef NO_CONFIGURE
static void print_options(cfoption *options) {
    for (int i = 0; options[i].type != cfoption::CF_NONE; ++i) {
        if (options[i].notify) {
            if (0 == strcmp("Look", options[i].name)) {
                printf("%s=%s\n", "Look", getLookName(wmLook));
            }
            continue;
        }
        switch (options[i].type) {
        case cfoption::CF_BOOL:
            printf("%s=%d\n", options[i].name, *options[i].v.bool_value);
            break;
        case cfoption::CF_INT:
            printf("%s=%d\n", options[i].name, *options[i].v.i.int_value);
            break;
        case cfoption::CF_STR:
            printf("%s=\"%s\"\n", options[i].name,
                    options[i].v.s.string_value && *options[i].v.s.string_value
                    ? *options[i].v.s.string_value : "");
            break;
#ifndef NO_KEYBIND
        case cfoption::CF_KEY:
            printf("%s=\"%s\"\n", options[i].name,
                    options[i].v.k.key_value->name);
            break;
#endif
        case cfoption::CF_NONE:
            break;
        }
    }
}
#endif

void WMConfig::print_preferences() {
#ifndef NO_CONFIGURE
    print_options(icewm_preferences);
    print_options(icewm_themable_preferences);
#endif
}

