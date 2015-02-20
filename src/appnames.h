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

#endif
