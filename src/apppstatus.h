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
#ifdef CONFIG_NET_STATUS

#define HAVE_NET_STATUS

#include "ywindow.h"
#include "ytimer.h"

#include <sys/socket.h>

#define NET_UPDATE_INTERVAL 500
#define NET_SAMPLES 20

class NetStatus: public YWindow, public YTimerListener {
public:
    NetStatus(const char *netCommand, YWindow *aParent = 0);
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
    
    const char *fNetCommand;
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

#endif

#endif

#endif
