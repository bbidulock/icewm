/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 *
 * Window list
 */
#pragma implementation
#include "config.h"

#include "yxkeydef.h"
#include "ylistbox.h"

#include "ykeyevent.h"
#include "ybuttonevent.h"
#include "ymotionevent.h"
#include "yscrollview.h"
#include "yapp.h"
#include "deffonts.h"
#include "ycstring.h"
#include "yrect.h"
#include "ypaint.h"
#include "base.h"

#include <string.h>

#define ISLOWER(c) ((c) >= 'a' && (c) <= 'z')
#define TOUPPER(c) (ISLOWER(c) ? (c) - 'a' + 'A' : (c))

YColorPrefProperty YListBox::gListBoxBg("system", "ColorListBox", "rgb:C0/C0/C0");
YColorPrefProperty YListBox::gListBoxFg("system", "ColorListBoxText", "rgb:00/00/00");
YColorPrefProperty YListBox::gListBoxSelBg("system", "ColorListBoxSelection", "rgb:80/80/80");
YColorPrefProperty YListBox::gListBoxSelFg("system", "ColorListBoxSelectionText", "rgb:00/00/00");
YFontPrefProperty YListBox::gListBoxFont("system", "ListBoxFontName", FONT(120));
YPixmapPrefProperty YListBox::gListBoxPixmapBg("system", "PixmapListBoxBg", 0, 0);

int YListBox::fAutoScrollDelta = 0;

YListItem::YListItem(): fSelected(false), fPrevItem(0), fNextItem(0) {
}

YListItem::~YListItem() {
}

YListItem *YListItem::getNext() {
    return fNextItem;
}

YListItem *YListItem::getPrev() {
    return fPrevItem;
}

void YListItem::setNext(YListItem *next) {
    fNextItem = next;
}

void YListItem::setPrev(YListItem *prev) {
    fPrevItem = prev;
}

void YListItem::setSelected(bool aSelected) {
    fSelected = aSelected;
}

const CStr *YListItem::getText() {
    return 0;
}

YIcon *YListItem::getIcon() {
    return 0;
}

int YListItem::getOffset() {
    return 0;
}

YListBox::YListBox(YScrollView *view, YWindow *aParent): YWindow(aParent) {
    //???setBitGravity(NorthWestGravity);
    fView = view;
    if (fView) {
        fVerticalScroll = view->getVerticalScrollBar();
        fHorizontalScroll = view->getHorizontalScrollBar();
    } else {
        fHorizontalScroll = 0;
        fVerticalScroll = 0;
    }
    if (fVerticalScroll)
        fVerticalScroll->setScrollBarListener(this);
    if (fHorizontalScroll)
        fHorizontalScroll->setScrollBarListener(this);
    fOffsetX = 0;
    fOffsetY = 0;
    fFirst = fLast = 0;
    fFocusedItem = 0;
    fSelectStart = fSelectEnd = -1;
    fDragging = false;
    fSelect = false;
    fItemCount = 0;
    fItems = 0;
}

YListBox::~YListBox() {
    fFirst = fLast = 0;
    freeItems();
}

bool YListBox::isFocusTraversable() {
    return true;
}

int YListBox::addAfter(YListItem *prev, YListItem *item) {
    PRECONDITION(item->getPrev() == 0);
    PRECONDITION(item->getNext() == 0);

    freeItems();

    item->setNext(prev->getNext());
    if (item->getNext())
        item->getNext()->setPrev(item);
    else
        fLast = item;

    item->setPrev(prev);
    prev->setNext(item);
    fItemCount++;
    repaint();
    return 1;
}

int YListBox::addItem(YListItem *item) {
    PRECONDITION(item->getPrev() == 0);
    PRECONDITION(item->getNext() == 0);

    freeItems();

    item->setNext(0);
    item->setPrev(fLast);
    if (fLast)
        fLast->setNext(item);
    else
        fFirst = item;
    fLast = item;
    fItemCount++;
    repaint();
    return 1;
}

void YListBox::removeItem(YListItem *item) {
    freeItems();

    if (item->getPrev())
        item->getPrev()->setNext(item->getNext());
    else
        fFirst = item->getNext();

    if (item->getNext())
        item->getNext()->setPrev(item->getPrev());
    else
        fLast = item->getPrev();
    item->setPrev(0);
    item->setNext(0);
    fItemCount--;
    repaint();
}

void YListBox::freeItems() {
    if (fItems) {
        delete fItems; fItems = 0;
    }
}

void YListBox::updateItems() {
    if (fItems == 0) {
        fMaxWidth = 0;
        fItems = new YListItem *[fItemCount];
        if (fItems) {
            YListItem *a = getFirst();
            int n = 0;
            while (a) {
                fItems[n++] = a;

                int cw = 3 + 20 + a->getOffset();
                if (gListBoxFont.getFont()) {
                    const CStr *t = a->getText();
                    if (t)
                        cw += gListBoxFont.getFont()->textWidth(t) + 3;
                }
                if (cw > fMaxWidth)
                    fMaxWidth = cw;

                a = a->getNext();
            }
        }
    }
}

int YListBox::maxWidth() {
    updateItems();
    return fMaxWidth;
}

int YListBox::findItemByPoint(int /*pX*/, int pY) {
    int no = (pY + fOffsetY) / getLineHeight();

    if (no >= 0 && no < getItemCount())
        return no;
    return -1;
}

int YListBox::getItemCount() {
    return fItemCount;
}

int YListBox::findItem(YListItem *item) {
    YListItem *a = fFirst;
    int n;

    for (n = 0; a; a = a->getNext(), n++) {
        if (item == a)
            return n;
    }
    return -1;
}

YListItem *YListBox::getItem(int no) {
    if (no < 0 || no >= getItemCount())
        return 0;
    updateItems();

    if (fItems) {
        return fItems[no];
    } else {
        YListItem *a = getFirst();
        for (int n = 0; a; a = a->getNext(), n++) {
            if (n == no)
                return a;
        }
    }
    return 0;
}

int YListBox::getLineHeight() {
    int lh, fh = gListBoxFont.getFont()->height();
    if (ICON_SMALL > fh)
        lh = ICON_SMALL;
    else
        lh = fh;
    lh += 2;
    return lh;
}

#warning "fix horizontal visibility?"
void YListBox::ensureVisibility(int item) {
    if (item >= 0) {
        int oy = fOffsetY;
        int lh = getLineHeight();
        int fy = item * lh;

        if (fy + lh >= fOffsetY + (int)height())
            oy = fy + lh - height();
        if (fy <= fOffsetY)
            oy = fy;
        if (oy != fOffsetY) {
            fOffsetY = oy;
#warning "use scroll instead of repaint"
            repaint();
        }
    }
}

void YListBox::focusVisible() {
    int ty = fOffsetY;
    int by = ty + height();

    if (fFocusedItem >= 0) {
        int lh = getLineHeight();

        while (ty > fFocusedItem * lh)
            fFocusedItem++;
        while (by - lh < fFocusedItem * lh)
            fFocusedItem--;
    }
}

void YListBox::configure(const YRect &cr) {
    YWindow::configure(cr);
    //if (fFocusedItem != -1)
    //    paintItem(fFocusedItem);
    resetScrollBars();
}

bool YListBox::eventKey(const YKeyEvent &key) {
    if (key.type() == YEvent::etKeyPress) {

        bool clear = !key.isCtrl();
        bool extend = key.isShift();

        //int SelPos, OldPos = fFocusedItem, count = getItemCount();

        //if (m & ShiftMask) {
        //    SelPos = fFocusedItem;
        //} else {
        //    SelPos = -1;
        //}

        switch (key.getKey()) {
        case XK_Return:
        case XK_KP_Enter:
            {
                YListItem *i = getItem(fFocusedItem);
                if (i)
                    activateItem(i);
            }
            break;
        case ' ':
            if (fFocusedItem != -1) {
                selectItem(fFocusedItem,
                           isItemSelected(fFocusedItem) ? false : true);
            }
            break;
        case XK_Home:
            setFocusedItem(0, clear, extend, false);
            break;
        case XK_End:
            if (getItemCount() > 0)
                setFocusedItem(getItemCount() - 1, clear, extend, false);
            break;
        case XK_Up:
            if (fFocusedItem > 0)
                setFocusedItem(fFocusedItem - 1, clear, extend, false);
            break;
        case XK_Down:
            if (fFocusedItem < getItemCount() - 1)
                setFocusedItem(fFocusedItem + 1, clear, extend, false);
            break;
#if 0
        case XK_Prior:
            fVerticalScroll->setValue(fVerticalScroll->getValue() -
                                      fVerticalScroll->getBlockIncrement());
            fOffsetY = fVerticalScroll->getValue();
            fFocusedItem -= height() / getLineHeight();
            if (fFocusedItem < 0) {
                if (count > 0)
                    fFocusedItem = 0;
                else
                    fFocusedItem = -1;
            }
            repaint();
            break;
        case XK_Next:
            fVerticalScroll->setValue(fVerticalScroll->getValue() +
                                      fVerticalScroll->getBlockIncrement());
            fOffsetY = fVerticalScroll->getValue();
            fFocusedItem += height() / getLineHeight();
            if (fFocusedItem > count - 1)
                fFocusedItem = count - 1;
            repaint();
            break;
#endif
        case 'a':
        case '/':
        case '\\':
            if (key.isCtrl()) {
                for (int i = 0; i < getItemCount(); i++)
                    selectItem(i, (key.getKey() == '\\') ? false : true);
                break;
            }
        default:
            if (key.getKey() < 256) {
                unsigned char c = TOUPPER(key.getKey());
                int count = getItemCount();
                int i = fFocusedItem;
                YListItem *it = 0;
                const CStr *title;

                for (int n = 0; n < count; n++) {
                    i = (i + 1) % count;
                    it = getItem(i);
                    title = it->getText();
                    if (title && title->c_str() && TOUPPER(title->c_str()[0]) == c) {
                        setFocusedItem(i, clear, extend, false);
                        break;
                    }
                }
            } else {
                if (fVerticalScroll->handleScrollKeys(key) == false) {
                    //&& fHorizontalScroll->handleScrollKeys(key) == false
                    return YWindow::eventKey(key);
                }
            }
        }
#if 0
        if (fFocusedItem != OldPos) {
            if (SelPos == -1) {
                fSelectStart = fFocusedItem;
                fSelectEnd = fFocusedItem;
                fDragging = true;
                fSelect = true;
            } else {
                if (fSelectStart == OldPos)
                    fSelectStart = fFocusedItem;
                else if (fSelectEnd == OldPos)
                    fSelectEnd = fFocusedItem;
                else
                    fSelectStart = fSelectEnd = fFocusedItem;
                if (fSelectStart == -1)
                    fSelectStart = fSelectEnd;
                if (fSelectEnd == -1)
                    fSelectEnd = fSelectStart;

                fDragging = true;
                fSelect = true;
            }
            setSelection();
            ensureVisibility(fFocusedItem);
            repaint();
        }
#endif
        return true;
    }
    return YWindow::eventKey(key);
}

bool YListBox::eventButton(const YButtonEvent &button) {
    if (button.getButton() == 1) {
        int no = findItemByPoint(button.x(), button.y());

        if (button.type() == YEvent::etButtonPress) {
            bool clear = (button.isCtrl()) ? false : true;
            bool extend = (button.isShift()) ? true : false;

            if (no != -1) {
                fSelect = (!clear && isItemSelected(no)) ? false : true;
                setFocusedItem(no, clear, extend, true);
            } else {
                fSelect = true;
                setFocusedItem(no, clear, extend, true);
            }
        } else if (button.type() == YEvent::etButtonRelease) {
            if (no != -1)
                setFocusedItem(no, false, true, true);
            fDragging = false;
            applySelection();
            autoScroll(0, 0);
        }
    }
    if (fVerticalScroll->handleScrollMouse(button) == true)
        return true;

    return YWindow::eventButton(button);
}

bool YListBox::eventClick(const YClickEvent &up) {
    if (up.getButton() == 1 && up.isSingleClick()) {
        int no = findItemByPoint(up.x(), up.y());

        if (no != -1) {
            YListItem *i = getItem(no);
            activateItem(i);
        }
        return true;
    }
    return YWindow::eventClick(up);
}

bool YListBox::eventMotion(const YMotionEvent &motion) {
    if (motion.hasLeftButton()) {
        if (motion.y() < 0)
            autoScroll(-fVerticalScroll->getUnitIncrement(), &motion);
        else if (motion.y() >= int(height()))
            autoScroll(fVerticalScroll->getUnitIncrement(), &motion);
        else {
            autoScroll(0, &motion);
            int no = findItemByPoint(motion.x(), motion.y());

            if (no != -1)
                setFocusedItem(no, false, true, true);
        }
    }
    return YWindow::eventMotion(motion);
}

bool YListBox::eventDrag(const YButtonEvent &down, const YMotionEvent &motion) {
    if (down.getButton() == 2) {
        int dx = motion.y() - down.y();

        fVerticalScroll->setValue(fVerticalScroll->getValue() - dx);
        int fy = fVerticalScroll->getValue();
        if (fy != fOffsetY) {
            fOffsetY = fy;
            repaint();
        }
        return true;
    }
    return YWindow::eventDrag(down, motion);
}

void YListBox::scroll(YScrollBar *scroll, int delta) {
    bool needRepaint = false;

    if (scroll == fVerticalScroll) {
        fOffsetY += delta;
        focusVisible();
        needRepaint = true;
    }
    if (scroll == fHorizontalScroll) {
        fOffsetX += delta;
        focusVisible();
        needRepaint = true;
    }
    if (needRepaint)
        repaint();
}

void YListBox::move(YScrollBar *scroll, int pos) {
    bool needRepaint = false;
    if (scroll == fVerticalScroll) {
        fOffsetY = pos;
        focusVisible();
        needRepaint = true;
    }
    if (scroll == fHorizontalScroll) {
        fOffsetX = pos;
        focusVisible();
        needRepaint = true;
    }
    if (needRepaint)
        repaint();
}

void YListBox::paintItem(Graphics &g, int n) {
    YListItem *a = getItem(n);
    int x = 3;
    int lh = getLineHeight();
    int fh = gListBoxFont.getFont()->height();
    int xpos = 0;
    int y = n * lh;
    int yPos = y + lh - (lh - fh) / 2 - gListBoxFont.getFont()->descent();

    if (a == 0)
        return;

    xpos += a->getOffset();

    bool s = a->getSelected();

    if (fDragging) {
        int beg = (fSelectStart < fSelectEnd) ? fSelectStart : fSelectEnd;
        int end = (fSelectStart < fSelectEnd) ? fSelectEnd : fSelectStart;
        if (n >= beg && n <= end) {
            if (fSelect)
                s = true;
            else
                s = false;
        }
    }

    if (s)
        g.setColor(gListBoxSelBg);
    else
        g.setColor(gListBoxBg);

    YPixmap *bg = gListBoxPixmapBg.getPixmap();
    if (bg && !s)
        g.fillPixmap(bg, 0, y - fOffsetY, width(), lh);
    else
        g.fillRect(0, y - fOffsetY, width(), lh);
    if (fFocusedItem == n) {
        g.setColor(YColor::black);
        g.setPenStyle(Graphics::psDotted);
        int cw = 3 + 20 + a->getOffset();
        if (gListBoxFont.getFont()) {
            const CStr *t = a->getText();
            if (t)
                cw += gListBoxFont.getFont()->textWidth(t) + 3;
        }
        g.drawRect(0 - fOffsetX, y - fOffsetY, cw - 1, lh - 1);
        g.setPenStyle();
    }
    YIcon *icon = a->getIcon();
    if (icon && icon->small())
        g.drawPixmap(icon->small(), xpos + x - fOffsetX, y - fOffsetY + 1);
    if (s)
        g.setColor(gListBoxSelFg);
    else
        g.setColor(gListBoxFg);
    g.setFont(gListBoxFont.getFont());
    const CStr *title = a->getText();
    if (title && title->c_str()) {
        g.drawChars(title->c_str(), 0, title->length(),
                    xpos + x + 20 - fOffsetX, yPos - fOffsetY);
    }
}

void YListBox::paint(Graphics &g, const YRect &er) {
    int n = 0, min, max;
    int lh = getLineHeight();

    min = (fOffsetY + er.y()) / lh;
    max = (fOffsetY + er.y() + er.height()) / lh;

    for (n = min; n <= max; n++)
        paintItem(g, n);
    resetScrollBars();


    unsigned int y = contentHeight();

    if (y < height()) {
        g.setColor(gListBoxBg);

        YPixmap *bg = gListBoxPixmapBg.getPixmap();
        if (bg)
            g.fillPixmap(bg, 0, y, width(), height() - y);
        else
            g.fillRect(0, y, width(), height() - y);
    }
}

void YListBox::paintItem(int i) {
    if (i >= 0 && i < getItemCount())
        paintItem(beginPaint(), i);
}

void YListBox::activateItem(YListItem */*item*/) {
}

void YListBox::repaintItem(YListItem *item) {
    int i = findItem(item);
    if (i != -1)
        paintItem(i);
}

bool YListBox::hasSelection() {
    YListItem *a = getFirst();
    while (a) {
        if (isSelected(a))
            return true;
        a = a->getNext();
    }
    return false;
}

void YListBox::resetScrollBars() {
    int lh = getLineHeight();
    int y =  getItemCount() * lh;
    fVerticalScroll->setValues(fOffsetY, height(), 0, y);
    fVerticalScroll->setBlockIncrement(height());
    fVerticalScroll->setUnitIncrement(lh);
    fHorizontalScroll->setValues(fOffsetX, width(), 0, maxWidth());
    fHorizontalScroll->setBlockIncrement(width());
    fHorizontalScroll->setUnitIncrement(8);
    if (fView)
        fView->layout();
}

bool YListBox::handleAutoScroll(const YMotionEvent & /*mouse*/) {
    fVerticalScroll->scroll(fAutoScrollDelta);

    if (fAutoScrollDelta != 0) {
        int no = -1;
        if (fAutoScrollDelta < 0)
            no = findItemByPoint(0, 0);
        else
            no = findItemByPoint(0, height() - 1);

        if (no != -1)
            setFocusedItem(no, false, true, true);
    }
    return true;
}

void YListBox::autoScroll(int delta, const YMotionEvent *motion) {
    fAutoScrollDelta = delta;
    beginAutoScroll(delta ? true : false, motion);
}

bool YListBox::isItemSelected(int item) {
    YListItem *i = getItem(item);
    if (i)
        return i->getSelected();
    return false;
}

void YListBox::selectItem(int item, bool select) {
    YListItem *i = getItem(item);
    if (i && i->getSelected() != select) {
        i->setSelected(select);
        //printf("%d=%d\n", item, select);
        paintItem(beginPaint(), item);
    }
}

void YListBox::clearSelection() {
    for (int i = 0; i < getItemCount(); i++)
        selectItem(i, false);
    fSelectStart = fSelectEnd = -1;
}

void YListBox::applySelection() {
    PRECONDITION(fDragging == false);
    if (fSelectStart != -1) {
        PRECONDITION(fSelectEnd != -1);

        int beg = (fSelectStart < fSelectEnd) ? fSelectStart : fSelectEnd;
        int end = (fSelectStart < fSelectEnd) ? fSelectEnd : fSelectStart;

        for (int n = beg; n <= end; n++) {
            YListItem *i = getItem(n);
            if (i)
                i->setSelected(fSelect);
        }
    }
    fSelectStart = fSelectEnd = -1;
}

void YListBox::paintItems(int selStart, int selEnd) {
    //printf("paint: %d %d\n", selStart, selEnd);
    if (selStart == -1) // !!! check
        return;
    //PRECONDITION(selStart != -1);
    PRECONDITION(selEnd != -1);

    int beg = (selStart < selEnd) ? selStart : selEnd;
    int end = (selStart < selEnd) ? selEnd : selStart;

    Graphics &g = beginPaint();

    for (int i = beg; i <= end; i++)
        paintItem(g, i);
}

void YListBox::selectItems(int selStart, int selEnd, bool sel) {
    PRECONDITION(selStart != -1);
    PRECONDITION(selEnd != -1);

    int beg = (selStart < selEnd) ? selStart : selEnd;
    int end = (selStart < selEnd) ? selEnd : selStart;

    for (int i = beg; i <= end; i++)
        selectItem(i, sel);
}

void YListBox::setFocusedItem(int item, bool clear, bool extend, bool virt) {
    int oldItem = fFocusedItem;

    //printf("%d (%d-%d=%d): clear:%d extend:%d virt:%d\n",
    //       item, fSelectStart, fSelectEnd, fSelect, clear, extend, virt);

    if (virt)
        fDragging = true;
    else {
        fDragging = false;
    }
    bool sel = true;
    if (oldItem != -1 && extend && !clear)
        sel = isItemSelected(oldItem);
    if (clear)
        clearSelection();
    if (item != -1) {
        int oldEnd;
        if (fSelectEnd == -1)
            oldEnd = item;
        else
            oldEnd = fSelectEnd;
        if (extend && fSelectStart == -1)
            fSelectStart = oldEnd = oldItem;
        if (!extend || fSelectStart == -1) {
            //int oldBeg = fSelectStart;
            fSelectStart = item;
            //paintItems(oldBeg, item);
        }
        fSelectEnd = item;
        if (!virt)
            fSelect = sel;
        if (clear || extend) {
            if (virt)
                paintItems(oldEnd, fSelectEnd);
            else if (extend)
                selectItems(fSelectStart, fSelectEnd, sel);
            else
                selectItem(item, sel);
        }
    }
    if (item != fFocusedItem) {
        fFocusedItem = item;

        Graphics &g = beginPaint();

        if (oldItem != -1)
            paintItem(g, oldItem);
        if (fFocusedItem != -1)
            paintItem(g, fFocusedItem);

        ensureVisibility(fFocusedItem);
    }
}

bool YListBox::isSelected(int item) {
    YListItem *a = getItem(item);

    if (a == 0)
        return false;
    return isSelected(a);
}

#warning "remove isSelected(YListItem *)"
bool YListBox::isSelected(YListItem *item) {
    bool s = item->getSelected();
    int n = findItem(item);

    if (n == -1)
        return false;

    if (fDragging) {
        int beg = (fSelectStart < fSelectEnd) ? fSelectStart : fSelectEnd;
        int end = (fSelectStart < fSelectEnd) ? fSelectEnd : fSelectStart;
        if (n >= beg && n <= end) {
            if (fSelect)
                s = true;
            else
                s = false;
        }
    }
    return s;
}

int YListBox::contentWidth() {
    return maxWidth();
}

int YListBox::contentHeight() {
    return getItemCount() * getLineHeight();
}

YWindow *YListBox::getWindow() {
    return this;
}
