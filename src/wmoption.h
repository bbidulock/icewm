#ifndef __WMOPTION_H
#define __WMOPTION_H

#ifndef NO_WINDOW_OPTIONS

#include <X11/Xproto.h>
#include "yarray.h"
#include "mstring.h"
#include "upath.h"

struct WindowOption {
    WindowOption(ustring n_class_instance);
    ~WindowOption();

    ustring w_class_instance;

    char *icon;
    unsigned long functions, function_mask;
    unsigned long decors, decor_mask;
    unsigned long options, option_mask;
    long workspace;
    long layer;
#ifdef CONFIG_TRAY
    long tray;
#endif
    int gflags;
    int gx, gy;
    unsigned gw, gh;
};

class WindowOptions {
public:
    WindowOption *getWindowOption(ustring a_class_instance,
                                  bool create, bool remove = false);
    void setWinOption(ustring n_class_instance,
                      const char *opt, const char *arg);

    void mergeWindowOption(WindowOption &cm,
                           ustring a_class_instance,
                           bool remove);

private:
    YObjectArray<WindowOption> fWinOptions;

    static void combineOptions(WindowOption &cm, WindowOption &n);
};

extern WindowOptions *defOptions;
extern WindowOptions *hintOptions;
extern upath winOptFile;

//void setWinOptions(WindowOptions *optionSet, ref<YDocument> doc);
void loadWinOptions(upath optFile);

#endif

#endif
