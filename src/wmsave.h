#ifndef WMSAVE_H
#define WMSAVE_H

#include "upath.h"
#include "yarray.h"
#include "ypointer.h"

struct cfoption;

class SavePrefs {
public:
    SavePrefs(YArray<int>& modified, const char* configFile);
    SavePrefs();

    bool loadText(upath path);
    bool saveText(upath path);
    void applyMods(YArray<int>& modified, cfoption* options);

    static char* retrieveComment(cfoption* o);

private:
    fcsmart text;

    static int compare(const void* left, const void* right);

    upath defaultPrefs(upath path);
    upath preferencesPath(const char* configFile);
    bool updateOption(cfoption* o, char* buf, size_t blen);
    bool insertOption(cfoption* o, char* buf, size_t blen);
    bool writePrefs(upath dest, const char* text, size_t tlen);
    void replace(char* s1, size_t n1, char* s2, size_t n2, char* s3 = nullptr,
                 size_t n3 = 0, char* s4 = nullptr, size_t n4 = 0);
    char* nextline(char* s);
};

#endif
