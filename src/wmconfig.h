#ifndef __WMCONFIG_H
#define __WMCONFIG_H

class IApp;

class WMConfig {
public:
    static void loadConfiguration(IApp *app, const char *fileName);
    static void loadThemeConfiguration(IApp *app, const char *themeName);
    static void freeConfiguration();
    static int setDefault(const char *basename, const char *config);
    static void print_preferences();
};

// functions which are used in preferences options:

void addWorkspace(const char *name, const char *value, bool append);
void setLook(const char *name, const char *value, bool append);

#endif

// vim: set sw=4 ts=4 et:
