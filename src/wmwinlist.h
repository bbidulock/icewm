#ifndef __WINLIST_H
#define __WINLIST_H

#include "wmclient.h" // !!! should be ywindow
#include "ylistbox.h"
#include "yscrollview.h"
#include "yaction.h"

#ifdef CONFIG_WINLIST

class WindowListItem;
class WindowListBox;

class WindowListItem: public YListItem {
public:
    WindowListItem(ClientData *frame);
    virtual ~WindowListItem();

    virtual int getOffset();
    
    virtual const char *getText();
    virtual YIcon *getIcon();
    ClientData *getFrame() const { return fFrame; }
private:
    ClientData *fFrame;
};

class WindowListBox:
public YListBox,
public YAction::Listener {
public:
    WindowListBox(YScrollView *view, YWindow *aParent);
    virtual ~WindowListBox();

    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleClick(const XButtonEvent &up, int count);
    
    virtual void activateItem(YListItem *item);
    virtual void actionPerformed(YAction *action, unsigned int modifiers);
};

class WindowList: public YFrameClient {
public:
    WindowList(YWindow *aParent);
    virtual ~WindowList();

    void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClose();

    virtual void configure(const int x, const int y, 
    			   const unsigned width, const unsigned height,
			   const bool resized);
    void relayout();

    WindowListItem *addWindowListApp(YFrameWindow *frame);
    void removeWindowListApp(WindowListItem *item);

    void repaintItem(WindowListItem *item) { list->repaintItem(item); }
    void showFocused(int x, int y);

    WindowListBox *getList() const { return list; }

private:
    WindowListBox *list;
    YScrollView *scroll;
};

extern WindowList *windowList;

#endif

#endif
