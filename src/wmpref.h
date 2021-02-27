#ifndef WMPREF_H
#define WMPREF_H

#include "ymenu.h"
#include "yaction.h"
#include "yarray.h"
#include "ymsgbox.h"

struct cfoption;

class PrefsMenu:
    public YMenu,
    private YActionListener,
    private YMsgBoxListener
{
public:
    PrefsMenu();

private:
    const int count;
    const YAction actionSaveMod;
    static YArray<int> mods;
    YMsgBox* message;
    cfoption* modify;

    void query(cfoption* opt, const char* old);
    void modified(cfoption* opt, bool set = true);

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleMsgBox(YMsgBox *msgbox, int operation);

    static int countPrefs();
    static int sortPrefs(const void* p1, const void* p2);
};

class SavePrefs {
public:
    SavePrefs(YArray<int>& modified);

    static char* retrieveComment(cfoption* o);

private:
    YArray<int> mods;
    fcsmart text;
    static int compare(const void* left, const void* right);

    upath defaultPrefs(upath path);
    upath preferencesPath();
    bool updateOption(cfoption* o, char* buf, size_t blen);
    bool insertOption(cfoption* o, char* buf, size_t blen);
    void writePrefs(upath dest, const char* text, size_t tlen);
    void replace(char* s1, size_t n1, char* s2, size_t n2, char* s3 = nullptr,
                 size_t n3 = 0, char* s4 = nullptr, size_t n4 = 0);
    char* nextline(char* s);
};

#endif
