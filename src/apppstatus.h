// //////////////////////////////////////////////////////////////////////////
// IceWM: src/pppstatus.h
//
//
// PPP Status
//
// //////////////////////////////////////////////////////////////////////////
#ifndef NETSTATUS_H
#define NETSTATUS_H

#include "ycollections.h"
#include "base.h"

#if HAVE_NET_STATUS

class IAppletContainer;
class NetStatusControl;

class NetStatus: public YWindow {
public:
    NetStatus(IApp *app, YSMListener *smActionListener, mstring netdev, IAppletContainer *taskBar, YWindow *aParent = 0);
    ~NetStatus();
private:
    IAppletContainer *fTaskBar;
    YColor *color[3];
    YSMListener *smActionListener;
    IApp *app;
    friend class NetStatusControl;

    long *ppp_in; /* long could be really enough for rate in B/s */
    long *ppp_out;

    unsigned long long prev_ibytes, start_ibytes, cur_ibytes, offset_ibytes;
    unsigned long long prev_obytes, start_obytes, cur_obytes, offset_obytes;

    time_t start_time;
    struct timeval prev_time;

    bool wasUp;               // previous link status
    bool useIsdn;             // netdevice is an IsdnDevice
    mstring fNetDev;            // name of the device

    char phoneNumber[32];

    void updateVisible(bool aVisible);
    // methods local to this class
    bool isUp();
    bool isUpIsdn();
    void getCurrent(long *in, long *out, const void* sharedData);
    void updateStatus(const void* sharedData);
    void updateToolTip();
    void handleTimer(const void* sharedData, bool forceDown);

    // methods overridden from superclasses
    virtual void handleClick(const XButtonEvent &up, int count) OVERRIDE;
    virtual void paint(Graphics & g, const YRect &r) OVERRIDE;
};

class NetStatusControl : public YTimerListener, public refcounted {
    YTimer* fUpdateTimer;
    //YSortedMap<ustring,NetStatus*> fNetStatus;
    YVec<NetStatus*> fNetStatus;

    IApp* app;
    YSMListener* smActionListener;
    IAppletContainer* taskBar;
    YWindow* aParent;

#ifdef __linux__
    // preprocessed data from procfs with offset table (name, values, name, vaues, ...)
    YVec<char> cachedStats;
    YVec<const char *> cachedStatsIdx;
    YVec<NetStatus*> covered;

    YVec<mstring> matchPatterns;
    void fetchSystemData();
#endif

public:
    NetStatusControl(IApp *app, YSMListener *smActionListener, IAppletContainer *taskBar, YWindow *aParent);
    ~NetStatusControl();
    YVec<NetStatus*>::iterator getIterator() { return fNetStatus.getIterator();}
    // subclassing method overrides
    virtual bool handleTimer(YTimer *t) OVERRIDE;
};

#endif

#endif // NETSTATUS_H

// vim: set sw=4 ts=4 et:
