#ifndef WINLIST_H
#define WINLIST_H

#include "wmclient.h"
#include "ylistbox.h"
#include "yaction.h"
#include "yarray.h"
#include "ypointer.h"

class WindowListItem;
class WindowListBox;
class YMenu;
class YActionListener;

class WindowListItem: public YListItem {
public:
    WindowListItem(ClientData *frame, int workspace);
    virtual ~WindowListItem();

    virtual int getOffset();

    virtual mstring getText();
    virtual ref<YIcon> getIcon();
    ClientData *getFrame() const { return fFrame; }
    int getWorkspace() const { return fWorkspace; }
private:
    ClientData *fFrame;
    int fWorkspace;
};

class WindowListBox: public YListBox, public YActionListener {
public:
    WindowListBox(YScrollView *view, YWindow *aParent);
    virtual ~WindowListBox();

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleClick(const XButtonEvent &up, int count);

    virtual void activateItem(YListItem *item);
    virtual void actionPerformed(YAction action, unsigned int modifiers);

    void enableCommands(YMenu *popup);
    void getSelectedWindows(YArray<YFrameWindow *> &frames);
};

class WindowListPopup : public YMenu {
public:
    WindowListPopup();
};

class WindowListAllPopup : public YMenu {
public:
    WindowListAllPopup();
};

class WindowList: public YFrameClient {
public:
    WindowList(YWindow *aParent);
    virtual ~WindowList();

    void updateWorkspaces();

    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleExpose(const XExposeEvent&) {}
    virtual void handleClose();

    virtual void configure(const YRect2 &r);
    void relayout();

    WindowListItem *addWindowListApp(YFrameWindow *frame);
    void removeWindowListApp(WindowListItem *item);
    void updateWindowListApp(WindowListItem *item);
    void updateWindowListApps();

    void repaintItem(WindowListItem *item);
    void showFocused(int x, int y);

    YMenu* getWindowListPopup();
    YMenu* getWindowListAllPopup();

private:
    osmart<WindowListPopup> windowListPopup;
    osmart<WindowListAllPopup> windowListAllPopup;
    osmart<YScrollView> scroll;
    osmart<WindowListBox> list;
    YObjectArray<WindowListItem> workspaceItem;

    void setupClient();
    void insertApp(WindowListItem *item);
};

extern class WindowListProxy {
    WindowList* wlist;
public:
    WindowListProxy() : wlist(nullptr) { }
    ~WindowListProxy() { release(); }
    operator bool() { return wlist; }
    operator WindowList*() { return wlist ? wlist : acquire(); }
    WindowList* operator->() { return *this; }
    WindowList* _ptr() { return wlist; }
    void release() { if (wlist) { delete wlist; wlist = nullptr; } }
    void operator=(null_ref&) { release(); }
private:
    WindowList* acquire();
    operator void* ();  // undefined
    operator int ();    // undefined
} windowList;

#endif

// vim: set sw=4 ts=4 et:
