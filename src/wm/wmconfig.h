#ifndef __WMCONFIG_H
#define __WMCONFIG_H

extern int configurationLoaded;

void __loadConfiguration(const char *fileName);
void __addWorkspace(const char *name);
void freeConfig();

#endif
