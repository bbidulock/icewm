#ifndef __BASE_H
#define __BASE_H

#ifdef NEED_BOOL
enum bool_t { false = 0, true = 1 };
typedef int bool;
#endif

#ifndef null
#define null 0
#endif

#ifndef O_TEXT
#define O_TEXT 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif

#define ABORT() do { *(char *)0 = 0x42; } while(1)

void die(int exitcode, const char *msg, ...);
void warn(const char *msg, ...);

#define PRECONDITION(x) \
    if (!(x)) \
    do { \
    warn("PRECONDITION FAILED at %s:%d: (" #x ")", __FILE__, __LINE__); \
    ABORT(); \
    } while (0)

#ifdef DEBUG
extern bool debug;
void msg(const char *msg, ...);

#define DBG if (debug)
#define MSG(x) do { DBG msg x ; } while(0)
#else
#define DBG if (0)
#define MSG(x)
//#define PRECONDITION(x) // nothing
#endif

char *__newstr(const char *str);
char *__newstr(const char *str, int len);
char *__strJoin(const char *str, ...);

// !!! remove this
void *MALLOC(unsigned int len);
void *REALLOC(void *p, unsigned int new_len);
void FREE(void *p);

extern "C" {
#ifdef __EMX__
char* __XOS2RedirRoot(const char*);
#define REDIR_ROOT(path) __XOS2RedirRoot(path)
#else
#define REDIR_ROOT(path) (path)
#endif
}

// !!! move there somewhere else probably
int __findPath(const char *path, int mode, const char *name, char **fullname, bool path_relative = false);

typedef struct {
    const char **root;
    const char *rdir;
    const char **sub;
}  pathelem;

extern pathelem icon_paths[10];

char *__joinPath(pathelem *pe, const char *base, const char *name);
void __verifyPaths(pathelem *search, const char *base);

#endif
