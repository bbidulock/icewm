#ifndef __YMSGBOX_H
#define __YMSGBOX_H

#include "ydialog.h"
#include "ylabel.h"
#include "yactionbutton.h"

class YMsgBox;

class YMsgBoxListener {
public:
    virtual void handleMsgBox(YMsgBox *msgbox, int operation) = 0;
};

class YMsgBox: public YDialog, public YActionListener {
public:
    YMsgBox(int buttons, YWindow *owner = 0);
    virtual ~YMsgBox();

    void setTitle(const char *title);
    void setText(const char *text);
    void setPixmap(YPixmap *pixmap);

    void setMsgBoxListener(YMsgBoxListener *listener) { fListener = listener; }
    YMsgBoxListener *getMsgBoxListener() const { return fListener; }

    void actionPerformed(YAction *action, unsigned int modifiers);
    virtual void handleClose();
    virtual void handleFocus(const XFocusChangeEvent &focus);

    enum {
        mbOK = 0x1,
        mbCancel = 0x2
    };

    void showFocused();

    void getMaxButton(unsigned int &w, unsigned int &h);
    void autoSize();

private:
    YLabel *fLabel;
    YActionButton *fButtonOK;
    YActionButton *fButtonCancel;

    int addButton(YButton *button);

    YMsgBoxListener *fListener;
private: // not-used
    YMsgBox(const YMsgBox &);
    YMsgBox &operator=(const YMsgBox &);
};

#endif
