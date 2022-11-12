#ifndef WINLIST_H
#define WINLIST_H

#include "wmclient.h"
#include "ylistbox.h"
#include "yaction.h"
#include "yarray.h"
#include "ypointer.h"

class WindowListBox;
class YMenu;
class YActionListener;
class YArrange;

class ClientListItem: public YListItem, public YClientItem {
public:
    ClientListItem(YFrameClient* client) : fClient(client) { }
    virtual void goodbye();
    virtual void update();
    virtual void repaint();

    virtual int getOffset();
    virtual mstring getText() { return fClient->windowTitle(); }
    virtual ref<YIcon> getIcon() { return fClient->getIcon(); }
    int getOrder() const;
    int getWorkspace() const;
    virtual void activate();
    YFrameClient* getClient() { return fClient; }

private:
    YFrameClient* fClient;
};

class DesktopListItem: public YListItem {
public:
    DesktopListItem(int desktop) : fDesktop(desktop) { }

    virtual int getOffset() { return -20; }
    virtual mstring getText();
    virtual ref<YIcon> getIcon() { return null; }
    int getWorkspace() const { return fDesktop; }
    virtual void activate();

private:
    int fDesktop;
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
    YArrange getSelectedWindows();
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
    virtual bool handleTimer(YTimer *timer);
    virtual void configure(const YRect2 &r);
    void relayout();

    void addWindowListApp(YFrameClient* client);
    void removeWindowListApp(ClientListItem* item);
    void updateWindowListApp(ClientListItem* item);
    void updateWindowListApps();

    void repaintItem(ClientListItem* item);
    void showFocused(int x, int y);

    YMenu* getWindowListPopup();
    YMenu* getWindowListAllPopup();

private:
    DesktopListItem* allWorkspacesItem;
    osmart<WindowListPopup> windowListPopup;
    osmart<WindowListAllPopup> windowListAllPopup;
    osmart<YScrollView> scroll;
    osmart<WindowListBox> list;
    YObjectArray<DesktopListItem> workspaceItem;
    YTimer focusTimer;

    void setupClient();
    void insertApp(ClientListItem* item);
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
