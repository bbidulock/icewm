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
#include "base.h"
#include "intl.h"
#include <string.h>

#ifdef CONFIG_I18N
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>
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

private:
    void getConverters();
    iconv_t getConverter(char const *from, char const **& to);
    const char* getCodeset();

    iconv_t toUnicode;
    iconv_t toLocale;
    const char* fLocaleName;
    const char* fCodeset;
};

YConverter::YConverter(char const* localeName) :
    toUnicode(invalid),
    toLocale(invalid),
    fLocaleName(setlocale(LC_ALL, localeName))
{
    if ( !fLocaleName || !XSupportsLocale()) {
        warn(_("Locale not supported by C library or Xlib. "
               "Falling back to 'C' locale'."));
        fLocaleName = setlocale(LC_ALL, "C");
    }

    fCodeset = getCodeset();

    extern bool multiByte;
    multiByte = true;

    MSG(("locale: %s, MB_CUR_MAX: %zd, "
         "multibyte: %d, codeset: %s, endian: %c",
         fLocaleName, MB_CUR_MAX, multiByte, fCodeset, little() ? 'l' : 'b'));

    getConverters();
}

const char* YConverter::getCodeset() {
    char const* codeset = nullptr;
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

    char const * unicodeCharsets[] = {
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

    char const* localeCharsets[] = {
        cstrJoin(fCodeset, "//TRANSLIT", nullptr),
        fCodeset,
        nullptr
    };

    char const** ucs(unicodeCharsets);
    toUnicode = getConverter(localeCharsets[1], ucs);
    if (toUnicode == invalid)
        die(1, _("iconv doesn't supply (sufficient) "
                 "%s to %s converters."), localeCharsets[1], "Unicode");

    MSG(("toUnicode converts from %s to %s", localeCharsets[1], *ucs));

    char const** lcs(localeCharsets);
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

YLocale::YLocale(char const* localeName)
    : converter(nullptr)
{
    if (instance == nullptr) {
        instance = this;
#ifdef CONFIG_I18N
        converter = new YConverter(localeName);
#endif
        bindtextdomain(PACKAGE, LOCDIR);
        textdomain(PACKAGE);
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
YLChar * YLocale::localeString(YUChar const *uStr, size_t uLen, size_t &lLen) {
    PRECONDITION(instance);
    if (uStr == nullptr)
        return nullptr;

    YLChar* lStr(new YLChar[uLen + 1]);
#ifdef __NetBSD__
    const
#endif
    char* inbuf((char *) uStr);
    char* outbuf((char *) lStr);
    size_t inlen(uLen), outlen(uLen);

    errno = 0;
    size_t count = iconv(instance->converter->localer(),
                         &inbuf, &inlen, &outbuf, &outlen);
    if (count == size_t(-1)) {
        static unsigned count, shift;
        if (++count >= (1U << shift)) {
            ++shift;
            warn("Invalid unicode string: %s", strerror(errno));
        }
    }

    *((YLChar *) outbuf) = 0;
    lLen = ((YLChar *) outbuf) - lStr;

    return lStr;
}

YUChar *YLocale::unicodeString(const YLChar* lStr, size_t const lLen,
                               size_t& uLen)
{
    PRECONDITION(instance);
    if (lStr == nullptr)
        return nullptr;

    YUChar* uStr(new YUChar[lLen + 1]);
#ifdef __NetBSD__
    const
#endif
    char* inbuf((char *) lStr);
    char* outbuf((char *) uStr);
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

    *((YUChar *) outbuf) = 0;
    uLen = ((YUChar *) outbuf) - uStr;

    return uStr;
}
#endif

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

// vim: set sw=4 ts=4 et:
