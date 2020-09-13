#ifndef __YCLIENT_H
#define __YCLIENT_H

#include "ywindow.h"
#include "ymenu.h"
#include "MwmUtil.h"
#ifndef InputHint
#include <X11/Xutil.h>
#endif

#define InvalidFrameState   (-1)

class YFrameWindow;
class WindowListItem;
class YIcon;

typedef int FrameState;

class ClassHint : public XClassHint {
public:
    ClassHint() { res_name = res_class = nullptr; }
    ClassHint(const char* name, const char* klas) {
        res_name = strdup(name);
        res_class = strdup(klas);
    }
    ClassHint(const ClassHint& hint) {
        res_name = hint.res_name ? strdup(hint.res_name) : nullptr;
        res_class = hint.res_class ? strdup(hint.res_class) : nullptr;
    }
    ~ClassHint() { reset(); }
    void reset() {
        if (res_name) { XFree(res_name); res_name = nullptr; }
        if (res_class) { XFree(res_class); res_class = nullptr; }
    }
    bool match(const char* resource) const;
    char* resource() const;
    void operator=(const ClassHint& hint) {
        if (this != &hint) {
            reset();
            if (hint.res_name) {
                res_name = strdup(hint.res_name);
            }
            if (hint.res_class) {
                res_class = strdup(hint.res_class);
            }
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
};

class ClientData {
public:
    virtual void setWinListItem(WindowListItem *i) = 0;
    virtual YFrameWindow *owner() const = 0;
    virtual ref<YIcon> getIcon() const = 0;
    virtual mstring getTitle() const = 0;
    virtual mstring getIconTitle() const = 0;
    virtual void activateWindow(bool raise, bool curWork) = 0;
    virtual bool isHidden() const = 0;
    virtual bool isMaximized() const = 0;
    virtual bool isMaximizedVert() const = 0;
    virtual bool isMaximizedHoriz() const = 0;
    virtual bool isMaximizedFully() const = 0;
    virtual bool isMinimized() const = 0;
    virtual bool isFullscreen() const = 0;
    virtual bool isRollup() const = 0;
    virtual void actionPerformed(YAction action, unsigned int modifiers) = 0;
    virtual bool focused() const = 0;
    virtual bool visibleNow() const = 0;
    virtual bool canClose() const = 0;
    virtual bool canShow() const = 0;
    virtual bool canHide() const = 0;
    virtual bool canLower() const = 0;
    virtual bool canMinimize() const = 0;
    virtual bool canMaximize() const = 0;
    virtual bool canRaise() = 0;
    virtual bool canRestore() const = 0;
    virtual bool canRollup() const = 0;
    virtual void wmRaise() = 0;
    virtual void wmLower() = 0;
    virtual void wmMinimize() = 0;
    virtual int getWorkspace() const = 0;
    virtual int getTrayOrder() const = 0;
    virtual long getTrayOption() const = 0;
    virtual unsigned frameOptions() const = 0;
    virtual bool isSticky() const = 0;
    virtual bool isAllWorkspaces() const = 0;
    virtual bool startMinimized() const = 0;
    virtual void wmOccupyWorkspace(int workspace) = 0;
    virtual void wmOccupyOnlyWorkspace(int workspace) = 0;
    virtual void popupSystemMenu(YWindow *owner) = 0;
    virtual void popupSystemMenu(YWindow *owner, int x, int y,
                         unsigned int flags,
                         YWindow *forWindow = nullptr) = 0;
    virtual void updateSubmenus() = 0;
    virtual Time since() const = 0;
protected:
    virtual ~ClientData() {}
};

class YFrameClient: public YWindow
                  , public YTimerListener
{
public:
    YFrameClient(YWindow *parent, YFrameWindow *frame, Window win = 0,
                 int depth = 0, Visual *visual = nullptr, Colormap cmap = 0);
    virtual ~YFrameClient();

    virtual void handleProperty(const XPropertyEvent &property);
    virtual void handleColormap(const XColormapEvent &colormap);
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

    enum {
        wpDeleteWindow = 1 << 0,
        wpTakeFocus    = 1 << 1,
        wpPing         = 1 << 2,
    } WindowProtocols;

    void sendMessage(Atom msg, Time timeStamp);
    bool sendTakeFocus();
    bool sendDelete();
    bool sendPing();
    void recvPing(const XClientMessageEvent &message);
    bool killPid();

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

    void getSizeHints();
    XSizeHints *sizeHints() const { return fSizeHints; }

    // for going fullscreen and back
    XSizeHints savedSizeHints;
    void saveSizeHints();
    void restoreSizeHints();

    unsigned protocols() const { return fProtocols; }
    void getProtocols(bool force);

    void getTransient();
    Window ownerWindow() const { return fTransientFor; }

    void getClassHint();
    ClassHint* classHint() { return &fClassHint; }

    void getNameHint();
    void getNetWmName();
    void getIconNameHint();
    void getNetWmIconName();
    void setWindowTitle(const char *title);
    void setIconTitle(const char *title);
    mstring windowTitle() { return fWindowTitle; }
    mstring iconTitle() { return fIconTitle; }

    void setWinWorkspaceHint(long workspace);
    bool getWinWorkspaceHint(long *workspace);

    void setWinLayerHint(long layer);
    bool getWinLayerHint(long *layer);

    void setWinTrayHint(long tray_opt);
    bool getWinTrayHint(long *tray_opt);

    void setWinStateHint(long mask, long state);
    bool getWinStateHint(long *mask, long *state);

    void setWinHintsHint(long hints);
    bool getWinHintsHint(long *hints);
    long winHints() const { return fWinHints; }

    bool getWinIcons(Atom* type, long* count, long** elem);
    bool getKwmIcon(long* count, Pixmap** pixmap);
    bool getNetWMIcon(long* count, long** elem);

    bool getNetWMStateHint(long *mask, long *state);
    bool getNetWMDesktopHint(long *workspace);
    bool getNetWMPid(long *pid);
    bool getNetWMStrut(int *left, int *right, int *top, int *bottom);
    bool getNetWMStrutPartial(int *left, int *right, int *top, int *bottom);
    bool getNetStartupId(unsigned long &time);
    bool getNetWMUserTime(Window window, unsigned long &time);
    bool getNetWMUserTimeWindow(Window &window);
    bool getNetWMWindowOpacity(long &opacity);
    bool getNetWMWindowType(Atom *window_type);
    void setNetWMFullscreenMonitors(int top, int bottom, int left, int right);
    void setNetFrameExtents(int left, int right, int top, int bottom);
    void setNetWMAllowedActions(Atom *actions, int count);

    bool isPinging() const { return fPinging; }
    bool pingTime() const { return fPingTime; }
    virtual bool handleTimer(YTimer *t);

    MwmHints *mwmHints() const { return fMwmHints; }
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

    bool isEmbed() { return prop.xembed_info; }

private:
    YFrameWindow *fFrame;
    int fProtocols;
    int haveButtonGrab;
    unsigned int fBorder;
    FrameState fSavedFrameState;
    long fSavedWinState[2];
    XSizeHints *fSizeHints;
    ClassHint fClassHint;
    XWMHints *fHints;
    Colormap fColormap;
    bool fShaped;
    bool fPinging;
    long fPingTime;
    lazy<YTimer> fPingTimer;
    long fWinHints;
    long fPid;

    mstring fWindowTitle;
    mstring fIconTitle;

    Window fClientLeader;
    mstring fWMWindowRole;
    mstring fWindowRole;

    MwmHints *fMwmHints;

    Window fTransientFor;

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
        bool win_hints : 1;
        bool win_workspace : 1; // no property notify
        bool win_state : 1; // no property notify
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
