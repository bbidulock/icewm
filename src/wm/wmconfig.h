#ifndef __WMCONFIG_H
#define __WMCONFIG_H

extern int configurationLoaded;

void loadConfiguration(const char *fileName);
void addWorkspace(const char *name);
void freeConfig();

#endif
