/*
 *  IceWM - C++ wrapper for locale/unicode conversion
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Released under terms of the GNU Library General Public License
 *
 *  2001/07/21: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *	- initial revision
 */

#include "config.h"
#include "ylocale.h"

#include "default.h"
#include "base.h"

#include "intl.h"

#ifdef CONFIG_I18N
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <X11/Xlib.h>
#endif

#ifdef CONFIG_I18N
YLocale * YLocale::locale(NULL);
#endif

#ifndef CONFIG_I18N
YLocale::YLocale(char const * ) {
#else
YLocale::YLocale(char const * localeName) {
    locale = this;

    if (NULL == setlocale(LC_ALL, localeName))// || False == XSupportsLocale())
	setlocale(LC_ALL, "C");

    multiByte = (MB_CUR_MAX > 1);

    MSG(("I18N: locale: %s, MB_CUR_MAX: %d, multibyte: %d, codeset: %s",
    	 setlocale(LC_ALL, NULL), MB_CUR_MAX, multiByte, QUERY_CODESET));

    union { int i; char c[sizeof(int)]; } endian; endian.i = 1;

    char const * unicode_charsets[] = {
#ifdef CONFIG_UNICODE_SET
	CONFIG_UNICODE_SET,
#endif    
	"WCHAR_T//TRANSLIT",
	(*endian.c ? "UCS-4LE//TRANSLIT" : "UCS-4BE//TRANSLIT"),
	"WCHAR_T", (*endian.c ? "UCS-4LE" : "UCS-4BE"),
	NULL
    };

    char const * locale_charsets[] = {
	strJoin (QUERY_CODESET, "//TRANSLIT", NULL), QUERY_CODESET, NULL
    };

    char const ** ucs(unicode_charsets);
    if ((iconv_t) -1 == (toUnicode = getConverter (locale_charsets[1], ucs)))
	die(1, _("iconv doesn't supply (sufficient) "
		 "%s to %s converters."), locale_charsets[1], "Unicode");

    MSG(("toUnicode converts from %s to %s", locale_charsets[1], *ucs));

    char const ** lcs(locale_charsets);
    if ((iconv_t) -1 == (toLocale = getConverter (*ucs, lcs)))
	die(1, _("iconv doesn't supply (sufficient) "
		 "%s to %s converters."), "Unicode", locale_charsets[1]);

    MSG(("toLocale converts from %s to %s", *ucs, *lcs));

    delete[] *locale_charsets;
#endif

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif
}

YLocale::~YLocale() {
#ifdef CONFIG_I18N
    locale = NULL;

    if ((iconv_t) -1 != toUnicode) iconv_close(toUnicode);
    if ((iconv_t) -1 != toLocale) iconv_close(toLocale);
#endif    
}

#ifdef CONFIG_I18N
iconv_t YLocale::getConverter (char const * from, char const **& to) {
    iconv_t cd = (iconv_t) -1;

    while (NULL != *to)
	if ((iconv_t) -1 != (cd = iconv_open(*to, from))) return cd;
	else ++to;

    return (iconv_t) -1;
}

/*
lchar_t * YLocale::localeString(uchar_t const * uStr) {
    lchar_t * lStr(NULL);
    
    return lStr;
}
*/

uchar_t * YLocale::unicodeString(lchar_t const * lStr, size_t const lLen,
    				 size_t & uLen) {
    if (NULL == lStr || NULL == locale)
	return NULL;

    uchar_t * uStr(new uchar_t[lLen + 1]);
    char * inbuf((char *) lStr), * outbuf((char *) uStr);
    size_t inlen(lLen), outlen(4 * lLen);

    if (0 > (int) iconv(locale->toUnicode, &inbuf, &inlen, &outbuf, &outlen))
	warn(_("Invalid multibyte string \"%s\": %s"), lStr, strerror(errno));

    *((uchar_t *) outbuf) = 0;
    uLen = ((uchar_t *) outbuf) - uStr;
    
    return uStr;
}

#endif
