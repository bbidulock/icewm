#ifndef __YMSGBOX_H
#define __YMSGBOX_H

#include "ydialog.h"
#include "ylabel.h"
#include "yactionbutton.h"

class YMsgBox;

class YMsgBox:
public YDialog,
public YAction::Listener {
public:
    class Listener {
    public:
        virtual void handleMsgBox(YMsgBox *msgbox, int operation) = 0;
    };

    YMsgBox(int buttons, YWindow *owner = 0);
    virtual ~YMsgBox();

    void setTitle(const char *title);
    void setText(const char *text);
    void setPixmap(YPixmap *pixmap);

    void msgBoxListener(Listener *listener) { fListener = listener; }
    Listener *msgBoxListener() const { return fListener; }

    void actionPerformed(YAction *action, unsigned int modifiers);
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

    Listener *fListener;
};

#endif
