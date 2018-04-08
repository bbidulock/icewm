#ifndef __YMSGBOX_H
#define __YMSGBOX_H

#include "ydialog.h"
#include "ylabel.h"
#include "yactionbutton.h"

class YMsgBox;

class YMsgBoxListener {
public:
    virtual void handleMsgBox(YMsgBox *msgbox, int operation) = 0;
protected:
    virtual ~YMsgBoxListener() {}
};

class YMsgBox: public YDialog, public YActionListener {
public:
    YMsgBox(int buttons, YWindow *owner = 0);
    virtual ~YMsgBox();

    void setTitle(const ustring &title);
    void setText(const ustring &text);
    void setPixmap(ref<YPixmap> pixmap);

    void setMsgBoxListener(YMsgBoxListener *listener) { fListener = listener; }
    YMsgBoxListener *getMsgBoxListener() const { return fListener; }

    virtual void actionPerformed(YAction action, unsigned int modifiers);
    virtual void handleClose();
    virtual void handleFocus(const XFocusChangeEvent &focus);

    enum {
        mbOK = 0x1,
        mbCancel = 0x2
    };

    void showFocused();

    void autoSize();

private:
    YLabel *fLabel;
    YActionButton *fButtonOK;
    YActionButton *fButtonCancel;

    int addButton(YButton *button);

    YMsgBoxListener *fListener;
};

#endif

// vim: set sw=4 ts=4 et:
