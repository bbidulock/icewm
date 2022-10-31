#ifndef SWITCH_H
#define SWITCH_H

class YFrameWindow;
class YWindowManager;
class YIcon;

// data model, default implementation is list of windows
class ISwitchItems
{
public:
    virtual void updateList() = 0;
    virtual int getCount() = 0;
    virtual bool isEmpty() = 0;
    virtual ~ISwitchItems() {}

    // move the focused target up or down and return the new focused element
    virtual void moveTarget(bool zdown) = 0;
    // run closing/destruction process for the pointed object
    virtual void destroyTarget() {}
    // set the target explicitly rather than switching around
    virtual void setTarget(int zPosition) = 0;
    /// Show changed focus preview to user
    virtual void displayFocusChange() { }

    // set target cursor and setup
    virtual void begin(bool zdown) = 0;
    virtual void cancel() = 0;
    virtual void accept() = 0;

    virtual int getActiveItem() = 0;
    virtual mstring getTitle(int idx) = 0;
    virtual ref<YIcon> getIcon(int idx) = 0;

    // Manager notification about windows disappearing under the fingers
    virtual bool destroyedItem(YFrameWindow* frame, YFrameClient* client) {
                    return false; }
    virtual bool createdItem(YFrameWindow* frame, YFrameClient* client) {
                    return false; }
    virtual void transfer(YFrameClient* client, YFrameWindow* frame) { }

    virtual bool isKey(KeySym k, unsigned mod) = 0;
    virtual unsigned modifiers() = 0;

    // Filter items by WM_CLASS
    virtual void setWMClass(char* wmclass) = 0;
    virtual char* getWMClass() = 0;

    virtual YFrameWindow* current() const { return nullptr; }
    virtual void sort() { }
};

class SwitchWindow: public YPopupWindow {
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
    void destroyedClient(YFrameClient* client);
    void destroyedFrame(YFrameWindow* frame);
    void createdFrame(YFrameWindow* frame);
    void createdClient(YFrameWindow* frame, YFrameClient* client);
    void transfer(YFrameClient* client, YFrameWindow* frame);
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

    int fWorkspace;
    ref<YImage> fGradient;

    YColorName switchFg;
    YColorName switchBg;
    YColorName switchHl;
    YColorName switchMbg;
    YColorName switchMfg;
    YFont switchFont;

    unsigned modsDown;

    bool modDown(unsigned m);
    bool isModKey(KeyCode c);
    bool isKey(KeySym k, unsigned mod);
    void target(int delta);

    void resize(int xiscreen, bool reposition);
    unsigned modifiers();

    void cancel();
    void close();
    void accept();
    void displayFocus();
    YFrameWindow* nextWindow(bool zdown);

    void paintHorizontal(Graphics& g);
    void paintVertical(Graphics& g);
    // -1: no hint, -2: hinting not supported
    int hintedItem(int x, int y);

private: // not-used
    SwitchWindow(const SwitchWindow &);
    SwitchWindow& operator=(const SwitchWindow &);
};

#endif

// vim: set sw=4 ts=4 et:
