// //////////////////////////////////////////////////////////////////////////
// IceWM: src/pppstatus.h
//
//
// PPP Status
//
// //////////////////////////////////////////////////////////////////////////
#ifndef NETSTATUS_H
#define NETSTATUS_H

#include "ypointer.h"

class IAppletContainer;
class NetStatusControl;

typedef unsigned long long netbytes;

class NetStatusHandler {
public:
    virtual ~NetStatusHandler() { }
    virtual void relayout() = 0;
    virtual void runCommandOnce(const char *resource, const char *cmdline) = 0;
    virtual void handleClick(const XButtonEvent &up, mstring netdev) = 0;
};

class NetDevice {
public:
    NetDevice(mstring netdev) : fDevName(netdev) {}
    virtual bool isUp() = 0;
    virtual void getCurrent(netbytes *in, netbytes *out, const void* sharedData) = 0;
    virtual const char* getPhoneNumber() { return ""; }
    virtual ~NetDevice() {}

    mstring name() const { return fDevName; }

protected:
    mstring fDevName;
};

class NetLinuxDevice : public NetDevice {
public:
    NetLinuxDevice(mstring netdev) : NetDevice(netdev) {}
    virtual bool isUp();
    virtual void getCurrent(netbytes *in, netbytes *out, const void* sharedData);
};

class NetIsdnDevice : public NetLinuxDevice {
public:
    NetIsdnDevice(mstring netdev) : NetLinuxDevice(netdev) { *phoneNumber = 0; }
    virtual bool isUp();
    virtual const char* getPhoneNumber() { return phoneNumber; }
private:
    char phoneNumber[32];
};

class NetFreeDevice : public NetDevice {
public:
    NetFreeDevice(mstring netdev) : NetDevice(netdev) {}
    virtual bool isUp();
    virtual void getCurrent(netbytes *in, netbytes *out, const void* sharedData);
};

class NetOpenDevice : public NetDevice {
public:
    NetOpenDevice(mstring netdev) : NetDevice(netdev) {}
    virtual bool isUp();
    virtual void getCurrent(netbytes *in, netbytes *out, const void* sharedData);
};

class NetDummyDevice : public NetDevice {
public:
    NetDummyDevice(mstring netdev) : NetDevice(netdev) {}
    virtual bool isUp() { return false; }
    virtual void getCurrent(netbytes *in, netbytes *out, const void* sharedData)
    { }
};

class NetStatus: public IApplet, private Picturer {
public:
    NetStatus(mstring netdev, NetStatusHandler* handler, YWindow *aParent = nullptr);
    ~NetStatus();

    mstring name() const { return fDevName; }
    void timedUpdate(const void* sharedData, bool forceDown = false);
    bool isUp() const { return fDevice && fDevice->isUp(); }

private:
    NetStatusHandler* fHandler;
    YColorName color[3];

    long *ppp_in; /* long could be really enough for rate in B/s */
    long *ppp_out;

    netbytes prev_ibytes, start_ibytes, cur_ibytes, offset_ibytes;
    netbytes prev_obytes, start_obytes, cur_obytes, offset_obytes;

    timeval start_time;
    timeval prev_time;

    long oldMaxBytes;
    int statusUpdateCount;
    int unchanged;

    bool wasUp;               // previous link status
    bool useIsdn;             // netdevice is an IsdnDevice
    mstring fDevName;         // name of the device
    osmart<NetDevice> fDevice;

    void updateVisible(bool aVisible);
    // methods local to this class
    void getCurrent(long *in, long *out, const void* sharedData);
    void updateStatus(const void* sharedData);
    virtual void updateToolTip() override;

    // methods overridden from superclasses
    virtual void handleClick(const XButtonEvent &up, int count) override;

    virtual bool picture() override;
    void fill(Graphics& g);
    void draw(Graphics& g);
};

#ifdef __linux__
class netpair : public pair<const char *, const char *> {
public:
    netpair(const char* name, const char* data) : pair(name, data) { }
    const char* name() const { return left; }
    const char* data() const { return right; }
};
#endif

class NetStatusControl :
    private YTimerListener,
    private YActionListener,
    private NetStatusHandler
{
private:
    lazy<YTimer> fUpdateTimer;
    YAssocArray<NetStatus*> fNetStatus;

    YSMListener* smActionListener;
    IAppletContainer* taskBar;
    YWindow* aParent;
    long fPid;
    osmart<YMenu> fMenu;

#ifdef __linux__
    // preprocessed data from procfs with offset table (name, values, name, vaues, ...)
    fcsmart devicesText;
    YArray<netpair> devStats;
    typedef YArray<netpair>::IterType IterStats;

    void fetchSystemData();
    void linuxUpdate();
#endif
    YStringArray patterns;
    YStringArray interfaces;

    NetStatus* createNetStatus(mstring netdev);
    void getInterfaces(YStringArray& interfaces);

public:
    NetStatusControl(IApp *app, YSMListener *smActionListener, IAppletContainer *taskBar, YWindow *aParent);
    ~NetStatusControl();

    typedef YAssocArray<NetStatus*>::IterType IterType;
    IterType getIterator() { return fNetStatus.iterator(); }

    // subclassing method overrides
    virtual bool handleTimer(YTimer *t) override;
    virtual void handleClick(const XButtonEvent &up, mstring netdev) override;
    virtual void runCommandOnce(const char *resource, const char *cmdline) override;
    virtual void actionPerformed(YAction, unsigned int) override;
    virtual void relayout() override;
};

#endif // NETSTATUS_H

// vim: set sw=4 ts=4 et:
