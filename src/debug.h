#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DEBUG
extern bool debug;
extern bool debug_z;

#define DBG if (debug)
#define MSG(x) DBG msg x
#define PRECONDITION(x) \
    if (!(x)) \
    do { \
    precondition("PRECONDITION FAILED at %s:%d: (" #x ")", __FILE__, __LINE__); \
    } while (0)
#else
#define DBG if (0)
#define MSG(x)
#define PRECONDITION(x) // nothing
#endif

#endif
