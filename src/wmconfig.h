#ifndef __WMCONFIG_H
#define __WMCONFIG_H

extern bool configurationNeeded;

void loadConfiguration(const char *fileName);
void loadThemeConfiguration(const char *fileName);
void addWorkspace(const char *name, const char *value, bool append);
void setLook(const char *name, const char *value, bool append);
void freeConfiguration();

#endif
