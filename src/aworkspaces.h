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
    virtual YFont * getFont();
    virtual YColor * getColor();
    virtual YSurface getSurface();

private:
    static YTimer *fRaiseTimer;
    long fWorkspace;

    static YColor * normalButtonBg;
    static YColor * normalButtonFg;

    static YColor * activeButtonBg;
    static YColor * activeButtonFg;

    static YFont * normalButtonFont;
    static YFont * activeButtonFont;
};

class WorkspacesPane: public YWindow {
public:
    WorkspacesPane(YWindow *parent);
    ~WorkspacesPane();

    void configure(const YRect &r, const bool resized);

    WorkspaceButton *workspaceButton(long n);
private:
    WorkspaceButton **fWorkspaceButton;
};

extern YPixmap *workspacebuttonPixmap;
extern YPixmap *workspacebuttonactivePixmap;

#ifdef CONFIG_GRADIENTS
extern class YPixbuf *workspacebuttonPixbuf;
extern class YPixbuf *workspacebuttonactivePixbuf;
#endif

#endif
