/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Window list
 */
#include "config.h"

#include "ykey.h"
#include "ylistbox.h"
#include "yrect.h"
#include "yicon.h"
#include "wpixmaps.h"
#include "yscrollview.h"

#include "yxapp.h"
#include "prefs.h"
#include "ascii.h"

#include <string.h>

static ref<YFont> listBoxFont;
static YColorName listBoxBg(&clrListBox);
static YColorName listBoxFg(&clrListBoxText);
static YColorName listBoxSelBg(&clrListBoxSelected);
static YColorName listBoxSelFg(&clrListBoxSelectedText);

int YListBox::fAutoScrollDelta = 0;

YListItem::YListItem() {
    fPrevItem = fNextItem = 0;
    fSelected = false;
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

ustring YListItem::getText() {
    return null;
}

ref<YIcon> YListItem::getIcon() {
    return null;
}

int YListItem::getOffset() {
    return 0;
}

YListBox::YListBox(YScrollView *view, YWindow *aParent):
    YWindow(aParent),
    fGradient(null)
{
    if (listBoxFont == null)
        listBoxFont = YFont::getFont(XFA(listBoxFontName));
    setBitGravity(NorthWestGravity);
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
    fGradient = null;
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
        delete[] fItems; fItems = 0;
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
                if (listBoxFont != null) {
                    ustring t = a->getText();
                    if (t != null)
                        cw += listBoxFont->textWidth(t) + 3;
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

    for (n = 0; a; a = a->getNext(), n++)
        if (item == a)
            return n;
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
        for (int n = 0; a; a = a->getNext(), n++)
            if (n == no)
                return a;
    }
    return 0;
}

int YListBox::getLineHeight() {
    return max((int) YIcon::smallSize(), (int) listBoxFont->height()) + 2;
}

void YListBox::ensureVisibility(int item) { //!!! horiz too
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
            repaint();///!!!fix (use scroll)
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

void YListBox::configure(const YRect &r) {
    YWindow::configure(r);

    resetScrollBars();

    if (listbackPixbuf != null
        && !(fGradient != null &&
             fGradient->width() == r.width() &&
             fGradient->height() == r.height()))
    {
        fGradient = listbackPixbuf->scale(r.width(), r.height());
        repaint();
    }
}

bool YListBox::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);

        bool clear = (m & ControlMask) ? false : true;
        bool extend = (m & ShiftMask) ? true : false;

        //int SelPos, OldPos = fFocusedItem, count = getItemCount();

        //if (m & ShiftMask) {
        //    SelPos = fFocusedItem;
        //} else {
        //    SelPos = -1;
        //}

        switch (k) {
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
        case XK_Up: {
            int const oldFocus(fFocusedItem);

            focusVisible();
            if (fFocusedItem > 0)
                setFocusedItem(oldFocus == fFocusedItem ? fFocusedItem - 1
                               : fFocusedItem,
                               clear, extend, false);

            break;
        }
        case XK_Down: {
            int const oldFocus(fFocusedItem);

            focusVisible();
            if (fFocusedItem < getItemCount() - 1)
                setFocusedItem(oldFocus == fFocusedItem ? fFocusedItem + 1
                               : fFocusedItem,
                               clear, extend, false);
            break;
        }
#if 0
        case XK_Prior:
            fVerticalScroll->setValue(fVerticalScroll->getValue() -
                                      fVerticalScroll->getBlockIncrement());
            fOffsetY = fVerticalScroll->getValue();
            fFocusedItem -= height() / getLineHeight();
            if (fFocusedItem < 0)
                if (count > 0)
                    fFocusedItem = 0;
                else
                    fFocusedItem = -1;
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
            if (m & ControlMask) {
                for (int i = 0; i < getItemCount(); i++)
                    selectItem(i, (k == '\\') ? false : true);
                break;
            }
            break;
        default:
            if (k < 256) {
                unsigned char c = ASCII::toUpper((char)k);
                int count = getItemCount();
                int i = fFocusedItem;
                YListItem *it = 0;

                for (int n = 0; n < count; n++) {
                    i = (i + 1) % count;
                    it = getItem(i);
                    ustring title = it->getText();
                    if (title != null && title.length() > 0 && ASCII::toUpper(title.charAt(0)) == c) {
                        setFocusedItem(i, clear, extend, false);
                        break;
                    }
                }
            } else {
                if (fVerticalScroll->handleScrollKeys(key) == false
                    //&& fHorizontalScroll->handleScrollKeys(key) == false
                   )
                    return YWindow::handleKey(key);
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
    return YWindow::handleKey(key);
}

void YListBox::handleButton(const XButtonEvent &button) {
    if (button.button == 1) {
        int no = findItemByPoint(button.x, button.y);

        if (button.type == ButtonPress) {
            bool clear = (button.state & ControlMask) ? false : true;
            bool extend = (button.state & ShiftMask) ? true : false;

            if (no != -1) {
                fSelect = (!clear && isItemSelected(no)) ? false : true;
                setFocusedItem(no, clear, extend, true);
            } else {
                fSelect = true;
                setFocusedItem(no, clear, extend, true);
            }
        } else if (button.type == ButtonRelease) {
            if (no != -1)
                setFocusedItem(no, false, true, true);
            fDragging = false;
            applySelection();
            autoScroll(0, 0);
        }
    }
    if (fVerticalScroll->handleScrollMouse(button) == false)
        YWindow::handleButton(button);
}

void YListBox::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1 && (count % 2) == 0) {
        int no = findItemByPoint(up.x, up.y);

        if (no != -1) {
            YListItem *i = getItem(no);
            activateItem(i);
        }
    }
}

void YListBox::handleMotion(const XMotionEvent &motion) {
    if (motion.state & Button1Mask) {
        if (motion.y < 0)
            autoScroll(-fVerticalScroll->getUnitIncrement(), &motion);
        else if (motion.y >= int(height()))
            autoScroll(fVerticalScroll->getUnitIncrement(), &motion);
        else {
            autoScroll(0, &motion);
            int no = findItemByPoint(motion.x, motion.y);

            if (no != -1)
                setFocusedItem(no, false, true, true);
        }
    }
    YWindow::handleMotion(motion);
}

void YListBox::handleDrag(const XButtonEvent &down, const XMotionEvent &motion) {
    if (down.button == 2) {
        int dx = motion.y - down.y;

        fVerticalScroll->setValue(fVerticalScroll->getValue() - dx);
        int fy = fVerticalScroll->getValue();
        if (fy != fOffsetY) {
            fOffsetY = fy;
            repaint();
        }
    }
}

void YListBox::scroll(YScrollBar *scroll, int delta) {
    bool needRepaint = false;

    if (scroll == fVerticalScroll) {
        fOffsetY += delta;
        needRepaint = true;
    }
    if (scroll == fHorizontalScroll) {
        fOffsetX += delta;
        needRepaint = true;
    }
    if (needRepaint)
        repaint();
}

void YListBox::move(YScrollBar *scroll, int pos) {
    bool needRepaint = false;
    if (scroll == fVerticalScroll) {
        fOffsetY = pos;
        needRepaint = true;
    }
    if (scroll == fHorizontalScroll) {
        fOffsetX = pos;
        needRepaint = true;
    }
    if (needRepaint)
        repaint();
}

void YListBox::paintItem(Graphics &g, int n) {
    YListItem *a = getItem(n);
    int x = 3;
    int lh = getLineHeight();
    int fh = listBoxFont->height();
    int xpos = 0;
    int y = n * lh;
    int yPos = y + lh - (lh - fh) / 2 - listBoxFont->descent();

    if (a == 0)
        return ;

    xpos += a->getOffset();

    bool s(a->getSelected());

    if (fDragging) {
        int const beg(fSelectStart < fSelectEnd ? fSelectStart : fSelectEnd);
        int const end(fSelectStart < fSelectEnd ? fSelectEnd : fSelectStart);

        if (n >= beg && n <= end)
            s = fSelect;
    }

    if (s) {
        g.setColor(listBoxSelBg);
        g.fillRect(0, y - fOffsetY, width(), lh);
    } else {
        if (fGradient != null)
            g.drawImage(fGradient, 0, y - fOffsetY, width(), lh,
                         0, y - fOffsetY);
        else if (listbackPixmap != null)
            g.fillPixmap(listbackPixmap, 0, y - fOffsetY, width(), lh);
        else {
            g.setColor(listBoxBg);
            g.fillRect(0, y - fOffsetY, width(), lh);
        }
    }

    if (fFocusedItem == n) {
        g.setColor(YColor::black);
        g.setPenStyle(true);
        int cw = 3 + 20 + a->getOffset();
        if (listBoxFont != null) {
            ustring t = a->getText();
            if (t != null)
                cw += listBoxFont->textWidth(t) + 3;
        }
        g.drawRect(0 - fOffsetX, y - fOffsetY, cw - 1, lh - 1);
        g.setPenStyle(false);
    }

    ref<YIcon> icon = a->getIcon();

    if (icon != null)
        icon->draw(g, xpos + x - fOffsetX, y - fOffsetY + 1, YIcon::smallSize());

    ustring title = a->getText();

    if (title != null) {
        g.setColor(s ? listBoxSelFg : listBoxFg);
        g.setFont(listBoxFont);

        g.drawChars(title,
                    xpos + x + 20 - fOffsetX, yPos - fOffsetY);
    }
}

void YListBox::paint(Graphics &g, const YRect &r) {
    int ry = r.y(), rheight = r.height();
    int const lh(getLineHeight());
    int const min((fOffsetY + ry) / lh);
    int const max((fOffsetY + ry + rheight) / lh);

    for (int n(min); n <= max; n++) paintItem(g, n);
    resetScrollBars();

    unsigned const y(contentHeight());

    if (y < height()) {
        if (fGradient != null)
            g.drawImage(fGradient, 0, y, width(), height() - y, 0, y);
        else if (listbackPixmap != null)
            g.fillPixmap(listbackPixmap, 0, y, width(), height() - y);
        else {
            g.setColor(listBoxBg);
            g.fillRect(0, y, width(), height() - y);
        }
    }
}

void YListBox::paintItem(int i) {
/// TODO #warning "fix this to use an invalidate region"
    if (i >= 0 && i < getItemCount()) {
        paintExpose(0, i * getLineHeight() - fOffsetY,
                    width(), getLineHeight());
    }
}

void YListBox::activateItem(YListItem */*item*/) {
}

void YListBox::repaintItem(YListItem *item) {
    int i = findItem(item);
    if (i != -1)
        paintItem(i);
}

bool YListBox::hasSelection() {//!!!fix
    //if (fSelectStart != -1 && fSelectEnd != -1)
        return true;
    //return false;
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

bool YListBox::handleAutoScroll(const XMotionEvent & /*mouse*/) {
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

void YListBox::autoScroll(int delta, const XMotionEvent *motion) {
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
        //msg("%d=%d", item, select);
        paintItem(item);
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

        for (int n = beg; n <= end ; n++) {
            YListItem *i = getItem(n);
            if (i)
                i->setSelected(fSelect);
        }
    }
    fSelectStart = fSelectEnd = -1;
}

void YListBox::paintItems(int selStart, int selEnd) {
    //msg("paint: %d %d", selStart, selEnd);
    if (selStart == -1) // !!! check
        return;
    //PRECONDITION(selStart != -1);
    PRECONDITION(selEnd != -1);

    int beg = (selStart < selEnd) ? selStart : selEnd;
    int end = (selStart < selEnd) ? selEnd : selStart;

    for (int i = beg; i <= end; i++)
        paintItem(i);
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

    //msg("%d (%d-%d=%d): clear:%d extend:%d virt:%d",
    //       item, fSelectStart, fSelectEnd, fSelect, clear, extend, virt);

    if (virt)
        fDragging = true;
    else {
        fDragging = false;
    }
    bool sel = true;
    if (oldItem != -1 && extend && !clear)
        sel = isItemSelected(oldItem);
    if (clear && !extend)
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

        if (oldItem != -1)
            paintItem(oldItem);
        if (fFocusedItem != -1)
            paintItem(fFocusedItem);

        ensureVisibility(fFocusedItem);
    }
}

bool YListBox::isSelected(int item) {
    YListItem *a = getItem(item);

    if (a == 0)
        return false;
    return isSelected(a);
}

bool YListBox::isSelected(YListItem *item) { // !!! remove this !!!
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

unsigned YListBox::contentWidth() {
    return maxWidth();
}

unsigned YListBox::contentHeight() {
    return getItemCount() * getLineHeight();
}

YWindow *YListBox::getWindow() {
    return this;
}

// vim: set sw=4 ts=4 et:
