#ifndef __YINPUT_H
#define __YINPUT_H

#include "ywindow.h"
#include "ytimer.h"
#include "yaction.h"

class YMenu;

class YInputLine: public YWindow, public YTimerListener, public YActionListener {
public:
    YInputLine(YWindow *parent = 0);
    virtual ~YInputLine();

    void setText(const ustring &text);
    ustring getText();

    virtual void paint(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClickDown(const XButtonEvent &down, int count);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handleSelection(const XSelectionEvent &selection);

    bool move(int pos, bool extend);
    bool hasSelection() const { return (curPos != markPos) ? true : false; }
    void replaceSelection(const ustring &str);
    bool deleteSelection();
    bool deleteNextChar();
    bool deletePreviousChar();
    bool insertChar(char ch);
    int nextWord(int pos, bool sep);
    int prevWord(int pos, bool sep);
    bool deleteNextWord();
    bool deletePreviousWord();
    bool deleteToEnd();
    bool deleteToBegin();
    void selectAll();
    void unselectAll();
    void cutSelection();
    void copySelection();

private:
    ustring fText;
    int markPos;
    int curPos;
    int leftOfs;
    bool fHasFocus;
    bool fCursorVisible;
    bool fSelecting;

    static int fAutoScrollDelta;

    void limit();
    int offsetToPos(int offset);
    void autoScroll(int delta, const XMotionEvent *mouse);
    virtual bool handleTimer(YTimer *timer);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    static YColor *inputBg;
    static YColor *inputFg;
    static YColor *inputSelectionBg;
    static YColor *inputSelectionFg;
    static ref<YFont> inputFont;
    static YTimer *cursorBlinkTimer;
    static YMenu *inputMenu;

private: // not-used
    YInputLine(const YInputLine &);
    YInputLine &operator=(const YInputLine &);

};

#endif
