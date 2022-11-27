#ifndef YCLIENT_H
#define YCLIENT_H

#include "ywindow.h"
#include "ymenu.h"
#include "MwmUtil.h"
#include "wmoption.h"
#ifndef InputHint
#include <X11/Xutil.h>
#endif

class YFrameWindow;
class YClientContainer;
class YIcon;

typedef int FrameState;

enum WindowType {
    wtCombo,
    wtDesktop,
    wtDialog,
    wtDND,
    wtDock,
    wtDropdownMenu,
    wtMenu,
    wtNormal,
    wtNotification,
    wtPopupMenu,
    wtSplash,
    wtToolbar,
    wtTooltip,
    wtUtility,
};

class ClassHint : public XClassHint {
public:
    ClassHint() { res_name = res_class = nullptr; }
    ClassHint(const char* name, const char* klas) { init(name, klas); }
    ClassHint(const ClassHint& ch) { init(ch.res_name, ch.res_class); }
    ~ClassHint() { reset(); }
    void init(const char* name, const char* klas) {
        res_name = name ? strdup(name) : nullptr;
        res_class = klas ? strdup(klas) : nullptr;
    }
    void reset() {
        if (res_name) { XFree(res_name); res_name = nullptr; }
        if (res_class) { XFree(res_class); res_class = nullptr; }
    }
    bool match(const char* resource) const;
    char* resource() const;
    void operator=(const ClassHint& hint) {
        if (this != &hint) {
            reset();
            init(hint.res_name, hint.res_class);
        }
    }
    bool operator==(const ClassHint& hint) {
        return ((res_name && hint.res_name) ?
                !strcmp(res_name, hint.res_name) :
                res_name == hint.res_name) &&
               ((res_class && hint.res_class) ?
                !strcmp(res_class, hint.res_class) :
                res_class == hint.res_class);
    }
    bool operator!=(const ClassHint& hint) {
        return !operator==(hint);
    }
    bool nonempty() {
        return ::nonempty(res_name) || ::nonempty(res_class);
    }
    bool get(Window win);
};

/*
 * X11 time state to support _NET_WM_USER_TIME.
 * Keep track of the time in seconds when we receive a X11 time stamp.
 * Only compare two X11 time stamps if they are in a time interval.
 */
class UserTime {
private:
    unsigned long xtime;
    bool valid;
    unsigned long since;
    enum : unsigned long {
        XTimeMask = 0xFFFFFFFFUL,       // milliseconds
        XTimeRange = 0x7FFFFFFFUL,      // milliseconds
        SInterval = 0x3FFFFFFFUL / 1000,     // seconds
    };
public:
    UserTime() : xtime(0), valid(false), since(0) { }
    explicit UserTime(unsigned long xtime, bool valid = true) :
        xtime(xtime & XTimeMask), valid(valid), since(seconds()) { }
    unsigned long time() const { return xtime; }
    bool good() const { return valid; }
    long when() const { return since; }
    bool update(unsigned long xtime, bool valid = true) {
        UserTime u(xtime, valid);
        return *this < u || xtime == 0 ? (*this = u, true) : false;
    }
    bool operator<(const UserTime& u) const {
        if (since == 0 || u.since == 0) return u.since != 0;
        if (valid == false || u.valid == false) return u.valid;
        if (since < u.since && u.since - since > SInterval) return true;
        if (since > u.since && since - u.since > SInterval) return false;
        if (xtime < u.xtime) return u.xtime - xtime <= XTimeRange;
        if (xtime > u.xtime) return xtime - u.xtime >  XTimeRange;
        return false;
    }
    bool operator==(const UserTime& u) const {
        return !(*this < u) && !(u < *this);
    }
};

class ClientData {
public:
    virtual ref<YIcon> getIcon() const = 0;
    virtual mstring getTitle() const = 0;
    virtual mstring getIconTitle() const = 0;
    virtual void activateWindow(bool raise, bool curWork) = 0;
    virtual bool isMinimized() const = 0;
    virtual bool focused() const = 0;
    virtual bool visibleNow() const = 0;
    virtual bool canRaise(bool ignoreTaskBar = false) const = 0;
    virtual void wmClose() = 0;
    virtual void wmRaise() = 0;
    virtual void wmLower() = 0;
    virtual void wmMinimize() = 0;
    virtual int getTrayOrder() const = 0;
    virtual void wmOccupyCurrent() = 0;
    virtual void popupSystemMenu(YWindow *owner, int x, int y, unsigned flags,
                                 YWindow *forWindow = nullptr) = 0;
    virtual ClassHint* classHint() const = 0;
    virtual bool isUrgent() const = 0;
    virtual void updateAppStatus() = 0;
    virtual void removeAppStatus() = 0;
protected:
    virtual ~ClientData() {}
};

class YClientItem {
public:
    virtual void goodbye() = 0;
    virtual void update() = 0;
    virtual void repaint() = 0;
};

class YFrameClient: public YDndWindow
                  , public YTimerListener
{
    typedef YDndWindow super;
public:
    YFrameClient(YWindow *parent, YFrameWindow *frame, Window win,
                 int depth = 0, Visual *visual = nullptr, Colormap cmap = 0);
    virtual ~YFrameClient();

    virtual void handleProperty(const XPropertyEvent &property);
    virtual void handleColormap(const XColormapEvent &colormap);
    virtual void handleMapNotify(const XMapEvent& map);
    virtual void handleUnmap(const XUnmapEvent &unmap);
    virtual void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    virtual void handleClientMessage(const XClientMessageEvent &message);
#ifdef CONFIG_SHAPE
    virtual void handleShapeNotify(const XShapeEvent &shape);
#endif

    int getBorder() const { return fBorder; }
    void setBorder(unsigned int border) { fBorder = border; }
    void setFrame(YFrameWindow *newFrame);
    YFrameWindow *getFrame() const { return fFrame; };
    YFrameWindow* obtainFrame() const;
    YClientContainer* getContainer() const;
    void setClientItem(YClientItem* item) { fClientItem = item; }
    YClientItem* getClientItem() const { return fClientItem; }

    enum WindowProtocols {
        wpDeleteWindow = 1 << 0,
        wpTakeFocus    = 1 << 1,
        wpPing         = 1 << 2,
    };
    enum { InvalidFrameState = -1 };

    bool protocol(WindowProtocols wp) const { return bool(fProtocols & wp); }
    void sendMessage(Atom msg, Time ts, long p2 = 0L, long p3 = 0L, long p4 = 0L);
    void sendTakeFocus();
    void sendDelete();
    void sendPing();
    void recvPing(const XClientMessageEvent &message);
    bool forceClose();
    bool isCloseForced();
    bool frameOption(int option);
    bool killPid();
    bool timedOut() const { return fTimedOut; }

    enum {
        csKeepX = 1,
        csKeepY = 2,
        csRound = 4
    };

    void constrainSize(int &w, int &h, int flags);

    void gravityOffsets(int &xp, int &yp);

    Colormap colormap() const { return fColormap; }
    void setColormap(Colormap cmap);

    FrameState getFrameState();
    void setFrameState(FrameState state);

    void getWMHints();
    XWMHints *hints() const { return fHints; }
    bool wmHint(int flag) const { return fHints && (fHints->flags & flag); }
    Window windowGroupHint() const;
    Window iconWindowHint() const;
    Pixmap iconPixmapHint() const;
    Pixmap iconMaskHint() const;
    bool urgencyHint() const { return wmHint(XUrgencyHint); }
    bool isDockApp() const;
    bool isDockAppIcon() const;
    bool isDockAppWindow() const;
    bool isDocked() const { return fDocked; }
    void setDocked(bool docked);

    void getSizeHints();
    XSizeHints *sizeHints() const { return fSizeHints; }
    int sizeHintsFlags() const { return fSizeHints ? int(fSizeHints->flags) : 0; }
    int winGravity() const { return hasbit(sizeHintsFlags(), PWinGravity)
                                  ? fSizeHints->win_gravity : NorthWestGravity; }

    void getProtocols(bool force);

    void setTransient(Window owner);
    void getTransient();
    bool hasTransient();
    bool isTransient() const { return fTransientFor != None; }
    bool isTransientFor(Window owner);
    Window ownerWindow() const { return fTransientFor; }
    YFrameClient* getOwner() const;
    YFrameClient* nextTransient();
    YFrameClient* firstTransient();
    static YFrameClient* firstTransient(Window handle);

    void getClassHint();
    ClassHint* classHint() { return &fClassHint; }

    void getNameHint();
    void getNetWmName();
    void getIconNameHint();
    void getNetWmIconName();
    void fixWindowTitle(bool fixed) { fFixedTitle = fixed; }
    void setWindowTitle(const char *title);
    void setIconTitle(const char *title);
    mstring windowTitle() const { return fWindowTitle; }
    mstring iconTitle() const { return fIconTitle; }
    void refreshIcon();
    void obtainIcon();
    ref<YIcon> getIcon();

    void setWorkspaceHint(int workspace);

    void setLayerHint(int layer);
    bool getLayerHint(int* layer);

    void setWinTrayHint(int tray_opt);
    bool getWinTrayHint(int* tray_opt);

    void setStateHint(int state);
    void setWinHintsHint(int hints);
    int winHints() const { return fWinHints; }

    bool getWinIcons(Atom* type, long* count, long** elem);
    bool getKwmIcon(long* count, Pixmap** pixmap);
    bool getNetWMIcon(long* count, long** elem);

    bool getNetWMStateHint(int* mask, int* state);
    bool getNetWMDesktopHint(int* workspace);
    bool getNetWMPid(long *pid);
    bool getNetWMStrut(int *left, int *right, int *top, int *bottom);
    bool getNetWMStrutPartial(int *left, int *right, int *top, int *bottom);
    bool getNetStartupId(unsigned long &time);
    bool getNetWMUserTime(Window window, unsigned long &time);
    bool getNetWMUserTimeWindow(Window &window);
    bool getNetWMWindowOpacity(long &opacity);
    bool getNetWMWindowType(WindowType *window_type);
    void setNetWMFullscreenMonitors(int top, int bottom, int left, int right);
    void setNetFrameExtents(int left, int right, int top, int bottom);
    void setNetWMAllowedActions(Atom *actions, int count);
    void netStateRequest(int action, int mask);
    void actionPerformed(YAction action);

    bool isPinging() const { return fPinging; }
    bool pingTime() const { return fPingTime; }
    virtual bool handleTimer(YTimer *t);

    MwmHints *mwmHints() const { return fMwmHints._ptr(); }
    void getMwmHints();
    void setMwmHints(const MwmHints &mwm);
    long mwmFunctions();
    long mwmDecors();

    bool shaped() const { return fShaped; }
    void queryShape();

    void getClientLeader();
    void getWindowRole();

    Window clientLeader() const { return fClientLeader; }
    mstring windowRole() const { return fWindowRole; }

    mstring getClientId(Window leader);
    void getPropertiesList();

    // virtual void configure(const YRect2 &rect);
    virtual void handleGravityNotify(const XGravityEvent &gravity);

    bool isKdeTrayWindow() { return prop.kde_net_wm_system_tray_window_for; }

    bool isEmbed() const { return prop.xembed_info; }

    const WindowOption* getWindowOption();
    bool activateOnMap();

private:
    YFrameWindow *fFrame;
    YClientItem* fClientItem;
    int fProtocols;
    unsigned fBorder;
    FrameState fSavedFrameState;
    int fWinStateHint;
    int fWinHints;
    XSizeHints *fSizeHints;
    ClassHint fClassHint;
    XWMHints *fHints;
    Colormap fColormap;
    bool fDocked;
    bool fShaped;
    bool fTimedOut;
    bool fFixedTitle;
    bool fIconize;
    bool fPinging;
    long fPingTime;
    long fPid;
    lazy<YTimer> fPingTimer;
    ref<YIcon> fIcon;

    mstring fWindowTitle;
    mstring fIconTitle;

    Window fClientLeader;
    mstring fWMWindowRole;
    mstring fWindowRole;

    lazy<MwmHints> fMwmHints;
    lazy<WindowOption> fWindowOption;
    void loadWindowOptions(WindowOptions* list, bool remove);

    struct transience {
        Window trans, owner;
        transience(Window t, Window o) : trans(t), owner(o) { }
    };
    static YArray<transience> fTransients;
    Window fTransientFor;
    int findTransient(Window handle);

    Pixmap *kwmIcons;
    struct {
        bool wm_state : 1; // no property notify
        bool wm_hints : 1;
        bool wm_normal_hints : 1;
        bool wm_transient_for : 1;
        bool wm_name : 1;
        bool wm_icon_name : 1;
        bool wm_class : 1;
        bool wm_protocols : 1;
        bool wm_client_leader : 1;
        bool wm_window_role : 1;
        bool window_role : 1;
        bool sm_client_id : 1;
        bool kwm_win_icon : 1;
        bool kde_net_wm_system_tray_window_for : 1;
        bool net_wm_name : 1;
        bool net_wm_icon_name : 1;
        bool net_wm_icon : 1;
        bool net_wm_strut : 1;
        bool net_wm_strut_partial : 1;
        bool net_wm_desktop : 1; // no property notify
        bool net_wm_state : 1; // no property notify
        bool net_wm_window_type : 1;
        bool net_startup_id : 1; // no property notify
        bool net_wm_user_time : 1;
        bool net_wm_user_time_window : 1;
        bool net_wm_window_opacity : 1;
        bool net_wm_pid : 1;
        bool mwm_hints : 1;
        bool win_tray : 1; // no property notify
        bool win_layer : 1; // no property notify
        bool win_icons : 1;
        bool xembed_info : 1;
    } prop;
private: // not-used
    YFrameClient(const YFrameClient &);
    YFrameClient &operator=(const YFrameClient &);
};

#endif // YCLIENT_H

// vim: set sw=4 ts=4 et:
