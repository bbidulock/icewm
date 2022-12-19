#ifndef WMABOUT_H
#define WMABOUT_H

#include "ydialog.h"
#include "yaction.h"

class YLabel;
class Sizeable;

class AboutDlg: public YDialog {
public:
    AboutDlg(YActionListener* al = nullptr);
    virtual ~AboutDlg();

    void showFocused();
    virtual void handleClose();

private:
    YLabel* label(const char* text);
    Sizeable* fSizeable;
    YActionListener* fListener;
};

#endif

// vim: set sw=4 ts=4 et:
