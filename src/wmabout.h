#ifndef WMABOUT_H
#define WMABOUT_H

#include "ydialog.h"

class YLabel;
class Sizeable;

class AboutDlg: public YDialog {
public:
    AboutDlg();
    ~AboutDlg();

    void showFocused();
    virtual void handleClose();

private:
    YLabel* label(const char* text);
    Sizeable* fSizeable;
};

#endif

// vim: set sw=4 ts=4 et:
