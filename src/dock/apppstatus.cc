// //////////////////////////////////////////////////////////////////////////
// IceWM: src/NetStatus.cc
// by Mark Lawrence <nomad@null.net>
//
// !!! share code with cpu status
// //////////////////////////////////////////////////////////////////////////
#include "config.h"

#include "yapp.h"
#include "ycstring.h"

#include "apppstatus.h"
#ifdef HAVE_NET_STATUS

#include "ybuttonevent.h"

#include "sysdep.h"
#include "prefs.h"
#include "ypaint.h"
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_mib.h>
#endif

YColorPrefProperty NetStatus::gColorReceive("netstatus_applet", "ColorReceive", "rgb:FF/00/FF");
YColorPrefProperty NetStatus::gColorSend("netstatus_applet", "ColorSend", "rgb:FF/FF/00");
YColorPrefProperty NetStatus::gColorIdle("netstatus_applet", "ColorIdle", "rgb:00/00/00");

static const char *gDefaultDevice = "ppp0";

NetStatus::NetStatus(YWindow *aParent):
    YWindow(aParent),
    fUpdateTimer(this, NET_UPDATE_INTERVAL),
    fNetDevice("netstatus_applet", "NetDevice")
{
    YPref prefSamples("cpustatus_applet", "TaskBarCPUSamples");
    long pvSamples = prefSamples.getNum(20);

    fNumSamples = pvSamples;

    // clear out the data
    for (int i = 0; i < fNumSamples + 1; i++) {
        ppp_in[i] = ppp_out[i] = ppp_tot[i] = 0;
    }

    color[0] = gColorReceive.getColor();
    color[1] = gColorSend.getColor();
    color[2] = gColorIdle.getColor();

    setSize(fNumSamples, 20);

    //fUpdateTimer = new YTimer();
    //if (fUpdateTimer) {
    //    fUpdateTimer->setInterval(NET_UPDATE_INTERVAL);
    //    fUpdateTimer->setTimerListener(this);
    fUpdateTimer.startTimer();
    //}
    prev_ibytes = prev_obytes = 0;
    // set prev values for first updateStatus
    maxBytes = 0; // initially
    getCurrent(0, 0, 0);
    wasUp = false;
    updateStatus();
    start_time = time(NULL);
    start_ibytes = cur_ibytes;
    start_obytes = cur_obytes;
    updateToolTip();
    maxBytes = 0; // initially
}

NetStatus::~NetStatus() {
    delete [] color;
    //delete fUpdateTimer;
}

bool NetStatus::handleTimer(YTimer *t) {
    if (t != &fUpdateTimer)
        return false;

    bool up = isUp();

    if (up) {
        if (!wasUp) {
            // clear out the data
            for (int i = 0; i < fNumSamples + 1; i++) {
                ppp_in[i] = ppp_out[i] = ppp_tot[i] = 0;
            }
            start_time = time(NULL);
            start_ibytes = cur_ibytes;
            start_obytes = cur_obytes;

            updateStatus();
            prev_ibytes = cur_ibytes;
            prev_obytes = cur_obytes;
            this->show();
        }
        updateStatus();
        if (toolTipVisible())
            updateToolTip();
    }
    else // link is down
        if (wasUp) this->hide();

    wasUp = up;
    return true;
}

void NetStatus::updateToolTip() {
    //char status[64];
    CStr *status = 0;
    int t = time(NULL) - start_time;
    int o = cur_obytes - start_obytes;
    int i = cur_ibytes - start_ibytes;

    if (t <= 0)
        status = CStr::format("%s:",
                              fNetDevice.getStr(gDefaultDevice));
    else
        status = CStr::format("%s: Sent: %db Rcvd: %db in %ds",
                              fNetDevice.getStr(gDefaultDevice),
                              o, i, t);

    setToolTip(status);
}

bool NetStatus::eventClick(const YClickEvent &up) {
    if (up.getButton() == 1 && up.isDoubleClick()) {
        if (up.isCtrl()) {
            start_time = time(NULL);
            start_ibytes = cur_ibytes;
            start_obytes = cur_obytes;
        } else {
            YPref prefCommand("netstatus_applet", "NetStatusCommand");
            const char *pvCommand = prefCommand.getStr(0);

            if (pvCommand && pvCommand[0])
                app->runCommand(pvCommand);
        }
        return true;
    }
    return YWindow::eventClick(up);
}

void NetStatus::paint(Graphics &g, const YRect &/*er*/) {
    int h = height();

    //!!! this should really be unified with acpustatus.cc
    for (int i = 0; i < fNumSamples; i++) {
        if (ppp_tot[i] > 0) {
            int in = (h * ppp_in[i] + maxBytes - 1) / maxBytes;
            int out = (h * ppp_out[i] + maxBytes - 1) / maxBytes;
            int t = h - in - 1;
            int l = out;

            //printf("in: %d:%d:%d, out: %d:%d:%d\n", in, t, ppp_in[i], out, l, ppp_out[i]);

            if (t < h - 1) {
                g.setColor(color[0]);
                g.drawLine(i, t, i, h - 1);
                t--;
            }

            if (l > 0) {
                g.setColor(color[1]);
                //g.drawLine (i, h - tot -1, i, h - in);
                g.drawLine(i, 0, i, l);
                l++;
            }

            if (l < t) {
                g.setColor(color[2]);
                //g.drawLine (i, 0, i, h - tot - 2);
                g.drawLine(i, l, i, t - l);
            }
        }
        else {
            g.setColor(color[2]);
            g.drawLine(i, 0, i, h - 1);
        }
    }
}

bool NetStatus::isUp() {
#if 0
    return true;
#else
    char buffer[32 * sizeof(struct ifreq)];
    struct ifconf ifc;
    struct ifreq *ifr;
    int len;

    const char *netDevice = fNetDevice.getStr(gDefaultDevice);

    if (netDevice == 0)
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
        if (strcmp(netDevice, ifr->ifr_name) == 0) {
            close(s);
            return true;
        }
        len -= sizeof(struct ifreq);
        ifr++;
    }

    close(s);
    return false;
#endif
}

void NetStatus::updateStatus() {
    for (int i = 1; i < fNumSamples; i++) {
        ppp_in[i - 1] = ppp_in[i];
        ppp_out[i - 1] = ppp_out[i];
        ppp_tot[i - 1] = ppp_tot[i];
    }

    int last = fNumSamples - 1;
    getCurrent(&ppp_in[last], &ppp_out[last], &ppp_tot[last]);
    repaint();
}


void NetStatus::getCurrent(int *in, int *out, int *tot) {
#if 0
    struct ifpppstatsreq req; // from <net/if_ppp.h> in the linux world

    memset(&req, 0, sizeof(req));

#ifdef linux
#undef ifr_name
#define ifr_name ifr__name

    req.stats_ptr = (caddr_t) &req.stats;

#endif // linux

    sprintf(req.ifr_name, fNetDevice.getStr(gDefaultDevice));

    if (ioctl(s, SIOCGPPPSTATS, &req) != 0) {
        if (errno == ENOTTY) {
            //perror ("ioctl");
        }
        else { // just not connected?
            //perror("ioctl?");
            return;
        }
    }

#endif

    cur_ibytes = 0;
    cur_obytes = 0;


#ifdef linux
    const char *netDevice = fNetDevice.getStr(gDefaultDevice);
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return;

    char buf[512];

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        char *p = buf;
        while (*p == ' ')
            p++;
        if (strncmp(p, netDevice, strlen(netDevice)) == 0 &&
            p[strlen(netDevice)] == ':')
        {
            int ipackets, opackets;
            int ierrs, oerrs;
            int idrop, odrop;
            int ififo, ofifo;
            int iframe;
            int ocolls;
            int ocarrier;
            int icomp, ocomp;
            int imcast;

            p = strchr(p, ':') + 1;

            if (sscanf(p, "%lu %d %d %d %d %d %d %d" " %lu %d %d %d %d %d %d %d",
                       &cur_ibytes, &ipackets, &ierrs, &idrop, &ififo, &iframe, &icomp, &imcast,
                       &cur_obytes, &opackets, &oerrs, &odrop, &ofifo, &ocolls, &ocarrier, &ocomp) != 16)
            {
                ipackets = opackets = 0;
                sscanf(p, "%d %d %d %d %d" " %d %d %d %d %d %d",
                       &ipackets, &ierrs, &idrop, &ififo, &iframe,
                       &opackets, &oerrs, &odrop, &ofifo, &ocolls, &ocarrier);
                // for linux<2.0 fake packets as bytes (we only need relative values anyway)
                cur_ibytes = ipackets;
                cur_obytes = opackets;
            }

            //printf("cur:%d %d %d\n", cur_ibytes, cur_obytes, maxBytes);

            break;
        }
    }
    fclose(fp);
#endif //linux
#ifdef __FreeBSD__
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
                printf("%s@%d: %s\n", __FILE__, __LINE__, strerror(errno));
                continue;
            }
            if (strncmp(ifmd.ifmd_name, netDevice, strlen(netDevice)) == 0) {
                cur_ibytes = ifmd.ifmd_data.ifi_ibytes;
                cur_obytes = ifmd.ifmd_data.ifi_obytes;
                break;
            }
        }
    }
#endif //FreeBSD

    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);

    double delta_t = (double) ((curr_time.tv_sec  - prev_time.tv_sec) * 1000000L
                             + (curr_time.tv_usec - prev_time.tv_usec)) / 1000000.0;

    int ni = (int)((cur_ibytes - prev_ibytes) / delta_t);
    int no = (int)((cur_obytes - prev_obytes) / delta_t);

    //printf("%d %d\n", ni, no);

    if (in) {
        *in  = ni;
        *out = no;
        *tot = *in + *out;
    }

    prev_time.tv_sec = curr_time.tv_sec;
    prev_time.tv_usec = curr_time.tv_usec;

    if (maxBytes == 0) // skip first read
        maxBytes = 1;
    else if (ni + no > maxBytes)
        maxBytes = ni + no;

    //printf("dif:%d %d %d\n", ni, no, maxBytes);

    prev_ibytes = cur_ibytes;
    prev_obytes = cur_obytes;
}

#endif
