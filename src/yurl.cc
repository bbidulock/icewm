/*
 *  IceWM - URL decoder
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/02/25: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */

#include "config.h"
#include "yurl.h"
#include "base.h"
#include "intl.h"
#include "binascii.h"

#include <string.h>

/*******************************************************************************
 * An URL decoder
 ******************************************************************************/

YURL::YURL():
    fScheme(NULL), fUser(NULL), fPassword(NULL),
    fHost(NULL), fPort(NULL), fPath(NULL) {
}

YURL::YURL(char const * url, bool expectInetScheme):
    fScheme(NULL), fUser(NULL), fPassword(NULL),
    fHost(NULL), fPort(NULL), fPath(NULL) {
    assign(url, expectInetScheme);
}

YURL::~YURL() {
    delete[] fScheme;
    delete[] fPath;
}

void YURL::assign(char const * url, bool expectInetScheme) {
    delete[] fScheme;
    fScheme = newstr(url);

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
                if (*str) fPath = unescape(newstr(str));
                *str++ = '\0';
            }

            if ((str = strchr(fHost, ':'))) { // --------------- custom port ---
                *str++ = '\0';
                if (*str) fPort = unescape(str);
            }

            fHost = unescape(fHost);

        } else if (expectInetScheme)
            warn(_("\"%s\" doesn't describe a common internet scheme"), url);

    } else {
        warn(_("\"%s\" contains no scheme description"), url);

        delete[] fScheme;
        fScheme = NULL;
    }
}

char * YURL::unescape(char * str) {
    if (str) {
        for (char * s = str; *s; ++s) {
            if (*s == '%') {
                int a, b;

                if ((a = BinAscii::unhex(s[1])) != -1 &&
                    (b = BinAscii::unhex(s[2])) != -1) {
                    *s = ((a << 4) + b);
                    memmove(s + 1, s + 3, strlen(s + 3) + 1);
                } else
                    warn(_("Not a hexadecimal number: %c%c (in \"%s\")"),
                         s[1], s[2], str);
            }
        }
    }

    return str;
}

char * YURL::unescape(char const * str) {
    return unescape(newstr(str));
}
