/*
 *  IceWM - Locale related stuff
 *
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

YLocale * YLocale::locale(NULL);

YLocale::YLocale(char const * localeName) {
#ifdef CONFIG_I18N
    locale = this;

    if (NULL == setlocale(LC_ALL, localeName))// || False == XSupportsLocale())
	setlocale(LC_ALL, "C");

    multiByte = (MB_CUR_MAX > 1);

    msg("I18N: locale: %s, MB_CUR_MAX: %d, multibyte: %d, codeset: %s",
    	setlocale(LC_ALL, NULL), MB_CUR_MAX, multiByte, QUERY_CODESET);

    char const * lcs(QUERY_CODESET);
    char const * ucs("WCHAR_T//TRANSLIT");

    if ((iconv_t) -1 == (toUnicode = iconv_open(ucs, lcs)))
	die(1, _("Can't establish %s to %s conversion"), lcs, ucs);

    ucs = "WCHAR_T";
    lcs = strJoin(lcs, "//TRANSLIT", NULL);
    if ((iconv_t) -1 == (toLocale = iconv_open(lcs, ucs)))
	die(1, _("Can't establish %s to %s conversion"), lcs, ucs);

    delete[] lcs;
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
