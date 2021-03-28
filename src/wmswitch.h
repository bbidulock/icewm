#ifndef SWITCH_H
#define SWITCH_H

class YFrameWindow;
class YWindowManager;
class YIcon;

class IClosablePopup
{
public:
    // report true if window was up; close in any case
    virtual bool close() = 0;
    virtual ~IClosablePopup() {}
};

// data model, default implementation is list of windows
class ISwitchItems
{
public:
    virtual void updateList() = 0;
    virtual int getCount() = 0;
    virtual ~ISwitchItems() {}

    // move the focused target up or down and return the new focused element
    virtual int moveTarget(bool zdown) = 0;
    // run closing/destruction process for the pointed object
    virtual void destroyTarget() {}
    // set the target explicitly rather than switching around
    virtual int setTarget(int zPosition) = 0;
    /// Show changed focus preview to user
    virtual void displayFocusChange(int idxFocused) = 0;

    // set target cursor and implementation specific stuff in the beginning
    virtual void begin(bool zdown) = 0;
    // reset the cursor; the apparent state might be inconsistent
    virtual void reset() = 0;
    virtual void cancel() = 0;
    virtual void accept(IClosablePopup* parent) = 0;

    // XXX: convert to iterator
    virtual int getActiveItem() = 0;
    virtual mstring getTitle(int idx) = 0;
    virtual ref<YIcon> getIcon(int idx) = 0;

    // Manager notification about windows disappearing under the fingers
    virtual void destroyedItem(YFrameWindow* framePtr) = 0;

    // Filter items by WM_CLASS
    virtual void setWMClass(char* wmclass) = 0;
    virtual char* getWMClass() = 0;

    virtual YFrameWindow* current() const { return nullptr; }
};

class SwitchWindow: public YPopupWindow, private IClosablePopup {
public:
    SwitchWindow(YWindow* parent, ISwitchItems* items, bool verticalStyle);
    ~SwitchWindow();

    virtual void paint(Graphics& g, const YRect& r) override;
    virtual void repaint() override;

    void begin(bool zdown, unsigned mods, char* wmclass = nullptr);

    virtual void activatePopup(int flags) override;
    virtual void deactivatePopup() override;

    virtual void handleExpose(const XExposeEvent& expose) override {}
    virtual bool handleKey(const XKeyEvent& key) override;
    virtual void handleButton(const XButtonEvent& button) override;
    virtual void handleMotion(const XMotionEvent& motion) override;
    void destroyedFrame(YFrameWindow* frame);
    YFrameWindow* current();

private:
    ISwitchItems* zItems;
    bool m_verticalStyle;
    // backup of user's config, needs to be enforced temporarily
    bool m_oldMenuMouseTracking;
    // remember what was highlighted by mouse tracking
    int m_hlItemFromMotion;
    // hints for fast identification of the entry under the cursor
    int m_hintAreaStart, m_hintAreaStep;

    ref<YImage> fGradient;

    YColorName switchFg;
    YColorName switchBg;
    YColorName switchHl;
    YColorName switchMbg;
    YColorName switchMfg;
    ref<YFont> switchFont;

    unsigned modsDown;
    bool isUp;

    bool modDown(unsigned m);
    bool isModKey(KeyCode c);
    bool isKey(KeySym k, unsigned mod);
    bool target(int delta);

    void resize(int xiscreen);
    unsigned modifiers();

    void cancel();
    virtual bool close() override;
    void accept();
    void displayFocus(int itemIdx);
    YFrameWindow* nextWindow(bool zdown);

    void paintHorizontal(Graphics& g);
    void paintVertical(Graphics& g);
    // -1: no hint, -2: hinting not supported
    int calcHintedItem(int x, int y);

private: // not-used
    SwitchWindow(const SwitchWindow &);
    SwitchWindow& operator=(const SwitchWindow &);
};

#endif

// vim: set sw=4 ts=4 et:
