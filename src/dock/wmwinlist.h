#ifndef __WINLIST_H
#define __WINLIST_H

//#include "wmclient.h" // !!! should be ywindow
#include "ylistbox.h"
#include "yscrollview.h"
#include "yaction.h"
#include "ymenu.h"

class WindowListItem;
class WindowListBox;
class WindowList;
class YWindowManager;
class WindowInfo;
class ClientData;

class WindowListItem: public YListItem {
public:
    WindowListItem(WindowInfo *frame);
    virtual ~WindowListItem();

    virtual int getOffset();
    
    virtual const CStr *getText();
    virtual YIcon *getIcon();
    WindowInfo *getFrame() const { return fFrame; }
private:
    WindowInfo *fFrame;
};

class WindowListBox: public YListBox {
public:
    WindowListBox(WindowList *windowList, YScrollView *view, YWindow *aParent);
    virtual ~WindowListBox();

    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);
    virtual void handleClick(const XButtonEvent &up, int count);
    
    virtual void activateItem(YListItem *item);
private:
    WindowList *fWindowList;
};

class WindowList: public YWindow, public YActionListener {
public:
    WindowList(YWindowManager *root, YWindow *aParent);
    virtual ~WindowList();

    void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClose();

    virtual void configure(int x, int y, unsigned int width, unsigned int height);
    void relayout();

    WindowListItem *addWindowListApp(WindowInfo *frame);
    void removeWindowListApp(WindowListItem *item);

    void repaintItem(WindowListItem *item) { list->repaintItem(item); }
    void showFocused(int x, int y);

    WindowListBox *getList() const { return list; }
    virtual void actionPerformed(YAction *action, unsigned int modifiers);

private:
    YWindowManager *fRoot;
    WindowListBox *list;
    YScrollView *scroll;
};

extern YMenu *logoutMenu;
extern int rebootOrShutdown;
extern YMenu *windowListMenu;
extern YMenu *windowListPopup;
extern YMenu *windowListAllPopup;

#endif
