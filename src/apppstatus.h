// //////////////////////////////////////////////////////////////////////////
// IceWM: src/pppstatus.h
//
//
// PPP Status
//
// //////////////////////////////////////////////////////////////////////////
#ifndef NETSTATUS_H
#define NETSTATUS_H

#if defined(linux) || defined(__FreeBSD__)
#ifdef CONFIG_APPLET_NET_STATUS

#define HAVE_NET_STATUS 1

#include "ywindow.h"
#include "ytimer.h"

#include <sys/socket.h>

#define NET_UPDATE_INTERVAL 500
#define NET_SAMPLES 20

class NetStatus:
public YWindow, 
public YTimer::Listener {
public:
    NetStatus(char const * netdev, YWindow *aParent = 0);
    ~NetStatus();
private:
    YColor *color[3];
    YTimer *fUpdateTimer;
    int maxBytes;

    int ppp_in[NET_SAMPLES];
    int ppp_out[NET_SAMPLES];
    int ppp_tot[NET_SAMPLES];

    unsigned long prev_ibytes, start_ibytes, cur_ibytes;
    unsigned long prev_obytes, start_obytes, cur_obytes;

    time_t start_time;

    struct timeval prev_time;

    bool wasUp;               // previous link status
    bool useIsdn;             // netdevice is an IsdnDevice
    char *fNetDev;		// name of the device
    
    char phoneNumber[32];

    // methods local to this class
    bool isUp();
    bool isUpIsdn();
    void getCurrent(int *in, int *out, int *tot);
    void updateStatus();
    void updateToolTip();

    // methods overloaded from superclasses
    virtual bool handleTimer(YTimer *t);
    virtual void handleClick(const XButtonEvent &up, int count);
    virtual void paint(Graphics & g, int x, int y, unsigned int width, unsigned int height);
};

#endif // CONFIG_APPLET_NET_STATUS

#endif // linux || __FreeBSD__

#endif // NETSTATUS_H
