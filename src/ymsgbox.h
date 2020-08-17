#ifndef YMSGBOX_H
#define YMSGBOX_H

#include "ydialog.h"
#include "yactionbutton.h"

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

class YMsgBox: public YDialog, private YActionListener {
public:
    YMsgBox(int buttons);
    virtual ~YMsgBox();

    void setTitle(mstring title);
    void setText(mstring text);
    void setPixmap(ref<YPixmap> pixmap);

    void setMsgBoxListener(YMsgBoxListener *listener) { fListener = listener; }
    YMsgBoxListener *getMsgBoxListener() const { return fListener; }
    YInputLine* input() const { return fInput; }

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleClose();
    virtual void handleFocus(const XFocusChangeEvent &focus);

    enum {
        mbOK = 0x1,
        mbCancel = 0x2,
        mbInput = 0x4,
    };

    void showFocused();
    void autoSize();

private:
    YLabel* fLabel;
    YInputLine* fInput;
    YActionButton* fButtonOK;
    YActionButton* fButtonCancel;
    YMsgBoxListener* fListener;
};

#endif

// vim: set sw=4 ts=4 et:
