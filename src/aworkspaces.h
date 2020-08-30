#ifndef AWORKSPACES_H_
#define AWORKSPACES_H_

#include "ybutton.h"
#include "yinputline.h"

class WorkspaceDragger {
 public:
    virtual void drag(int ws, int dx, bool start, bool end) = 0;
    virtual void stopDrag() = 0;
    virtual void relabel(int ws) = 0;
    virtual unsigned width() const = 0;
    virtual ~WorkspaceDragger() {}
};

class WorkspaceButton:
    public YButton,
    private YTimerListener,
    private YInputListener
{
    typedef YButton super;

public:
    WorkspaceButton(int workspace, YWindow *parent, WorkspaceDragger *dragger);
    int workspace() const { return fWorkspace; }
    const char* name() const;
    void updateName();
    mstring baseName();
    void setPosition(int x, int y);
    int extent() const { return x() + int(width()); }
    virtual void repaint();
    static void freeFonts() { normalButtonFont = null; activeButtonFont = null; }

private:
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void handleCrossing(const XCrossingEvent &crossing);

    virtual void handleDNDEnter();
    virtual void handleDNDLeave();
    virtual void handleBeginDrag(const XButtonEvent&, const XMotionEvent&);
    virtual void handleDrag(const XButtonEvent&, const XMotionEvent&);
    virtual void handleEndDrag(const XButtonEvent&, const XButtonEvent&);
    virtual void handleExpose(const XExposeEvent& expose) {}
    virtual void configure(const YRect2& r);
    virtual bool handleTimer(YTimer *t);

    virtual void actionPerformed(YAction button, unsigned int modifiers);
    virtual ref<YFont> getActiveFont();
    virtual ref<YFont> getFont();
    virtual YColor   getColor();
    virtual YSurface getSurface();
    virtual YDimension getTextSize();

    virtual void inputReturn(YInputLine* input);
    virtual void inputEscape(YInputLine* input);
    virtual void inputLostFocus(YInputLine* input);
    virtual void paint(Graphics &g, const YRect &r);

    int fWorkspace;
    int fDelta;
    int fDownX;
    bool fDragging;
    GraphicsBuffer fGraphics;
    lazy<YTimer> fRaiseTimer;
    osmart<YInputLine> fInput;
    WorkspaceDragger* fPane;

    static YColorName normalButtonBg;
    static YColorName normalBackupBg;
    static YColorName normalButtonFg;

    static YColorName activeButtonBg;
    static YColorName activeBackupBg;
    static YColorName activeButtonFg;

    static ref<YFont> normalButtonFont;
    static ref<YFont> activeButtonFont;
};

class WorkspaceIcons {
    YStringArray files;
 public:
    WorkspaceIcons();
    ref<YImage> load(const char* name);
};

class AWorkspaces : public YWindow {
public:
    AWorkspaces(YWindow *parent) : YWindow(parent) {
        addStyle(wsNoExpose);
        setParentRelative();
    }
    virtual ~AWorkspaces() {}

    virtual void repaint() {}
    virtual void relabelButtons() {}
    virtual void setPressed(long ws, bool set) {}
    virtual void updateButtons() {}
};

class WorkspacesPane:
    public AWorkspaces,
    private YTimerListener,
    private WorkspaceDragger
{
    typedef AWorkspaces super;

public:
    WorkspacesPane(YWindow *parent);
    ~WorkspacesPane() { WorkspaceButton::freeFonts(); }

    virtual void repaint();
    virtual void relabelButtons();
    virtual void setPressed(long ws, bool set);
    virtual void updateButtons();
    virtual unsigned width() const { return YWindow::width(); }

private:
    typedef YObjectArray<WorkspaceButton> ArrayType;
    typedef ArrayType::IterType IterType;

    int fActive;
    int fDelta;
    int fMoved;
    double fSpeed;
    timeval fTime;
    long const fMillis;
    bool fRepositioning;
    bool fReconfiguring;
    bool fRepaintSpaces;
    lazy<YTimer> fDragTimer;
    lazy<YTimer> fRepaintTimer;
    lazy<WorkspaceIcons> paths;
    ArrayType fButtons;
    int count() const { return fButtons.getCount(); }
    IterType iterator() { return fButtons.iterator(); }
    WorkspaceButton* last() const { return fButtons[count()-1]; }
    int extent() const { return 0 < count() ? last()->extent() : 0; }

    WorkspaceButton* create(int workspace, unsigned height);
    void label(WorkspaceButton* wk);
    void repositionButtons();
    void resize(unsigned width, unsigned height);
    long limitWidth(long paneWidth);

    virtual void drag(int ws, int dx, bool start, bool end);
    virtual void stopDrag();
    virtual void relabel(int ws);
    virtual bool handleTimer(YTimer *t);
    virtual void configure(const YRect2 &r);
    virtual void handleExpose(const XExposeEvent &e) {}
};

extern YColorName taskBarBg;

#endif

// vim: set sw=4 ts=4 et:
