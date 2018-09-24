/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "globit.h"
#include "yinputline.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "yxapp.h"
#include "prefs.h"
#include "intl.h"

#ifndef UINT_MAX
#include <limits.h>
#endif

YInputLine::YInputLine(YWindow *parent):
    YWindow(parent),
    fText(null),
    markPos(0),
    curPos(0),
    leftOfs(0),
    fAutoScrollDelta(0),
    fHasFocus(false),
    fCursorVisible(true),
    fSelecting(false),
    fBlinkTime(333),
    inputFont(YFont::getFont(XFA(inputFontName))),
    inputBg(&clrInput),
    inputFg(&clrInputText),
    inputSelectionBg(&clrInputSelection),
    inputSelectionFg(&clrInputSelectionText),
    inputMenu(new YMenu())
{
    inputMenu->setActionListener(this);
    inputMenu->addItem(_("_Copy"), -2, _("Ctrl+C"), actionCopy)
             ->setEnabled(true);
    inputMenu->addItem(_("Cu_t"), -2, _("Ctrl+X"), actionCut)
             ->setEnabled(true);
    inputMenu->addItem(_("_Paste"), -2, _("Ctrl+V"), actionPaste)
             ->setEnabled(true);
    inputMenu->addItem(_("Paste _Selection"), -2, _("Ctrl+P"), actionPasteSelection)
             ->setEnabled(true);
    inputMenu->addSeparator();
    inputMenu->addItem(_("Select _All"), -2, _("Ctrl+A"), actionSelectAll);

    if (inputFont != null)
        setSize(width(), inputFont->height() + 2);
}

YInputLine::~YInputLine() {
}

void YInputLine::setText(const ustring &text) {
    fText = text;
    markPos = curPos = leftOfs = 0;
    curPos = fText.length();
    limit();
    repaint();
}

ustring YInputLine::getText() {
    return fText;
}

void YInputLine::paint(Graphics &g, const YRect &/*r*/) {
    ref<YFont> font = inputFont;
    int min, max, minOfs = 0, maxOfs = 0;
    int textLen = fText.length();

    if (curPos > markPos) {
        min = markPos;
        max = curPos;
    } else {
        min = curPos;
        max = markPos;
    }

    if (curPos == markPos || fText == null || font == null || !fHasFocus) {
        g.setColor(inputBg);
        g.fillRect(0, 0, width(), height());
    } else {
        minOfs = font->textWidth(fText.substring(0, min)) - leftOfs;
        maxOfs = font->textWidth(fText.substring(0, max)) - leftOfs;

        if (minOfs > 0) {
            g.setColor(inputBg);
            g.fillRect(0, 0, minOfs, height());
        }
        /// !!! optimize (0, width)
        if (minOfs < maxOfs) {
            g.setColor(inputSelectionBg);
            g.fillRect(minOfs, 0, maxOfs - minOfs, height());
        }
        if (maxOfs < int(width())) {
            g.setColor(inputBg);
            g.fillRect(maxOfs, 0, width() - maxOfs, height());
        }
    }

    if (font != null) {
        int yp = 1 + font->ascent();
        int curOfs = font->textWidth(fText.substring(0, curPos));
        int cx = curOfs - leftOfs;

        g.setFont(font);

        if (curPos == markPos || !fHasFocus || fText == null) {
            g.setColor(inputFg);
            if (fText != null)
                g.drawChars(fText.substring(0, textLen), -leftOfs, yp);
            if (fHasFocus && fCursorVisible)
                g.drawLine(cx, 0, cx, font->height() + 2);
        } else {
            if (min > 0) {
                g.setColor(inputFg);
                g.drawChars(fText.substring(0, min), -leftOfs, yp);
            }
            /// !!! same here
            if (min < max) {
                g.setColor(inputSelectionFg);
                g.drawChars(fText.substring(min, max - min), minOfs, yp);
            }
            if (max < textLen) {
                g.setColor(inputFg);
                g.drawChars(fText.substring(max, textLen - max), maxOfs, yp);
            }
        }
    }
}

bool YInputLine::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);

        switch (k) {
        case XK_KP_Home:
        case XK_KP_Up:
        case XK_KP_Prior:
        case XK_KP_Left:
        case XK_KP_Begin:
        case XK_KP_Right:
        case XK_KP_End:
        case XK_KP_Down:
        case XK_KP_Next:
        case XK_KP_Insert:
        case XK_KP_Delete:
            if (key.state & xapp->NumLockMask) {
                k = keyCodeToKeySym(key.keycode, 1);
            }
            break;
        }

        int m = KEY_MODMASK(key.state);
        bool extend = (m & ShiftMask) ? true : false;
        unsigned textLen = fText.length();

        if (m & ControlMask) {
            switch(k) {
            case 'A':
            case 'a':
            case '/':
                selectAll();
                return true;

            case '\\':
                unselectAll();
                return true;
            case 'v':
            case 'V':
                requestSelection(false);
                return true;
            case 'X':
            case 'x':
                cutSelection();
                return true;
            case 'c':
            case 'C':
            case XK_Insert:
            case XK_KP_Insert:
                copySelection();
                return true;
            case 'i':
            case 'I':
                complete();
                return true;
            }
        }
        if (m & ShiftMask) {
            switch (k) {
            case XK_Insert:
            case XK_KP_Insert:
                requestSelection(false);
                return true;
            case XK_Delete:
            case XK_KP_Delete:
                cutSelection();
                break;
            }
        }
        switch (k) {
        case XK_Left:
        case XK_KP_Left:
            if (m & ControlMask) {
                unsigned p = prevWord(curPos, false);
                if (p != curPos) {
                    if (move(p, extend))
                        return true;
                }
            } else {
                if (curPos > 0) {
                    if (move(curPos - 1, extend))
                        return true;
                }
            }
            break;
        case XK_Right:
        case XK_KP_Right:
            if (m & ControlMask) {
                unsigned p = nextWord(curPos, false);
                if (p != curPos) {
                    if (move(p, extend))
                        return true;
                }
            } else {
                if (curPos < textLen) {
                    if (move(curPos + 1, extend))
                        return true;
                }
            }
            break;
        case XK_Home:
        case XK_KP_Home:
            move(0, extend);
            return true;
        case XK_End:
        case XK_KP_End:
            move(textLen, extend);
            return true;
        case XK_Delete:
        case XK_KP_Delete:
        case XK_BackSpace:
            if (hasSelection()) {
                if (deleteSelection())
                    return true;
            } else {
                switch (k) {
                case XK_Delete:
                case XK_KP_Delete:
                    if (m & ControlMask) {
                        if (m & ShiftMask) {
                            if (deleteToEnd())
                                return true;
                        } else {
                            if (deleteNextWord())
                                return true;
                        }
                    } else {
                        if (deleteNextChar())
                            return true;
                    }
                    break;
                case XK_BackSpace:
                    if (m & ControlMask) {
                        if (m & ShiftMask) {
                            if (deleteToBegin())
                                return true;
                        } else {
                            if (deletePreviousWord())
                                return true;
                        }
                    } else {
                        if (deletePreviousChar())
                            return true;
                    }
                    break;
                }
            }
            break;
        case XK_Tab:
            complete();
            break;
        default:
            {
                char s[16];

                if (getCharFromEvent(key, s, sizeof(s))) {
                    replaceSelection(ustring(s, strlen(s)));
                    return true;
                }
            }
        }
    }
    return YWindow::handleKey(key);
}

void YInputLine::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (button.button == 1) {
            if (fHasFocus == false) {
                setWindowFocus();
            } else {
                fSelecting = true;
                curPos = markPos = offsetToPos(button.x + leftOfs);
                limit();
                repaint();
            }
        }
    } else if (button.type == ButtonRelease) {
        autoScroll(0, 0);
        if (fSelecting && button.button == 1) {
            fSelecting = false;
            //curPos = offsetToPos(button.x + leftOfs);
            //limit();
            repaint();
        }
    }
    YWindow::handleButton(button);
}

void YInputLine::handleMotion(const XMotionEvent &motion) {
    if (fSelecting && (motion.state & Button1Mask)) {
        if (motion.x < 0)
            autoScroll(-8, &motion); // fix
        else if (motion.x >= int(width()))
            autoScroll(8, &motion); // fix
        else {
            autoScroll(0, &motion);
            unsigned c = offsetToPos(motion.x + leftOfs);
            if (getClickCount() == 2) {
                if (c >= markPos) {
                    if (markPos > curPos)
                        markPos = curPos;
                    c = nextWord(c, true);
                } else if (c < markPos) {
                    if (markPos < curPos)
                        markPos = curPos;
                    c = prevWord(c, true);
                }
            }
            if (curPos != c) {
                curPos = c;
                limit();
                repaint();
            }
        }
    }
    YWindow::handleMotion(motion);
}

void YInputLine::handleClickDown(const XButtonEvent &down, int count) {
    if (down.button == 1) {
        if ((count % 4) == 2) {
            unsigned l = prevWord(curPos, true);
            unsigned r = nextWord(curPos, true);
            if (l != markPos || r != curPos) {
                markPos = l;
                curPos = r;
                limit();
                repaint();
            }
        } else if ((count % 4) == 3) {
            markPos = curPos = 0;
            curPos = fText.length();
            fSelecting = false;
            limit();
            repaint();
        }
    }
}

void YInputLine::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3 && IS_BUTTON(up.state, Button3Mask)) {
        if (inputMenu)
            inputMenu->popup(this, 0, 0, up.x_root, up.y_root,
                             YPopupWindow::pfCanFlipVertical |
                             YPopupWindow::pfCanFlipHorizontal);
    } else if (up.button == 2 && IS_BUTTON(up.state, Button2Mask)) {
        requestSelection(true);
    }
}

void YInputLine::handleSelection(const XSelectionEvent &selection) {
    if (selection.property != None) {
        Atom type;
        long extra;
        int format;
        long nitems;
        union {
            char *ptr;
            unsigned char *xptr;
        } data;

        XGetWindowProperty(xapp->display(),
                           selection.requestor, selection.property,
                           0L, 32 * 1024, True,
                           selection.target, &type, &format,
                           (unsigned long *)&nitems,
                           (unsigned long *)&extra,
                           &(data.xptr));

        if (nitems > 0 && data.ptr != NULL) {
            replaceSelection(mstring(data.ptr, nitems));
        }
        if (data.xptr != NULL)
            XFree(data.xptr);
    }
}

unsigned YInputLine::offsetToPos(int offset) {
    ref<YFont> font = inputFont;
    int ofs = 0, pos = 0;
    int textLen = fText.length();

    if (font != null) {
        while (pos < textLen) {
            ofs += font->textWidth(fText.substring(pos, 1));
            if (ofs < offset)
                pos++;
            else
                break;
        }
    }
    return pos;
}

void YInputLine::handleFocus(const XFocusChangeEvent &focus) {
    if (focus.type == FocusIn /* && fHasFocus == false*/
        && focus.detail != NotifyPointer
        && focus.detail != NotifyPointerRoot)
    {
        fHasFocus = true;
        selectAll();
        cursorBlinkTimer->setTimer(fBlinkTime, this, true);
    }
    else if (focus.type == FocusOut/* && fHasFocus == true*/) {
        fHasFocus = false;
        repaint();
        cursorBlinkTimer = null;
    }
}

bool YInputLine::handleAutoScroll(const XMotionEvent & /*mouse*/) {
    curPos += fAutoScrollDelta;
    leftOfs += fAutoScrollDelta;
    int c = curPos;

    if (fAutoScrollDelta < 0)
        c = offsetToPos(leftOfs);
    else if (fAutoScrollDelta > 0)
        c = offsetToPos(leftOfs + width());
    curPos = c;
    limit();
    repaint();
    return true;
}

bool YInputLine::handleTimer(YTimer *t) {
    if (t == cursorBlinkTimer) {
        fCursorVisible ^= true;
        repaint();
        return fHasFocus;
    }
    return false;
}

bool YInputLine::move(unsigned pos, bool extend) {
    unsigned textLen = fText.length();

    if (curPos > textLen)
        return false;

    if (curPos != pos || (!extend && curPos != markPos)) {

        curPos = pos;
        if (!extend)
            markPos = curPos;

        limit();
        repaint();
    }
    return true;
}

void YInputLine::limit() {
    unsigned textLen = fText.length();

    if (curPos > textLen)
        curPos = textLen;
    if (markPos > textLen)
        markPos = textLen;

    ref<YFont> font = inputFont;
    if (font != null) {
        int curOfs = font->textWidth(fText.substring(0, curPos));
        int curLen = font->textWidth(fText.substring(0, textLen));

        if (curOfs >= leftOfs + int(width()) + 1)
            leftOfs = curOfs - width() + 2;
        if (curOfs < leftOfs)
            leftOfs = curOfs;
        if (leftOfs + int(width()) + 1 > curLen)
            leftOfs = curLen - width() + 1;
        if (leftOfs < 0)
            leftOfs = 0;
    }
}

void YInputLine::replaceSelection(const ustring &str) {
    unsigned from=min(curPos, markPos);
    unsigned to=max(curPos, markPos);
    ustring newStr = fText.replace(from, to - from, str);

    if (newStr != null) {
        fText = newStr;
        curPos = markPos = from + str.length();
        limit();
        repaint();
    }
}

bool YInputLine::deleteSelection() {
    if (hasSelection()) {
        replaceSelection(null);
        return true;
    }
    return false;
}

bool YInputLine::deleteNextChar() {
    unsigned textLen = fText.length();

    if (curPos < textLen) {
        markPos = curPos + (curPos < UINT_MAX);
        deleteSelection();
        return true;
    }
    return false;
}

bool YInputLine::deletePreviousChar() {
    if (curPos > 0) {
        markPos = curPos - 1;
        deleteSelection();
        return true;
    }
    return false;
}

bool YInputLine::insertChar(char ch) {
    char s[2] = { ch, 0 };
    replaceSelection(s);
    return true;
}

#define CHCLASS(c) ((c) == ' ')

unsigned YInputLine::nextWord(unsigned p, bool sep) {
    unsigned textLen = fText.length();

    while (p < textLen &&
           (CHCLASS(fText.charAt(p)) == CHCLASS(fText.charAt(p + 1)) ||
            (!sep && CHCLASS(fText.charAt(p)))))
    {
        p++;
    }
    if (p < textLen)
        p++;
    return p;
}

unsigned YInputLine::prevWord(unsigned p, bool sep) {
    if (p > 0 && !sep)
        p--;
    while (p > 0 &&
           (CHCLASS(fText.charAt(p)) == CHCLASS(fText.charAt(p - 1)) ||
            (!sep && CHCLASS(fText.charAt(p)))))
    {
        p--;
    }
    return p;
}

bool YInputLine::deleteNextWord() {
    unsigned p = nextWord(curPos, false);
    if (p != curPos) {
        markPos = p;
        return deleteSelection();
    }
    return false;
}

bool YInputLine::deletePreviousWord() {
    unsigned p = prevWord(curPos, false);
    if (p != curPos) {
        markPos = p;
        return deleteSelection();
    }
    return false;
}

bool YInputLine::deleteToEnd() {
    unsigned textLen = fText.length();

    if (curPos < textLen) {
        markPos = textLen;
        return deleteSelection();
    }
    return false;
}

bool YInputLine::deleteToBegin() {
    if (curPos > 0) {
        markPos = 0;
        return deleteSelection();
    }
    return false;
}

void YInputLine::selectAll() {
    markPos = curPos = 0;
    curPos = fText.length();
    fSelecting = false;
    limit();
    repaint();
}

void YInputLine::unselectAll() {
    fSelecting = false;
    if (markPos != curPos) {
        markPos = curPos;
        repaint();
    }
}
void YInputLine::cutSelection() {
    if (fText == null)
        return ;
    if (hasSelection()) {
        copySelection();
        deleteSelection();
    }
}

void YInputLine::copySelection() {
    int min, max;
    if (hasSelection()) {
        if (fText == null)
            return ;
        if (curPos > markPos) {
            min = markPos;
            max = curPos;
        } else {
            min = curPos;
            max = markPos;
        }
        xapp->setClipboardText(fText.substring(min, max - min));
    }
}

void YInputLine::actionPerformed(YAction action, unsigned int /*modifiers*/) {
    if (action == actionSelectAll)
        selectAll();
    else if (action == actionPaste)
        requestSelection(false);
    else if (action == actionPasteSelection)
        requestSelection(true);
    else if (action == actionCopy)
        copySelection();
    else if (action == actionCut)
        cutSelection();
}

void YInputLine::autoScroll(int delta, const XMotionEvent *motion) {
    fAutoScrollDelta = delta;
    beginAutoScroll(delta ? true : false, motion);
}

void YInputLine::complete() {
    char* res = 0;
    if (1 == globit_best(cstring(fText), &res))
        setText(res);
    free(res);
}

// vim: set sw=4 ts=4 et:
