/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#pragma implementation "yinputline.h"
#include "config.h"

#include "yxkeydef.h"
#include "yxlib.h"
#include "yinputline.h"

#include "ymenu.h"
#include "ymenuitem.h"
#include "ykeyevent.h"
#include "ybuttonevent.h"
#include "ymotionevent.h"
#include "yfocusevent.h"

#include "yapp.h"
#include "deffonts.h"
#include "ypaint.h"
#include "base.h"

#include <string.h>

YColorPrefProperty YInputLine::inputBg("system", "ColorInput", "rgb:FF/FF/FF");
YColorPrefProperty YInputLine::inputFg("system", "ColorInputText", "rgb:00/00/00");
YColorPrefProperty YInputLine::inputSelectionBg("system", "ColorInputSelection", "rgb:80/80/80");
YColorPrefProperty YInputLine::inputSelectionFg("system", "ColorInputSelectionText", "rgb:00/00/00");
YFontPrefProperty YInputLine::gInputFont("system", "InputFontName", TTFONT(120));
YMenu *YInputLine::gInputMenu = 0;
YTimer *YInputLine::cursorBlinkTimer = 0;

int YInputLine::fAutoScrollDelta = 0;

static YAction *actionCut, *actionCopy, *actionPaste, *actionSelectAll, *actionPasteSelection;

YInputLine::YInputLine(YWindow *parent): YWindow(parent) {
    fText = 0;
    curPos = 0;
    markPos = 0;
    leftOfs = 0;
    fHasFocus = false;
    fSelecting = false;
    fCursorVisible = true;
    if (gInputFont.getFont())
        setSize(width(), gInputFont.getFont()->height() + 2);
}

YInputLine::~YInputLine() {
    if (cursorBlinkTimer) {
        if (cursorBlinkTimer->getTimerListener() == this) {
            cursorBlinkTimer->stopTimer();
            cursorBlinkTimer->setTimerListener(0);
        }
    }
    delete fText; fText = 0;
}

void YInputLine::__setText(const char *text) {
    delete fText;
    int len = strlen(text);
    fText = __newstr(text, len);
    markPos = curPos = leftOfs = 0;
    if (fText)
        curPos = len;
    limit();
    repaint();
}

const char *YInputLine::__getText() {
    return fText;
}

void YInputLine::paint(Graphics &g, const YRect &/*er*/) {
    YFont *font = gInputFont.getFont();
    int min, max, minOfs = 0, maxOfs = 0;
    int textLen = fText ? strlen(fText) : 0;

    if (curPos > markPos) {
        min = markPos;
        max = curPos;
    } else {
        min = curPos;
        max = markPos;
    }

    if (curPos == markPos || !fText || !font || !fHasFocus) {
        g.setColor(inputBg);
        g.fillRect(0, 0, width(), height());
    } else {
        minOfs = font->__textWidth(fText, min) - leftOfs;
        maxOfs = font->__textWidth(fText, max) - leftOfs;

        if (minOfs > 0) {
            g.setColor(inputBg);
            g.fillRect(0, 0, minOfs, height());
        }
#warning "optimize drawing of InputLine to visible area (0,width)"
        if (minOfs < maxOfs) {
            g.setColor(inputSelectionBg);
            g.fillRect(minOfs, 0, maxOfs - minOfs, height());
        }
        if (maxOfs < int(width())) {
            g.setColor(inputBg);
            g.fillRect(maxOfs, 0, width() - maxOfs, height());
        }
    }

    if (font) {
        int yp = 1 + font->ascent();
        int curOfs = fText ? font->__textWidth(fText, curPos) : 0;
        int cx = curOfs - leftOfs;

        g.setFont(font);

        if (curPos == markPos || !fHasFocus || !fText) {
            g.setColor(inputFg);
            if (fText)
                g.__drawChars(fText, 0, textLen, -leftOfs, yp);
            if (fHasFocus && fCursorVisible)
                g.drawLine(cx, 0, cx, font->height() + 2);
        } else {
            if (min > 0) {
                g.setColor(inputFg);
                g.__drawChars(fText, 0, min, -leftOfs, yp);
            }
            /// optimize here too
            if (min < max) {
                g.setColor(inputSelectionFg);
                g.__drawChars(fText, min, max - min, minOfs, yp);
            }
            if (max < textLen) {
                g.setColor(inputFg);
                g.__drawChars(fText, max, textLen - max, maxOfs, yp);
            }
        }
    }
}

bool YInputLine::eventKey(const YKeyEvent &key) {
    if (key.type() == YEvent::etKeyPress) {
        bool extend = key.isShift();
        int textLen = fText ? strlen(fText) : 0;

        if (key.isCtrl()) {
            switch (key.getKey()) {
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
            }
        }
        if (key.isShift()) {
            switch (key.getKey()) {
            case XK_Insert:
            case XK_KP_Insert:
                requestSelection(false);
                return true;
            case XK_Delete:
            case XK_KP_Delete:
                cutSelection();
                return true;
            }
        }
        switch (key.getKey()) {
        case XK_Left:
        case XK_KP_Left:
            if (key.isCtrl()) {
                int p = prevWord(curPos, false);
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
            if (key.isCtrl()) {
                int p = nextWord(curPos, false);
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
                switch (key.getKey()) {
                case XK_Delete:
                case XK_KP_Delete:
                    if (key.isCtrl()) {
                        if (key.isShift()) {
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
                    if (key.isCtrl()) {
                        if (key.isShift()) {
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
        default:
            {
#warning "fix unicode char"
                if (key.getUnichar() != -1) {
                    char c = key.getUnichar();

                    if (insertChar(c))
                        return true;
                }
            }
        }
    }
    return YWindow::eventKey(key);
}

bool YInputLine::eventButton(const YButtonEvent &button) {
    if (button.type() == YEvent::etButtonPress) {
        if (button.getButton() == 1) {
            if (!fHasFocus) {
                setWindowFocus();
            } else {
                fSelecting = true;
                curPos = markPos = offsetToPos(button.x() + leftOfs);
                limit();
                repaint();
            }
        }
    } else if (button.type() == YEvent::etButtonRelease) {
        autoScroll(0, 0);
        if (fSelecting && button.getButton() == 1) {
            fSelecting = false;
            //curPos = offsetToPos(button.x + leftOfs);
            //limit();
            repaint();
        }
    }
    return YWindow::eventButton(button);
}

bool YInputLine::eventMotion(const YMotionEvent &motion) {
    if (fSelecting && motion.hasLeftButton()) {
        if (motion.x() < 0)
            autoScroll(-8, &motion); // fix
        else if (motion.x() >= int(width()))
            autoScroll(8, &motion); // fix
        else {
            autoScroll(0, &motion);
            int c = offsetToPos(motion.x() + leftOfs);
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
    return YWindow::eventMotion(motion);
}

bool YInputLine::eventClickDown(const YClickEvent &down) {
    if (down.getButton() == 1) {
        if ((down.getCount() % 4) == 2) {
            int l = prevWord(curPos, true);
            int r = nextWord(curPos, true);
            if (l != markPos || r != curPos) {
                markPos = l;
                curPos = r;
                limit();
                repaint();
            }
            return true;
        } else if ((down.getCount() % 4) == 3) {
            markPos = curPos = 0;
            if (fText)
                curPos = strlen(fText);
            fSelecting = false;
            limit();
            repaint();
            return true;
        }
    }
    return YWindow::eventClickDown(down);
}

bool YInputLine::eventClick(const YClickEvent &up) {
    if (up.getButton() == 3 /*&& IS_BUTTON(up.state, Button3Mask)*/) {
        YMenu *m = getInputMenu();
        if (m)
            m->popup(0, 0, up.x_root(), up.y_root(), -1, -1,
                     YPopupWindow::pfCanFlipVertical |
                     YPopupWindow::pfCanFlipHorizontal);
        return true;
    } else if (up.getButton() == 2 /*&& IS_BUTTON(up.state, Button2Mask)*/) {
        requestSelection(true);
        return true;
    }
    return YWindow::eventClick(up);
}

void YInputLine::handleSelection(const XSelectionEvent &selection) {
    if (selection.property != None) {
        Atom type;
        long extra;
        int format;
        long nitems;
        char *data;

        XGetWindowProperty(app->display(),
                           selection.requestor, selection.property,
                           0L, 32 * 1024, True,
                           selection.target, &type, &format,
                           (unsigned long *)&nitems,
                           (unsigned long *)&extra,
                           (unsigned char **)&data);

        if (nitems > 0 && data != NULL) {
            __replaceSelection(data, nitems);
        }
        if (data != NULL)
            XFree(data);
    }
}

int YInputLine::offsetToPos(int offset) {
    YFont *font = gInputFont.getFont();
    int ofs = 0, pos = 0;
    int textLen = fText ? strlen(fText) : 0;

    if (font) {
        while (pos < textLen) {
            ofs += font->__textWidth(fText + pos, 1);
            if (ofs < offset)
                pos++;
            else
                break;
        }
    }
    return pos;
}

bool YInputLine::eventFocus(const YFocusEvent &focus) {
#warning "fix focus details"
    if (focus.type() == YEvent::etFocusIn /* && fHasFocus == false*/
        /*&& focus.detail != NotifyPointer && focus.detail != NotifyPointerRoot*/)
    {
        fHasFocus = true;
        selectAll();
        if (cursorBlinkTimer == 0)
            cursorBlinkTimer = new YTimer(this, 300);
        if (cursorBlinkTimer) {
            cursorBlinkTimer->setTimerListener(this);
            cursorBlinkTimer->startTimer();
        }
    } else if (focus.type() == YEvent::etFocusOut/* && fHasFocus == true*/) {
        fHasFocus = false;
        repaint();
        if (cursorBlinkTimer) {
            if (cursorBlinkTimer->getTimerListener() == this) {
                cursorBlinkTimer->stopTimer();
                cursorBlinkTimer->setTimerListener(0);
            }
        }
    }
    return true;
}

bool YInputLine::handleAutoScroll(const YMotionEvent & /*mouse*/) {
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
        fCursorVisible = fCursorVisible ? false : true;
        repaint();
        return true;
    }
    return false;
}

bool YInputLine::move(int pos, bool extend) {
    int textLen = fText ? strlen(fText) : 0;

    if (curPos < 0 || curPos > textLen)
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
    int textLen = fText ? strlen(fText) : 0;

    if (curPos > textLen)
        curPos = textLen;
    if (curPos < 0)
        curPos = 0;
    if (markPos > textLen)
        markPos = textLen;
    if (markPos < 0)
        markPos = 0;

    YFont *font = gInputFont.getFont();
    if (font) {
        int curOfs = font->__textWidth(fText, curPos);
        int curLen = font->__textWidth(fText, textLen);

        if (curOfs >= leftOfs + int(width()))
            leftOfs = curOfs - width() + 1;
        if (curOfs < leftOfs)
            leftOfs = curOfs;
        if (leftOfs + int(width()) > curLen)
            leftOfs = curLen - width();
        if (leftOfs < 0)
            leftOfs = 0;
    }
}

void YInputLine::__replaceSelection(const char *str, int len) {
    int newStrLen;
    char *newStr;
    int textLen = fText ? strlen(fText) : 0;
    int min, max;

    if (curPos > markPos) {
        min = markPos;
        max = curPos;
    } else {
        min = curPos;
        max = markPos;
    }

    newStrLen = min + len + (textLen - max);
    newStr = new char[newStrLen + 1];
    if (newStr) {
        if (min)
            memcpy(newStr, fText, min);
        if (len)
            memcpy(newStr + min, str, len);
        if (max < textLen)
            memcpy(newStr + min + len, fText + max, textLen - max);
        newStr[newStrLen] = 0;
        delete fText;
        fText = newStr;
        curPos = markPos = min + len;
        limit();
        repaint();
    }
}

bool YInputLine::deleteSelection() {
    if (hasSelection()) {
        __replaceSelection(0, 0);
        return true;
    }
    return false;
}

bool YInputLine::deleteNextChar() {
    int textLen = fText ? strlen(fText) : 0;

    if (curPos < textLen) {
        markPos = curPos + 1;
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
    __replaceSelection(&ch, 1);
    return true;
}

#define CHCLASS(c) (((c) == ' ') ? 1 : 0)

int YInputLine::nextWord(int p, bool sep) {
    int textLen = fText ? strlen(fText) : 0;

    while (p < textLen && (CHCLASS(fText[p]) == CHCLASS(fText[p + 1]) ||
                           !sep && CHCLASS(fText[p]) == 1))
        p++;
    if (p < textLen)
        p++;
    return p;
}

int YInputLine::prevWord(int p, bool sep) {
    if (p > 0 && !sep)
        p--;
    while (p > 0 && (CHCLASS(fText[p]) == CHCLASS(fText[p - 1]) ||
                     !sep && CHCLASS(fText[p]) == 1))
        p--;
    return p;
}

bool YInputLine::deleteNextWord() {
    int p = nextWord(curPos, false);
    if (p != curPos) {
        markPos = p;
        return deleteSelection();
    }
    return false;
}
bool YInputLine::deletePreviousWord() {
    int p = prevWord(curPos, false);
    if (p != curPos) {
        markPos = p;
        return deleteSelection();
    }
    return false;
}

bool YInputLine::deleteToEnd() {
    int textLen = fText ? strlen(fText) : 0;

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
    if (fText)
        curPos = strlen(fText);
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
    if (!fText)
        return;
    if (hasSelection()) {
        copySelection();
        deleteSelection();
    }
}

void YInputLine::copySelection() {
    int min, max;
    if (hasSelection()) {
        if (!fText)
            return;
        if (curPos > markPos) {
            min = markPos;
            max = curPos;
        } else {
            min = curPos;
            max = markPos;
        }
        app->setClipboardText(fText + min, max - min);
    }
}

void YInputLine::actionPerformed(YAction *action, unsigned int /*modifiers*/) {
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

void YInputLine::autoScroll(int delta, const YMotionEvent *motion) {
    fAutoScrollDelta = delta;
    beginAutoScroll(delta ? true : false, motion);
}

YMenu *YInputLine::getInputMenu() {
    if (gInputMenu == 0) {
        gInputMenu = new YMenu();
        if (gInputMenu) {
            actionCut = new YAction();
            actionCopy = new YAction();
            actionPaste = new YAction();
            actionPasteSelection = new YAction();
            actionSelectAll = new YAction();
            gInputMenu->setActionListener(this);
            gInputMenu->__addItem("Cut", 2, "Ctrl+X", actionCut)->setEnabled(true);
            gInputMenu->__addItem("Copy", 0, "Ctrl+C", actionCopy)->setEnabled(true);
            gInputMenu->__addItem("Paste", 0, "Ctrl+V", actionPaste)->setEnabled(true);
            gInputMenu->__addItem("Paste Selection", 6, 0, actionPasteSelection)->setEnabled(true);
            gInputMenu->addSeparator();
            gInputMenu->__addItem("Select All", 7, "Ctrl+A", actionSelectAll);
        }
    }
    return gInputMenu;
}
