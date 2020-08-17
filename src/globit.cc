#include "config.h"
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include "globit.c"

bool YCmdLineInputListener::inputRequestCompletion(const mstring &input,
        mstring &result, bool &asMarked) {
    char *res = nullptr;
    // XXX: can avoid const_cast when the const-correct c_str() is available,
    // see SSO branch
    int res_count = globit_best(const_cast<mstring&>(input).c_str(), &res,
            nullptr, nullptr);
    // directory is not a final match
    if (res_count == 1 && upath(res).dirExists())
        res_count++;
    bool update = (1 <= res_count);
    if (update) {
        asMarked = (res_count == 1);
        result = res;
    }
    free(res);
    return update;
}

void YCmdLineInputListener::inputReturn(YInputLine *input) {
}

void YCmdLineInputListener::inputEscape(YInputLine *input) {
}

void YCmdLineInputListener::inputLostFocus(YInputLine *input) {
}
// vim: set sw=4 ts=4 et:
