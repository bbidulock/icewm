#ifndef _APPNAMES_H_
#define _APPNAMES_H_

#define DO_QUOTE(X)        #X
#define QUOTE(X)           DO_QUOTE(X)

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

#endif
