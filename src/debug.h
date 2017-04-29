#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DEBUG
extern bool debug;
extern bool debug_z;

#define DBG if (debug)
#define MSG(x) DBG msg x
#define PRECONDITION(x) if (x); else precondition( #x , __FILE__, __LINE__)
#else
#define DBG if (0)
#define MSG(x)
#define PRECONDITION(x) // nothing
#endif

#endif
