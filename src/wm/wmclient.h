#ifndef __YCLIENT_H
#define __YCLIENT_H

#include "ywindow.h"
#include "ymenu.h"
#include "yconfig.h"
#include "MwmUtil.h"

class YFrameWindow;
class WindowListItem;

typedef int FrameState;

#ifndef __YIMP_UTIL__
//!!! remove these if possible
typedef struct XWMHints;
typedef struct XSizeHints;
typedef struct XClassHint;
#endif

class ClientData {
public:
    virtual YFrameWindow *owner() const = 0;
    virtual YIcon *getIcon() = 0;
    virtual const CStr *getTitle() = 0;
    virtual const CStr *getIconTitle() = 0;
    virtual void activateWindow(bool raise) = 0;
    virtual bool isHidden() const = 0;
    virtual bool isMinimized() const = 0;
    virtual void actionPerformed(YAction *action, unsigned int modifiers) = 0;
    virtual bool focused() const = 0;
    virtual int visibleNow() = 0;
    virtual bool canRaise() = 0;
    virtual void wmRaise() = 0;
    virtual void wmLower() = 0;
    virtual void wmMinimize() = 0;
    virtual void wmOccupyWorkspace(long workspace) = 0;
    virtual void wmOccupyOnlyWorkspace(long workspace) = 0;
    virtual void popupSystemMenu() = 0;
    virtual void popupSystemMenu(int x, int y,
                         int x_delta, int y_delta,
                         unsigned int flags,
                         YWindow *forWindow = 0) = 0;
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
#ifdef SHAPE
    virtual void handleShapeNotify(const XShapeEvent &shape);
#endif

    unsigned int getBorder() const { return fBorder; }
    void setBorder(unsigned int border) { fBorder = border; }
    void setFrame(YFrameWindow *newFrame);
    YFrameWindow *getFrame() const { return fFrame; };

    enum {
        wpDeleteWindow = 1 << 0,
        wpTakeFocus    = 1 << 1
    } WindowProtocols;

    void sendMessage(Atom msg, Time timeStamp = CurrentTime);

    enum {
        csKeepX = 1,
        csKeepY = 2,
        csRound = 4
    };
    
    void constrainSize(int &w, int &h, long layer, int flags = 0);
    void gravityOffsets (int &xp, int &yp);

    Colormap colormap() const { return fColormap; }
    void setColormap(Colormap cmap);

    FrameState getFrameState();
    void setFrameState(FrameState state);

    void getWMHints();
    XWMHints *hints() const { return fHints; }

    void getSizeHints();
    XSizeHints *sizeHints() const { return fSizeHints; }

    unsigned long protocols() const { return fProtocols; }
    void getProtocols();

    void getTransient();
    Window ownerWindow() const { return fTransientFor; }

    void getClassHint();
    XClassHint *classHint() const { return fClassHint; }

    void getNameHint();
    void getIconNameHint();
    void setWindowTitle(const char *aWindowTitle);
    void setIconTitle(const char *aIconTitle);
#ifdef I18N
    void setWindowTitle(XTextProperty *aWindowTitle);
    void setIconTitle(XTextProperty *aIconTitle);
#endif
    const CStr *windowTitle() { return fWindowTitle; }
    const CStr *iconTitle() { return fIconTitle; }

#ifdef GNOME1_HINTS
    bool getWinIcons(Atom *type, int *count, long **elem);

    void setWinWorkspaceHint(long workspace);
    bool getWinWorkspaceHint(long *workspace);

    void setWinLayerHint(long layer);
    bool getWinLayerHint(long *layer);

    void setWinStateHint(long mask, long state);
    bool getWinStateHint(long *mask, long *state);

    void setWinHintsHint(long hints);
    bool getWinHintsHint(long *hints);
    long winHints() const { return fWinHints; }
#endif

#ifdef WMSPEC_HINTS
    bool getNetWMStrut(int *left, int *right, int *top, int *bottom);
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

#ifdef SHAPE
    int shaped() const { return fShaped; }
    void queryShape();
#endif

    void getClientLeader();
    void getWindowRole();

    Window clientLeader() const { return fClientLeader; }
    const CStr *windowRole() const { return fWindowRole; }

    char *getClientId(Window leader);
    
private:
    YFrameWindow *fFrame;
    int fProtocols;
    int haveButtonGrab;
    unsigned int fBorder;
    XSizeHints *fSizeHints;
    XClassHint *fClassHint;
    XWMHints *fHints;
    Colormap fColormap;
    int fShaped;
    long fWinHints;

    CStr *fWindowTitle;
    CStr *fIconTitle;

    Window fClientLeader;
    CStr *fWindowRole;

    MwmHints *fMwmHints;

    Window fTransientFor;
    Pixmap *kwmIcons;

    static YBoolPrefProperty gLimitSize; // remove this from this class!!!
private: // not-used
    YFrameClient(const YFrameClient &);
    YFrameClient &operator=(const YFrameClient &);
};

#endif
