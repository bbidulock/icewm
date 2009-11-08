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
// KNOWN BUGS: - first measurement is throwed off
//
// //////////////////////////////////////////////////////////////////////////

#define NEED_TIME_H

#include "config.h"
#include "ylib.h"
#include "sysdep.h"

#include "wmtaskbar.h"
#include "apppstatus.h"

#include "wmapp.h"

#ifdef HAVE_NET_STATUS
#include "prefs.h"
#include "intl.h"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#include <net/if_mib.h>
#endif

extern ref<YPixmap> taskbackPixmap;

NetStatus::NetStatus(mstring netdev, IAppletContainer *taskBar, YWindow *aParent):
    YWindow(aParent), fNetDev(netdev)
{
    fTaskBar = taskBar;
    ppp_in = new long[taskBarNetSamples];
    ppp_out = new long[taskBarNetSamples];

    // clear out the data
    for (int i = 0; i < taskBarNetSamples; i++)
        ppp_in[i] = ppp_out[i] = 0;

    color[0] = new YColor(clrNetReceive);
    color[1] = new YColor(clrNetSend);
    color[2] = *clrNetIdle
        ? new YColor(clrNetIdle) : NULL;

    setSize(taskBarNetSamples, 20);

    fUpdateTimer = new YTimer();
    if (fUpdateTimer) {
        fUpdateTimer->setInterval(taskBarNetDelay);
        fUpdateTimer->setTimerListener(this);
        fUpdateTimer->startTimer();
    }
    prev_ibytes = prev_obytes = offset_ibytes = offset_obytes = 0;
    memset(&prev_time, 0, sizeof(prev_time));
    // set prev values for first updateStatus

    getCurrent(0, 0);
    wasUp = true;

    // test for isdn-device
    useIsdn = fNetDev.startsWith("ippp");
    // unset phoneNumber
    strcpy(phoneNumber,"");

    updateStatus();
    start_time = time(NULL);
    start_ibytes = cur_ibytes;
    start_obytes = cur_obytes;
    updateToolTip();
    updateVisible(true);
}

NetStatus::~NetStatus() {
    delete[] color;
    delete[] ppp_in;
    delete[] ppp_out;
    delete fUpdateTimer;
}


void NetStatus::updateVisible(bool aVisible) {
    if (visible() != aVisible) {
        if (aVisible)
            show();
        else
            hide();

        fTaskBar->relayout();
    }
}

bool NetStatus::handleTimer(YTimer *t) {
    if (t != fUpdateTimer)
        return false;

    bool up = isUp();

    if (up) {
        if (!wasUp) {
            for (int i = 0; i < taskBarNetSamples; i++)
                ppp_in[i] = ppp_out[i] = 0;

            start_time = time(NULL);
            cur_ibytes = 0;
            cur_obytes = 0;

            updateStatus();
            start_ibytes = cur_ibytes;
            start_obytes = cur_obytes;
            updateVisible(true);
        }
        updateStatus();

        if (toolTipVisible())
            updateToolTip();
    }
    else // link is down
        if (wasUp)
            updateVisible(false);

    wasUp = up;
    return true;
}

void NetStatus::updateToolTip() {
    char status[400];
    cstring netdev(fNetDev);
    
    if (isUp()) {
        char const * const sizeUnits[] = { "B", "KiB", "MiB", "GiB", "TiB", NULL };
        char const * const rateUnits[] = { "Bps", "Kps", "Mps", NULL };

        long const t(time(NULL) - start_time);

        long long vi(cur_ibytes - start_ibytes);
        long long vo(cur_obytes - start_obytes);

        long ci(ppp_in[taskBarNetSamples - 1]);
        long co(ppp_out[taskBarNetSamples - 1]);

        /* ai and oi were keeping nonsenses (if were not reset by
         * double-click) because of bad control of start_obytes and
         * start_ibytes (these were zeros that means buggy)
         *
         * Note: as start_obytes and start_ibytes now keep right values,
         * 'Transferred' now displays amount related to 'Online time' (and not
         * related to uptime of machine as was displayed before) -stibor- */
        long ai(t ? vi / t : 0);
        long ao(t ? vo / t : 0);
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
        const char * const aiUnit(niceUnit(ai, rateUnits));
        const char * const aoUnit(niceUnit(ao, rateUnits));
        const char * const caoUnit(niceUnit(cao, rateUnits));
        const char * const caiUnit(niceUnit(cai, rateUnits));

        sprintf(status,
                _("Interface %s:\n"
                  "  Current rate (in/out):\t%li %s/%li %s\n"
                  "  Current average (in/out):\t%lli %s/%lli %s\n"
                  "  Total average (in/out):\t%li %s/%li %s\n"
                  "  Transferred (in/out):\t%lli %s/%lli %s\n"
                  "  Online time:\t%ld:%02ld:%02ld"
                  "%s%s"),
                netdev.c_str(),
                ci, ciUnit, co, coUnit,
                cai, caiUnit, cao, caoUnit,
                ai, aiUnit, ao, aoUnit,
                vi, viUnit, vo, voUnit,
                t / 3600, t / 60 % 60, t % 60,
                *phoneNumber ? _("\n  Caller id:\t") : "", phoneNumber);
    } else {
        sprintf(status, "%.50s:", netdev.c_str());
    }

    setToolTip(status);
}

void NetStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2)) {
            if (up.state & ControlMask) {
                start_time = time(NULL);
                start_ibytes = cur_ibytes;
                start_obytes = cur_obytes;
            } else {
                if (netCommand && netCommand[0])
                    wmapp->runCommandOnce(netClassHint, netCommand);
            }
        }
    }
}

void NetStatus::paint(Graphics &g, const YRect &/*r*/) {
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

    long maxBytes = b_in_max + b_out_max;
    if (maxBytes < 1024)
	maxBytes = 1024;
    ///!!! this should really be unified with acpustatus.cc
    for (int i = 0; i < taskBarNetSamples; i++) {
        if (1 /* ppp_in[i] > 0 || ppp_out[i] > 0 */) {
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
#ifdef CONFIG_GRADIENTS
                    ref<YImage> gradient(parent()->getGradient());

                    if (gradient != null)
                        g.drawImage(gradient,
                                     x() + i, y() + l, width(), t - l, i, l);
                    else
#endif
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
#ifdef CONFIG_GRADIENTS
                ref<YImage> gradient(parent()->getGradient());

                if (gradient != null)
                    g.drawImage(gradient,
                                 x() + i, y(), width(), h, i, 0);
                else
#endif
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
bool NetStatus::isUpIsdn() {
#ifdef linux
    char str[2048];
    char val[5][32];
    char *p = str;
    int busage;
    int bflags;
    long long len, i;
    int f = open("/dev/isdninfo", O_RDONLY);

    if (f < 0)
        return false;

    len = read(f, str, 2047);

    close(f);

    if (len <=0)
        return false;

    str[len]='\0';

    bflags=0;
    busage=0;

    //msg("dbs: len is %d", len);

    while( true ) {
        if (strncmp(p, "flags:", 6)==0) {
            sscanf(p, "%s %s %s %s %s", val[0], val[1], val[2], val[3], val[4]);
            for (i = 0 ; i < 4; i++) {
                if (strcmp(val[i+1],"0") != 0)
                    bflags|=1<<i;
            }
        }
        else if (strncmp(p, "usage:", 6)==0) {
            sscanf(p, "%s %s %s %s %s", val[0], val[1], val[2], val[3], val[4]);
            for (i = 0 ; i < 4; i++) {
                if (strcmp(val[i+1],"0") != 0)
                    busage|=1<<i;
            }
        }
        else if (strncmp(p, "phone:", 6)== 0) {
            sscanf(p, "%s %s %s %s %s", val[0], val[1], val[2], val[3], val[4]);
            for (i = 0; i < 4; i++) {
                if (strncmp(val[i+1], "?", 1) != 0)
                    strncpy(phoneNumber, val[i+1], 32);
            }
        }

        do { // find next line
            p++;
        } while((*p != '\0') &&
                (*p != '\n'));

        if (     *p == '\0' ||
            *(p+1) == '\0')
            break;

        p++; // skip '\n'
    }

    //msg("dbs: flags %d usage %d", bflags, busage);

    if (bflags != 0 && busage != 0)
        return true; // one or more ISDN-Links active
    else
        return false;
#else
    return false;
#endif // ifdef linux
}

bool NetStatus::isUp() {
#ifdef linux
    if (useIsdn)
        return isUpIsdn();
#endif

#if defined (__NetBSD__) || defined (__OpenBSD__)
    struct ifreq ifr;

    if (fNetDev == null)
        return false;

    int s = socket(AF_INET, SOCK_DGRAM, 0);

    if (s != -1) {
        cstring cs(fNetDev);
        strncpy(ifr.ifr_name, cs.c_str(), sizeof(ifr.ifr_name));
        if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) != -1) {
            if (ifr.ifr_flags & IFF_UP) {
                close(s);
                return true;
            }
        }
        close(s);
    }
    return false;
#else
#if defined (__FreeBSD__)
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
            cstring cs(fNetDev);
            if (strncmp(ifmd.ifmd_name, cs.c_str(), cs.c_str_len()) == 0) {
                return (ifmd.ifmd_flags & IFF_UP);
            }
        }
    }
    return false;
#else
    char buffer[32 * sizeof(struct ifreq)];
    struct ifconf ifc;
    struct ifreq *ifr;
    long long len;

    if (fNetDev == null)
        return false;  
    
    int s = socket(PF_INET, SOCK_STREAM, 0);

    if (s == -1)
        return false;

    ifc.ifc_len = sizeof(buffer);
    ifc.ifc_buf = buffer;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
        close(s);
        return false;
    }
    len = ifc.ifc_len;
    ifr = ifc.ifc_req;
    while (len > 0) {
        if (fNetDev.equals(ifr->ifr_name)) {
            close(s);
            return true;
        }
        len -= sizeof(struct ifreq);
        ifr++;
    }

    close(s);
    return false;
#endif
#endif
}

void NetStatus::updateStatus() {
    int last = taskBarNetSamples - 1;

    for (int i = 0; i < last; i++) {
        ppp_in[i] = ppp_in[i + 1];
        ppp_out[i] = ppp_out[i + 1];
    }
    getCurrent(&ppp_in[last], &ppp_out[last]);
    /* These two lines clears first measurement; you can throw these lines
     * off, but bug will occur: on startup, the _second_ bar will show
     * always zero -stibor- */
    if (!wasUp)
        ppp_in[last] = ppp_out[last] = 0;

    repaint();
}


void NetStatus::getCurrent(long *in, long *out) {
#if 0
    struct ifpppstatsreq req; // from <net/if_ppp.h> in the linux world

    memset(&req, 0, sizeof(req));

#ifdef linux
#undef ifr_name
#define ifr_name ifr__name

    req.stats_ptr = (caddr_t) &req.stats;

#endif // linux

    sprintf(req.ifr_name, PPP_DEVICE);

    if (ioctl(s, SIOCGPPPSTATS, &req) != 0) {
        if (errno == ENOTTY) {
            //perror("ioctl");
        }
        else { // just not connected?
            //perror("?? ioctl?");
            return;
        }
    }

#endif

    cur_ibytes = 0;
    cur_obytes = 0;


#ifdef linux
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return ;

    char buf[512];

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        char *p = buf;
        while (*p == ' ')
            p++;
        cstring cs(fNetDev);
        if (strncmp(p, cs.c_str(), cs.c_str_len()) == 0 &&
            p[cs.c_str_len()] == ':')
        {
            int dummy;
            p = strchr(p, ':') + 1;

            if (sscanf(p, "%llu %*d %*d %*d %*d %*d %*d %*d" " %llu %*d %*d %*d %*d %*d %*d %d",
                       &cur_ibytes, &cur_obytes, &dummy) != 3)
            {
                long long ipackets = 0, opackets = 0;

                sscanf(p, "%lld %*d %*d %*d %*d" " %lld %*d %*d %*d %*d %*d",
                       &ipackets, &opackets);
                // for linux<2.0 fake packets as bytes (we only need relative values anyway)
                cur_ibytes = ipackets;
                cur_obytes = opackets;
            }

            //msg("cur:%lld %lld", cur_ibytes, cur_obytes);

            break;
        }
    }
    fclose(fp);
#endif //linux
#ifdef __FreeBSD__
    // FreeBSD code by Ronald Klop <ronald@cs.vu.nl>
    struct ifmibdata ifmd;
    size_t ifmd_size=sizeof ifmd;
    int nr_network_devs;
    size_t int_size=sizeof nr_network_devs;
    int name[6];
    name[0] = CTL_NET;
    name[1] = PF_LINK;
    name[2] = NETLINK_GENERIC;
    name[3] = IFMIB_IFDATA;
    name[5] = IFDATA_GENERAL;

    if(sysctlbyname("net.link.generic.system.ifcount",&nr_network_devs,
                    &int_size,(void*)0,0) == -1) {
        printf("%s@%d: %s\n",__FILE__,__LINE__,strerror(errno));
    } else {
        for(int i=1;i<=nr_network_devs;i++) {
            name[4] = i; /* row of the ifmib table */

            if (sysctl(name, 6, &ifmd, &ifmd_size, (void *)0, 0) == -1) {
                warn("%s@%d: %s\n",__FILE__,__LINE__,strerror(errno));
                continue;
            }
	    if (mstring(ifmd.ifmd_name).compareTo(fNetDev) == 0) {
                cur_ibytes = ifmd.ifmd_data.ifi_ibytes;
                cur_obytes = ifmd.ifmd_data.ifi_obytes;
                break;
            }
        }
    }
#endif //FreeBSD
#ifdef __NetBSD__
    struct ifdatareq ifdr;
    struct if_data * const ifi = &ifdr.ifdr_data;
    int s;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s != -1) {
	fNetDev.copy(ifdr.ifdr_name, sizeof(ifdr.ifdr_name));
        if (ioctl(s, SIOCGIFDATA, &ifdr) != -1) {
            cur_ibytes = ifi->ifi_ibytes;
            cur_obytes = ifi->ifi_obytes;
        }
        close(s);
    }
#endif //__NetBSD__
#ifdef __OpenBSD__
    struct ifreq ifdr;
    struct if_data ifi;
    int s;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s != -1) {
	fNetDev.copy(ifdr.ifr_name, sizeof(ifdr.ifr_name));
        ifdr.ifr_data = (caddr_t) &ifi;
        if (ioctl(s, SIOCGIFDATA, &ifdr) != -1) {
            cur_ibytes = ifi.ifi_ibytes;
            cur_obytes = ifi.ifi_obytes;
        }
        close(s);
    }
#endif //__OpenBSD__
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

    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);

    double delta_t = (double) ((curr_time.tv_sec  - prev_time.tv_sec) * 1000000L
                               + (curr_time.tv_usec - prev_time.tv_usec)) / 1000000.0;

    if (in) {
        *in  = (long) ((cur_ibytes - prev_ibytes) / delta_t);
        *out = (long) ((cur_obytes - prev_obytes) / delta_t);
    }

    //msg("%d %d", *in, *out);

    prev_time.tv_sec = curr_time.tv_sec;
    prev_time.tv_usec = curr_time.tv_usec;


    prev_ibytes = cur_ibytes;
    prev_obytes = cur_obytes;
}

#endif // HAVE_NET_STATUS
