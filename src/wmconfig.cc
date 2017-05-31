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

void loadConfiguration(IApp *app, const char *fileName) {
#ifndef NO_CONFIGURE
    YConfig::findLoadConfigFile(app, icewm_preferences, fileName);
    YConfig::findLoadConfigFile(app, icewm_themable_preferences, fileName);
#endif
}

void loadThemeConfiguration(IApp *app, const char *themeName) {
#ifndef NO_CONFIGURE
    YConfig::findLoadConfigFile(app, icewm_themable_preferences, upath("themes").child(themeName));
#endif
}

void freeConfiguration() {
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

int setDefault(const char *basename, const char *config) {
    upath confDir(YApplication::getPrivConfDir());
    if (confDir.dirExists() == false)
        confDir.mkdir(0777);
    upath conf = confDir + basename;
    ustring confTmp = conf.path() + ".new.tmp";
    const char *confNew = cstring(confTmp);

    int fd = open(confNew, O_RDWR | O_TEXT | O_CREAT | O_TRUNC | O_EXCL, 0666);
    if (fd == -1) {
       fail("Unable to write %s", confNew);
       return -1;
    }
    int len = strlen(config);
    int nlen = write(fd, config, len);
    
    FILE *fdold = fopen(cstring(conf), "r");
    if (fdold) {
        char tmpbuf[300];
        *tmpbuf = '#';
        for (int i = 0; i < 10; i++)
            if (fgets(tmpbuf + 1, 298, fdold)) {
                int tlen = strlen(tmpbuf);
                int n, ret;
                for (n = 0; n < tlen;) {
                    ret = write(fd, tmpbuf + n, tlen - n);
                    if (ret == 0 || (ret < 0 && errno != EINTR)) {
                        nlen = -1;
                        break;
                    }
                    if (ret > 0)
                        n += ret;
                }
            }
            else
                break;
        fclose(fdold);
    }

    close(fd);
    if (nlen == len) {
        rename(confNew, cstring(conf));
    } else {
        remove(confNew);
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

void print_preferences() {
    print_options(icewm_preferences);
    print_options(icewm_themable_preferences);
}

