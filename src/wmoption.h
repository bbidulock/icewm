#ifndef __WMOPTION_H
#define __WMOPTION_H

#include "upath.h"
#include "yarray.h"
#include "ypointer.h"

struct WindowOption {
    WindowOption(mstring n_class_instance = null);
    void combine(const WindowOption& n);

    mstring w_class_instance;
    mstring keyboard;
    mstring icon;
    unsigned functions, function_mask;
    unsigned decors, decor_mask;
    unsigned options, option_mask;
    int workspace;
    int layer;
    int tray;
    int order;
    int opacity;
    int gflags;
    int gx, gy;
    unsigned gw, gh;
};

class WindowOptions {
public:
    void setWinOption(mstring n_class_instance,
                      const char *opt, const char *arg);

    void mergeWindowOption(WindowOption &cm,
                           mstring a_class_instance,
                           bool remove);

    int getCount() const { return fWinOptions.getCount(); }
    bool nonempty() const { return fWinOptions.nonempty(); }
    bool isEmpty() const { return fWinOptions.isEmpty(); }

private:
    YObjectArray<WindowOption> fWinOptions;

    bool findOption(mstring a_class_instance, int *index);

    WindowOption *getOption(mstring a_class_instance);
};

extern lazy<WindowOptions> defOptions;
extern lazy<WindowOptions> hintOptions;

void loadWinOptions(upath optFile);

#endif

// vim: set sw=4 ts=4 et:
