#ifndef AWORKSPACES_H_
#define AWORKSPACES_H_

#include "ywindow.h"
#include "ybutton.h"
#include "ytimer.h"

//class YWindowManager;
class WorkspacesPane;

class WorkspaceButton: public YButton, public YTimerListener {
public:
    WorkspaceButton(WorkspacesPane *root, long workspace, YWindow *parent);

    virtual void handleClick(const XButtonEvent &up, int count);

    virtual void handleDNDEnter(int nTypes, Atom *types);
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction *button, unsigned int modifiers);
private:
    //static YTimer *fRaiseTimer;
    WorkspacesPane *fRoot;
    long fWorkspace;
};

class WorkspacesPane: public YWindow {
public:
    WorkspacesPane(YWindow *parent);
    ~WorkspacesPane();

    WorkspaceButton *workspaceButton(long n);

    long workspaceCount();
    const char *workspaceName(long workspace);
    void activateWorkspace(long workspace);
private:
    //YWindowManager *fRoot;

    WorkspaceButton **fWorkspaceButton;
    int fWorkspaceCount;
};

#endif
