#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DEBUG
extern bool debug;
extern bool debug_z;

#define DBG if (debug)
#define MSG(x) DBG tlog x
#define TLOG(x) tlog x
#else
#define DBG if (0)
#define MSG(x)
#define TLOG(x)
#endif

#if defined(DEBUG) || defined(PRECON)
#define PRECONDITION(x) if (x); else precondition( #x , __FILE__, __LINE__)
#define NOTE(x)   tlog("%s:%d:%s: %s", __FILE__, __LINE__, __func__, #x )
#define INFO(x,y) tlog("%s:%d:%s: " x, __FILE__, __LINE__, __func__, y )
#else
#define PRECONDITION(x) // nothing
#define NOTE(x)   // nothing
#define INFO(x,y) // nothing
#endif
#define CARP(x) tlog("%s:%d:%s: %s", __FILE__, __LINE__, __func__, #x )

#if defined(DEBUG) && !defined(LOGEVENTS)
#define LOGEVENTS 1
#endif

#endif

// vim: set sw=4 ts=4 et:
