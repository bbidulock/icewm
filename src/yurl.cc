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
    fScheme(null), fUser(null), fPassword(null),
    fHost(null), fPort(null), fPath(null) {
}

YURL::YURL(ustring url, bool expectInetScheme):
    fScheme(null), fUser(null), fPassword(null),
    fHost(null), fPort(null), fPath(null) {
    assign(url, expectInetScheme);
}

YURL::~YURL() {
}

void YURL::assign(ustring url, bool expectInetScheme) {
    fScheme = null;
    fUser = null;
    fPassword = null;
    fHost = null;
    fPort = null;
    fPath = null;
    ustring rest(url);

    int i = rest.indexOf(':');
    if (i != -1) {
        fScheme = rest.substring(0, i);
        rest = rest.substring(i + 1);

        if (rest.length() > 2 &&
            rest.charAt(0) == '/' && rest.charAt(1) == '/')
        {
            rest = rest.substring(2);

            i = rest.indexOf('/');
            if (i != -1) {
                fPath = rest.substring(i);
                fPath = unescape(fPath);
                fHost = rest.substring(0, i);
            } else {
                fHost = rest;
            }

            i = fHost.indexOf('@'); // ???last

            if (i != -1) {
                fUser = fHost.substring(0, i);
                fHost = fHost.substring(i + 1);

                i = fUser.indexOf(':');
                if (i != -1) {
                    fPassword = fUser.substring(i + 1);
                    fUser = fUser.substring(0, i);

                    fPassword = unescape(fPassword);
                }
                fUser = unescape(fUser);
            }
            fHost = unescape(fHost);
        } else if (expectInetScheme)
            warn(_("\"%s\" doesn't describe a common internet scheme"), cstring(url).c_str());

    } else {
        warn(_("\"%s\" contains no scheme description"), cstring(url).c_str());
    }
}

ustring YURL::unescape(ustring str) {
    if (str != null) {
        char *nstr = new char[str.length()];
        if (nstr == 0)
            return null;
        char *d = nstr;

        for (int i = 0; i < str.length(); i++) {
            int c = str.charAt(i);

            if (c == '%') {
                if (i + 3 > str.length()) {
                    return null;
                }
                int a = BinAscii::unhex(str.charAt(i + 1));
                int b = BinAscii::unhex(str.charAt(i + 2));
                if (a == -1 || b == -1) {
                    return null;
                }
                i += 2;
                c = (char)((a << 4) + b);
            }
            *d++ = (char)c;
        }
        str = ustring(nstr, d - nstr);
    }
    return str;
}
