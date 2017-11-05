#ifndef __ABOUT_H
#define __ABOUT_H

#include "ydialog.h"
#include "ylabel.h"

class AboutDlg: public YDialog {
public:
    AboutDlg();
    ~AboutDlg();

    void autoSize();
    void showFocused();
    virtual void handleClose();
private:
    YLabel *fProgTitle;
    YLabel *fCopyright;
    YLabel *fThemeName;
    YLabel *fThemeDescription;
    YLabel *fThemeAuthor;
    YLabel *fThemeNameS;
    YLabel *fThemeDescriptionS;
    YLabel *fThemeAuthorS;
    YLabel *fCodeSetS;
    YLabel *fCodeSet;
    YLabel *fLanguageS;
    YLabel *fLanguage;
};

#endif

// vim: set sw=4 ts=4 et:
