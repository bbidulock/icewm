/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"

#ifndef LITE
#include "ykey.h"
#include "yinputline.h"
#include "ymenu.h"
#include "ymenuitem.h"
#include "ylistbox.h"
#include "yhistory.h"

#include "yapp.h"
#include "prefs.h"

#include <string.h>

#include "intl.h"

YFont *YInputLine::inputFont(NULL);

YColor *YInputLine::inputBg(NULL);
YColor *YInputLine::inputFg(NULL);
YColor *YInputLine::inputSelectionBg(NULL);
YColor *YInputLine::inputSelectionFg(NULL);
YColor *YInputLine::inputPopupBg(NULL);
YColor *YInputLine::inputPopupFg(NULL);

YTimer *YInputLine::cursorBlinkTimer(NULL);
YMenu *YInputLine::inputMenu(NULL);
int YInputLine::fAutoScrollDelta(0);

static YAction *actionCut, *actionCopy, *actionPaste,
               *actionSelectAll, *actionPasteSelection;

YInputLine::YInputLine(YWindow *parent, char const *historyId):
    YWindow(parent),
    fText(NULL), fCurPos(0), fSelPos(0), fHisPos(-1), fLeftOfs(0),
    fHasFocus(false), fCursorVisible(true), fSelecting(false),
    fHistory(historyId ? new YHistory(historyId) : NULL),
    fHistoryPopup(historyId ? new YListPopup(NULL) : NULL) {
    if (NULL == inputFont)
        inputFont = YFont::getFont(inputFontName);
    if (NULL == inputBg)
        inputBg = new YColor(clrInput);
    if (NULL == inputFg)
        inputFg = new YColor(clrInputText);
    if (NULL == inputSelectionBg)
        inputSelectionBg = new YColor(clrInputSelection);
    if (NULL == inputSelectionFg)
        inputSelectionFg = new YColor(clrInputSelectionText);
    if (NULL == inputPopupBg)
        inputPopupBg = new YColor(*clrInputButton ? clrInputButton : clrInput);
    if (NULL == inputPopupFg)
        inputPopupFg = new YColor(*clrInputArrow ? clrInputArrow : clrInputText);
    if (NULL == inputMenu) {
        if ((inputMenu = new YMenu())) {
            actionCut = new YAction();
            actionCopy = new YAction();
            actionPaste = new YAction();
            actionPasteSelection = new YAction();
            actionSelectAll = new YAction();
            inputMenu->setActionListener(this);
            inputMenu->addItem(_("Cu_t"), -2, _("Ctrl+X"), actionCut)->setEnabled(true);
            inputMenu->addItem(_("_Copy"), -2, _("Ctrl+C"), actionCopy)->setEnabled(true);
            inputMenu->addItem(_("_Paste"), -2, _("Ctrl+V"), actionPaste)->setEnabled(true);
            inputMenu->addItem(_("Paste _Selection"), -2, 0, actionPasteSelection)->setEnabled(true);
            inputMenu->addSeparator();
            inputMenu->addItem(_("Select _All"), -2, _("Ctrl+A"), actionSelectAll);
        }
    }

    if (inputFont)
        setSize(width(), inputFont->height() + (inputDrawBorder ? 8 : 2));
}

YInputLine::~YInputLine() {
    if (cursorBlinkTimer &&
        cursorBlinkTimer->getTimerListener() == this) {
        cursorBlinkTimer->stopTimer();
        cursorBlinkTimer->setTimerListener(0);
    }

    delete[] fText;

    delete fHistory;
    delete fHistoryPopup;
}

void YInputLine::setText(const char *text) {
    delete[] fText; fText = newstr(text);
    fCurPos = fSelPos = fLeftOfs = 0; fHisPos = -1;
    if (fText) fCurPos = strlen(fText);
    limit();
    repaint();
}

void YInputLine::paint(Graphics &g, int, int, unsigned int, unsigned int) {
    int x(0), xi(0), y(0), yi(0),
        w(inputWidth()), wi(inputWidth()), h(height()), hi(height());

    g.setColor(inputBg);
    if (inputDrawBorder) {
	if (wmLook == lookMetal) {
	    g.drawBorderM(x, y, width() - 1, h - 1, false);
	    w -= 4; h -= 4;
	} else if (wmLook == lookGtk) {
            g.drawBorderG(x, y, width() - 1, h - 1, false);
            w -= 3; h -= 3;
	} else {
            g.drawBorderW(x, y, width() - 1, h - 1, false);
            w -= 3; h -= 3;
	}

        x = y = 2; xi = yi = 3; wi = inputWidth() - 6, hi = height() - 6;
    }
    
    int const arrowSize(Graphics::arrowSize(inputFont->height() - 2));

    int bgn, end;

    if (fCurPos > fSelPos) {
        bgn = fSelPos;
        end = fCurPos;
    } else {
        bgn = fCurPos;
        end = fSelPos;
    }

    int const textLen(fText ? strlen(fText) : 0);
    int const minOfs(xi + (int)inputFont->textWidth(fText, bgn) - fLeftOfs);
    int const maxOfs(xi + (int)inputFont->textWidth(fText, end) - fLeftOfs);

    if (fCurPos == fSelPos || !(fText && inputFont && fHasFocus)) {
        g.fillRect(x, y, w, h);
    } else {
        int const xa(max(minOfs, xi));    
        int const xe(min(maxOfs, xi + wi));
        
        if (xa > x) g.fillRect(x, y, xa, h);

        if (xa < xe) {
            if (inputDrawBorder) g.fillRect(xa, y, xe - xa, 1);

            g.setColor(inputSelectionBg);
            g.fillRect(xa, yi, xe - xa, hi);
            g.setColor(inputBg);

            if (inputDrawBorder) g.fillRect(xa, y + h - 1, xe - xa, 1);
        }

        if (xe < x + w) g.fillRect(xe, y, x + w - xe, h);
    }

    if (inputFont) {
        int const yp(yi + 1 + inputFont->ascent());

        g.setFont(inputFont);
        g.setColor(inputFg);
        
        XRectangle clipRect = { 0, 0, wi, hi };
        g.setClipRects(xi, yi, &clipRect);

        if (fCurPos == fSelPos || !fHasFocus || !fText) {
            if (fText) g.drawChars(fText, 0, textLen, xi - fLeftOfs, yp);
        } else {
            if (bgn > 0)
                g.drawChars(fText, 0, bgn, xi - fLeftOfs, yp);

            if (bgn < end) {
                g.setColor(inputSelectionFg);
                g.drawChars(fText, bgn, end - bgn, minOfs, yp);
                g.setColor(inputFg);
            }

            if (end < textLen)
                g.drawChars(fText, end, textLen - end, maxOfs, yp);
        }

        g.setClipMask();
        
        if (fHasFocus && fCursorVisible) {
            int const curOfs(fText ? inputFont->textWidth(fText, fCurPos) : 0);
            int const cx(xi + curOfs - fLeftOfs);
            g.drawLine(cx, yi, cx, yi + inputFont->height() + 1);
        }
    }

    if (fHistory) {
        int const len(Graphics::arrowLength(arrowSize));

        g.setColor(inputPopupBg);
        g.fillRect(x + w, x, arrowSize + 6, h);

        g.setColor(inputPopupFg);
        g.drawArrow(Down, x + w + 2, yi + (hi - len)/2 + 1, arrowSize);
    }
}

bool YInputLine::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym const k(XKeycodeToKeysym(app->display(), key.keycode, 0));
        int const m(KEY_MODMASK(key.state));
        bool const extend(m & ShiftMask);

        int const textLen(fText ? strlen(fText) : 0);

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
                    int const p(prevWord(fCurPos, false));
                    if (p != fCurPos && move(p, extend))
                        return true;
                } else {
                    if (fCurPos > 0 && move(fCurPos - 1, extend))
                        return true;
                }
                break;

            case XK_Right:
            case XK_KP_Right:
                if (m & ControlMask) {
                    int const p(nextWord(fCurPos, false));
                    if (p != fCurPos && move(p, extend))
                        return true;
                } else {
                    if (fCurPos < textLen && move(fCurPos + 1, extend))
                        return true;
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
                    if (deleteSelection()) return true;
                } else if (XK_BackSpace == k) {
                    if (m & ControlMask) {
                        if (m & ShiftMask) {
                            if (deleteToBegin()) return true;
                        } else {
                            if (deletePreviousWord()) return true;
                        }
                    } else {
                        if (deletePreviousChar()) return true;
                    }
                } else {
                    if (m & ControlMask) {
                        if (m & ShiftMask) {
                            if (deleteToEnd()) return true;
                        } else {
                            if (deleteNextWord()) return true;
                        }
                    } else {
                        if (deleteNextChar()) return true;
                    }
                }
                break;

            case XK_Up:
            case XK_KP_Up:
                if (fHistory && nextHistoryItem()) return true;
                break;

            case XK_Down:
            case XK_KP_Down:
                if (fHistory && prevHistoryItem()) return true;
                break;

            case XK_Next:
            case XK_KP_Next:
                if (fHistory && showHistoryPopup()) return true;
                break;

            case XK_Tab:
                if (fHistory && showHistoryPopup(fText)) return true;
                break;

            case XK_Return:
            case XK_KP_Enter:
                if (fHistory && textLen) {
                    fHistory->add(fText);
                    return true;
                }
                break;

            default: {
                char c;

                if (getCharFromEvent(key, &c) && insertChar(c)) return true;
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
                fCurPos = fSelPos = offsetToPos(button.x + fLeftOfs);
                limit();
                repaint();
            }
        }
    } else if (button.type == ButtonRelease) {
        autoScroll(0, 0);
        if (fSelecting && button.button == 1) {
            fSelecting = false;
            //fCurPos = offsetToPos(button.x + fLeftOfs);
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
        else if (motion.x >= int(inputWidth()))
            autoScroll(8, &motion); // fix
        else {
            autoScroll(0, &motion);
            int c = offsetToPos(motion.x + fLeftOfs);
            if (getClickCount() == 2) {
                if (c >= fSelPos) {
                    if (fSelPos > fCurPos)
                        fSelPos = fCurPos;
                    c = nextWord(c, true);
                } else if (c < fSelPos) {
                    if (fSelPos < fCurPos)
                        fSelPos = fCurPos;
                    c = prevWord(c, true);
                }
            }
            if (fCurPos != c) {
                fCurPos = c;
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
            int l = prevWord(fCurPos, true);
            int r = nextWord(fCurPos, true);
            if (l != fSelPos || r != fCurPos) {
                fSelPos = l;
                fCurPos = r;
                limit();
                repaint();
            }
        } else if ((count % 4) == 3) {
            fSelPos = fCurPos = 0;
            if (fText) fCurPos = strlen(fText);
            fSelecting = false;
            limit();
            repaint();
        }
    }
}

void YInputLine::handleClick(const XButtonEvent &up, int /*count*/) {
    if (fHistoryPopup && fHistoryPopup->visible()) {
        fHistoryPopup->popdown();
    } else if (fHistory && up.x >= inputWidth()) {
        showHistoryPopup();
    } else if (up.button == 3 && IS_BUTTON(up.state, Button3Mask)) {
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
    int const textLen(fText ? strlen(fText) : 0);
    int ofs(0), pos(0);

    if (inputFont) {
        while (pos < textLen) {
            ofs += inputFont->textWidth(fText + pos, 1);
            if (ofs < offset) ++pos;
            else break;
        }
    }

    return pos;
}

void YInputLine::handleFocus(const XFocusChangeEvent &focus) {
    if (focus.type == FocusIn /* && fHasFocus == false*/ &&
        focus.detail != NotifyPointer &&
        focus.detail != NotifyPointerRoot) {
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
    fCurPos += fAutoScrollDelta;
    fLeftOfs += fAutoScrollDelta;

    fCurPos = (fAutoScrollDelta < 0 ? offsetToPos(fLeftOfs) :
               fAutoScrollDelta > 0 ? offsetToPos(fLeftOfs + inputWidth()) : 0);

    limit();
    repaint();
    return true;
}

bool YInputLine::handleTimer(YTimer *t) {
    if (t == cursorBlinkTimer) {
        fCursorVisible = !fCursorVisible;
        repaint();
        return true;
    }
    return false;
}

bool YInputLine::move(int pos, bool extend) {
    int const textLen(fText ? strlen(fText) : 0);

    if (fCurPos < 0 || fCurPos > textLen) return false;

    if (fCurPos != pos || (!extend && fCurPos != fSelPos)) {
        fCurPos = pos;

        if (!extend) fSelPos = fCurPos;

        limit();
        repaint();
    }

    return true;
}

void YInputLine::limit() {
    int const textLen(fText ? strlen(fText) : 0);

    fCurPos = clamp(fCurPos, 0, textLen);
    fSelPos = clamp(fSelPos, 0, textLen);

    if (inputFont) {
        int const curOfs(inputFont->textWidth(fText, fCurPos));
        int const curLen(inputFont->textWidth(fText, textLen));

        int const width(inputWidth() - (inputDrawBorder ? 6 : 1));

        fLeftOfs = clamp(fLeftOfs, curOfs - width, curOfs);
        fLeftOfs = clamp(fLeftOfs, 0, curLen - width);
    }
}

void YInputLine::replaceSelection(const char *str, int len) {
    int min, max;

    if (fCurPos > fSelPos) {
        min = fSelPos;
        max = fCurPos;
    } else {
        min = fCurPos;
        max = fSelPos;
    }

    int const textLen(fText ? strlen(fText) : 0);
    int const newLen(min + len + (textLen - max));

    char *newStr(new char[newLen + 1]);

    if (newStr) {
        if (min)
            memcpy(newStr, fText, min);
        if (len)
            memcpy(newStr + min, str, len);
        if (max < textLen)
            memcpy(newStr + min + len, fText + max, textLen - max);

        newStr[newLen] = '\0';
        delete[] fText; fText = newStr;

        fCurPos = fSelPos = min + len;
        fHisPos = -1;

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

    if (fCurPos < textLen) {
        fSelPos = fCurPos + 1;
        deleteSelection();
        return true;
    }
    return false;
}

bool YInputLine::deletePreviousChar() {
    if (fCurPos > 0) {
        fSelPos = fCurPos - 1;
        deleteSelection();
        return true;
    }
    return false;
}

bool YInputLine::insertChar(char ch) {
    replaceSelection(&ch, 1);
    return true;
}

#define CHCLASS(c) ((c) == ' ')

int YInputLine::nextWord(int p, bool sep) {
    int textLen = fText ? strlen(fText) : 0;

    while (p < textLen && (CHCLASS(fText[p]) == CHCLASS(fText[p + 1]) ||
                           !sep && CHCLASS(fText[p])))
        p++;
    if (p < textLen)
        p++;
    return p;
}

int YInputLine::prevWord(int p, bool sep) {
    if (p > 0 && !sep)
        p--;
    while (p > 0 && (CHCLASS(fText[p]) == CHCLASS(fText[p - 1]) ||
                     !sep && CHCLASS(fText[p])))
        p--;
    return p;
}

bool YInputLine::deleteNextWord() {
    int const p(nextWord(fCurPos, false));

    if (p != fCurPos) {
        fSelPos = p;
        return deleteSelection();
    }

    return false;
}
bool YInputLine::deletePreviousWord() {
    int const p(prevWord(fCurPos, false));

    if (p != fCurPos) {
        fSelPos = p;
        return deleteSelection();
    }

    return false;
}

bool YInputLine::deleteToEnd() {
    int const textLen(fText ? strlen(fText) : 0);

    if (fCurPos < textLen) {
        fSelPos = textLen;
        return deleteSelection();
    }

    return false;
}

bool YInputLine::deleteToBegin() {
    if (fCurPos > 0) {
        fSelPos = 0;
        return deleteSelection();
    }

    return false;
}

void YInputLine::selectAll() {
    fSelPos = fCurPos = 0;
    if (fText) fCurPos = strlen(fText);
    fSelecting = false;
    limit();
    repaint();
}

void YInputLine::unselectAll() {
    fSelecting = false;
    if (fSelPos != fCurPos) {
        fSelPos = fCurPos;
        repaint();
    }
}
void YInputLine::cutSelection() {
    if (fText && hasSelection()) {
        copySelection();
        deleteSelection();
    }
}

void YInputLine::copySelection() {
    if (hasSelection() && fText) {
        if (fCurPos > fSelPos)
            app->setClipboardText(fText + fSelPos, fCurPos - fSelPos);
        else
            app->setClipboardText(fText + fCurPos, fSelPos - fCurPos);
    }
}

bool YInputLine::selectHistoryItem(int item) {
    if (fHistory && fHistory->count()) {
        (item+= fHistory->count())%= fHistory->count();
        setText(fHistory->get(item));
        fHisPos = item;
        return true;
    }

    return false;
}

bool YInputLine::prevHistoryItem() {
    return selectHistoryItem(fHisPos - 1);
}

bool YInputLine::nextHistoryItem() {
    return selectHistoryItem(fHisPos + 1);
}

bool YInputLine::showHistoryPopup(char const *prefix) {
    if (fHistoryPopup && !fHistoryPopup->visible()) {
        int const pfxLen(prefix ? strlen(prefix) : 0);
        int const lastItem(fHistory->count() - 1);
        int count(0);

        fHistoryPopup->clear();
        
        for (int n(lastItem); n >= 0; --n) {
            char const *item(fHistory->get(n));

            if (!(pfxLen && strncmp(prefix, item, pfxLen))) {
                fHistoryPopup->add(n, item);
                ++count;
            }
        }

        if (count == 1 && pfxLen) {
            selectHistoryItem(fHistoryPopup->get(0)->getId());
            return true;
        } else if (count > 0) {
            fHistoryPopup->select(fHisPos >= 0 && !pfxLen ?
                                  lastItem - fHisPos : 0);

            int x(-2), y(0); mapToGlobal(x, y);

            unsigned const w(fHistoryPopup->preferredWidth(x, width()));
            unsigned const h(fHistoryPopup->preferredHeight(w));

            x-= (w > width() ? w - width() : 0);
            y = (y > (int) (desktop->height() - h) ? y - h : y + height());

            fHistoryPopup->setGeometry(x, y, w + 4, h);
            fHistoryPopup->popup(NULL, this, 0);

            return true;
        } else
            app->alert();
    }

    return false;
}

int YInputLine::inputWidth() const {
    return width() - (fHistory ? Graphics::arrowSize(inputFont->height() - 2)
                               + (inputDrawBorder ? 6 : 4) : 0);
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
    beginAutoScroll(delta, motion);
}

void YInputLine::handlePopDown(YPopupWindow *popup) {
    if (popup == fHistoryPopup) {
        int const historyId(fHistoryPopup->id());
        if (historyId >= 0) selectHistoryItem(historyId);
    }
}

#endif
