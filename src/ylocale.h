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

#if defined(CONFIG_LIBICONV) && !defined (_LIBICONV_H)
#error libiconv in use but included iconv.h not from libiconv
#endif
#if !defined(CONFIG_LIBICONV) && defined (_LIBICONV_H)
#error libiconv not in use but included iconv.h is from libiconv
#endif

#endif

typedef char lchar_t;
typedef wchar_t uchar_t;

class YLocale {
public:
    YLocale(char const * localeName = "");
    ~YLocale();
    
#ifdef CONFIG_I18N
    static lchar_t * localeString(uchar_t const * uStr, size_t const uLen,
    				  size_t & lLen);
    static uchar_t * unicodeString(lchar_t const * lStr, size_t const lLen,
    				   size_t & uLen);

private:
    static YLocale * locale;

    iconv_t toUnicode;
    iconv_t toLocale;
#endif
};

#endif
