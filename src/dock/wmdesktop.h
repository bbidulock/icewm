#ifndef __WMDESKTOP_H
#define __WMDESKTOP_H

#include "ywindow.h"

class WindowListItem;
class YIcon;
class TaskPane;
class TaskBarApp;

class WindowInfo {
public:
    WindowInfo(Window w);
    ~WindowInfo();

    const char *getTitle();// { return "foo";
    const char *getIconTitle();// { return 0; }

    void getNameHint();
    void getIconNameHint();
    void setWindowTitle(const char *aWindowTitle);
    void setIconTitle(const char *aIconTitle);
#ifdef I18N
    void setWindowTitle(XTextProperty *aWindowTitle);
    void setIconTitle(XTextProperty *aIconTitle);
#endif


    YIcon *getIcon() { return 0; }
    bool visibleNow() { return true; }
    bool isMinimized() { return false; }
    bool focused() { return false; }
    bool canRaise() { return true; }

    WindowInfo *owner() const { return fOwner; }

    WindowListItem *winListItem() const { return fWinListItem; }
    void setWinListItem(WindowListItem *i) { fWinListItem = i; }

    void activateWindow(bool raise) {}

    void wmMinimize() {}
    void wmLower() {}
    void wmRaise() {}

    bool isMarked() { return fMarked; }
    void mark(bool mark = true) { fMarked = mark; }

    Window handle() { return fHandle; }

    TaskBarApp *fTaskBarApp;
private:
    WindowListItem *fWinListItem;
    WindowInfo *fOwner;
    Window fHandle;
    bool fMarked;

    char *fWindowTitle;
    char *fIconTitle;
};

class DesktopInfo: public YDesktop {
public:
    DesktopInfo(YWindow *parent, Window win);
    ~DesktopInfo();

    void setTaskPane(TaskPane *tasks);
    void updateTasks();

    WindowInfo *getInfo(Window w);

    virtual void handleProperty(const XPropertyEvent &property);

private:
    TaskPane *fTasks;
    XContext wmContext;
};

#endif
