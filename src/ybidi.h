#ifndef YBIDI_H
#define YBIDI_H

#ifdef CONFIG_FRIBIDI
#ifdef CONFIG_I18N
        // remove deprecated warnings for now...
        #include <fribidi/fribidi-config.h>
        #if FRIBIDI_USE_GLIB+0
                #include <glib.h>
                #undef G_GNUC_DEPRECATED
                #define G_GNUC_DEPRECATED
        #endif
        #include <fribidi/fribidi.h>
#else
#undef CONFIG_FRIBIDI
#endif
#endif

class YBidi {
public:
    YBidi(wchar_t* string, size_t length)
        : str(string)
        , len(length)
#ifdef CONFIG_FRIBIDI
        , big(new wchar_t[length + 1])
#endif
        , rtl(false)
    {
#ifdef CONFIG_FRIBIDI
        if (big) {
            FriBidiCharType pbase_dir = FRIBIDI_PAR_ON;
            switch (true) {
                case sizeof(wchar_t) == sizeof(unsigned):
                    if (fribidi_log2vis((const FriBidiChar *) str,
                                        len, &pbase_dir,
                                        (FriBidiChar *) big.data(),
                                        nullptr, nullptr, nullptr))
                    {
                        str = big;
                        rtl = (pbase_dir & 1);
                    }
                case false: ;
            }
        }
#endif
    }
    wchar_t* string() const { return str; }
    size_t length() const { return len; }
    bool isRTL() const { return rtl; }
private:
    wchar_t* str;
    size_t len;
#ifdef CONFIG_FRIBIDI
    asmart<wchar_t> big;
#endif
    bool rtl;
};

#endif
