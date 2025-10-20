#ifndef DEBUG_H
#define DEBUG_H

#if __GNUC__
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
#endif
#if __clang__
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wno-unknown-pragmas"
#pragma clang diagnostic ignored "-Wvla-cxx-extension"
#pragma clang diagnostic ignored "-Wc++11-narrowing"
#endif

#ifdef DEBUG
extern bool debug;

#define DBG if (debug)
#define MSG(x) DBG tlog x
#else
#define DBG if (0)
#define MSG(x)
#endif

#if defined(DEBUG) || defined(PRECON)
#define PRECONDITION(x) if (x); else precondition( #x , __FILE__, __LINE__)
#define NOTE(x)   tlog("%s:%d:%s: %s", __FILE__, __LINE__, __func__, #x )
#define INFO(x,y) tlog("%s:%d:%s: " x, __FILE__, __LINE__, __func__, y )
#define XDBG    true
#define TLOG(x) tlog x
#else
#define PRECONDITION(x) // nothing
#define NOTE(x)   // nothing
#define INFO(x,y) // nothing
#define XDBG    false
#define TLOG(x)
#endif
#define CARP(x) tlog("%s:%d:%s: %s", __FILE__, __LINE__, __func__, #x )

#endif

// vim: set sw=4 ts=4 et:
