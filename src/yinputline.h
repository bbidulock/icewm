#ifndef __YINPUT_H
#define __YINPUT_H

#include "ywindow.h"
#include "ytimer.h"
#include "yaction.h"
#include "ypointer.h"

class YMenu;
class YInputLine;
class YInputMenu;

class YInputListener {
public:
    virtual void inputReturn(YInputLine* input) = 0;
    virtual void inputEscape(YInputLine* input) = 0;
    virtual void inputLostFocus(YInputLine* input) = 0;
protected:
    virtual ~YInputListener() {}
};

class YInputLine: public YWindow, public YTimerListener, public YActionListener {
public:
    YInputLine(YWindow *parent = nullptr, YInputListener *listener = nullptr);
    virtual ~YInputLine();

    void setText(const mstring &text, bool asMarked);
    mstring getText();
    ref<YFont> getFont() const { return inputFont; }
    void setListener(YInputListener* listener) { fListener = listener; }

    virtual void paint(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClickDown(const XButtonEvent &down, int count);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleSelection(const XSelectionEvent &selection);
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void configure(const YRect2& r);
    virtual void repaint();

    bool move(unsigned pos, bool extend);
    bool hasSelection() const { return (curPos != markPos) ? true : false; }
    void replaceSelection(const mstring &str);
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
    bool cutSelection();
    bool copySelection();
    void complete();

private:
    virtual bool handleTimer(YTimer *timer);
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    void limit();
    void autoScroll(int delta, const XMotionEvent *mouse);
    unsigned offsetToPos(int offset);

    mstring fText;
    unsigned markPos;
    unsigned curPos;
    int leftOfs;
    int fAutoScrollDelta;
    bool fHasFocus;
    bool fCursorVisible;
    bool fSelecting;
    const short fBlinkTime;
    YInputListener* fListener;

    ref<YFont> inputFont;
    YColorName inputBg;
    YColorName inputFg;
    YColorName inputSelectionBg;
    YColorName inputSelectionFg;
    lazy<YTimer> cursorBlinkTimer;
    lazy<YInputMenu> inputMenu;

private: // not-used
    YInputLine(const YInputLine &);
    YInputLine &operator=(const YInputLine &);

};

#endif

// vim: set sw=4 ts=4 et:
