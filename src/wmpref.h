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

#endif
