/*
 *  IceWM - URL decoder
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU General Public License
 *
 *  2001/02/25: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#include "yurl.h"
#include "base.h"
#include "intl.h"

#include <cstring>

/*******************************************************************************
 * An URL decoder
 ******************************************************************************/

YURL::YURL(char const * url, bool expectInetScheme):
    fScheme(newstr(url)),
    fUser(NULL), fPassword(NULL),
    fHost(NULL), fPort(NULL), fPath(NULL) {
    char * str;

    if ((str = strchr(fScheme, ':'))) { // ======================= parse URL ===
        *str++ = '\0';

	if ('/' == *str++ && '/' == *str++) { // ---- common internet scheme ---
	    fHost = str;
	    
	    if ((str = strchr(fHost, '@'))) { // ------- account information ---
		*str++ = '\0';
		fUser = fHost;
		fHost = str;
		
		if ((str = strchr(fUser, ':'))) { // -------------- password ---
		    *str++ = '\0';
		    fPassword = unescape(str);
		}
		
		fUser = unescape(fUser);
	    }
	    
	    if ((str = strchr(fHost, '/'))) { // ------------------ url-path  ---
		*str++ = '\0';
		if (*str) fPath = unescape(str);
	    }

	    if ((str = strchr(fHost, ':'))) { // --------------- custom port ---
		*str++ = '\0';
		if (*str) fPort = unescape(str);
	    }

	    if (*fHost == '\0') // ---------------- check for empty hostname ---
		warn(_("Looks like \"%s\" contains an empty hostname"), url);

	    fHost = unescape(fHost);

	} else if (expectInetScheme)
	    warn(_("\"%s\" doesn't describe a common internet scheme"), url);

    } else {
	warn(_("\"%s\" contains no scheme description"), url);

        delete[] fScheme;
	fScheme = NULL;
    }
}

YURL::~YURL() {
    delete[] fScheme;
}

char * YURL::unescape(char * str) {
    if (str)
	for (char * s = str; *s; ++s)
	    if (*s == '%') {
	        int a, b;
		
		if ((a = unhex(s[1])) != -1 &&
		    (b = unhex(s[2])) != -1) {
		    *s = ((a << 4) + b);
		    memmove(s + 1, s + 3, strlen(s + 3) + 1);
		} else
		    warn(_("Not a hexadecimal number: %c%c (in \"%s\")"),
		    	 s[1], s[2], str);
	    }

    return str;
}

char * YURL::unescape(char const * str) {
    return unescape(newstr(str));
}
