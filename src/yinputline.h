#ifndef YINPUT_H
#define YINPUT_H

#include "ytimer.h"
#include "yaction.h"
#include "ypopup.h"
#include "ystring.h"

class YMenu;
class YInputLine;
class YInputMenu;
class YWideString;

class YInputListener {
public:
    virtual void inputReturn(YInputLine* input, bool control) = 0;
    virtual void inputEscape(YInputLine* input) = 0;
    virtual void inputLostFocus(YInputLine* input) = 0;
protected:
    virtual ~YInputListener() {}
};

class YInputLine:
    public YWindow,
    protected YTimerListener,
    private YActionListener,
    private YPopDownListener
{
public:
    YInputLine(YWindow *parent = nullptr, YInputListener *listener = nullptr);
    virtual ~YInputLine();

    void setText(mstring text, bool asMarked);
    mstring getText();
    YFont getFont() const { return inputFont; }
    void setListener(YInputListener* listener) { fListener = listener; }

    virtual void paint(Graphics &g, const YRect &r);
    virtual bool handleKey(const XKeyEvent &key);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void handleClickDown(const XButtonEvent &down, int count);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handlePopDown(YPopupWindow *popup);
    virtual void handleSelection(const XSelectionEvent &selection);
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void configure(const YRect2& r);
    virtual bool isFocusTraversable();
    virtual void lostFocus();
    virtual void gotFocus();
    virtual void repaint();

    bool move(unsigned pos, bool extend);
    bool hasSelection() const { return curPos != markPos; }
    void replaceSelection(const char* str, int len);
    void replaceSelection(wchar_t* str, int len);
    bool deleteSelection();
    bool deleteNextChar();
    bool deletePreviousChar();
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

protected:
    virtual bool handleTimer(YTimer *timer);
private:
    virtual bool handleAutoScroll(const XMotionEvent &mouse);

    void limit();
    void autoScroll(int delta, const XMotionEvent *mouse);
    unsigned offsetToPos(int offset);
    static mstring completeVariable(mstring var);
    static mstring completeUsername(mstring user);
    int getWCharFromEvent(const XKeyEvent& key, wchar_t* s, int maxLen);

    YWideString fText;
    unsigned markPos;
    unsigned curPos;
    int leftOfs;
    int fAutoScrollDelta;
    bool fHasFocus;
    bool fCursorVisible;
    bool fSelecting;
    const short fBlinkTime;
    unsigned fKeyPressed;
    YInputListener* fListener;

    XIC inputContext;
    YFont inputFont;
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
