#ifndef __WMCONFIG_H
#define __WMCONFIG_H

class IApp;
#include "mstring.h"

class WMConfig {
public:
    static void loadConfiguration(IApp *app, const char *fileName);
    static bool loadThemeConfiguration(IApp *app, const char *themeName);
    static void freeConfiguration();
    static void setDefault(const char *basename, cstring config);
    static void setDefaultFocus(long focusMode);
    static void setDefaultTheme(mstring themeName);
    static void printPrefs(long focus, bool log, bool sync, const char* spl);
    static void print_preferences();
};

// functions which are used in preferences options:

void addWorkspace(const char *name, const char *value, bool append);
void setLook(const char *name, const char *value, bool append);

#endif

// vim: set sw=4 ts=4 et:
