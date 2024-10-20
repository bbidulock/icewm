/*
 * IceWM
 *
 * Copyright (C) 1997-2001 Marko Macek
 */
#include "config.h"
#include "globit.h"
#include "yinputline.h"
#include "ymenu.h"
#include "yxapp.h"
#include "prefs.h"
#include "intl.h"
#include "regex.h"
#include <X11/keysym.h>

class YInputMenu: public YMenu {
public:
    YInputMenu() {
        addItem(_("_Copy"), -2, _("Ctrl+C"), actionCopy);
        addItem(_("Cu_t"), -2, _("Ctrl+X"), actionCut);
        addItem(_("_Paste"), -2, _("Ctrl+V"), actionPaste);
        addItem(_("Paste _Selection"), -2, _("Ctrl+P"), actionPasteSelection);
        addSeparator();
        addItem(_("Select _All"), -2, _("Ctrl+A"), actionSelectAll);
    }
    ~YInputMenu() {
        if (xapp->popup() == this) {
            xapp->popdown(this);
        }
    }
};

YInputLine::YInputLine(YWindow *parent, YInputListener *listener):
    YWindow(parent),
    markPos(0),
    curPos(0),
    leftOfs(0),
    fAutoScrollDelta(0),
    fHasFocus(false),
    fCursorVisible(true),
    fSelecting(false),
    fBlinkTime(333),
    fKeyPressed(0),
    fListener(listener),
    inputContext(nullptr),
    inputFont(inputFontName),
    inputBg(&clrInput),
    inputFg(&clrInputText),
    inputSelectionBg(&clrInputSelection),
    inputSelectionFg(&clrInputSelectionText),
    prefixRegex(nullptr)
{
    addStyle(wsNoExpose);
    if (inputFont != null)
        setSize(width(), inputFont->height() + 2);
    if (inputIgnorePfx && *inputIgnorePfx) {
        prefixRegex = new regex_t;
        mstring reFullPrefix("^(", inputIgnorePfx, ")[[:space:]]+");
        if (0 != regcomp(prefixRegex, reFullPrefix, REG_EXTENDED)) {
            delete prefixRegex;
            prefixRegex = nullptr;
        }
    }
}

YInputLine::~YInputLine() {
    if (inputContext)
        XDestroyIC(inputContext);
    if (prefixRegex) {
        regfree(prefixRegex);
        delete prefixRegex;
        prefixRegex = nullptr;
    }
}

void YInputLine::setText(mstring text, bool asMarked) {
    fText = text;
    leftOfs = 0;
    curPos = fText.length();
    markPos = asMarked ? 0 : curPos;
    limit();
    repaint();
}

mstring YInputLine::getText() {
    return fText;
}

void YInputLine::repaint() {
    if (width() > 1) {
        GraphicsBuffer(this).paint();
    }
}

void YInputLine::configure(const YRect2& r) {
    if (r.resized()) {
        repaint();
    }
}

void YInputLine::paint(Graphics &g, const YRect &/*r*/) {
    YFont font = inputFont;
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
        minOfs = font->textWidth(fText.data(), min) - leftOfs;
        maxOfs = font->textWidth(fText.data(), max) - leftOfs;

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
        int yo = ::max(0, (int(height()) - int(font->height())) / 2);
        int yp = font->ascent() + yo;
        int curOfs = font->textWidth(fText.data(), curPos);
        int cx = ::max(1, curOfs - leftOfs);

        g.setFont(font);

        if (curPos == markPos || !fHasFocus || fText == null) {
            g.setColor(inputFg);
            if (fText != null)
                g.drawChars(fText.data(), 0, textLen, -leftOfs, yp);
            if (fHasFocus && fCursorVisible)
                g.drawLine(cx, yo, cx, font->height() + 2);
        } else {
            if (min > 0) {
                g.setColor(inputFg);
                g.drawChars(fText.data(), 0, min, -leftOfs, yp);
            }
            /// !!! same here
            if (min < max) {
                g.setColor(inputSelectionFg);
                g.drawChars(fText.data(), min, max - min, minOfs, yp);
            }
            if (max < textLen) {
                g.setColor(inputFg);
                g.drawChars(fText.data(), max, textLen - max, maxOfs, yp);
            }
        }
    }
}

bool YInputLine::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        fKeyPressed = k;

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
            case 'u':
            case 'U':
                if ( !deleteSelection())
                    deleteToBegin();
                return true;
            case 'v':
            case 'V':
                requestSelection(false);
                return true;
            case 'w':
            case 'W':
                if ( !deleteSelection())
                    deletePreviousWord();
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
                if (curPos <= textLen) {
                    // advance cursor unless at EOL, where the move is a
                    // no-op BUT it would remove the unwanted text marking
                    if (move(curPos + (curPos < textLen), extend))
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
            if (fListener &&
                ((k == XK_Return && (m & ~ControlMask) == 0) ||
                 (k == XK_KP_Enter && (m & ~ControlMask) == 0) ||
                 (k == XK_j && m == ControlMask) ||
                 (k == XK_m && m == ControlMask)))
            {
                bool control =
                    (k == XK_Return || k == XK_KP_Enter)
                    && (m == ControlMask);
                fListener->inputReturn(this, control);
                return true;
            }
            else {
                const int n = 16;
                wchar_t* s = new wchar_t[n];

                int len = getWCharFromEvent(key, s, n);
                if (len) {
                    replaceSelection(s, len);
                    return true;
                } else {
                    delete[] s;
                }
            }
        }
    }
    else if (key.type == KeyRelease && fListener) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);
        if (k == XK_Escape && k == fKeyPressed && m == 0) {
            fListener->inputEscape(this);
            return true;
        }
    }
    return YWindow::handleKey(key);
}

int YInputLine::getWCharFromEvent(const XKeyEvent& key, wchar_t* s, int maxLen) {
    if (inputContext) {
        KeySym keysym = None;
        Status status = None;
        int len = XwcLookupString(inputContext, const_cast<XKeyEvent*>(&key),
                                  s, maxLen, &keysym, &status);

        if (inrange(len, 0, maxLen - 1)) {
            s[len] = None;
        }
        return len;
    } else {
        int len = 0;
        char buf[16];
        if (getCharFromEvent(key, buf, 16)) {
            len = int(strlen(buf));
            YWideString w(buf, len);
            memcpy(s, w.data(), w.length() * sizeof(wchar_t));
        }
        return len;
    }
}

void YInputLine::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonPress) {
        if (button.button == 1) {
            if (fHasFocus == false) {
                setWindowFocus();
                requestFocus(false);
            } else {
                fSelecting = true;
                curPos = markPos = offsetToPos(button.x + leftOfs);
                limit();
                repaint();
            }
        }
    } else if (button.type == ButtonRelease) {
        autoScroll(0, nullptr);
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
    if (up.button == 3 && xapp->isButton(up.state, Button3Mask)) {
        inputMenu->setActionListener(this);
        inputMenu->popup(this, nullptr, this, up.x_root, up.y_root,
                         YPopupWindow::pfCanFlipVertical |
                         YPopupWindow::pfCanFlipHorizontal);
        inputMenu->setPopDownListener(this);
    } else if (up.button == 2 && xapp->isButton(up.state, Button2Mask)) {
        requestSelection(true);
    } else if (up.button == 1 && xapp->isButton(up.state, Button1Mask)) {
        if (fHasFocus == false)
            gotFocus();
    }
}

void YInputLine::handleSelection(const XSelectionEvent &selection) {
    if (selection.property != None) {
        YProperty prop(selection.requestor, selection.property,
                       F8, 32 * 1024, selection.target, True);
        if (prop) {
            char* data = prop.data<char>();
            int size = int(prop.size());
            for (int i = size; 0 < i--; ) {
                if (data[i] == '\n')
                    data[i] = ' ';
            }
            replaceSelection(data, size);
        }
    }
}

unsigned YInputLine::offsetToPos(int offset) {
    YFont font = inputFont;
    int ofs = 0, pos = 0;
    int textLen = fText.length();

    if (font != null) {
        while (pos < textLen) {
            ofs += font->textWidth(fText.data() + pos, 1);
            if (ofs < offset)
                pos++;
            else
                break;
        }
    }
    return pos;
}

void YInputLine::handleFocus(const XFocusChangeEvent &focus) {
    if (focus.mode == NotifyGrab || focus.mode == NotifyUngrab)
        return;

    if (focus.type == FocusIn &&
        focus.detail != NotifyPointer &&
        focus.detail != NotifyPointerRoot)
    {
        selectAll();
        gotFocus();
    }
    else if (focus.type == FocusOut/* && fHasFocus == true*/) {
        lostFocus();
        if (inputMenu && inputMenu == xapp->popup()) {
        }
        else if (fListener) {
            fListener->inputLostFocus(this);
        }
    }
}

void YInputLine::handlePopDown(YPopupWindow *popup) {
    inputMenu = null;
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

    YFont font = inputFont;
    if (font != null) {
        int curOfs = font->textWidth(fText.data(), curPos);
        int curLen = font->textWidth(fText.data(), textLen);

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

void YInputLine::replaceSelection(const char* insert, int amount) {
    unsigned from = min(curPos, markPos);
    unsigned to = max(curPos, markPos);
    YWideString wide(insert, amount);
    fText.replace(from, to - from, wide);
    curPos = markPos = from + wide.length();
    limit();
    repaint();
}

void YInputLine::replaceSelection(wchar_t* insert, int amount) {
    unsigned from = min(curPos, markPos);
    unsigned to = max(curPos, markPos);
    YWideString wide(amount, insert);
    fText.replace(from, to - from, wide);
    curPos = markPos = from + wide.length();
    limit();
    repaint();
}

bool YInputLine::deleteSelection() {
    if (hasSelection()) {
        replaceSelection("", 0);
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

bool YInputLine::cutSelection() {
    return copySelection() && deleteSelection();
}

bool YInputLine::copySelection() {
    unsigned min = ::min(curPos, markPos), max = ::max(curPos, markPos);
    if (min < max && fText.length() <= max) {
        YWideString copy(fText.copy(min, max - min));
        xapp->setClipboardText(copy);
        return true;
    } else {
        return false;
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
    mstring mstr(fText);
    if (mstr[0] == '~' && mstr.length() > 1
        && mstr.lastIndexOf(' ') == -1
        && mstr.lastIndexOf('/') == -1) {
        mstring var = mstr.substring(1);
        mstring exp = completeUsername(var);
        if (exp != var) {
            setText(mstr.substring(0, 1) + exp, false);
            return;
        }
    }
    csmart res;

    mstring ignoredPfx;
    if (prefixRegex) {
        regmatch_t full_match;
        if (0 == regexec(prefixRegex, mstr, 1, &full_match, 0)) {
            ignoredPfx = mstr.substring(0, full_match.rm_eo);
            mstr = mstr.substring(full_match.rm_eo);
        }
    }
    int res_count = globit_best(mstr, &res, nullptr, nullptr);
    // directory is not a final match
    if (res_count == 1 && upath(res).dirExists())
        res_count++;
    if (1 <= res_count)
        setText(ignoredPfx + mstring(res), res_count == 1);
    else {
        int i = mstr.lastIndexOf(' ');
        if (i > 0 && size_t(i + 1) < mstr.length()) {
            mstring pre(mstr.substring(0, i + 1));
            mstring sub(mstr.substring(i + 1));
            if (sub[0] == '$' || sub[0] == '~') {
                mstring exp = upath(sub).expand();
                if (exp != sub && exp != null) {
                    setText(ignoredPfx + pre + exp, false);
                }
                else if (sub.indexOf('/') == -1) {
                    mstring var = sub.substring(1);
                    if (var.nonempty()) {
                        if (sub[0] == '$') {
                            exp = completeVariable(var);
                        } else {
                            exp = completeUsername(var);
                        }
                        if (exp != var) {
                            char doti[] = { char(sub[0]), '\0' };
                            setText(ignoredPfx + pre + doti + exp, false);
                        }
                    }
                }
                return;
            }

            YStringArray list;
            if (upath::glob(sub + "*", list, "/S") && list.nonempty()) {
                if (list.getCount() == 1) {
                    mstring found(mstr.substring(0, i + 1) + list[0]);
                    setText(ignoredPfx + found, true);
                } else {
                    int len = 0;
                    for (; list[0][len]; ++len) {
                        char ch = list[0][len];
                        int j = 1;
                        while (j < list.getCount() && ch == list[j][len])
                            ++j;
                        if (j < list.getCount())
                            break;
                    }
                    if (len) {
                        mstring common(list[0], len);
                        mstring found(mstr.substring(0, i + 1) + common);
                        setText(ignoredPfx + found, false);
                    }
                }
            }
        }
        else if (i == -1 && mstr.length() > 1 && mstr.indexOf('/') == -1) {
            if (mstr[0] == '$') {
                mstring var = completeVariable(mstr.substring(1));
                if (var != mstr.substring(1))
                    setText(ignoredPfx + mstr.substring(0, 1) + var, false);
            }
        }
    }
}

static size_t commonPrefix(const char* s, const char* t) {
    size_t i = 0;
    while (s[i] && s[i] == t[i])
        i++;
    return i;
}

mstring YInputLine::completeVariable(mstring var) {
    extern char** environ;
    char* best = nullptr;
    size_t len = 0;
    for (int i = 0; environ[i]; ++i) {
        if ( !strncmp(environ[i], var, var.length())) {
            char* eq = strchr(environ[i], '=');
            size_t k = eq - environ[i];
            if (eq && k >= var.length()) {
                if (best == nullptr) {
                    best = environ[i];
                    len = k;
                } else {
                    size_t c = commonPrefix(best, environ[i]);
                    if (len > c)
                        len = c;
                }
            }
        }
    }
    return (len > var.length()) ? mstring(best, len) : var;
}

mstring YInputLine::completeUsername(mstring var) {
    FILE* fp = fopen("/etc/passwd", "r");
    if (fp == nullptr)
        return var;
    char line[1024], best[1024];
    size_t len = 0;
    *best = '\0';
    while (fgets(line, sizeof line, fp)) {
        if ( !strncmp(line, var, var.length())) {
            char* sep = strchr(line, ':');
            if (sep && sep - line >= (ptrdiff_t) var.length()) {
                *sep = '\0';
                size_t k = sep - line;
                if (*best == '\0') {
                    memcpy(best, line, k + 1);
                    len = k;
                } else {
                    size_t c = commonPrefix(best, line);
                    if (len > c)
                        len = c;
                }
            }
        }
    }
    fclose(fp);
    return (len > var.length()) ? mstring(best, len) : var;
}

bool YInputLine::isFocusTraversable() {
    return true;
}

void YInputLine::gotFocus() {
    fHasFocus = true;
    fCursorVisible = true;
    cursorBlinkTimer->setTimer(fBlinkTime, this, true);

    if (focused() || (YWindow::gotFocus(), focused() == false))
        repaint();

    if (inputContext == nullptr) {
        inputContext =
            XCreateIC(xapp->xim(),
                      XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
                      XNClientWindow, handle(),
                      XNFocusWindow,  handle(),
                      nullptr);
        if (inputContext) {
            unsigned long mask = None;
            const char* name = XGetICValues(inputContext,
                                            XNFilterEvents, &mask, nullptr);
            if (name == nullptr && mask) {
                addEventMask(mask);
            }
        }
    }
    if (inputContext) {
        XSetICFocus(inputContext);
        XwcResetIC(inputContext);
    }
}

void YInputLine::lostFocus() {
    cursorBlinkTimer = null;
    fCursorVisible = false;
    fHasFocus = false;
    if (focused())
        YWindow::lostFocus();
    else
        repaint();

    if (inputContext)
        XUnsetICFocus(inputContext);
}

// vim: set sw=4 ts=4 et:
