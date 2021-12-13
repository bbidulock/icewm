/*
 *  IceWM - C++ wrapper for locale/unicode conversion
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Released under terms of the GNU Library General Public License
 *
 *  2001/07/21: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *      - initial revision
 */

#include "config.h"
#include "ylocale.h"
#include "ascii.h"
#include "base.h"
#include "intl.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#ifdef CONFIG_I18N
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <iconv.h>

const iconv_t invalid = iconv_t(-1);

class YConverter {
public:
    YConverter(const char* localeName);
    ~YConverter();

    iconv_t unicode() const { return toUnicode; }
    iconv_t localer() const { return toLocale; }
    const char* localeName() const { return fLocaleName; }
    const char* codesetName() const { return fCodeset; }
    const char* modifiers() const { return fModifiers; }

private:
    void getConverters();
    iconv_t getConverter(const char* from, const char**& to);
    const char* getCodeset();

    iconv_t toUnicode;
    iconv_t toLocale;
    const char* fLocaleName;
    const char* fModifiers;
    const char* fCodeset;
};

YConverter::YConverter(const char* localeName) :
    toUnicode(invalid),
    toLocale(invalid),
    fLocaleName(setlocale(LC_ALL, localeName))
{
    if ( !fLocaleName || !XSupportsLocale()) {
        warn(_("Locale not supported by C library or Xlib. "
               "Falling back to 'C' locale'."));
        fLocaleName = setlocale(LC_ALL, "C");
    }
    fModifiers = XSetLocaleModifiers("");
    fCodeset = getCodeset();

    MSG(("locale: %s, MB_CUR_MAX: %zd, codeset: %s, endian: %c",
         fLocaleName, MB_CUR_MAX, fCodeset, little() ? 'l' : 'b'));

    getConverters();
}

const char* YConverter::getCodeset() {
    const char* codeset = nullptr;
    int const codesetItems[] = {
#ifdef CONFIG_NL_CODESETS
        CONFIG_NL_CODESETS
#else
        CODESET,
#ifdef _NL_CTYPE_CODESET_NAME
        _NL_CTYPE_CODESET_NAME,
#endif
        0
#endif
    };

    for (int i = 0; i + 1 < int ACOUNT(codesetItems); ++i) {
        codeset = nl_langinfo(codesetItems[i]);
        if (nonempty(codeset)) {
            break;
        }
    }

    if (isEmpty(codeset)) {
        warn(_("Failed to determinate the current locale's codeset. "
               "Assuming ISO-8859-1.\n"));
        codeset = "ISO-8859-1";
    }
    return codeset;
}

void YConverter::getConverters() {

    // #warning "this is getting way too complicated"

    const char* unicodeCharsets[] = {
#ifdef CONFIG_UNICODE_SET
        CONFIG_UNICODE_SET,
#endif
//      "WCHAR_T//TRANSLIT",
        (little() ? "UCS-4LE//TRANSLIT" : "UCS-4BE//TRANSLIT"),
//      "WCHAR_T",
        (little() ? "UCS-4LE" : "UCS-4BE"),
        "UCS-4//TRANSLIT",
        "UCS-4",
        nullptr
    };

    const char* localeCharsets[] = {
        cstrJoin(fCodeset, "//TRANSLIT", nullptr),
        fCodeset,
        nullptr
    };

    const char** ucs(unicodeCharsets);
    toUnicode = getConverter(localeCharsets[1], ucs);
    if (toUnicode == invalid)
        die(1, _("iconv doesn't supply (sufficient) "
                 "%s to %s converters."), localeCharsets[1], "Unicode");

    MSG(("toUnicode converts from %s to %s", localeCharsets[1], *ucs));

    const char** lcs(localeCharsets);
    toLocale = getConverter(*ucs, lcs);
    if (toLocale == invalid)
        die(1, _("iconv doesn't supply (sufficient) "
                 "%s to %s converters."), "Unicode", localeCharsets[1]);

    MSG(("toLocale converts from %s to %s", *ucs, *lcs));

    delete[] localeCharsets[0];
}

YConverter::~YConverter() {
    iconv_close(toUnicode);
    iconv_close(toLocale);
}

iconv_t YConverter::getConverter(const char* from, const char**& to) {
    iconv_t ic;
    do {
        ic = iconv_open(*to, from);
    } while (ic == invalid && *++to);
    return ic;
}

#endif

YLocale* YLocale::instance;

YLocale::YLocale(const char* localeName)
    : converter(nullptr)
    , rightToLeft(false)
    , codesetUTF8(false)
{
    if (instance == nullptr) {
        instance = this;
#ifdef CONFIG_I18N
        converter = new YConverter(localeName);
        codesetUTF8 = (0 == strncmp(converter->codesetName(), "UTF-8", 5));
#endif
        bindtextdomain(PACKAGE, LOCDIR);
        textdomain(PACKAGE);
        getDirection();
    }
}

YLocale::~YLocale() {
    if (instance == this) {
        instance = nullptr;
#ifdef CONFIG_I18N
        delete converter;
#endif
    }
}

#ifdef CONFIG_I18N
char* YLocale::localeString(const wchar_t* uStr, size_t uLen, size_t &lLen) {
    PRECONDITION(instance);
    if (uStr == nullptr)
        return nullptr;

    iconv(instance->converter->localer(), nullptr, nullptr, nullptr, nullptr);

    size_t lSize = 4 * uLen;
    char* lStr = new char[lSize + 1];
#ifdef __NetBSD__
    const
#endif
    char* inbuf = (char *) uStr;
    char* outbuf = lStr;
    size_t inlen = uLen * sizeof(wchar_t);
    size_t outlen = lSize;

    errno = 0;
    size_t count = iconv(instance->converter->localer(),
                         &inbuf, &inlen, &outbuf, &outlen);
    if (count == size_t(-1)) {
        static unsigned count, shift;
        if (++count <= 2 || (count - 2) >= (1U << shift)) {
            ++shift;
            warn("Invalid unicode string: %s (%zd/%u)",
                 strerror(errno), ((wchar_t*)inbuf - uStr), *inbuf);
        }
    }

    *outbuf = '\0';
    lLen = outbuf - lStr;

    return lStr;
}

wchar_t* YLocale::unicodeString(const char* lStr, size_t const lLen,
                               size_t& uLen)
{
    PRECONDITION(instance);
    if (lStr == nullptr)
        return nullptr;

    iconv(instance->converter->unicode(), nullptr, nullptr, nullptr, nullptr);

    wchar_t* uStr(new wchar_t[lLen + 1]);
#ifdef __NetBSD__
    const
#endif
    char* inbuf(const_cast<char *>(lStr));
    char* outbuf(reinterpret_cast<char *>(uStr));
    size_t inlen(lLen), outlen(4 * lLen);

    errno = 0;
    size_t count = iconv(instance->converter->unicode(),
                         &inbuf, &inlen, &outbuf, &outlen);
    if (count == size_t(-1)) {
        static unsigned count, shift;
        if (++count >= (1U << shift)) {
            ++shift;
            warn(_("Invalid multibyte string \"%s\": %s"), lStr, strerror(errno));
        }
    }

    *(reinterpret_cast<wchar_t *>(outbuf)) = 0;
    uLen = reinterpret_cast<wchar_t *>(outbuf) - uStr;

    return uStr;
}
#else

wchar_t* YLocale::wideCharString(const char* str, size_t len, size_t& out) {
    wchar_t* text = new wchar_t[len + 1];
    size_t count = 0;
    mbtowc(nullptr, nullptr, size_t(0));
    for (size_t i = 0; i < len; ++i) {
        int k = mbtowc(&text[count], str + i, len - i);
        if (k < 1) {
            i++;
        } else {
            i += k;
            count++;
        }
    }
    text[count] = 0;
    out = count;
    return text;
}
#endif

char* YLocale::narrowString(const wchar_t* uStr, size_t uLen, size_t& lLen) {
    PRECONDITION(instance);
    if (uStr == nullptr || uLen == 0) {
        lLen = 0;
        return nullptr;
    }

    size_t size = 4 + 3 * uLen / 2;
    char* dest = new char[size + 1];
    size_t done;

    for (;;) {
        const wchar_t* ptr = uStr;
        mbstate_t state;
        memset(&state, 0, sizeof(mbstate_t));
        done = wcsrtombs(dest, &ptr, size, &state);
        if (done == size_t(-1)) {
            done = (ptr > uStr) ? ptr - uStr : 0;
            if (done + 4 >= size) {
                delete[] dest;
                size = 4 + 3 * size / 2;
                dest = new char[size + 1];
            } else {
                break;
            }
        } else {
            break;
        }
    }

    if (done == 0) {
        delete[] dest;
        dest = nullptr;
    }
    else if (2 * done < size && 30 < size) {
        char* copy = new char[done + 1];
        memcpy(copy, dest, done);
        copy[done] = '\0';
        delete[] dest;
        dest = copy;
    } else {
        dest[done] = '\0';
    }

    lLen = done;
    return dest;
}

const char *YLocale::getLocaleName() {
#ifdef CONFIG_I18N
    return instance->converter->localeName();
#else
    return "C";
#endif
}

int YLocale::getRating(const char *localeStr) {
    const char *s1 = getLocaleName();
    const char *s2 = localeStr;
    int i = 0;
    while (s1[i] && s1[i] == s2[i])
        i++;
    if (s1[i]) {
        while (i && strchr("_@.", s2[i - 1]))
            i--;
    }
    return i;
}

void YLocale::getDirection() {
#ifdef CONFIG_I18N
    using namespace ASCII;
    const char* loc = converter ? converter->localeName() : "C";
    if (loc && isLower(*loc) && isLower(loc[1]) && !isAlpha(loc[2])) {
        const char rtls[][4] = {
            "ar",   // arabic
            "fa",   // farsi
            "he",   // hebrew
            "ps",   // pashto
            "sd",   // sindhi
            "ur",   // urdu
        };
        for (auto rtl : rtls) {
            if (rtl[0] == loc[0] && rtl[1] == loc[1]) {
                rightToLeft = true;
                break;
            }
        }
    }
#endif
}

// vim: set sw=4 ts=4 et:
