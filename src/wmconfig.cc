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

#ifndef NO_CONFIGURE
void setLook(const char * /*name*/, const char *arg, bool) {
#ifdef CONFIG_LOOK_WARP4
    if (strcmp(arg, "warp4") == 0)
        wmLook = lookWarp4;
    else
#endif
#ifdef CONFIG_LOOK_WARP3
    if (strcmp(arg, "warp3") == 0)
        wmLook = lookWarp3;
    else
#endif
#ifdef CONFIG_LOOK_WIN95
    if (strcmp(arg, "win95") == 0)
        wmLook = lookWin95;
    else
#endif
#ifdef CONFIG_LOOK_MOTIF
    if (strcmp(arg, "motif") == 0)
        wmLook = lookMotif;
    else
#endif
#ifdef CONFIG_LOOK_NICE
    if (strcmp(arg, "nice") == 0)
        wmLook = lookNice;
    else
#endif
#ifdef CONFIG_LOOK_PIXMAP
    if (strcmp(arg, "pixmap") == 0)
        wmLook = lookPixmap;
    else
#endif
#ifdef CONFIG_LOOK_METAL
    if (strcmp(arg, "metal") == 0)
        wmLook = lookMetal;
    else
#endif
#ifdef CONFIG_LOOK_FLAT
    if (strcmp(arg, "flat") == 0)
        wmLook = lookFlat;
    else
#endif
#ifdef CONFIG_LOOK_GTK
    if (strcmp(arg, "gtk") == 0)
        wmLook = lookGtk;
    else
#endif
    {
        msg(_("Bad Look name"));
    }
}
#endif

static bool ensureDirectory(const upath& path) {
    if (path.dirExists())
        return true;
    if (path.mkdir() != 0) {
        fail(_("Unable to create directory %s"), path.string().c_str());
        return false;
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

static void print_options(cfoption *options) {
    for (int i = 0; options[i].type != cfoption::CF_NONE; ++i) {
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

void WMConfig::print_preferences() {
    print_options(icewm_preferences);
    print_options(icewm_themable_preferences);
}

