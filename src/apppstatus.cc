// //////////////////////////////////////////////////////////////////////////
// IceWM: src/NetStatus.cc
// by Mark Lawrence <nomad@null.net>
//
// Linux-ISDN/ippp-Upgrade
// by Denis Boehme <denis.boehme@gmx.net>
//     6.01.2000
//
// !!! share code with cpu status
// //////////////////////////////////////////////////////////////////////////

#define NEED_TIME_H

#include "config.h"
#include "ylib.h"
#include "sysdep.h"

#include "apppstatus.h"

#include "wmapp.h"
#include "wmtaskbar.h"

#ifdef HAVE_NET_STATUS
#include "prefs.h"
#include "intl.h"
#include <net/if.h>
#include <sys/ioctl.h>

#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_mib.h>
#endif

extern YPixmap *taskbackPixmap;

NetStatus::NetStatus(char const * netdev, YWindow *aParent): 
    YWindow(aParent), fNetDev(newstr(netdev)) {
    // clear out the data
    for (int i = 0; i < NET_SAMPLES + 1; i++) {
        ppp_in[i] = ppp_out[i] = ppp_tot[i] = 0;
    }

    color[0] = new YColor(clrNetReceive);
    color[1] = new YColor(clrNetSend);
    color[2] = *clrNetIdle
	     ? new YColor(clrNetIdle) : NULL;

    setSize(NET_SAMPLES, 20);

    fUpdateTimer = new YTimer();
    if (fUpdateTimer) {
        fUpdateTimer->interval(NET_UPDATE_INTERVAL);
        fUpdateTimer->timerListener(this);
        fUpdateTimer->start();
    }
    prev_ibytes = prev_obytes = 0;
    // set prev values for first updateStatus
    maxBytes = 0; // initially
    getCurrent(0, 0, 0);
    wasUp = false;

    // test for isdn-device
    useIsdn = !strncmp(fNetDev,"ippp", 4);
    // unset phoneNumber
    strcpy(phoneNumber,"");

    updateStatus();
    start_time = time(NULL);
    start_ibytes = cur_ibytes;
    start_obytes = cur_obytes;
    updateToolTip();
    maxBytes = 0; // initially
}

NetStatus::~NetStatus() {
    delete[] fNetDev;
    delete[] color;
    delete fUpdateTimer;
}

bool NetStatus::handleTimer(YTimer *t) {
    if (t != fUpdateTimer)
        return false;

    bool up = isUp();

    if (up) {
        if (!wasUp) {
            // clear out the data
	    memset(ppp_in, 0, sizeof(ppp_in));
	    memset(ppp_out, 0, sizeof(ppp_out));
	    memset(ppp_tot, 0, sizeof(ppp_tot));

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
    char status[400];

    if (isUp()) {
	char const * const sizeUnits[] = { "b", "kb", "Mb", "Gb", "Tb", NULL };
	char const * const rateUnits[] = { "bps", "kps", "Mps", NULL };

	int const t(time(NULL) - start_time);

	int vi(cur_ibytes - start_ibytes);
	int vo(cur_obytes - start_obytes);
	
	int ci(ppp_in[NET_SAMPLES - 1]);
	int co(ppp_out[NET_SAMPLES - 1]);

	int ai(t ? vi / t : 0);
	int ao(t ? vo / t : 0);
    
	const char * const viUnit(niceUnit(vi, sizeUnits));
	const char * const voUnit(niceUnit(vo, sizeUnits));
	const char * const ciUnit(niceUnit(ci, rateUnits));
	const char * const coUnit(niceUnit(co, rateUnits));
	const char * const aiUnit(niceUnit(ai, rateUnits));
	const char * const aoUnit(niceUnit(ao, rateUnits));

        snprintf(status, sizeof(status),
		_("Interface %s:\n"
		  "  Current rate (in/out):\t%d %s/%d %s\n"
		  "  Average rate (in/out):\t%d %s/%d %s\n"
		  "  Transferred (in/out):\t%d %s/%d %s\n"
		  "  Online time:\t%d:%02d:%02d"
		  "%s%s"),
		fNetDev,
		ci, ciUnit, co, coUnit, ai, aiUnit, ao, aoUnit,
		vi, viUnit, vo, voUnit,
                t / 3600, t / 60 % 60, t % 60,
		*phoneNumber ? _("\n  Caller id:\t") : "", phoneNumber);
    } else
        snprintf(status, sizeof(status), "%s:", fNetDev);

    setToolTip(status);
}

void NetStatus::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1) {
        if (taskBarLaunchOnSingleClick ? count == 1 : !(count % 2)) {
            if (up.state & ControlMask) {
                start_time = time(NULL);
                start_ibytes = cur_ibytes;
                start_obytes = cur_obytes;
                maxBytes = 0;
            } else {
                if (netCommand && netCommand[0])
		    wmapp->runCommandOnce(netClassHint, netCommand);
            }
        }
    } else if (up.button == 3) {
        if (count == 1 && IS_BUTTON(up.state, Button3Mask))
            taskBar->contextMenu(up.x_root, up.y_root);
    }
}

void NetStatus::paint(Graphics &g, int /*x*/, int /*y*/,
                      unsigned int /*width*/, unsigned int /*height*/ )
{
    int h = height();

    //!!! this should really be unified with acpustatus.cc
    for (int i = 0; i < NET_SAMPLES; i++) {
        if (ppp_tot[i] > 0) {
            int in = (h * ppp_in[i] + maxBytes - 1) / maxBytes;
            int out = (h * ppp_out[i] + maxBytes - 1) / maxBytes;
            int t = h - in - 1;
            int l = out;

            //msg("in: %d:%d:%d, out: %d:%d:%d", in, t, ppp_in[i], out, l, ppp_out[i]);

            if (t < h - 1) {
                g.setColor(color[0]);
                g.drawLine(i, t, i, h - 1);
                t--;
            }

            if (l > 0) {
                g.setColor(color[1]);
                //g.drawLine(i, h - tot -1, i, h - in);
                g.drawLine(i, 0, i, l);
                l++;
            }

            if (l < t) {
/*	    
                g.setColor(color[2]);
                //g.drawLine(i, 0, i, h - tot - 2);
                g.drawLine(i, l, i, t - l);
*/		
		if (color[2]) {
		    g.setColor(color[2]);
		    g.drawLine(i, l, i, t - 1);
		} else {
#ifdef CONFIG_GRADIENTS
		    class YPixbuf * gradient(parent()->getGradient());

		    if (gradient)
			g.copyPixbuf(*gradient,
				     x() + i, y() + l, width(), t - l, i, l);
		    else 
#endif		    
		    if (taskbackPixmap)
			g.fillPixmap(taskbackPixmap,
				     i, l, width(), t - l, x() + i, y() + l);
		}
            }
        }
        else {
	    if (color[2]) {
		g.setColor(color[2]);
		g.drawLine(i, 0, i, h - 1);
            } else {
#ifdef CONFIG_GRADIENTS
		class YPixbuf * gradient(parent()->getGradient());

		if (gradient)
		    g.copyPixbuf(*gradient,
		    		 x() + i, y(), width(), h, i, 0);
		else 
#endif
		if (taskbackPixmap)
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
    char busage;
    char bflags;
    int len, i;
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
                if (strcmp(val[i+1],"1") == 0)
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
#if 0
    return true;
#else

#ifdef linux
    if (useIsdn)
      return isUpIsdn();
#endif

    char buffer[32 * sizeof(struct ifreq)];
    struct ifconf ifc;
    struct ifreq *ifr;
    int len;

    if (fNetDev == 0)
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
        if (strcmp(fNetDev, ifr->ifr_name) == 0) {
            close(s);
            return true;
        }
        len -= sizeof(struct ifreq);
        ifr++;
    }

    close(s);
    return false;
#endif // if 0
}

void NetStatus::updateStatus() {
    for (int i = 1; i < NET_SAMPLES; i++) {
        ppp_in[i-1] = ppp_in[i];
        ppp_out[i-1] = ppp_out[i];
        ppp_tot[i-1] = ppp_tot[i];
    }

    int last = NET_SAMPLES - 1;
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

    sprintf(req.ifr_name, PPP_DEVICE);

    if (ioctl(s, SIOCGPPPSTATS, &req) != 0) {
        if (errno == ENOTTY) {
            //perror("ioctl");
        }
        else { // just not connected?
            //perror("??? ioctl?");
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
        if (strncmp(p, fNetDev, strlen(fNetDev)) == 0 &&
            p[strlen(fNetDev)] == ':')
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

            //msg("cur:%d %d %d", cur_ibytes, cur_obytes, maxBytes);

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

               if(sysctl(name, 6, &ifmd, &ifmd_size, (void *)0, 0) == -1) {
                               printf(_("%s@%d: %s\n"),__FILE__,__LINE__,strerror(errno));
                               continue;
                       }
               if (strncmp(ifmd.ifmd_name, fNetDev, strlen(fNetDev)) == 0) {
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

    //msg("%d %d", ni, no);

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

    //msg("dif:%d %d %d", ni, no, maxBytes);

    prev_ibytes = cur_ibytes;
    prev_obytes = cur_obytes;
}

#endif // HAVE_NET_STATUS
