/*
 *  IceWM - C++ wrapper for locale/unicode conversion
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef __YLOCALE_H
#define __YLOCALE_H

#ifdef CONFIG_I18N
#include <iconv.h>

#if defined(CONFIG_LIBICONV) && !defined (_LIBICONV_VERSION)
#error libiconv in use but included iconv.h not from libiconv
#endif
#if !defined(CONFIG_LIBICONV) && defined (_LIBICONV_VERSION)
#if !defined(__OpenBSD__) && !defined(__FreeBSD__)
#error libiconv not in use but included iconv.h is from libiconv
#endif
#endif

#endif

typedef char YLChar;
typedef wchar_t YUChar;

class YLocale {
public:
    YLocale(char const * localeName = "");
    ~YLocale();

#ifdef CONFIG_I18N
    static iconv_t getConverter (char const *from, char const **& to);
    static YLChar *localeString(YUChar const *uStr, size_t const uLen,
                                size_t &lLen);
    static YUChar *unicodeString(YLChar const *lStr, size_t const lLen,
                                 size_t &uLen);
#endif

    static const char *getLocaleName();
    static int getRating(const char *localeStr);

#ifdef CONFIG_I18N
private:
    static YLocale *instance;
    const char *fLocaleName;

    iconv_t toUnicode;
    iconv_t toLocale;

#endif
};

#endif

// vim: set sw=4 ts=4 et:
