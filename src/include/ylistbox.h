#ifndef __YLISTBOX_H
#define __YLISTBOX_H

#include "ywindow.h"
#include "yscrollbar.h"
#include "yscrollview.h"
#include "yconfig.h"

#pragma interface

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

    virtual const CStr *getText();
    virtual YIcon *getIcon();
private:
//#warning "make separate list of selected items?"
    bool fSelected;
    YListItem *fPrevItem, *fNextItem;
private: // not-used
    YListItem(const YListItem &);
    YListItem &operator=(const YListItem &);
};

class YListBox: public YWindow, public YScrollBarListener, public YScrollable {
public:
    YListBox(YScrollView *view, YWindow *aParent);
    virtual ~YListBox();

    int addItem(YListItem *item);
    int addAfter(YListItem *prev, YListItem *item);
    void removeItem(YListItem *item);

    virtual void configure(const YRect &cr);
    //virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod);
    virtual bool eventKey(const YKeyEvent &key);
    virtual bool eventButton(const YButtonEvent &button);
    virtual bool eventClick(const YClickEvent &up);
    virtual bool eventDrag(const YButtonEvent &down, const YMotionEvent &motion);
    virtual bool eventMotion(const YMotionEvent &motion);
    virtual bool handleAutoScroll(const YMotionEvent &mouse);

    virtual void paint(Graphics &g, const YRect &er);
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

    virtual int contentWidth();
    virtual int contentHeight();
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

    static YColorPrefProperty gListBoxBg;
    static YColorPrefProperty gListBoxFg;
    static YColorPrefProperty gListBoxSelBg;
    static YColorPrefProperty gListBoxSelFg;
    static YFontPrefProperty gListBoxFont;
    static YPixmapPrefProperty gListBoxPixmapBg;

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
    void autoScroll(int delta, const YMotionEvent *motion);
    void focusVisible();
    void ensureVisibility(int item);
private: // not-used
    YListBox(const YListBox &);
    YListBox &operator=(const YListBox &);
};

#endif
