#ifndef YWORDEXP_H
#define YWORDEXP_H

#ifdef __OpenBSD__
#include <glob.h>

inline int wordexp(const char *pattern, glob_t *pglob, int flags)
{
    return glob(pattern, flags | GLOB_BRACE | GLOB_TILDE, nullptr, pglob);
}

inline void wordfree(glob_t *pglob)
{
    return globfree(pglob);
}

#define wordexp_t glob_t
#define we_wordc gl_pathc
#define we_wordv gl_pathv
#define we_offs gl_offs
#define WRDE_NOCMD 0
#define WRDE_DOOFFS GLOB_DOOFFS

#else
#include <wordexp.h>
#endif

#endif
