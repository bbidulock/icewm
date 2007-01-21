// //////////////////////////////////////////////////////////////////////////
// IceWM: src/pppstatus.h
//
//
// PPP Status
//
// //////////////////////////////////////////////////////////////////////////
#ifndef NETSTATUS_H
#define NETSTATUS_H

#ifdef CONFIG_APPLET_NET_STATUS
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__)

#define HAVE_NET_STATUS 1

#include "ywindow.h"
#include "ytimer.h"
#include <sys/time.h>

class IAppletContainer;

class NetStatus: public YWindow, public YTimerListener {
public:
    NetStatus(mstring netdev, IAppletContainer *taskBar, YWindow *aParent = 0);
    ~NetStatus();
private:
    IAppletContainer *fTaskBar;
    YColor *color[3];
    YTimer *fUpdateTimer;

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
    void getCurrent(long *in, long *out);
    void updateStatus();
    void updateToolTip();

    // methods overloaded from superclasses
    virtual bool handleTimer(YTimer *t);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics & g, const YRect &r);
};


#else // linux || __FreeBSD__ || __NetBSD__
#undef CONFIG_APPLET_NET_STATUS
#endif
#endif // CONFIG_APPLET_NET_STATUS

#endif // NETSTATUS_H
