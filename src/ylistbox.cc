/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 *
 * Window list
 */
#include "config.h"

#ifndef LITE

#include "ypixbuf.h"
#include "ykey.h"
#include "ylistbox.h"

#include "yapp.h"
#include "prefs.h"

#include <string.h>

static YFont *listBoxFont(NULL);

static YColor *listBoxBg(NULL);
static YColor *listBoxFg(NULL);
static YColor *listBoxSelBg(NULL);
static YColor *listBoxSelFg(NULL);

int YListBox::fAutoScrollDelta(0);

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

const char *YListItem::getText() {
    return 0;
}

YIcon *YListItem::getIcon() {
    return 0;
}

int YListItem::getOffset() {
    return 0;
}

YListBox::YListBox(YScrollView *view, YWindow *aParent, bool drawIcons): 
    YWindow(aParent), fDrawIcons(drawIcons)
    INIT_GRADIENT(fGradient, NULL) {
    if (listBoxFont == 0)
        listBoxFont = YFont::getFont(listBoxFontName);
    if (listBoxBg == 0)
        listBoxBg = new YColor(clrListBox);
    if (listBoxFg == 0)
        listBoxFg = new YColor(clrListBoxText);
    if (listBoxSelBg == 0)
        listBoxSelBg = new YColor(clrListBoxSelected);
    if (listBoxSelFg == 0)
        listBoxSelFg = new YColor(clrListBoxSelectedText);
    setBitGravity(NorthWestGravity);
    fView = view;
    if (fView) {
        fVerticalScroll = view->getVerticalScrollBar();;
        fHorizontalScroll = view->getHorizontalScrollBar();
    } else {
        fHorizontalScroll = 0;
        fVerticalScroll = 0;
    }
    if (fVerticalScroll)
        fVerticalScroll->scrollBarListener(this);
    if (fHorizontalScroll)
        fHorizontalScroll->scrollBarListener(this);

    fOffsetX = 0;
    fOffsetY = 0;
    fFirst = fLast = 0;
    fFocusedItem = 0;
    fSelectStart = fSelectEnd = -1;
    fDragging = false;
    fSelect = false;
    fItemCount = 0;
    fItems = NULL;
}

YListBox::~YListBox() {
    fFirst = fLast = 0;
    freeItems();
#ifdef CONFIG_GRADIENTS
    delete fGradient;
#endif
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
    if (fItems) { delete[] fItems; fItems = NULL; }
}

void YListBox::updateItems() {
    if (NULL == fItems) {
        fMaxWidth = 0;
        fItems = new YListItem *[fItemCount];

        if (fItems) {
            int n = 0;
            for (YListItem *a(getFirst()); NULL != a; a = a->getNext()) {
                fItems[n++] = a;

                int cw((fDrawIcons ? 3 + 20 : 3) + a->getOffset());

                if (listBoxFont) {
                    const char *t(a->getText());
                    if (t) cw += listBoxFont->textWidth(t) + 3;
                }

                fMaxWidth = max(fMaxWidth, cw);
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
    if (no < 0 || no >= getItemCount()) return NULL;
    updateItems();

    if (fItems) {
        return fItems[no];
    } else {
        int n(0); for (YListItem *a(getFirst()); a; a = a->getNext(), n++)
            if (n == no) return a;
    }
    return 0;
}

int YListBox::getLineHeight() {
    return max(fDrawIcons ? (int) YIcon::sizeSmall : 0,
                            (int) listBoxFont->height()) + 2;
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

void YListBox::configure(const int x, const int y, 
			 const unsigned width, const unsigned height, 
			 const bool resized) {
    YWindow::configure(x, y, width, height, resized);
    //if (fFocusedItem != -1)
    //    paintItem(fFocusedItem);
    if (resized) {
        resetScrollBars();

#ifdef CONFIG_GRADIENTS
	if (listbackPixbuf && !(fGradient &&
				 fGradient->width() == width &&
				 fGradient->height() == height)) {
	    delete fGradient;
	    fGradient = new YPixbuf(*listbackPixbuf, width, height);
	    repaint();
	}
#endif
    }
}

bool YListBox::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym const k(XKeycodeToKeysym(app->display(), key.keycode, 0));
        int const m(KEY_MODMASK(key.state));

        bool const clear(!(m & ControlMask));
        bool const extend(m & ShiftMask);

        //int SelPos, OldPos = fFocusedItem, count = getItemCount();

        //if (m & ShiftMask) {
        //    SelPos = fFocusedItem;
        //} else {
        //    SelPos = -1;
        //}

        switch (k) {
            case XK_Return:
            case XK_KP_Enter: {
                YListItem *i(getItem(fFocusedItem));
                if (i) activateItem(i);
                break;
            }
            case ' ':
                if (fFocusedItem != -1)
                    selectItem(fFocusedItem, !isItemSelected(fFocusedItem));
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
                if (fFocusedItem < 0) fFocusedItem = (count > 0 ? 0 : -1);

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
                        selectItem(i, k != '\\');
                    break;
                }
                /* nobreak */

            default:
                if (k < 256) {
                    unsigned char const c(TOUPPER(k));
                    int const count(getItemCount());
                    int i(fFocusedItem);

                    for (int n = 0; n < count; n++) {
                        YListItem *it(getItem((++i)%= count));
                        char const *title(it->getText());
                        if (title && c == TOUPPER(*title)) {
                            setFocusedItem(i, clear, extend, false);
                            break;
                        }
                    }
                } else if (!fVerticalScroll->handleScrollKeys(key) /* &&
                           !fHorizontalScroll->handleScrollKeys(key) */)
                    return YWindow::handleKey(key);
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
            bool const clear(!(button.state & ControlMask));
            bool const extend(button.state & ShiftMask);

            if (no != -1) {
                fSelect = (clear || !isItemSelected(no));
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
#ifdef CONFIG_GRADIENTS
        if (fGradient)
	    g.copyPixbuf(*fGradient, 0, y - fOffsetY, width(), lh,
	    			     0, y - fOffsetY);
        else 
#endif	
	if (listbackPixmap)
	    g.fillPixmap(listbackPixmap, 0, y - fOffsetY, width(), lh);
	else {
	    g.setColor(listBoxBg);
	    g.fillRect(0, y - fOffsetY, width(), lh);
	}
    }

    if (fFocusedItem == n) {
        g.setColor(YColor::black);
        g.setPenStyle(true);

        int cw((fDrawIcons ? 3 + 20 : 3) + a->getOffset());

        if (listBoxFont) {
            const char *t(a->getText());
            if (t) cw += listBoxFont->textWidth(t) + 3;
        }
        g.drawRect(0 - fOffsetX, y - fOffsetY, cw - 1, lh - 1);
        g.setPenStyle(false);
    }

    if (fDrawIcons) {
        YIcon *icon = a->getIcon();
        if (icon && icon->small())
            g.drawImage(icon->small(), xpos + x - fOffsetX, y - fOffsetY + 1);
    }

    const char *title(a->getText());

    if (title) {
	g.setColor(s ? listBoxSelFg : listBoxFg);
	g.setFont(listBoxFont);

        g.drawChars(title, 0, strlen(title),
                    xpos + x + (fDrawIcons ? 20 : 0) - fOffsetX,
                    yPos - fOffsetY);
    }
}

void YListBox::paint(Graphics &g, int /*x*/, int ry, unsigned int /*width*/, unsigned int rheight) {
    int const lh(getLineHeight());
    int const min((fOffsetY + ry) / lh);
    int const max((fOffsetY + ry + rheight) / lh);

    for (int n(min); n <= max; n++) paintItem(g, n);
    resetScrollBars();

    unsigned const y(contentHeight());

    if (y < height()) {
#ifdef CONFIG_GRADIENTS
        if (fGradient)
            g.copyPixbuf(*fGradient, 0, y, width(), height() - y, 0, y);
        else 
#endif	
	if (listbackPixmap)
            g.fillPixmap(listbackPixmap, 0, y, width(), height() - y);
        else {
	    g.setColor(listBoxBg);
            g.fillRect(0, y, width(), height() - y);
	}
    }
}

void YListBox::paintItem(int i) {
    if (i >= 0 && i < getItemCount())
        paintItem(getGraphics(), i);
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
    beginAutoScroll(delta, motion);
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
        paintItem(getGraphics(), item);
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
        paintItem(getGraphics(), i);
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
            paintItem(getGraphics(), oldItem);
        if (fFocusedItem != -1)
            paintItem(getGraphics(), fFocusedItem);

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
        if (n >= beg && n <= end)
            if (fSelect)
                s = true;
            else
                s = false;
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

bool YSimpleListBox::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym const k(XKeycodeToKeysym(app->display(), key.keycode, 0));

        switch (k) {
            case XK_Home:
                focusSelectItem(0);
                break;

            case XK_End:
                focusSelectItem(getItemCount() - 1);
                break;

            case XK_Up: {
	        int const oldFocus(focusedItem());
	        focusVisible();

                if (focusedItem() > 0)
                    focusSelectItem(oldFocus == focusedItem() ?
                                    focusedItem() - 1 : focusedItem());
                break;
	    }
            case XK_Down: {
	        int const oldFocus(focusedItem());
	        focusVisible();

                if (focusedItem() < getItemCount() - 1)
                    focusSelectItem(oldFocus == focusedItem() ?
                                    focusedItem() + 1 : focusedItem());
                break;
	    }

            default:
                if (k < 256) {
                    unsigned char const c(TOUPPER(k));
                    int const count(getItemCount());
                    int i(focusedItem());

                    for (int n(0); n < count; n++) {
                        YListItem *it(getItem((++i)%= count));
                        char const *title(it->getText());
                        if (title && c == TOUPPER(*title)) {
                            focusSelectItem(i);
                            break;
                        }
                    }
                } else if (!getVerticalScrollBar()->handleScrollKeys(key) /* &&
                           !getHorizontalScrollBar()->handleScrollKeys(key) */)
                    return YWindow::handleKey(key);
        }

        return true;
    }

    return YWindow::handleKey(key);
}

void YSimpleListBox::handleButton(const XButtonEvent &button) {
    focusSelectItem(findItemByPoint(button.x, button.y));
}

void YSimpleListBox::handleMotion(const XMotionEvent &motion) {
    if (BUTTON_MODMASK(motion.state) == Button1Mask)
        focusSelectItem(findItemByPoint(motion.x, motion.y));
}

void YListPopup::configure(const int x, const int y,
			   const unsigned w, const unsigned h,
			   const bool resized) {
    YPopupWindow::configure(x, y, w, h, resized);
    if (resized) fScrollView->setGeometry(2, 2, w - 4, h - 4);
}
    
void YListPopup::paint(Graphics &g, int, int, unsigned, unsigned) {
    g.setColor(listBoxBg);

    if (wmLook == lookMetal) {
        g.drawBorderM(0, 0, width() - 1, height() - 1, true);
        g.fillRect(2, 2, width() - 4, height() - 4);
    } else if (wmLook == lookGtk) {
        g.drawBorderG(0, 0, width() - 1, height() - 1, true);
        g.fillRect(1, 1, width() - 3, height() - 3);
    } else {
        g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        g.fillRect(1, 1, width() - 3, height() - 3);
    }
}

bool YListPopup::handleKey(const XKeyEvent &key) {
    switch (XKeycodeToKeysym(app->display(), key.keycode, 0)) {
        case XK_Return:
        case XK_KP_Enter:
            popdown();
            return true;

        case XK_Escape:
        case XK_Prior:
        case XK_KP_Prior:
            fListBox->focusSelectItem(-1);
            popdown();
            return true;
    }
        
    if (fScrollView->getVerticalScrollBar()->visible() &&
        key.x >= fScrollView->getVerticalScrollBar()->x())
        return fScrollView->getVerticalScrollBar()->handleKey(key);
    else if (fScrollView->getHorizontalScrollBar()->visible() &&
             key.y >= fScrollView->getHorizontalScrollBar()->y())
        return fScrollView->getHorizontalScrollBar()->handleKey(key);
    else
        return fListBox->handleKey(key);
}

void YListPopup::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress &&
       (button.x < 0 || button.x >= (int) width() ||
        button.y < 0 || button.y >= (int) height())) {
        fListBox->focusSelectItem(-1);
        popdown();
    } else if (fScrollView->getVerticalScrollBar()->visible() &&
               button.x >= fScrollView->getVerticalScrollBar()->x())
        fScrollView->getVerticalScrollBar()->handleButton(button);
    else if (fScrollView->getHorizontalScrollBar()->visible() &&
             button.y >= fScrollView->getHorizontalScrollBar()->y())
        fScrollView->getHorizontalScrollBar()->handleButton(button);
    else {
        fListBox->handleButton(button);
        if (button.type == ButtonRelease && 
            button.button == Button1)
            popdown();
    }
}

void YListPopup::handleMotion(const XMotionEvent &motion) {
    if (fScrollView->getVerticalScrollBar()->visible() &&
        motion.x >= fScrollView->getVerticalScrollBar()->x())
        fScrollView->getVerticalScrollBar()->handleMotion(motion);
    else if (fScrollView->getHorizontalScrollBar()->visible() &&
             motion.y >= fScrollView->getHorizontalScrollBar()->y())
        fScrollView->getHorizontalScrollBar()->handleMotion(motion);
    else
        fListBox->handleMotion(motion);
}

void YListPopup::clear() {
    for (YListItem *item; NULL != (item = fListBox->getFirst()); )
        fListBox->removeItem(item);
}

unsigned YListPopup::preferredWidth(int x, unsigned width) {
    return clamp(4U + fListBox->contentWidth() +
                (fListBox->contentHeight() > (int)desktop->height() * 2/5 ?
                 scrollBarWidth : 0), width, x + width);
}

unsigned YListPopup::preferredHeight(int width) {
    return 4 + min(fListBox->contentHeight() +
                  (fListBox->contentWidth() > width ? scrollBarHeight : 0),
                  (int)desktop->height() * 2/5);
}

#endif
