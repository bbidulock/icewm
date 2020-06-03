/*
 *  IceWM - C++ wrapper for locale/unicode conversion
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef YLOCALE_H
#define YLOCALE_H

#include <stddef.h>

typedef char YLChar;
typedef wchar_t YUChar;

class YLocale {
public:
    YLocale(char const* localeName = "");
    ~YLocale();

    static const char* getLocaleName();
    static int getRating(const char* localeStr);

#ifdef CONFIG_I18N
    static YLChar* localeString(YUChar const* uStr, size_t uLen, size_t& lLen);
    static YUChar* unicodeString(YLChar const* lStr, size_t lLen, size_t& uLen);
#endif

private:
    class YConverter* converter;

    static YLocale* instance;
};

#endif

// vim: set sw=4 ts=4 et:
