/*
 * IceWM
 *
 * Copyright (C) 1997,1998 Marko Macek
 */
#include "config.h"

#include "ykey.h"
#include "yinputline.h"
#include "ymenu.h"
#include "ymenuitem.h"

#include "yapp.h"
#include "prefs.h"

#include <string.h>

YFont *YInputLine::inputFont = 0;
YColor *YInputLine::inputBg = 0;
YColor *YInputLine::inputFg = 0;
YColor *YInputLine::inputSelectionBg = 0;
YColor *YInputLine::inputSelectionFg = 0;
YTimer *YInputLine::cursorBlinkTimer = 0;
YMenu *YInputLine::inputMenu = 0;

int YInputLine::fAutoScrollDelta = 0;

static YAction *actionCut, *actionCopy, *actionPaste, *actionSelectAll, *actionPasteSelection;

YInputLine::YInputLine(YWindow *parent): YWindow(parent) {
    if (inputFont == 0)
        inputFont = YFont::getFont(inputFontName);
    if (inputBg == 0)
        inputBg = new YColor(clrInput);
    if (inputFg == 0)
        inputFg = new YColor(clrInputText);
    if (inputSelectionBg == 0)
        inputSelectionBg = new YColor(clrInputSelection);
    if (inputSelectionFg == 0)
        inputSelectionFg = new YColor(clrInputSelectionText);
    if (inputMenu == 0) {
        inputMenu = new YMenu();
        if (inputMenu) {
            actionCut = new YAction();
            actionCopy = new YAction();
            actionPaste = new YAction();
            actionPasteSelection = new YAction();
            actionSelectAll = new YAction();
            inputMenu->setActionListener(this);
            inputMenu->addItem("Cut", 2, "Ctrl+X", actionCut)->setEnabled(true);
            inputMenu->addItem("Copy", 0, "Ctrl+C", actionCopy)->setEnabled(true);
            inputMenu->addItem("Paste", 0, "Ctrl+V", actionPaste)->setEnabled(true);
            inputMenu->addItem("Paste Selection", 6, 0, actionPasteSelection)->setEnabled(true);
            inputMenu->addSeparator();
            inputMenu->addItem("Select All", 7, "Ctrl+A", actionSelectAll);
        }
    }

    fText = 0;
    curPos = 0;
    markPos = 0;
    leftOfs = 0;
    fHasFocus = false;
    fSelecting = false;
    fCursorVisible = true;
    if (inputFont)
        setSize(width(), inputFont->height() + 2);
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

void YInputLine::setText(const char *text) {
    delete fText;
    fText = newstr(text);
    markPos = curPos = leftOfs = 0;
    if (fText)
        curPos = strlen(fText);
    limit();
    repaint();
}

const char *YInputLine::getText() {
    return fText;
}

void YInputLine::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    YFont *font = inputFont;
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
        minOfs = font->textWidth(fText, min) - leftOfs;
        maxOfs = font->textWidth(fText, max) - leftOfs;

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

    if (font) {
        int yp = 1 + font->ascent();
        int curOfs = fText ? font->textWidth(fText, curPos) : 0;
        int cx = curOfs - leftOfs;

        g.setFont(font);

        if (curPos == markPos || !fHasFocus || !fText) {
            g.setColor(inputFg);
            if (fText)
                g.drawChars(fText, 0, textLen, -leftOfs, yp);
            if (fHasFocus && fCursorVisible)
                g.drawLine(cx, 0, cx, font->height() + 2);
        } else {
            if (min > 0) {
                g.setColor(inputFg);
                g.drawChars(fText, 0, min, -leftOfs, yp);
            }
            /// !!! same here
            if (min < max) {
                g.setColor(inputSelectionFg);
                g.drawChars(fText, min, max - min, minOfs, yp);
            }
            if (max < textLen) {
                g.setColor(inputFg);
                g.drawChars(fText, max, textLen - max, maxOfs, yp);
            }
        }
    }
}

bool YInputLine::handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
    if (key.type == KeyPress) {
        bool extend = (vmod & kfShift) ? true : false;
        int textLen = fText ? strlen(fText) : 0;

        if (vmod & kfCtrl) {
            switch (ksym) {
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
        if (vmod & kfShift) {
            switch (ksym) {
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
        switch (ksym) {
        case XK_Left:
        case XK_KP_Left:
            if (vmod & kfCtrl) {
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
            if (vmod & kfCtrl) {
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
                switch (ksym) {
                case XK_Delete:
                case XK_KP_Delete:
                    if (vmod & kfCtrl) {
                        if (vmod & kfShift) {
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
                    if (vmod & kfCtrl) {
                        if (vmod & kfShift) {
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
                char c;

                if (getCharFromEvent(key, &c)) {
                    if (insertChar(c))
                        return true;
                }
            }
        }
    }
    return YWindow::handleKeySym(key, ksym, vmod);
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
            int c = offsetToPos(motion.x + leftOfs);
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
            int l = prevWord(curPos, true);
            int r = nextWord(curPos, true);
            if (l != markPos || r != curPos) {
                markPos = l;
                curPos = r;
                limit();
                repaint();
            }
        } else if ((count % 4) == 3) {
            markPos = curPos = 0;
            if (fText)
                curPos = strlen(fText);
            fSelecting = false;
            limit();
            repaint();
        }
    }
}

void YInputLine::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3 && IS_BUTTON(up.state, Button3Mask)) {
        if (inputMenu)
            inputMenu->popup(0, 0, up.x_root, up.y_root, -1, -1,
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
        char *data;

        XGetWindowProperty(app->display(),
                           selection.requestor, selection.property,
                           0L, 32 * 1024, True,
                           selection.target, &type, &format,
                           (unsigned long *)&nitems,
                           (unsigned long *)&extra,
                           (unsigned char **)&data);

        if (nitems > 0 && data != NULL) {
            replaceSelection(data, nitems);
        }
        if (data != NULL)
            XFree(data);
    }
}

int YInputLine::offsetToPos(int offset) {
    YFont *font = inputFont;
    int ofs = 0, pos = 0;;
    int textLen = fText ? strlen(fText) : 0;

    if (font) {
        while (pos < textLen) {
            ofs += font->textWidth(fText + pos, 1);
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
        && focus.detail != NotifyPointer && focus.detail != NotifyPointerRoot)
    {
        fHasFocus = true;
        selectAll();
        if (cursorBlinkTimer == 0)
            cursorBlinkTimer = new YTimer(300);
        if (cursorBlinkTimer) {
            cursorBlinkTimer->setTimerListener(this);
            cursorBlinkTimer->startTimer();
        }
    } else if (focus.type == FocusOut/* && fHasFocus == true*/) {
        fHasFocus = false;
        repaint();
        if (cursorBlinkTimer) {
            if (cursorBlinkTimer->getTimerListener() == this) {
                cursorBlinkTimer->stopTimer();
                cursorBlinkTimer->setTimerListener(0);
            }
        }
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

    YFont *font = inputFont;
    if (font) {
        int curOfs = font->textWidth(fText, curPos);
        int curLen = font->textWidth(fText, textLen);

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

void YInputLine::replaceSelection(const char *str, int len) {
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
        replaceSelection(0, 0);
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
    replaceSelection(&ch, 1);
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
        return ;
    if (hasSelection()) {
        copySelection();
        deleteSelection();
    }
}

void YInputLine::copySelection() {
    int min, max;
    if (hasSelection()) {
        if (!fText)
            return ;
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

void YInputLine::autoScroll(int delta, const XMotionEvent *motion) {
    fAutoScrollDelta = delta;
    beginAutoScroll(delta ? true : false, motion);
}
