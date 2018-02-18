#ifndef __YINPUT_H
#define __YINPUT_H

#include "ywindow.h"
#include "ytimer.h"
#include "yaction.h"
#include "ypointer.h"

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
    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleSelection(const XSelectionEvent &selection);

    bool move(unsigned pos, bool extend);
    bool hasSelection() const { return (curPos != markPos) ? true : false; }
    void replaceSelection(const ustring &str);
    bool deleteSelection();
    bool deleteNextChar();
    bool deletePreviousChar();
    bool insertChar(char ch);
    unsigned nextWord(unsigned pos, bool sep);
    unsigned prevWord(unsigned pos, bool sep);
    bool deleteNextWord();
    bool deletePreviousWord();
    bool deleteToEnd();
    bool deleteToBegin();
    void selectAll();
    void unselectAll();
    void cutSelection();
    void copySelection();
    void complete();

private:
    virtual bool handleTimer(YTimer *timer);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    void limit();
    void autoScroll(int delta, const XMotionEvent *mouse);
    unsigned offsetToPos(int offset);

    ustring fText;
    unsigned markPos;
    unsigned curPos;
    int leftOfs;
    int fAutoScrollDelta;
    bool fHasFocus;
    bool fCursorVisible;
    bool fSelecting;
    const short fBlinkTime;

    ref<YFont> inputFont;
    YColorName inputBg;
    YColorName inputFg;
    YColorName inputSelectionBg;
    YColorName inputSelectionFg;
    lazy<YTimer> cursorBlinkTimer;
    osmart<YMenu> inputMenu;

    YAction actionCut;
    YAction actionCopy;
    YAction actionPaste;
    YAction actionSelectAll;
    YAction actionPasteSelection;

private: // not-used
    YInputLine(const YInputLine &);
    YInputLine &operator=(const YInputLine &);

};

#endif

// vim: set sw=4 ts=4 et:
