#ifndef __YINPUT_H
#define __YINPUT_H

#include "ywindow.h"
#include "ytimer.h"
#include "yaction.h"
#include "yconfig.h"

#pragma interface

class YMenu;

class YInputLine: public YWindow, public YTimerListener, public YActionListener {
public:
    YInputLine(YWindow *parent = 0);
    virtual ~YInputLine();

    void setText(const char *text);
    const char *getText();

    virtual void paint(Graphics &g, const YRect &er);
    virtual bool eventKey(const YKeyEvent &key);
    virtual bool eventButton(const YButtonEvent &button);
    virtual bool eventMotion(const YMotionEvent &motion);
    virtual bool eventClickDown(const YClickEvent &down);
    virtual bool eventClick(const YClickEvent &up);
    virtual bool eventFocus(const YFocusEvent &focus);
    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handleSelection(const XSelectionEvent &selection);

    bool move(int pos, bool extend);
    bool hasSelection() const { return (curPos != markPos) ? true : false; }
    void replaceSelection(const char *str, int len);
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
    char *fText;
    int markPos;
    int curPos;
    int leftOfs;
    bool fHasFocus;
    bool fCursorVisible;
    bool fSelecting;

    static int fAutoScrollDelta;

    void limit();
    int offsetToPos(int offset);
    void autoScroll(int delta, const YMotionEvent *mouse);
    virtual bool handleTimer(YTimer *timer);
    virtual bool handleAutoScroll(const YMotionEvent &mouse);

    static YColorPrefProperty inputBg;
    static YColorPrefProperty inputFg;
    static YColorPrefProperty inputSelectionBg;
    static YColorPrefProperty inputSelectionFg;
    static YFontPrefProperty gInputFont;

    static YTimer *cursorBlinkTimer;
    static YMenu *gInputMenu;

    YMenu *getInputMenu();
private: // not-used
    YInputLine(const YInputLine &);
    YInputLine &operator=(const YInputLine &);

};

#endif
