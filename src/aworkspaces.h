#ifndef AWORKSPACES_H_
#define AWORKSPACES_H_

#include "ywindow.h"
#include "ybutton.h"
#include "ytimer.h"

class WorkspaceButton: public YButton, public YTimerListener {
public:
    WorkspaceButton(long workspace, YWindow *parent);

    virtual void handleClick(const XButtonEvent &up, int count);

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction *button, unsigned int modifiers);
private:
    static YTimer *fRaiseTimer;
    long fWorkspace;
};

class WorkspacesPane: public YWindow {
public:
    WorkspacesPane(YWindow *parent);
    ~WorkspacesPane();

    WorkspaceButton *workspaceButton(long n);
private:
    WorkspaceButton **fWorkspaceButton;
};

#endif
