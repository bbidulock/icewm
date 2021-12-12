/*
 *  IceWM - C++ wrapper for locale/unicode conversion
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 */

#ifndef YLOCALE_H
#define YLOCALE_H

#include <stddef.h>

class YLocale {
public:
    YLocale(const char* localeName = "");
    ~YLocale();

    static const char* getLocaleName();
    static int getRating(const char* localeStr);
    static bool RTL() { return instance->rightToLeft; }
    static bool UTF8() { return instance->codesetUTF8; }

#ifdef CONFIG_I18N
    static char* localeString(const wchar_t* uStr, size_t uLen, size_t& lLen);
    static wchar_t* unicodeString(const char* lStr, size_t lLen, size_t& uLen);
#else
    static wchar_t* wideCharString(const char* str, size_t len, size_t& out);
#endif
    static char* narrowString(const wchar_t* uStr, size_t uLen, size_t& lLen);

private:
    class YConverter* converter;
    bool rightToLeft;
    bool codesetUTF8;
    void getDirection();

    static YLocale* instance;
};

#endif

// vim: set sw=4 ts=4 et:
