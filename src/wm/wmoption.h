#ifndef __WMOPTION_H
#define __WMOPTION_H

#ifndef NO_WINDOW_OPTIONS

//#include <X11/Xproto.h>

class CStr;

struct WindowOption {
    CStr *name;
    CStr *icon;
    unsigned long functions, function_mask;
    unsigned long decors, decor_mask;
    unsigned long options, option_mask;
    long workspace;
    long layer;
    bool gpos;
    bool gsize;
    int gx, gy;
    unsigned gw, gh;
};

class WindowOptions {
public:
    WindowOptions();
    ~WindowOptions();

    WindowOption *getWindowOption(const char *name, bool create, bool remove = false);
    void setWinOption(const char *class_instance, const char *opt, const char *arg);

    static void combineOptions(WindowOption &cm, WindowOption &n);
private:
    int winOptionsCount;
    WindowOption *winOptions;
private: // not-used
    WindowOptions(const WindowOptions &);
    WindowOptions &operator=(const WindowOptions &);
};

extern WindowOptions *defOptions;
extern WindowOptions *hintOptions;

extern char *winOptFile;

void loadWinOptions(const char *optFile);

char *getArgument(char *dest, int maxLen, char *p, bool comma);

#endif

#endif
