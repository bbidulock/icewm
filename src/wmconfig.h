#ifndef __WMCONFIG_H
#define __WMCONFIG_H

class IApp;

void loadConfiguration(IApp *app, const char *fileName);
void loadThemeConfiguration(IApp *app, const char *themeName);
void addWorkspace(const char *name, const char *value, bool append);
void setLook(const char *name, const char *value, bool append);
void freeConfiguration();
int setDefault(const char *basename, const char *config);

#endif
