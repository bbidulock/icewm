#ifndef GLOBIT_H
#define GLOBIT_H

#include "yinputline.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int globit_best(const char*, char**,
        void (*callback)(const void*, const char* const*, unsigned),
        const void *cback_user_parm);

#ifdef __cplusplus
}

class YCmdLineInputListener : public YInputListener {
public:
    bool inputRequestCompletion(const mstring &input, mstring &result,
            bool &asMarked) override;
    // convenient dummys
    void inputReturn(YInputLine* input) override;
    void inputEscape(YInputLine* input) override;
    void inputLostFocus(YInputLine* input) override;

};

#endif

#endif /* GLOBIT_H */

// vim: set sw=4 ts=4 et:
