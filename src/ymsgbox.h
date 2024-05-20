#ifndef YMSGBOX_H
#define YMSGBOX_H

#include "ydialog.h"
#include "yactionbutton.h"
#include "yinputline.h"

class YMsgBox;
class YLabel;
class YInputLine;
class YInputListener;

class YMsgBoxListener {
public:
    virtual void handleMsgBox(YMsgBox *msgbox, int operation) = 0;
protected:
    virtual ~YMsgBoxListener() {}
};

class YMsgBox:
    public YDialog,
    private YActionListener,
    private YInputListener
{
public:
    YMsgBox(int buttons,
            const char* title = nullptr,
            const char* text = nullptr,
            YMsgBoxListener* listener = nullptr,
            const char* iconName = nullptr);
    virtual ~YMsgBox();

    void setText(const char* text);
    void setPixmap(ref<YPixmap> pixmap);

    void setMsgBoxListener(YMsgBoxListener *listener) { fListener = listener; }
    YMsgBoxListener *getMsgBoxListener() const { return fListener; }
    YInputLine* input() const { return fInput; }

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void paint(Graphics &g, const YRect& r);
    virtual void handleClose();
    virtual void handleFocus(const XFocusChangeEvent &focus);
    virtual void inputReturn(YInputLine* input, bool control);
    virtual void inputEscape(YInputLine* input);
    virtual void inputLostFocus(YInputLine* input);

    enum {
        mbClose  = 0x0,
        mbOK     = 0x1,
        mbCancel = 0x2,
        mbBoth   = 0x3,
        mbInput  = 0x4,
        mbAll    = 0x7,
    };

    void showFocused();
    void autoSize();

private:
    YLabel* fLabel;
    YInputLine* fInput;
    YActionButton* fButtonOK;
    YActionButton* fButtonCancel;
    YMsgBoxListener* fListener;
    ref<YPixmap> fPixmap;
};

#endif

// vim: set sw=4 ts=4 et:
