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

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <sys/sysctl.h>
#include <net/if_mib.h>
#endif

extern ref<YPixmap> taskbackPixmap;

static NetDevice* getNetDevice(mstring netdev)
{
    return
#if defined(__linux__)
        netdev.startsWith("ippp")
            ? (NetDevice *) new NetIsdnDevice(netdev)
            : (NetDevice *) new NetLinuxDevice(netdev)
#elif defined(__FreeBSD__)
        new NetFreeDevice(netdev)
#elif defined(__OpenBSD__) || defined(__NetBSD__)
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
    useIsdn(netdev.startsWith("ippp")),
    fDevName(netdev),
    fDevice(getNetDevice(netdev))
{
    for (int i = 0; i < taskBarNetSamples; i++)
        ppp_in[i] = ppp_out[i] = 0;

    color[0] = &clrNetReceive;
    color[1] = &clrNetSend;
    color[2] = &clrNetIdle;

    setSize(taskBarNetSamples, taskBarGraphHeight);

    getCurrent(nullptr, nullptr, nullptr);
    updateStatus(nullptr);
    if (isUp()) {
        updateVisible(true);
    }
    setTitle("NET-" + netdev);
    updateToolTip();
}

NetStatus::~NetStatus() {
    delete[] ppp_in;
    delete[] ppp_out;
}

void NetStatus::updateVisible(bool aVisible) {
    if (visible() != aVisible) {
        setVisible(aVisible);
        fHandler->relayout();
        isVisible = min(isVisible, aVisible);
    }
}

void NetStatus::timedUpdate(const void* sharedData, bool forceDown) {

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
        }
        updateStatus(sharedData);

        if (toolTipVisible() || !wasUp)
            updateToolTip();
    }
    else // link is down
        if (wasUp) {
            updateVisible(false);
            updateToolTip();
        }

    wasUp = up;
}

void NetStatus::updateToolTip() {
    char status[400];

    if (isUp()) {
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

        const char* phoneNumber = fDevice->getPhoneNumber();
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
                *phoneNumber ? _("\n  Caller id:\t") : "", phoneNumber);
    } else {
        snprintf(status, sizeof status, "%.50s: down", fDevName.c_str());
    }

    setToolTip(status);
}

void NetStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2)) {
            if (up.state & ControlMask) {
                start_time = monotime();
                start_ibytes = cur_ibytes;
                start_obytes = cur_obytes;
            } else {
                if (netCommand && netCommand[0])
                    fHandler->runCommandOnce(netClassHint, netCommand);
            }
        }
    }
    else if (up.button == Button3) {
        fHandler->handleClick(up, name());
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
    if (color[2]) {
        g.setColor(color[2]);
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
                g.setColor(color[0]);   /* h - 1 means bottom */
                g.drawLine(i, h - 1, i, h - inbar);
            }

            if ((outbar = (h * (long long) (ppp_out[i] + round)) / maxBytes)) {
                g.setColor(color[1]);   /* 0 means top */
                g.drawLine(i, 0, i, outbar - 1);
            }

            if (inbar + outbar < h) {
                int l = outbar, t = h - inbar - 1;
                /*
                 g.setColor(color[2]);
                 //g.drawLine(i, 0, i, h - tot - 2);
                 g.drawLine(i, l, i, t - l);
                 */
                if (color[2]) {
                    g.setColor(color[2]);
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
            if (color[2]) {
                g.setColor(color[2]);
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

/**
 * Check isdnstatus, by parsing /dev/isdninfo.
 *
 * Need read-access on /dev/isdninfo.
 */
#ifdef __linux__
bool NetIsdnDevice::isUp() {
    auto str(filereader("/dev/isdninfo").read_all());
    char val[5][32];
    int busage = 0;
    int bflags = 0;

    for (char* p = str; p && *p && p[1]; p = strchr(p, '\n')) {
        p += strspn(p, " \n");
        if (strncmp(p, "flags:", 6) == 0) {
            sscanf(p, "%s %s %s %s %s", val[0], val[1], val[2], val[3], val[4]);
            for (int i = 0; i < 4; i++) {
                if (strcmp(val[i+1], "0") != 0)
                    bflags |= (1 << i);
            }
        }
        else if (strncmp(p, "usage:", 6) == 0) {
            sscanf(p, "%s %s %s %s %s", val[0], val[1], val[2], val[3], val[4]);
            for (int i = 0; i < 4; i++) {
                if (strcmp(val[i+1], "0") != 0)
                    busage |= (1 << i);
            }
        }
        else if (strncmp(p, "phone:", 6) == 0) {
            sscanf(p, "%s %s %s %s %s", val[0], val[1], val[2], val[3], val[4]);
            for (int i = 0; i < 4; i++) {
                if (strncmp(val[i+1], "?", 1) != 0)
                    strlcpy(phoneNumber, val[i + 1], sizeof phoneNumber);
            }
        }
    }

    //msg("dbs: flags %d usage %d", bflags, busage);

    return bflags && busage;
}
#endif // ifdef __linux__

#if defined (__NetBSD__) || defined (__OpenBSD__)
bool NetOpenDevice::isUp() {
    bool up = false;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s != -1) {
        struct ifreq ifr;
        strlcpy(ifr.ifr_name, fDevName, IFNAMSIZ);
        if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) != -1) {
            up = (ifr.ifr_flags & IFF_UP);
        }
        close(s);
    }
    return up;
}
#endif

#if defined (__FreeBSD__)
bool NetFreeDevice::isUp() {
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
                return (ifmd.ifmd_flags & IFF_UP);
            }
        }
    }
    return false;
}
#endif

#ifdef __linux__
bool NetLinuxDevice::isUp() {
    int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1)
        return false;

    struct ifreq ifr;
    strlcpy(ifr.ifr_name, fDevName, IFNAMSIZ);
    bool up = (ioctl(s, SIOCGIFFLAGS, &ifr) >= 0 && (ifr.ifr_flags & IFF_UP));
    close(s);
    return up;
}
#endif

void NetStatus::updateStatus(const void* sharedData) {
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


#ifdef __linux__
void NetLinuxDevice::getCurrent(netbytes *in, netbytes *out, const void* sharedData) {
#if defined(__FreeBSD_kernel__)
    NetFreeDevice(fDevName).getCurrent(in, out, sharedData);
#else
    const char *p = (const char*) sharedData;
    if (p)
        sscanf(p, "%llu %*d %*d %*d %*d %*d %*d %*d %llu", in, out);

#endif
}
#endif //__linux__

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
void NetFreeDevice::getCurrent(netbytes *in, netbytes *out, const void* sharedData) {
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

#if defined(__OpenBSD__) || defined(__NetBSD__)
void NetOpenDevice::getCurrent(netbytes *in, netbytes *out, const void* sharedData) {
    int s;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s != -1) {
#ifdef __OpenBSD__
        struct ifreq ifdr;
        struct if_data ifi;
        strlcpy(ifdr.ifr_name, fDevName, IFNAMSIZ);
        ifdr.ifr_data = (caddr_t) &ifi;
#endif
#ifdef __NetBSD__
        struct ifdatareq ifdr;
        struct if_data& ifi = ifdr.ifdr_data;
        strlcpy(ifdr.ifdr_name, fDevName, IFNAMSIZ);
#endif
        if (ioctl(s, SIOCGIFDATA, &ifdr) != -1) {
            *in = ifi.ifi_ibytes;
            *out = ifi.ifi_obytes;
        }
        close(s);
    }
}
#endif //__OpenBSD__

void NetStatus::getCurrent(long *in, long *out, const void* sharedData) {
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
}

#ifdef __linux__
void NetStatusControl::fetchSystemData() {
    devStats.clear();
    devicesText = filereader("/proc/net/dev").read_all();
    if (devicesText == nullptr)
        return;

    for (char* p = devicesText; (p = strchr(p, '\n')) != nullptr; ) {
        *p = 0;
        while (*++p == ' ');
        char* name = p;
        p += strcspn(p, " :|\n");
        if (*p == ':') {
            *p = 0;
            while (*++p && *p == ' ');
            if (*p && *p != '\n')
                devStats.append(netpair(name, p));
        }
    }
}
#endif

NetStatusControl::NetStatusControl(IApp* app, YSMListener* smActionListener,
        IAppletContainer* taskBar, YWindow* aParent) :
    smActionListener(smActionListener),
    taskBar(taskBar),
    aParent(aParent),
    fPid(0)
{
    mstring devName, devList(netDevice);
    while (devList.splitall(' ', &devName, &devList)) {
        if (devName.isEmpty())
            continue;
        mstring devStr(devName);

        if (strpbrk(devStr, "*?[]\\.")) {
            if (interfaces.isEmpty())
                getInterfaces(interfaces);
            YStringArray::IterType iter = interfaces.reverseIterator();
            while (++iter) {
                if (fNetStatus.has(iter))
                    continue;
                if (upath(iter).fnMatch(devStr) == 0) {
                    createNetStatus(*iter);
                }
            }
            patterns.append(devStr);
        }
        else {
            unsigned index = if_nametoindex(devStr);
            if (1 <= index)
                createNetStatus(devStr);
            else
                patterns.append(devStr);
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
    unsigned const stop(99);

    struct if_nameindex* ifs = if_nameindex(), *i = ifs;
    for (; i && i->if_index && i->if_name && i - ifs < stop; ++i)
        names.append(i->if_name);
    if (ifs)
        if_freenameindex(ifs);
    else
        fail("if_nameindex");

    names.sort();
}

bool NetStatusControl::handleTimer(YTimer *t)
{
    if (t != fUpdateTimer)
        return false;

#ifdef __linux__
    linuxUpdate();
#else
    for (IterType iter = getIterator(); ++iter; )
        if (*iter != 0)
            iter->timedUpdate(0);
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
        interfaces.clear();
        getInterfaces(interfaces);

        fMenu = new YMenu();
        fMenu->setActionListener(this);
        fMenu->addItem(_("NET"), -2, null, actionNull)->setEnabled(false);
        fMenu->addSeparator();
        YStringArray::IterType iter = interfaces.iterator();
        while (++iter) {
            bool enable = true;
            bool visible = false;
            if (fNetStatus.has(*iter) == false || fNetStatus[*iter] == 0) {
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

#ifdef __linux__
void NetStatusControl::linuxUpdate() {
    fetchSystemData();

    int const count(fNetStatus.getCount());
    bool covered[count];
    for (int i = 0; i < count; ++i)
        covered[i] = fNetStatus[i] == 0;

    YArray<netpair> pending;

    for (IterStats stat = devStats.iterator(); ++stat; ) {
        int index;
        if (fNetStatus.find((*stat).name(), &index)) {
            if (index < count && fNetStatus[index]) {
                if (covered[index] == false) {
                    fNetStatus[index]->timedUpdate((*stat).data());
                    covered[index] = true;
                }
            }
        }
        else {
            // oh, we got a new device? allowed?
            pending.append(stat);
        }
    }

    // mark disappeared devices as down without additional ioctls
    for (int i = 0; i < count; ++i)
        if (covered[i] == false && fNetStatus[i])
            fNetStatus[i]->timedUpdate(nullptr, true);

    for (int i = 0; i < pending.getCount(); ++i) {
        const netpair stat = pending[i];
        YStringArray::IterType pat = patterns.iterator();
        while (++pat && upath(stat.name()).fnMatch(pat));
        if (pat) {
            createNetStatus(stat.name())->timedUpdate(stat.data());
        } else {
            // ignore this device
            fNetStatus[stat.name()] = 0;
        }
    }

    devStats.clear();
    devicesText = nullptr;
}
#endif

// vim: set sw=4 ts=4 et:
