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
#include "ypointer.h"

#include <string.h>

/*******************************************************************************
 * An URL decoder
 ******************************************************************************/

YURL::YURL() {
}

YURL::YURL(ustring url) {
    *this = url;
}

void YURL::operator=(ustring url) {
    scheme = null;
    user = null;
    pass = null;
    host = null;
    port = null;
    path = null;

    if (url[0] == '/') {
        scheme = "file";
        path = url;
        return;
    }

    // parse scheme://[user[:password]@]server[:port][/path]
    int sep = url.find("://");
    if (sep >= 0) {
        scheme = url.substring(0, sep);
        ustring rest(url.substring(sep + 3));

        int sl = rest.indexOf('/');
        if (sl >= 0) {
            path = unescape(rest.substring(sl));
            rest = rest.substring(0, sl);
        }
        if (sl) {
            int at = rest.indexOf('@');
            if (at >= 0) {
                ustring login(rest.substring(0, at));
                rest = rest.substring(at + 1);

                int col = login.indexOf(':');
                if (col >= 0) {
                    pass = unescape(login.substring(col + 1));
                    user = unescape(login.substring(0, col));
                }
                else user = unescape(login);
            }

            int col = rest.indexOf(':');
            if (col >= 0) {
                port = rest.substring(col + 1);
                host = unescape(rest.substring(0, col));
            }
            else
                host = unescape(rest);
        }
    } else {
        warn(_("\"%s\" contains no scheme description"), cstring(url).c_str());
    }
}

ustring YURL::unescape(ustring str) {
    if (str != null) {
        csmart nstr(new char[str.length()]);
        if (nstr == 0)
            return null;
        char *d = nstr;

        for (unsigned i = 0; i < str.length(); i++) {
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

// vim: set sw=4 ts=4 et:
