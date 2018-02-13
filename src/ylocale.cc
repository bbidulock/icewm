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
#endif

#include "ylib.h"
#include "yprefs.h"


#ifdef CONFIG_I18N
YLocale * YLocale::instance(NULL);
#endif

#ifndef CONFIG_I18N
YLocale::YLocale(char const * ) {
#else
YLocale::YLocale(char const * localeName) {
    assert(NULL == instance);

    instance = this;

    if (NULL == (fLocaleName = setlocale(LC_ALL, localeName))
        || False == XSupportsLocale()) {
        warn(_("Locale not supported by C library or Xlib. "
               "Falling back to 'C' locale'."));
        fLocaleName = setlocale(LC_ALL, "C");
    }
    multiByte = true;

    char const * codeset = NULL;
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

    for (unsigned int i = 0;
         i + 1 < ACOUNT(codesetItems)
         && NULL != (codeset = nl_langinfo(codesetItems[i]))
         && '\0' == *codeset;
         ++i) {}

    if (NULL == codeset || '\0' == *codeset) {
        warn(_("Failed to determinate the current locale's codeset. "
               "Assuming ISO-8859-1.\n"));
        codeset = "ISO-8859-1";
    }

    union { int i; char c[sizeof(int)]; } endian; endian.i = 1;

    MSG(("locale: %s, MB_CUR_MAX: %zd, "
         "multibyte: %d, codeset: %s, endian: %c",
         fLocaleName, MB_CUR_MAX, multiByte, codeset, *endian.c ? 'l' : 'b'));

/// TODO #warning "this is getting way too complicated"

    char const * unicodeCharsets[] = {
#ifdef CONFIG_UNICODE_SET
        CONFIG_UNICODE_SET,
#endif
//      "WCHAR_T//TRANSLIT",
        (*endian.c ? "UCS-4LE//TRANSLIT" : "UCS-4BE//TRANSLIT"),
//      "WCHAR_T",
        (*endian.c ? "UCS-4LE" : "UCS-4BE"),
        "UCS-4//TRANSLIT",
        "UCS-4",
        NULL
    };

    char const * localeCharsets[] = {
        cstrJoin(codeset, "//TRANSLIT", NULL), codeset, NULL
    };

    char const ** ucs(unicodeCharsets);
    if ((iconv_t) -1 == (toUnicode = getConverter (localeCharsets[1], ucs)))
        die(1, _("iconv doesn't supply (sufficient) "
                 "%s to %s converters."), localeCharsets[1], "Unicode");

    MSG(("toUnicode converts from %s to %s", localeCharsets[1], *ucs));

    char const ** lcs(localeCharsets);
    if ((iconv_t) -1 == (toLocale = getConverter (*ucs, lcs)))
        die(1, _("iconv doesn't supply (sufficient) "
                 "%s to %s converters."), "Unicode", localeCharsets[1]);

    MSG(("toLocale converts from %s to %s", *ucs, *lcs));

    delete[] localeCharsets[0];
#endif

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
}

YLocale::~YLocale() {
#ifdef CONFIG_I18N
    instance = NULL;

    if ((iconv_t) -1 != toUnicode) iconv_close(toUnicode);
    if ((iconv_t) -1 != toLocale) iconv_close(toLocale);
#endif
}

#ifdef CONFIG_I18N
iconv_t YLocale::getConverter (const char *from, const char **&to) {
    iconv_t cd = (iconv_t) -1;

    while (NULL != *to)
        if ((iconv_t) -1 != (cd = iconv_open(*to, from))) return cd;
        else ++to;

    return (iconv_t) -1;
}

/*
YLChar * YLocale::localeString(YUChar const * uStr) {
    YLChar * lStr(NULL);

    return lStr;
}
*/

YUChar *YLocale::unicodeString(const YLChar *lStr, size_t const lLen,
                               size_t & uLen)
{
    PRECONDITION(instance != NULL);
    if (NULL == lStr)
        return NULL;

    YUChar * uStr(new YUChar[lLen + 1]);
    char * inbuf((char *) lStr), * outbuf((char *) uStr);
    size_t inlen(lLen), outlen(4 * lLen);

    if (0 > (int) iconv(instance->toUnicode, &inbuf, &inlen, &outbuf, &outlen))
        warn(_("Invalid multibyte string \"%s\": %s"), lStr, strerror(errno));

    *((YUChar *) outbuf) = 0;
    uLen = ((YUChar *) outbuf) - uStr;

    return uStr;
}
#endif

const char *YLocale::getLocaleName() {
#ifdef CONFIG_I18N
    return instance->fLocaleName;
#else
    return "C";
#endif
}

int YLocale::getRating(const char *localeStr) {
    const char *s1 = getLocaleName();
    const char *s2 = localeStr;

    while (*s1 && *s1++ == *s2++) {}
    if (*s1) while (--s2 > localeStr && !strchr("_@.", *s2)) {}

    return s2 - localeStr;
}

// vim: set sw=4 ts=4 et:
