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

inline int getIconSize() {
    return int(YIcon::smallSize());
}

inline ref<YFont> getListBoxFont() {
    if (listBoxFont == null)
        listBoxFont = YFont::getFont(XFA(listBoxFontName));
    return listBoxFont;
}

int YListBox::fAutoScrollDelta = 0;

int YListItem::getWidth() {
    int width = 3 + 20 + getOffset();
    if (getIcon() != null) {
        width += getIconSize();
    }
    if (getText() != null) {
        width += getListBoxFont()->textWidth(getText()) + 3;
    }
    return width;
}

YListBox::YListBox(YScrollView *view, YWindow *aParent):
    YWindow(aParent),
    fVerticalScroll(nullptr),
    fHorizontalScroll(nullptr),
    fView(view),
    fOffsetX(0),
    fOffsetY(0),
    fMaxWidth(0),
    fWidestItem(-1),
    fFocusedItem(0),
    fSelectStart(-1),
    fSelectEnd(-1),
    fDragging(false),
    fSelect(false),
    fVisible(false),
    fOutdated(false),
    fGraphics(this),
    fGradient(null)
{
    if (listBoxFont == null)
        listBoxFont = YFont::getFont(XFA(listBoxFontName));
    addStyle(wsNoExpose);
    setBitGravity(NorthWestGravity);
    addEventMask(VisibilityChangeMask);
    if (fView) {
        fVerticalScroll = view->getVerticalScrollBar();
        fHorizontalScroll = view->getHorizontalScrollBar();
    }
    if (fVerticalScroll)
        fVerticalScroll->setScrollBarListener(this);
    if (fHorizontalScroll)
        fHorizontalScroll->setScrollBarListener(this);
    setTitle("ListBox");
}

YListBox::~YListBox() {
    fGradient = null;
    listBoxFont = null;
}

bool YListBox::isFocusTraversable() {
    return true;
}

void YListBox::addAfter(YListItem *after, YListItem *item) {
    int i = findItem(after);
    if (i >= 0) {
        fItems.insert(i + 1, item);
        if (fFocusedItem > i)
            fFocusedItem++;
        if (fWidestItem > i)
            fWidestItem++;
    } else {
        fItems.append(item);
    }
    outdated();
}

void YListBox::addBefore(YListItem *before, YListItem *item) {
    int i = findItem(before);
    if (i >= 0) {
        fItems.insert(i, item);
        if (fFocusedItem >= i)
            fFocusedItem++;
        if (fWidestItem >= i)
            fWidestItem++;
    } else {
        fItems.append(item);
    }
    outdated();
}

void YListBox::addItem(YListItem *item) {
    fItems.append(item);
    outdated();
}

void YListBox::removeItem(YListItem *item) {
    int index = findItem(item);
    if (index >= 0) {
        fItems.remove(index);
        if (fFocusedItem > index)
            fFocusedItem--;
        else if (index == fFocusedItem) {
            while (index-- && 0 < fItems[index]->getOffset()) { }
            fFocusedItem = index;
        }
        outdated();
    }
}

void YListBox::updateItems() {
    if (fOutdated) {
        fMaxWidth = 0;
        fWidestItem = -1;
        for (IterType a(getIterator()); ++a; ) {
            int width = a->getWidth();
            if (width > fMaxWidth) {
                fMaxWidth = width;
                fWidestItem = a.where();
            }
        }
        fOutdated = false;
    }
}

int YListBox::maxWidth() {
    updateItems();
    return fMaxWidth;
}

int YListBox::findItemByPoint(int /*pX*/, int pY) {
    int no = (pY + fOffsetY) / getLineHeight();
    return (0 <= no && no < getItemCount()) ? no : -1;
}

YListItem *YListBox::getItem(int no) {
    return (0 <= no && no < getItemCount()) ? fItems[no] : nullptr;
}

int YListBox::getLineHeight() {
    return max(getIconSize(), listBoxFont->height()) + 2;
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
            outdated();///!!!fix (use scroll)
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

void YListBox::configure(const YRect2& r) {
    if (listbackPixbuf != null
        && (fGradient == null ||
            fGradient->width() != r.width() ||
            fGradient->height() != r.height()))
    {
        fGradient = listbackPixbuf->scale(r.width(), r.height());
    }
    if (r.resized()) {
        if (r.enlarged()) {
            repaint();
        } else {
            outdated();
        }
    }
}

void YListBox::handleVisibility(const XVisibilityEvent& visib) {
    bool prev = fVisible;
    fVisible = (visib.state != VisibilityFullyObscured);
    if (prev < fVisible)
        repaint();
}

bool YListBox::handleTimer(YTimer* timer) {
    if (timer == fTimer) {
        repaint();
    }
    return false;
}

void YListBox::outdated() {
    fOutdated = true;
    fTimer->setTimer(20L, this, true);
}

void YListBox::repaint() {
    if (fVisible) {
        updateItems();
        resetScrollBars();
        fGraphics.paint();
    }
}

bool YListBox::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);

        bool clear = notbit(m, ControlMask);
        bool extend = hasbit(m, ShiftMask);

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
                           isSelected(fFocusedItem) ? false : true);
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
        case XK_Prior: {
            int const oldFocus(fFocusedItem);

            focusVisible();
            if (fFocusedItem > 0)
                setFocusedItem(oldFocus == fFocusedItem
                               ? max(0, fFocusedItem - 10)
                               : fFocusedItem,
                               clear, extend, false);

            break;
        }
        case XK_Next: {
            int const oldFocus(fFocusedItem);

            focusVisible();
            if (fFocusedItem < getItemCount() - 1)
                setFocusedItem(oldFocus == fFocusedItem
                               ? min(getItemCount() - 1, fFocusedItem + 10)
                               : fFocusedItem,
                               clear, extend, false);
            break;
        }
        case 'a':
        case '/':
        case '\\':
            if (m & ControlMask) {
                for (int i = 0; i < getItemCount(); i++)
                    selectItem(i, (k != '\\'));
                break;
            }
            /* fall-through */
        default:
            if (k < 256) {
                const int ksize = 16;
                char kstr[ksize] = { char(k), 0, };
                getCharFromEvent(key, kstr, ksize);
                const int klen = int(strnlen(kstr, ksize));
                if (ASCII::isControl(kstr[0]) && kstr[0] != char(k)) {
                    extend = false;
                }

                for (int n = 0; n < 2 * getItemCount(); ++n) {
                    int i = (fFocusedItem + 1 + n) % getItemCount();
                    mstring title(getItem(i)->getText());
                    if (n < getItemCount()
                        ? strncasecmp(title, kstr, klen) == 0
                        : strstr(title, kstr) != nullptr)
                    {
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
            bool clear = notbit(button.state, ControlMask);
            bool extend = hasbit(button.state, ShiftMask);

            if (no != -1) {
                fSelect = (!clear && isSelected(no)) ? false : true;
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
            autoScroll(0, nullptr);
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
    int fh = getListBoxFont()->height();
    int xpos = 0;
    int y = n * lh;
    int yPos = y + lh - (lh - fh) / 2 - listBoxFont->descent();

    if (a == nullptr)
        return ;

    xpos += a->getOffset();

    bool s(a->getSelected());

    if (fDragging) {
        int const beg(min(fSelectStart, fSelectEnd));
        int const end(max(fSelectStart, fSelectEnd));

        if (n >= beg && n <= end)
            s = fSelect;
    }

    g.clearArea(0, y - fOffsetY, width(), lh);
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
            mstring t = a->getText();
            if (t != null)
                cw += listBoxFont->textWidth(t) + 3;
        }
        g.drawRect(0 - fOffsetX, y - fOffsetY, cw - 1, lh - 1);
        g.setPenStyle(false);
    }

    ref<YIcon> icon = a->getIcon();
    if (icon != null) {
        ref<YImage> scaled = icon->getScaledIcon(getIconSize());
        if (scaled != null) {
            int dx = xpos + x - fOffsetX;
            int dy = y - fOffsetY + 1;
            g.drawImage(scaled, dx, dy);
        }
    }

    mstring title = a->getText();

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

    for (int n(min); n <= max; n++) {
        paintItem(g, n);
    }

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
    if (!fGraphics)
        return;

    if (i >= 0 && i < getItemCount() && fVisible) {
        int y = i * getLineHeight() - fOffsetY;
        YRect r(0, y, width(), getLineHeight());
        fGraphics.paint(r);
    }
}

void YListBox::activateItem(YListItem *item) {
}

void YListBox::repaintItem(YListItem *item) {
    int i = findItem(item);
    if (i != -1) {
        if (i == fWidestItem) {
            if (item->getWidth() != fMaxWidth) {
                outdated();
            }
        }
        else if (item->getWidth() > fMaxWidth) {
            outdated();
        }
        paintItem(i);
    }
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

        int beg = min(fSelectStart, fSelectEnd);
        int end = max(fSelectStart, fSelectEnd);

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

    int beg = min(selStart, selEnd);
    int end = max(selStart, selEnd);

    for (int i = beg; i <= end; i++)
        paintItem(i);
}

void YListBox::selectItems(int selStart, int selEnd, bool sel) {
    PRECONDITION(selStart != -1);
    PRECONDITION(selEnd != -1);

    int beg = min(selStart, selEnd);
    int end = max(selStart, selEnd);

    for (int i = beg; i <= end; i++)
        selectItem(i, sel);
}

void YListBox::setFocusedItem(int item, bool clear, bool extend, bool virt) {
    int oldItem = fFocusedItem;

    //msg("%d (%d-%d=%d): clear:%d extend:%d virt:%d",
    //       item, fSelectStart, fSelectEnd, fSelect, clear, extend, virt);

    fDragging = virt;
    bool sel = true;
    if (oldItem != -1 && extend && !clear)
        sel = isSelected(oldItem);
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
    return a && isSelected(a);
}

bool YListBox::isSelected(YListItem *item) { // !!! remove this !!!
    bool s = item->getSelected();
    int n = findItem(item);

    if (n == -1)
        return false;

    if (fDragging) {
        int beg = min(fSelectStart, fSelectEnd);
        int end = max(fSelectStart, fSelectEnd);
        if (n >= beg && n <= end) {
            s = fSelect;
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
