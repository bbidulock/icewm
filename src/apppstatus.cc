// //////////////////////////////////////////////////////////////////////////
// IceWM: src/NetStatus.cc
// by Mark Lawrence <nomad@null.net>
//
// Linux-ISDN/ippp-Upgrade
// by Denis Boehme <denis.boehme@gmx.net>
//     6.01.2000
//
// !!! share code with cpu status
//
// KNOWN BUGS: - first measurement is thrown off
//
// //////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "wmtaskbar.h"
#include "apppstatus.h"

#include "wmapp.h"
#include "prefs.h"
#include "sysdep.h"
#include "ymenuitem.h"
#include "intl.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

#if __FreeBSD__ || __FreeBSD_kernel__
#include <sys/sysctl.h>
#include <net/if_mib.h>
#endif

extern ref<YPixmap> taskbackPixmap;

class NetDevice {
public:
    NetDevice(mstring devName) : fDevName(devName), fFlags(0) {}
    virtual ~NetDevice() { discard(); }
    int flags() const { return fFlags; }
    bool isUp() {
        fFlags = 0;
        getFlags();
        int flag = netstatusShowOnlyRunning ? IFF_RUNNING : IFF_UP;
        return (flag & fFlags) != 0;
    }
    virtual void getCurrent(netbytes *in, netbytes *out,
                            const char* sharedData) = 0;
protected:
    mstring fDevName;
    int fFlags;
    int descriptor() {
        if (fSocket < 0) {
#if __linux__
            fSocket = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
#else
            fSocket = socket(AF_INET, SOCK_DGRAM, 0);
            if (fSocket > 0) {
                fcntl(fSocket, F_SETFD, FD_CLOEXEC);
            }
#endif
            if (fSocket < 0) {
                fail("socket");
            }
        }
        return fSocket;
    }
    void discard() {
        if (fSocket > 0) {
            close(fSocket);
            fSocket = -1;
        }
    }
    virtual void getFlags() = 0;
private:
    static int fSocket;
};

int NetDevice::fSocket = -1;

class NetLinuxDevice : public NetDevice {
public:
    NetLinuxDevice(mstring netdev) : NetDevice(netdev) {}
    virtual void getFlags();
    virtual void getCurrent(netbytes *in, netbytes *out, const char* sharedData);
};

class NetFreeDevice : public NetDevice {
public:
    NetFreeDevice(mstring netdev) : NetDevice(netdev) {}
    virtual void getFlags();
    virtual void getCurrent(netbytes *in, netbytes *out, const char* sharedData);
};

class NetOpenDevice : public NetDevice {
public:
    NetOpenDevice(mstring netdev) : NetDevice(netdev) {}
    virtual void getFlags();
    virtual void getCurrent(netbytes *in, netbytes *out, const char* sharedData);
};

class NetDummyDevice : public NetDevice {
public:
    NetDummyDevice(mstring netdev) : NetDevice(netdev) {}
    virtual void getFlags() { }
    virtual void getCurrent(netbytes *in, netbytes *out, const char* sharedData)
    { }
};

static NetDevice* getNetDevice(mstring netdev)
{
    return
#if __linux__
        new NetLinuxDevice(netdev)
#elif __FreeBSD__
        new NetFreeDevice(netdev)
#elif __OpenBSD__ || __NetBSD__
        new NetOpenDevice(netdev)
#else
        new NetDummyDevice(netdev)
#endif
        ;
}

NetStatus::NetStatus(
    mstring netdev,
    NetStatusHandler* handler,
    YWindow *aParent):
    IApplet(this, aParent),
    fHandler(handler),
    fColorRecv(&clrNetReceive),
    fColorSend(&clrNetSend),
    fColorIdle(&clrNetIdle),
    ppp_in(new long[taskBarNetSamples]),
    ppp_out(new long[taskBarNetSamples]),
    prev_ibytes(0),
    start_ibytes(0),
    cur_ibytes(0),
    offset_ibytes(0),
    prev_obytes(0),
    start_obytes(0),
    cur_obytes(0),
    offset_obytes(0),
    start_time(monotime()),
    prev_time(start_time),
    oldMaxBytes(None),
    statusUpdateCount(0),
    unchanged(0),
    wasUp(false),
    fDevName(netdev),
    fDevice(getNetDevice(netdev))
{
    for (int i = 0; i < taskBarNetSamples; i++)
        ppp_in[i] = ppp_out[i] = 0;

    setSize(taskBarNetSamples, taskBarGraphHeight);

    getCurrent(nullptr, nullptr, nullptr);
    updateStatus(nullptr);
    if (isUp()) {
        updateVisible(true);
    }
    setTitle("NET-" + netdev);
}

NetStatus::~NetStatus() {
    delete[] ppp_in;
    delete[] ppp_out;
    delete fDevice;
}

bool NetStatus::isUp() {
    return fDevice->isUp();
}

void NetStatus::updateVisible(bool aVisible) {
    if (visible() != aVisible) {
        setVisible(aVisible);
        fHandler->relayout();
        isVisible = min(isVisible, aVisible);
    }
}

void NetStatus::timedUpdate(const char* sharedData, bool forceDown) {

    bool up = !forceDown && isUp();

    if (up) {
        if (!wasUp) {
            for (int i = 0; i < taskBarNetSamples; i++)
                ppp_in[i] = ppp_out[i] = 0;

            start_time = monotime();
            cur_ibytes = 0;
            cur_obytes = 0;

            updateStatus(sharedData);
            start_ibytes = cur_ibytes;
            start_obytes = cur_obytes;
            updateVisible(true);
            wasUp = true;
        }
        updateStatus(sharedData);

        if (toolTipVisible())
            updateToolTip();
    }
    else if (wasUp) {
        wasUp = false;
        updateVisible(false);
        setToolTip(null);
    }
}

void NetStatus::updateToolTip() {
    char status[400];

    if (wasUp) {
        char const * const sizeUnits[] = { "B", "KiB", "MiB", "GiB", "TiB", nullptr };
        char const * const rateUnits[] = { "B/s", "kB/s", "MB/s", nullptr };

        long const period(long(toDouble(monotime() - start_time)));

/*      long long vi(cur_ibytes - start_ibytes);
        long long vo(cur_obytes - start_obytes); */
        long long vi(cur_ibytes);
        long long vo(cur_obytes);

        long ci(ppp_in[taskBarNetSamples - 1]);
        long co(ppp_out[taskBarNetSamples - 1]);

        /* ai and oi were keeping nonsenses (if were not reset by
         * double-click) because of bad control of start_obytes and
         * start_ibytes (these were zeros that means buggy)
         *
         * Note: as start_obytes and start_ibytes now keep right values,
         * 'Transferred' now displays amount related to 'Online time' (and not
         * related to uptime of machine as was displayed before) -stibor- */
/*      long ai(t ? vi / t : 0);
        long ao(t ? vo / t : 0); */
        long long cai = 0;
        long long cao = 0;

        for (int ii = 0; ii < taskBarNetSamples; ii++) {
            cai += ppp_in[ii];
            cao += ppp_out[ii];
        }
        cai /= taskBarNetSamples;
        cao /= taskBarNetSamples;

        const char * const viUnit(niceUnit(vi, sizeUnits));
        const char * const voUnit(niceUnit(vo, sizeUnits));
        const char * const ciUnit(niceUnit(ci, rateUnits));
        const char * const coUnit(niceUnit(co, rateUnits));
/*      const char * const aiUnit(niceUnit(ai, rateUnits));
        const char * const aoUnit(niceUnit(ao, rateUnits)); */
        const char * const caoUnit(niceUnit(cao, rateUnits));
        const char * const caiUnit(niceUnit(cai, rateUnits));

        snprintf(status, sizeof status,
           /*   _("Interface %s:\n"
                  "  Current rate (in/out):\t%li %s/%li %s\n"
                  "  Current average (in/out):\t%lli %s/%lli %s\n"
                  "  Total average (in/out):\t%li %s/%li %s\n"
                  "  Transferred (in/out):\t%lli %s/%lli %s\n"
                  "  Online time:\t%ld:%02ld:%02ld"
                  "%s%s"), */
                _("Interface %s:\n"
                  "  Current rate (in/out):\t%li %s/%li %s\n"
                  "  Current average (in/out):\t%lli %s/%lli %s\n"
                  "  Transferred (in/out):\t%lli %s/%lli %s\n"
                  "  Online time:\t%ld:%02ld:%02ld"
                  "%s%s"),
                fDevName.c_str(),
                ci, ciUnit, co, coUnit,
                cai, caiUnit, cao, caoUnit,
/*              ai, aiUnit, ao, aoUnit, */
                vi, viUnit, vo, voUnit,
                period / 3600, period / 60 % 60, period % 60,
                "", "");
    } else {
        snprintf(status, sizeof status, "%.50s: %s", fDevName.c_str(),
                 (fDevice->flags() & (IFF_UP | IFF_RUNNING)) == IFF_UP ?
                 _("disconnected") : _("down"));
    }

    setToolTip(status);
}

void NetStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == Button1) {
        if (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2)) {
            if (up.state & ControlMask) {
                start_time = monotime();
                start_ibytes = cur_ibytes;
                start_obytes = cur_obytes;
            }
            else if (nonempty(netCommand)) {
                fHandler->runCommandOnce(netClassHint, netCommand);
            }
        }
    }
    else if (up.button == Button3) {
        fHandler->handleClick(up, fDevName);
    }
}

bool NetStatus::picture() {
    bool create = (hasPixmap() == false);

    Graphics G(getPixmap(), width(), height(), depth());

    if (create)
        fill(G);

    return (statusUpdateCount && unchanged < taskBarNetSamples)
        ? draw(G), true : create;
}

void NetStatus::fill(Graphics& g) {
    if (fColorIdle) {
        g.setColor(fColorIdle);
        g.fillRect(0, 0, width(), height());
    } else {
        ref<YImage> gradient(getGradient());

        if (gradient != null)
            g.drawImage(gradient,
                        x(), y(), width(), height(), 0, 0);
        else
            if (taskbackPixmap != null)
                g.fillPixmap(taskbackPixmap,
                             0, 0, width(), height(), x(), y());
    }
}

void NetStatus::draw(Graphics &g) {
    long h = height();

    long b_in_max = 0;
    long b_out_max = 0;

    for (int i = 0; i < taskBarNetSamples; i++) {
        long in = ppp_in[i];
        long out = ppp_out[i];
        if (in > b_in_max)
            b_in_max = in;
        if (out > b_out_max)
            b_out_max = out;
    }

    long maxBytes = max(b_in_max + b_out_max, 1024L);
    int first = (maxBytes != oldMaxBytes) ? 0 :
                max(0, taskBarNetSamples - statusUpdateCount);
    if (0 < first && first < taskBarNetSamples)
        g.copyArea(taskBarNetSamples - first, 0, first, h, 0, 0);
    const int limit = (first && statusUpdateCount <= (1 + unchanged) / 2)
                    ? taskBarNetSamples - statusUpdateCount : taskBarNetSamples;
    statusUpdateCount = 0;
    oldMaxBytes = maxBytes;

    for (int i = first; i < limit; i++) {
        if (true /* ppp_in[i] > 0 || ppp_out[i] > 0 */) {
            long round = maxBytes / h / 2;
            int inbar, outbar;

            if ((inbar = (h * (long long) (ppp_in[i] + round)) / maxBytes)) {
                g.setColor(fColorRecv);   /* h - 1 means bottom */
                g.drawLine(i, h - 1, i, h - inbar);
            }

            if ((outbar = (h * (long long) (ppp_out[i] + round)) / maxBytes)) {
                g.setColor(fColorSend);   /* 0 means top */
                g.drawLine(i, 0, i, outbar - 1);
            }

            if (inbar + outbar < h) {
                int l = outbar, t = h - inbar - 1;
                /*
                 g.setColor(fColorIdle);
                 //g.drawLine(i, 0, i, h - tot - 2);
                 g.drawLine(i, l, i, t - l);
                 */
                if (fColorIdle) {
                    g.setColor(fColorIdle);
                    g.drawLine(i, l, i, t);
                } else {
                    ref<YImage> gradient(getGradient());

                    if (gradient != null)
                        g.drawImage(gradient,
                                     x() + i, y() + l, width(), t - l, i, l);
                    else
                        if (taskbackPixmap != null)
                            g.fillPixmap(taskbackPixmap,
                                         i, l, width(), t - l, x() + i, y() + l);
                }
            }
        } else { /* Not reached: */
            if (fColorIdle) {
                g.setColor(fColorIdle);
                g.drawLine(i, 0, i, h - 1);
            } else {
                ref<YImage> gradient(getGradient());

                if (gradient != null)
                    g.drawImage(gradient,
                                 x() + i, y(), width(), h, i, 0);
                else
                    if (taskbackPixmap != null)
                        g.fillPixmap(taskbackPixmap,
                                     i, 0, width(), h, x() + i, y());
            }
        }
    }
}

#if __linux__
void NetLinuxDevice::getFlags() {
    struct ifreq ifr;
    strlcpy(ifr.ifr_name, fDevName, IFNAMSIZ);
    int s = descriptor();
    if (s != -1 && ioctl(s, SIOCGIFFLAGS, &ifr) != -1) {
       fFlags = ifr.ifr_flags;
    } else {
        discard();
    }
}
#endif

#if __NetBSD__ || __OpenBSD__
void NetOpenDevice::getFlags() {
    struct ifreq ifr;
    strlcpy(ifr.ifr_name, fDevName, IFNAMSIZ);
    int s = descriptor();
    if (s != -1 && ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) != -1) {
        fFlags = ifr.ifr_flags;
    } else {
        discard();
    }
}
#endif

#if __FreeBSD__
void NetFreeDevice::getFlags() {
    struct ifmibdata ifmd;
    size_t ifmd_size = sizeof ifmd;
    int nr_network_devs;
    size_t int_size = sizeof nr_network_devs;
    int name[6];
    name[0] = CTL_NET;
    name[1] = PF_LINK;
    name[2] = NETLINK_GENERIC;
    name[3] = IFMIB_IFDATA;
    name[5] = IFDATA_GENERAL;

    if (sysctlbyname("net.link.generic.system.ifcount", &nr_network_devs,
                    &int_size, (void*)0, 0) == -1) {
        printf("%s@%d: %s\n", __FILE__, __LINE__, strerror(errno));
    } else {
        for (int i = 1; i <= nr_network_devs; i++) {
            name[4] = i; /* row of the ifmib table */

            if (sysctl(name, 6, &ifmd, &ifmd_size, (void *)0, 0) == -1) {
                printf("%s@%d: %s\n", __FILE__, __LINE__, strerror(errno));
                continue;
            }
            if (fDevName == ifmd.ifmd_name) {
                fFlags = ifmd.ifmd_flags;
                return;
            }
        }
    }
}
#endif

void NetStatus::updateStatus(const char* sharedData) {
    int last = taskBarNetSamples - 1;

    for (int i = 0; i < last; i++) {
        ppp_in[i] = ppp_in[i + 1];
        ppp_out[i] = ppp_out[i + 1];
    }
    getCurrent(&ppp_in[last], &ppp_out[last], sharedData);
    /* These two lines clears first measurement; you can throw these lines
     * off, but bug will occur: on startup, the _second_ bar will show
     * always zero -stibor- */
    if (!wasUp)
        ppp_in[last] = ppp_out[last] = 0;

    ++statusUpdateCount;

    bool same = (0 < last &&
                 ppp_in[last] == ppp_in[last - 1] &&
                 ppp_out[last] == ppp_out[last - 1]);
    unchanged = same ? 1 + unchanged : 0;

    repaint();
}


#if __linux__
void NetLinuxDevice::getCurrent(netbytes *in, netbytes *out, const char* sharedData) {
#if __FreeBSD_kernel__
    NetFreeDevice(fDevName).getCurrent(in, out, sharedData);
#else
    if (sharedData)
        sscanf(sharedData, "%llu %*d %*d %*d %*d %*d %*d %*d %llu", in, out);
#endif
}
#endif //__linux__

#if __FreeBSD__ || __FreeBSD_kernel__
void NetFreeDevice::getCurrent(netbytes *in, netbytes *out, const char* sharedData) {
    // FreeBSD code by Ronald Klop <ronald@cs.vu.nl>
    struct ifmibdata ifmd;
    size_t ifmd_size = sizeof ifmd;
    int nr_network_devs;
    size_t int_size = sizeof nr_network_devs;
    int name[6];
    name[0] = CTL_NET;
    name[1] = PF_LINK;
    name[2] = NETLINK_GENERIC;
    name[3] = IFMIB_IFDATA;
    name[5] = IFDATA_GENERAL;

    if (sysctlbyname("net.link.generic.system.ifcount", &nr_network_devs,
                    &int_size, (void*)0, 0) == -1) {
        printf("%s@%d: %s\n", __FILE__, __LINE__, strerror(errno));
    } else {
        for (int i = 1; i <= nr_network_devs; i++) {
            name[4] = i; /* row of the ifmib table */

            if (sysctl(name, 6, &ifmd, &ifmd_size, (void *)0, 0) == -1) {
                warn("%s@%d: %s\n", __FILE__, __LINE__, strerror(errno));
                continue;
            }
            if (fDevName == ifmd.ifmd_name) {
                *in = ifmd.ifmd_data.ifi_ibytes;
                *out = ifmd.ifmd_data.ifi_obytes;
                break;
            }
        }
    }
}
#endif //FreeBSD

#if __OpenBSD__ || __NetBSD__
void NetOpenDevice::getCurrent(netbytes *in, netbytes *out, const char* sharedData) {
#if __OpenBSD__
    struct ifreq ifdr;
    struct if_data ifi;
    strlcpy(ifdr.ifr_name, fDevName, IFNAMSIZ);
    ifdr.ifr_data = (caddr_t) &ifi;
#elif __NetBSD__
    struct ifdatareq ifdr;
    struct if_data& ifi = ifdr.ifdr_data;
    strlcpy(ifdr.ifdr_name, fDevName, IFNAMSIZ);
#endif
    int s = descriptor();
    if (s != -1 && ioctl(s, SIOCGIFDATA, &ifdr) != -1) {
        *in = ifi.ifi_ibytes;
        *out = ifi.ifi_obytes;
    }
}
#endif //__OpenBSD__

void NetStatus::getCurrent(long *in, long *out, const char* sharedData) {
    cur_ibytes = 0;
    cur_obytes = 0;

    fDevice->getCurrent(&cur_ibytes, &cur_obytes, sharedData);

    // correct the values and look for overflows
    //msg("w/o corrections: ibytes: %lld, prev_ibytes; %lld, offset: %lld", cur_ibytes, prev_ibytes, offset_ibytes);

    cur_ibytes += offset_ibytes;
    cur_obytes += offset_obytes;

    if (cur_ibytes < prev_ibytes)
        // har, har, overflow. Use the recent prev_ibytes value as offset this time
        cur_ibytes = offset_ibytes = prev_ibytes;

    if (cur_obytes < prev_obytes)
        // har, har, overflow. Use the recent prev_obytes value as offset this time
        cur_obytes = offset_obytes = prev_obytes;

    timeval curr_time = monotime();
    double delta_t = max(0.001, toDouble(curr_time - prev_time));

    if (in && out) {
        *in  = (long) ((cur_ibytes - prev_ibytes) / delta_t);
        *out = (long) ((cur_obytes - prev_obytes) / delta_t);
    }

    prev_time = curr_time;
    prev_ibytes = cur_ibytes;
    prev_obytes = cur_obytes;
}

NetStatusControl::~NetStatusControl() {
    for (int i = 0; i < fNetStatus.getCount(); ++i) {
        NetStatus* status = nullptr;
        swap(status, fNetStatus[i].value);
        if (status)
            delete status;
    }
    if (fNetDev > 0) {
        close(fNetDev);
        fNetDev = -1;
    }
}

NetStatusControl::NetStatusControl(IApp* app, YSMListener* smActionListener,
        IAppletContainer* taskBar, YWindow* aParent) :
    smActionListener(smActionListener),
    taskBar(taskBar),
    aParent(aParent),
    fPid(0),
    fNetDev(-1)
{
    getInterfaces(interfaces);
    mstring devName, devList(netDevice);
    while (devList.splitall(' ', &devName, &devList)) {
        if (devName.isEmpty())
            ;
        else if (strpbrk(devName, "*?[]\\.")) {
            YStringArray::IterType iter = interfaces.reverseIterator();
            while (++iter) {
                if (upath(iter).fnMatch(devName) == 0 && !fNetStatus.has(iter)) {
                    createNetStatus(*iter);
                }
            }
            patterns.append(devName);
        }
        else if (!fNetStatus.has(devName)) {
            if (interfaces.find(devName) != -1)
                createNetStatus(devName);
            else
                patterns.append(devName);
        }
    }
    interfaces.clear();

    fUpdateTimer->setTimer(taskBarNetDelay, this, true);
}

NetStatus* NetStatusControl::createNetStatus(mstring netdev) {
    NetStatus*& status = fNetStatus[netdev];
    if (status == nullptr)
        status = new NetStatus(netdev, this, aParent);
    return status;
}

void NetStatusControl::getInterfaces(YStringArray& names)
{
    names.clear();

    struct if_nameindex* ifs = if_nameindex(), *i = ifs;
    if (ifs) {
        int const stop(99);
        for (; i && i->if_index && i->if_name && i < ifs + stop; ++i) {
            names.append(i->if_name);
        }
        if_freenameindex(ifs);
        names.sort();
    }
    else
        fail("if_nameindex");
}

bool NetStatusControl::handleTimer(YTimer *t)
{
    if (t != fUpdateTimer)
        return false;

#if __linux__
    linuxUpdate();
#else
    for (IterType iter = getIterator(); ++iter; )
        if (*iter)
            iter->timedUpdate();
#endif

    return true;
}

void NetStatusControl::runCommandOnce(const char *resource, const char *cmdline)
{
    smActionListener->runCommandOnce(resource, cmdline, &fPid);
}

void NetStatusControl::relayout()
{
    taskBar->relayout();
}

void NetStatusControl::handleClick(const XButtonEvent &up, mstring netdev)
{
    if (up.button == Button3) {
        getInterfaces(interfaces);

        fMenu = new YMenu();
        fMenu->setActionListener(this);
        fMenu->addItem(_("NET"), -2, null, actionNull)->setEnabled(false);
        fMenu->addSeparator();
        YStringArray::IterType iter = interfaces.iterator();
        while (++iter) {
            bool enable = true;
            bool visible = false;
            if (fNetStatus.has(*iter) == false || fNetStatus[*iter] == nullptr) {
                enable = osmart<NetDevice>(getNetDevice(*iter))->isUp();
            }
            else if (fNetStatus[*iter]->visible()) {
                visible = true;
                enable = true;  // fNetStatus[*iter]->isUp();
            }
            YAction act(EAction(visible + 2 * (300 + iter.where())));
            YMenuItem* item = fMenu->addItem(*iter, -2, null,
                                             enable ? act : actionNull);
            item->setChecked(visible);
            item->setEnabled(enable);
        }
        fMenu->popup(nullptr, nullptr, nullptr, up.x_root, up.y_root,
                     YPopupWindow::pfCanFlipVertical |
                     YPopupWindow::pfCanFlipHorizontal |
                     YPopupWindow::pfPopupMenu);
    }
}

void NetStatusControl::actionPerformed(YAction action, unsigned int modifiers) {
    bool hide(hasbit(action.ident(), true));
    int index(action.ident() / 2 - 300);
    if (inrange(index, 0, interfaces.getCount() - 1)) {
        const char* name = interfaces[index];
        if (hide) {
            if (fNetStatus.has(name) && fNetStatus[name]->visible()) {
                fNetStatus[name]->hide();
                relayout();
            }
        }
        else {
            createNetStatus(name)->show();
            relayout();
        }
    }
    fMenu = nullptr;
    interfaces.clear();
}

#if __linux__
bool NetStatusControl::readNetDev(char* data, size_t size) {
    const char path[] = "/proc/net/dev";

    if (fNetDev >= 0 && lseek(fNetDev, 0, SEEK_SET) < 0) {
        fail("seek %s", path);
        close(fNetDev);
        fNetDev = -1;
    }

    if (fNetDev < 0) {
        fNetDev = open(path, O_RDONLY | O_CLOEXEC);
        if (fNetDev < 0) {
            fail("open %s", path);
            return false;
        }
    }

    ssize_t len = read(fNetDev, data, size - 1);
    if (len <= 0) {
        fail("read %s", path);
        close(fNetDev);
        fNetDev = -1;
        return false;
    }

    data[len] = '\0';
    return true;
}

void NetStatusControl::linuxUpdate() {
    size_t const textSize = 8192;
    char devicesText[textSize];
    if (readNetDev(devicesText, textSize) == false) {
        return;
    }

    int const count(fNetStatus.getCount());
    bool covered[count];
    for (int i = 0; i < count; ++i) {
        covered[i] = (nullptr == fNetStatus[i]);
    }
    using netpair = pair<const char *, const char *>;
    YArray<netpair> pending;
    auto checkIfInput = [&](const char* name, const char* data) {
        int index;
        if (fNetStatus.find(name, &index)) {
            if (index < count && fNetStatus[index]) {
                if (covered[index] == false) {
                    fNetStatus[index]->timedUpdate(data);
                    covered[index] = true;
                }
            }
        }
        else {
            // oh, we got a new device? allowed?
            pending.append(netpair(name, data));
        }
    };

    for (char* p = devicesText; (p = strchr(p, '\n')) != nullptr; ) {
        *p = 0;
        while (*++p == ' ');
        char* name = p;
        p += strcspn(p, " :|\n");
        if (*p == ':') {
            *p = 0;
            while (*++p && *p == ' ');
            if (*p && *p != '\n')
                checkIfInput(name, p);
        }
    }
    // mark disappeared devices as down without additional ioctls
    for (int i = 0; i < count; ++i) {
        if (covered[i] == false && fNetStatus[i])
            fNetStatus[i]->timedUpdate(nullptr, true);
    }
    for (const auto& nameAndData : pending) {
        auto pat = patterns.iterator();
        while (++pat && upath(nameAndData.left).fnMatch(pat));
        if (pat) {
            createNetStatus(nameAndData.left)->timedUpdate(nameAndData.right);
        } else {
            // ignore this device
            fNetStatus[nameAndData.left] = nullptr;
        }
    }
}
#endif

// vim: set sw=4 ts=4 et:
