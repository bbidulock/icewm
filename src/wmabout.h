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
    osmart<YLabel> fProgTitle;
    osmart<YLabel> fCopyright;
    osmart<YLabel> fThemeName;
    osmart<YLabel> fThemeDescription;
    osmart<YLabel> fThemeAuthor;
    osmart<YLabel> fThemeNameS;
    osmart<YLabel> fThemeDescriptionS;
    osmart<YLabel> fThemeAuthorS;
    osmart<YLabel> fCodeSetS;
    osmart<YLabel> fCodeSet;
    osmart<YLabel> fLanguageS;
    osmart<YLabel> fLanguage;
};

#endif

// vim: set sw=4 ts=4 et:
