#ifndef __WMDIALOG_H
#define __WMDIALOG_H

#include "ywindow.h"
#include "yaction.h"
#include "yconfig.h"

class YActionButton;

class CtrlAltDelete: public YWindow, public YActionListener {
public:
    CtrlAltDelete(YWindow *parent);
    virtual ~CtrlAltDelete();

    virtual void paint(Graphics &g, const YRect &er);
    virtual bool eventKey(const YKeyEvent &key);
    virtual void actionPerformed(YAction *action, unsigned int modifiers);

    void activate();
    void deactivate();
private:
    YActionButton *lockButton;
    YActionButton *logoutButton;
    YActionButton *restartButton;
    YActionButton *cancelButton;
    YActionButton *rebootButton;
    YActionButton *shutdownButton;

    YPref fShutdownCommand;
    YPref fRebootCommand;
    YPref fLogoutCommand;
    YPref fLockCommand;

    bool canShutdown(bool reboot);

    static YColorPrefProperty gBackgroundColor;
};

extern YPixmap *logoutPixmap;

#endif
