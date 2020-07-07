#ifndef YLISTBOX_H
#define YLISTBOX_H

#include "yscrollbar.h"
#include "yscrollview.h"

class YScrollView;
class YIcon;

class YListItem {
public:
    YListItem() : fSelected(false) { }
    virtual ~YListItem() { }

    bool getSelected() { return fSelected; }
    void setSelected(bool aSelected) { fSelected = aSelected; }

    virtual int getOffset() { return 0; }
    virtual int getWidth();
    virtual mstring getText() { return null; }
    virtual ref<YIcon> getIcon() { return null; }
private:
    bool fSelected;
};

class YListBox:
    public YWindow,
    public YScrollBarListener,
    public YScrollable,
    public YTimerListener
{
public:
    YListBox(YScrollView *view, YWindow *aParent);
    virtual ~YListBox();

    void addItem(YListItem *item);
    void addAfter(YListItem *after, YListItem *item);
    void addBefore(YListItem *before, YListItem *item);
    void removeItem(YListItem *item);

    virtual void configure(const YRect2 &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleDrag(const XButtonEvent &down, const XMotionEvent &motion);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);
    virtual bool handleTimer(YTimer* timer);
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void handleVisibility(const XVisibilityEvent& visib);

    virtual void outdated();
    virtual void repaint();
    virtual void paint(Graphics &g, const YRect &r);
    virtual void scroll(YScrollBar *sb, int delta);
    virtual void move(YScrollBar *sb, int pos);

    virtual bool isFocusTraversable();
    bool hasSelection();
    virtual void activateItem(YListItem *item);

    int getItemCount() const { return fItems.getCount(); }
    YListItem *getItem(int item);
    int findItemByPoint(int x, int y);
    int findItem(YListItem *item) const { return find(fItems, item); }
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

    int fOffsetX;
    int fOffsetY;
    int fMaxWidth;
    int fWidestItem;
    int fFocusedItem;
    int fSelectStart, fSelectEnd;
    bool fDragging;
    bool fSelect;
    bool fVisible;
    bool fOutdated;
    GraphicsBuffer fGraphics;

    static int fAutoScrollDelta;

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

    lazy<YTimer> fTimer;
    ref<YImage> fGradient;

    typedef YArray<YListItem*> ArrayType;
    ArrayType fItems;

protected:
    typedef ArrayType::IterType IterType;
    IterType getIterator() { return fItems.iterator(); }
};

#endif

// vim: set sw=4 ts=4 et:
