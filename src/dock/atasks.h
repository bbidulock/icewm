#ifndef ATASKS_H_
#define ATASKS_H_

#include "ywindow.h"
//#include "wmclient.h"
#include "ytimer.h"
#include "yconfig.h"

class TaskBarApp;
class YWindowManager;
class WindowInfo;
class DesktopInfo;

class TaskBarApp: public YWindow, public YTimerListener {
public:
    TaskBarApp(WindowInfo *frame, YWindow *aParent);
    virtual ~TaskBarApp();

    virtual bool isFocusTraversable();

    virtual void paint(Graphics &g, const YRect &er);
    virtual bool eventButton(const YButtonEvent &button);
    virtual bool eventClick(const YClickEvent &up);
    virtual bool eventCrossing(const YCrossingEvent &crossing);
    virtual void handleDNDEnter(int nTypes, XAtomId *types);
    virtual void handleDNDLeave();
    virtual bool handleTimer(YTimer *t);

    WindowInfo *getFrame() const { return fFrame; }

    void setShown(bool show);
    bool getShown() const { return fShown; }
    
    TaskBarApp *getNext() const { return fNext; }
    TaskBarApp *getPrev() const { return fPrev; }
    void setNext(TaskBarApp *next) { fNext = next; }
    void setPrev(TaskBarApp *prev) { fPrev = prev; }

#warning "cleanup needed"
    bool isMarked() { return fMarked; }
    void mark(bool mark = true) { fMarked = mark; }
    bool fMarked;

private:
    WindowInfo *fFrame;
    TaskBarApp *fPrev, *fNext;
    bool fShown;
    int selected;
    static YTimer fRaiseTimer;

    static YColorPrefProperty gNormalAppBg;
    static YColorPrefProperty gNormalAppFg;
    static YColorPrefProperty gActiveAppBg;
    static YColorPrefProperty gActiveAppFg;
    static YColorPrefProperty gMinimizedAppBg;
    static YColorPrefProperty gMinimizedAppFg;
    static YColorPrefProperty gInvisibleAppBg;
    static YColorPrefProperty gInvisibleAppFg;
    static YFontPrefProperty gNormalFont;
    static YFontPrefProperty gActiveFont;
#warning "fix taskbar background styles"
    static YPixmapPrefProperty gPixmapTaskBarBackground;
    static YPixmapPrefProperty gPixmapNormalBackground;
    static YPixmapPrefProperty gPixmapActiveBackground;
    static YPixmapPrefProperty gPixmapMinimizedBackground;
};

class TaskPane: public YWindow {
public:
    TaskPane(DesktopInfo *root, YWindow *parent);
    ~TaskPane();

    TaskBarApp *addApp(WindowInfo *frame);
    void removeApp(WindowInfo *frame);
    TaskBarApp *getFirst() const { return fFirst; }
    TaskBarApp *getLast() const { return fLast; }

    void relayout() { fNeedRelayout = true; }
    void relayoutNow();

    void unmark();
    void removeUnmarked();

    virtual bool eventClick(const YClickEvent &up);
    virtual void paint(Graphics &g, const YRect &er);
private:
    void insert(TaskBarApp *tapp);
    void remove(TaskBarApp *tapp);

    DesktopInfo *fRoot;
    TaskBarApp *fFirst, *fLast;
    int fCount;
    bool fNeedRelayout;

    YPref fShowAllWindows;

    static YColorPrefProperty gTaskBarBg;
};

#endif
