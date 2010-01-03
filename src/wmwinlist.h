#ifndef __WINLIST_H
#define __WINLIST_H

#ifdef CONFIG_WINLIST

#include "wmclient.h" // !!! should be ywindow
#include "ylistbox.h"
#include "yscrollview.h"
#include "yaction.h"
#include "yarray.h"

class WindowListItem;
class WindowListBox;
class YMenu;

class WindowListItem: public YListItem {
public:
    WindowListItem(ClientData *frame, int workspace = 0);
    virtual ~WindowListItem();

    virtual int getOffset();
    
    virtual ustring getText();
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
    virtual void actionPerformed(YAction *action, unsigned int modifiers);

    void enableCommands(YMenu *popup);
    void getSelectedWindows(YArray<YFrameWindow *> &frames);
};

class WindowList: public YFrameClient {
public:
    WindowList(YWindow *aParent);
    virtual ~WindowList();

    void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClose();

    virtual void configure(const YRect &r);
    void relayout();

    WindowListItem *addWindowListApp(YFrameWindow *frame);
    void removeWindowListApp(WindowListItem *item);
    void updateWindowListApp(WindowListItem *item);

    void repaintItem(WindowListItem *item) { list->repaintItem(item); }
    void showFocused(int x, int y);

    WindowListBox *getList() const { return list; }

private:
    WindowListBox *list;
    YScrollView *scroll;
    WindowListItem **workspaceItem;

    void insertApp(WindowListItem *item);
};

extern WindowList *windowList;

#endif

#endif
