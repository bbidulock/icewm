#ifndef __WMCONFIG_H
#define __WMCONFIG_H

class IApp;
struct cfoption;
#include "mstring.h"

class WMConfig {
public:
    static void loadConfiguration(IApp *app, const char *fileName);
    static bool loadThemeConfiguration(IApp *app, const char *themeName);
    static void freeConfiguration();
    static void setDefault(const char *basename, mstring config);
    static void setDefaultFocus(long focusMode);
    static void setDefaultTheme(mstring themeName);
    static void printPrefs(long focus, cfoption* startup);
    static void print_options(cfoption* options);
};

// functions which are used in preferences options:

void addWorkspace(const char *name, const char *value, bool append);
void addKeyboard(const char *name, const char *value, bool append);
void setLook(const char *name, const char *value, bool append);

extern class YStringArray configWorkspaces;
extern class MStringArray configKeyboards;

#endif

// vim: set sw=4 ts=4 et:
