#ifndef __ABOUT_H
#define __ABOUT_H

#include "ydialog.h"
#include "ylabel.h"

class AboutDlg: public YDialog {
public:
    AboutDlg();

    void autoSize();
    void showFocused();
    virtual void handleClose();
private:
    YLabel *fProgTitle;
    YLabel *fCopyright;
#if 0
    YLabel *fThemeName;
    YLabel *fThemeDescription;
    YLabel *fThemeAuthor;
    YLabel *fThemeNameS;
    YLabel *fThemeDescriptionS;
    YLabel *fThemeAuthorS;
#endif
};

#endif
