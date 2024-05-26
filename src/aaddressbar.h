#ifndef ADDRESS_BAR_H
#define ADDRESS_BAR_H

#include "yinputline.h"
#include "upath.h"

class IApp;

class AddressBar:
    public YInputLine,
    private YInputListener
{
public:
    AddressBar(IApp *app, YWindow *parent = nullptr);
    virtual ~AddressBar();

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleFocus(const XFocusChangeEvent &focus);

    void showNow();
    void hideNow();

private:
    void changeLocation(int newLocation);
    void handleReturn(bool control);
    bool appendCommand(const char* cmd, class YStringArray& args);
    bool internal(mstring line);
    void loadHistory();
    void saveHistory();
    upath historyFile();

    void inputReturn(YInputLine* input, bool control);
    void inputEscape(YInputLine* input);
    void inputLostFocus(YInputLine* input);
    bool handleTimer(YTimer* timer);

    IApp *app;
    MStringArray history;
    int location;
    int restored;
    const int saveLines = 64;
    upath curdir, olddir;
    lazy<YTimer> loadHistoryTimer;
    lazy<YTimer> saveHistoryTimer;
};

#endif

// vim: set sw=4 ts=4 et:
