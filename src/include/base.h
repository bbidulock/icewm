#ifndef __BASE_H
#define __BASE_H

#ifdef NEED_BOOL
enum bool_t { false = 0, true = 1 };
typedef int bool;
#endif

char *newstr(const char *str);
char *newstr(const char *str, int len);
char *strJoin(const char *str, ...);

void die(int exitcode, const char *msg, ...);
void warn(const char *msg, ...);

// !!! remove this
void *MALLOC(unsigned int len);
void *REALLOC(void *p, unsigned int new_len);
void FREE(void *p);

#define ACOUNT(x) (sizeof(x)/sizeof(x[0]))

extern "C" {
#ifdef __EMX__
char* __XOS2RedirRoot(const char*);
#define REDIR_ROOT(path) __XOS2RedirRoot(path)
#else
#define REDIR_ROOT(path) (path)
#endif
}

// !!! move there somewhere else probably
int findPath(const char *path, int mode, const char *name, char **fullname, bool path_relative = false);

typedef struct {
    const char **root;
    const char *rdir;
    const char **sub;
}  pathelem;

extern pathelem icon_paths[10];

char *joinPath(pathelem *pe, const char *base, const char *name);
void verifyPaths(pathelem *search, const char *base);

//int is_reg(const char *path);

//!!! clean these up
#define KEY_MODMASK(x) ((x) & (app->KeyMask))
#define BUTTON_MASK(x) ((x) & (app->ButtonMask))
#define BUTTON_MODMASK(x) ((x) & (app->ButtonKeyMask))
#define IS_BUTTON(s,b) (BUTTON_MODMASK(s) == (b))

#define ISMASK(w,e,n) (((w) & ~(n)) == (e))
#define HASMASK(w,e,n) ((((w) & ~(n)) & (e)) == (e))

#define ISLOWER(c) ((c) >= 'a' && (c) <= 'z')
#define TOUPPER(c) (ISLOWER(c) ? (c) - 'a' + 'A' : (c))

#include "debug.h"

#endif
