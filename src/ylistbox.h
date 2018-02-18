#ifndef __YLISTBOX_H
#define __YLISTBOX_H

#include "ywindow.h"
#include "yscrollbar.h"
#include "yscrollview.h"

class YScrollView;
class YIcon;

class YListItem {
public:
    YListItem();
    virtual ~YListItem();

    YListItem *getNext();
    YListItem *getPrev();
    void setNext(YListItem *next);
    void setPrev(YListItem *prev);

    bool getSelected() { return fSelected; }
    void setSelected(bool aSelected);

    virtual int getOffset();

    virtual ustring getText();
    virtual ref<YIcon> getIcon();
private:
    bool fSelected; // !!! remove this from here
    YListItem *fPrevItem, *fNextItem;
};

class YListBox: public YWindow, public YScrollBarListener, public YScrollable {
public:
    YListBox(YScrollView *view, YWindow *aParent);
    virtual ~YListBox();

    int addItem(YListItem *item);
    int addAfter(YListItem *prev, YListItem *item);
    void removeItem(YListItem *item);

    virtual void configure(const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    virtual void paint(Graphics &g, const YRect &r);
    virtual void scroll(YScrollBar *sb, int delta);
    virtual void move(YScrollBar *sb, int pos);

    virtual bool isFocusTraversable();
    bool hasSelection();
    virtual void activateItem(YListItem *item);

    YListItem *getFirst() const { return fFirst; }
    YListItem *getLast() const { return fLast; }

    int getItemCount();
    YListItem *getItem(int item);
    int findItemByPoint(int x, int y);
    int findItem(YListItem *item);
    int getLineHeight();

    int maxWidth();

    bool isSelected(int item);
    bool isSelected(YListItem *item);

    virtual unsigned contentWidth();
    virtual unsigned contentHeight();
    virtual YWindow *getWindow();

    void focusSelectItem(int no) { setFocusedItem(no, true, false, false); }

    void repaintItem(YListItem *item);

private:
    YScrollBar *fVerticalScroll;
    YScrollBar *fHorizontalScroll;
    YScrollView *fView;
    YListItem *fFirst, *fLast;
    int fItemCount;
    YListItem **fItems;

    int fOffsetX;
    int fOffsetY;
    int fMaxWidth;
    int fFocusedItem;
    int fSelectStart, fSelectEnd;
    bool fDragging;
    bool fSelect;

    static int fAutoScrollDelta;

    bool isItemSelected(int item);
    void selectItem(int item, bool sel);
    void selectItems(int selStart, int selEnd, bool sel);
    void paintItems(int selStart, int selEnd);
    void clearSelection();
    void setFocusedItem(int item, bool clear, bool extend, bool virt);

    void applySelection();
    void paintItem(int i);
    void paintItem(Graphics &g, int n);
    void resetScrollBars();
    void freeItems();
    void updateItems();
    void autoScroll(int delta, const XMotionEvent *motion);
    void focusVisible();
    void ensureVisibility(int item);

    ref<YImage> fGradient;
};

#endif

// vim: set sw=4 ts=4 et:
