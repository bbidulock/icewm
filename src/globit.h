#ifndef GLOBIT_H
#define GLOBIT_H

#ifdef __cplusplus
extern "C" {
#endif

extern int globit_best(const char *, char **,
        void(*callback)(const void *, const char * const *, unsigned), const void* cback_user_parm);

#ifdef __cplusplus
}
#endif

#endif /* GLOBIT_H */

// vim: set sw=4 ts=4 et:
