#ifndef AWORKSPACES_H_
#define AWORKSPACES_H_

#include "ywindow.h"
#include "obj.h"
#include "objbutton.h"

class WorkspaceButton: public ObjectButton, public YTimerListener {
public:
    WorkspaceButton(long workspace, YWindow *parent);

    virtual void handleClick(const XButtonEvent &up, int count);

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction button, unsigned int modifiers);
    virtual ref<YFont> getFont();
    virtual YColor   getColor();
    virtual YSurface getSurface();

    void updateName();
    mstring baseName();

private:
    virtual void paint(Graphics &g, const YRect &r);

    static lazy<YTimer> fRaiseTimer;
    long fWorkspace;

    static YColorName normalButtonBg;
    static YColorName normalBackupBg;
    static YColorName normalButtonFg;

    static YColorName activeButtonBg;
    static YColorName activeBackupBg;
    static YColorName activeButtonFg;

    static ref<YFont> normalButtonFont;
    static ref<YFont> activeButtonFont;
};

class WorkspacesPane: public YWindow {
public:
    WorkspacesPane(YWindow *parent);
    ~WorkspacesPane();

    void repaint();

    void relabelButtons();
    void configure(const YRect &r);
    void updateButtons();

    WorkspaceButton *workspaceButton(long n);
private:
    WorkspaceButton **fWorkspaceButton;
    long fWorkspaceButtonCount;
    void repositionButtons();
};

#endif

// vim: set sw=4 ts=4 et:
