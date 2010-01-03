#ifndef AWORKSPACES_H_
#define AWORKSPACES_H_

#include "ywindow.h"
#include "obj.h"
#include "objbutton.h"
#include "ytimer.h"

class WorkspaceButton: public ObjectButton, public YTimerListener {
public:
    WorkspaceButton(long workspace, YWindow *parent);

    virtual void handleClick(const XButtonEvent &up, int count);

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction *button, unsigned int modifiers);
    virtual ref<YFont> getFont();
    virtual YColor * getColor();
    virtual YSurface getSurface();

private:
    virtual void paint(Graphics &g, const YRect &r);

    static YTimer *fRaiseTimer;
    long fWorkspace;

    static YColor * normalButtonBg;
    static YColor * normalButtonFg;

    static YColor * activeButtonBg;
    static YColor * activeButtonFg;

    static ref<YFont> normalButtonFont;
    static ref<YFont> activeButtonFont;
};

class WorkspacesPane: public YWindow {
public:
    WorkspacesPane(YWindow *parent);
    ~WorkspacesPane();

    void repaint();

    void configure(const YRect &r);

    WorkspaceButton *workspaceButton(long n);
private:
    WorkspaceButton **fWorkspaceButton;
};

extern ref<YPixmap> workspacebuttonPixmap;
extern ref<YPixmap> workspacebuttonactivePixmap;

#ifdef CONFIG_GRADIENTS
extern ref<YImage> workspacebuttonPixbuf;
extern ref<YImage> workspacebuttonactivePixbuf;
#endif

#endif
