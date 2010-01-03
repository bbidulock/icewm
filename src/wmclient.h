#ifndef __YCLIENT_H
#define __YCLIENT_H

#include "yutil.h"
#include "ywindow.h"
#include "ymenu.h"
#include "MwmUtil.h"

class YFrameWindow;
class WindowListItem;
class YIcon;

typedef int FrameState;

class ClientData {
public:
#ifdef CONFIG_WINLIST
    virtual void setWinListItem(WindowListItem *i) = 0;
#endif
    virtual YFrameWindow *owner() const = 0;
#ifndef LITE
    virtual ref<YIcon> getIcon() const = 0;
#endif
    virtual ustring getTitle() const = 0;
    virtual ustring getIconTitle() const = 0;
    virtual void activateWindow(bool raise) = 0;
    virtual bool isHidden() const = 0;
    virtual bool isMinimized() const = 0;
    virtual void actionPerformed(YAction *action, unsigned int modifiers) = 0;
    virtual bool focused() const = 0;
    virtual bool visibleNow() const = 0;
    virtual bool canRaise() = 0;
    virtual void wmRaise() = 0;
    virtual void wmLower() = 0;
    virtual void wmMinimize() = 0;
    virtual long getWorkspace() const = 0;
    virtual bool isSticky() const = 0;
    virtual void wmOccupyWorkspace(long workspace) = 0;
    virtual void wmOccupyOnlyWorkspace(long workspace) = 0;
    virtual void popupSystemMenu(YWindow *owner) = 0;
    virtual void popupSystemMenu(YWindow *owner, int x, int y,
                         unsigned int flags,
                         YWindow *forWindow = 0) = 0;
protected:
    virtual ~ClientData() {};
};

class YFrameClient: public YWindow  {
public:
    YFrameClient(YWindow *parent, YFrameWindow *frame, Window win = 0);
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
        wpTakeFocus    = 1 << 1
    } WindowProtocols;

    void sendMessage(Atom msg, Time timeStamp);
    bool sendTakeFocus();
    bool sendDelete();

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

    unsigned long protocols() const { return fProtocols; }
    void getProtocols(bool force);

    void getTransient();
    Window ownerWindow() const { return fTransientFor; }

    void getClassHint();
    XClassHint *classHint() const { return fClassHint; }

    void getNameHint();
    void getIconNameHint();
    void setWindowTitle(const char *title);
    void setIconTitle(const char *title);
#ifdef CONFIG_I18N
    void setWindowTitle(const XTextProperty & title);
    void setIconTitle(const XTextProperty & title);
#endif
    ustring windowTitle() { return fWindowTitle; }
    ustring iconTitle() { return fIconTitle; }

    bool getWinIcons(Atom *type, int *count, long **elem);

    void setWinWorkspaceHint(long workspace);
    bool getWinWorkspaceHint(long *workspace);

    void setWinLayerHint(long layer);
    bool getWinLayerHint(long *layer);

#ifdef CONFIG_TRAY
    void setWinTrayHint(long tray_opt);
    bool getWinTrayHint(long *tray_opt);
#endif

    void setWinStateHint(long mask, long state);
    bool getWinStateHint(long *mask, long *state);

    void setWinHintsHint(long hints);
    bool getWinHintsHint(long *hints);
    long winHints() const { return fWinHints; }

#ifdef WMSPEC_HINTS
    bool getNetWMIcon(int *count, long **elem);
    bool getNetWMStateHint(long *mask, long *state);
    bool getNetWMDesktopHint(long *workspace);
    bool getNetWMStrut(int *left, int *right, int *top, int *bottom);
    bool getNetWMWindowType(Atom *window_type);
#endif

#ifndef NO_MWM_HINTS
    MwmHints *mwmHints() const { return fMwmHints; }
    void getMwmHints();
    void setMwmHints(const MwmHints &mwm);
    long mwmFunctions();
    long mwmDecors();
#endif

#ifndef NO_KWM_HINTS
    bool getKwmIcon(int *count, Pixmap **pixmap);
#endif

#ifdef CONFIG_SHAPE
    bool shaped() const { return fShaped; }
    void queryShape();
#endif

    void getClientLeader();
    void getWindowRole();
    void getWMWindowRole();

    Window clientLeader() const { return fClientLeader; }
    ustring windowRole() const { return fWMWindowRole != null ? fWMWindowRole : fWindowRole; }

    ustring getClientId(Window leader);
    void getPropertiesList();

    void configure(const YRect &/*r*/);

    bool isKdeTrayWindow() { return prop.kde_net_wm_system_tray_window_for; }

    bool getWmUserTime(long *userTime);
    bool isEmbed() { return prop.xembed_info; }
    
private:
    YFrameWindow *fFrame;
    int fProtocols;
    int haveButtonGrab;
    unsigned int fBorder;
    XSizeHints *fSizeHints;
    XClassHint *fClassHint;
    XWMHints *fHints;
    Colormap fColormap;
    bool fShaped;
    long fWinHints;

    ustring fWindowTitle;
    ustring fIconTitle;

    Window fClientLeader;
    ustring fWMWindowRole;
    ustring fWindowRole;

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
#ifdef WMSPEC_HINTS
        bool net_wm_icon : 1;
        bool net_wm_strut : 1;
        bool net_wm_desktop : 1; // no property notify
        bool net_wm_state : 1; // no property notify
        bool net_wm_window_type : 1;
        bool net_wm_user_time : 1;
#endif
#ifndef NO_MWM_HINTS
        bool mwm_hints : 1;
#endif
#ifdef GNOME1_HINTS
        bool win_hints : 1;
        bool win_workspace : 1; // no property notify
        bool win_state : 1; // no property notify
        bool win_layer : 1; // no property notify
        bool win_icons : 1;
#endif
        bool xembed_info : 1;
    } prop;
private: // not-used
    YFrameClient(const YFrameClient &);
    YFrameClient &operator=(const YFrameClient &);
};

#endif // YCLIENT_H
