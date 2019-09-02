#ifndef _APPNAMES_H_
#define _APPNAMES_H_

#ifndef DO_QUOTE
#define DO_QUOTE(X)        #X
#endif
#ifndef QUOTE
#define QUOTE(X)           DO_QUOTE(X)
#endif

#ifndef EXEEXT
#define EXEEXT
#endif

#ifndef ICEWMEXE
#define ICEWMEXE "icewm" QUOTE(EXEEXT)
#endif

#ifndef ICEWMBGEXE
#define ICEWMBGEXE "icewmbg" QUOTE(EXEEXT)
#endif

#ifndef ICEWMTRAYEXE
#define ICEWMTRAYEXE "icewmtray" QUOTE(EXEEXT)
#endif

#ifndef ICESMEXE
#define ICESMEXE "icewm-session" QUOTE(EXEEXT)
#endif

#ifndef ICEHELPEXE
#define ICEHELPEXE "icehelp" QUOTE(EXEEXT)
#endif

#ifndef ICESOUNDEXE
#define ICESOUNDEXE "icesound" QUOTE(EXEEXT)
#endif

#ifndef ICESPLASH
#define ICESPLASH nullptr
#endif

#ifndef XTERMCMD
#define XTERMCMD xterm
#endif

#define TERM    QUOTE(XTERMCMD)

/* restart via icesm if icewm has --notify */
#define ICESM_EXIT_RESTART 101

#endif

// vim: set sw=4 ts=4 et:
