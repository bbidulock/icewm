#ifndef __YTOPWINDOW_H
#define __YTOPWINDOW_H

#include "ywindow.h"
#include "MwmUtil.h"

#pragma interface

typedef struct XWMHints;
typedef struct XClassHint;
struct MwmHints;
class YIcon;

class YTopWindow: public YWindow {
public:
    YTopWindow();
    ~YTopWindow();

    void setIcon(YIcon *icon);
    void setWMHints(const XWMHints &wmh);
    void setClassHint(const XClassHint &clh);
    void setMwmHints(const MwmHints &mwm);
    void setTitle(const char *title);
    void setIconTitle(const char *iconTitle);
    void setResizeable(bool canResize);

    virtual void configure(const YRect &cr);

private:
    bool fCanResize;

};

#endif
