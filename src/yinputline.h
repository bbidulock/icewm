#ifndef __YINPUT_H
#define __YINPUT_H

#include "ywindow.h"
#include "ytimer.h"
#include "yaction.h"

class YMenu;
class YHistory;

class YInputLine: public YWindow, public YTimerListener, public YActionListener {
public:
    YInputLine(YWindow *parent = 0, char const *historyId = NULL);
    virtual ~YInputLine();

    void setText(const char *text);
    const char *getText() const { return fText; }
    YHistory *getHistory() const { return fHistory; }

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClickDown(const XButtonEvent &down, int count);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handleSelection(const XSelectionEvent &selection);

    bool move(int pos, bool extend);
    bool hasSelection() const { return (fCurPos != fSelPos); }
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
    int fCurPos;
    int fSelPos;
    int fLeftOfs;
    bool fHasFocus;
    bool fCursorVisible;
    bool fSelecting;

    YHistory * fHistory;
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
    static YFont *inputFont;
    static YTimer *cursorBlinkTimer;
    static YMenu *inputMenu;
};

#endif
