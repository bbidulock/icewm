#ifndef __WMCONFIG_H
#define __WMCONFIG_H

extern int configurationLoaded;

void loadConfiguration(const char *fileName);
char *getArgument(char *dest, int maxLen, char *p, bool comma);
void addWorkspace(const char *name);
void freeConfig();


#endif
