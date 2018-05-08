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
#include <sys/types.h>
#include <regex.h>

class Matches {
private:
    ustring const str;
    int const size;
    asmart<regmatch_t> const match;
public:
    Matches(ustring s, int n) : str(s), size(n), match(new regmatch_t[n]) {}
    int num() const { return size; }
    int beg(int i) const { return match[i].rm_so; }
    int end(int i) const { return match[i].rm_eo; }
    int len(int i) const { return end(i) - beg(i); }
    bool has(int i) const { return 0 <= beg(i) && beg(i) <= end(i); }
    ustring get(int i) const { return str.substring(beg(i), len(i)); }
    ustring operator[](int i) const { return has(i) ? get(i) : null; }
    regmatch_t* ptr() const { return match; }
};

class Pattern {
private:
    regex_t pat;
    int comp;
    int exec;
public:
    Pattern(const char* re) :
        comp(regcomp(&pat, re, REG_EXTENDED | REG_NEWLINE)),
        exec(0)
    {
        if (comp) {
            char err[99] = "";
            regerror(comp, &pat, err, sizeof err);
            warn("regcomp failed for %s: %s\n", re, err);
        }
    }
    ~Pattern() {
        if (0 == comp)
            regfree(&pat);
    }
    bool match(const char* str, const Matches& m) {
        if (0 == comp)
            exec = regexec(&pat, str, m.num(), m.ptr(), 0);
        return *this;
    }
    operator bool() const { return !(comp | exec); }
};

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

    // parse scheme://[user[:password]@]server[:port][/path]

    enum {
        Path = 1, File = 2, Scheme = 3, User = 5, Pass = 7,
        Host = 8, Port = 10, Inbox = 11, Count = 12,
    };
    const char re[] =
        // 0:
        "^"
        // 1: path
        "(/.*)"
        "|"
        // 2: file
        "file://(.*)"
        "|"
        // 3: scheme
        "([a-z][a-z0-9]+)"
        "://"
        // 4:
        "("
        // 5: user
        "([^:/@]+)"
        // 6:
        "(:"
        // 7: pass
        "([^:/@]*)"
        ")?"
        "@)?"
        // 8: host
        "([^:@/]+)"
        // 9:
        "(:"
        // 10: port
        "([0-9]+|[a-z][a-z0-9]+)"
        ")?"
        // 11: inbox
        "(/.*)?"
        "$";

    cstring str(url);
    Matches mat(str, Count);
    Pattern rex(re);

    if (rex.match(str, mat)) {
        if (mat.has(Path)) {
            path = mat[Path];
            scheme = "file";
        }
        else if (mat.has(File)) {
            path = unescape(mat[File]);
            scheme = "file";
        }
        else if (mat.has(Scheme)) {
            scheme = mat[Scheme];
            user = unescape(mat[User]);
            pass = unescape(mat[Pass]);
            host = unescape(mat[Host]);
            port = mat[Port];
            path = unescape(mat[Inbox]);
        }
    } else {
        warn(_("Failed to parse URL \"%s\"."), str.c_str());
    }
}

ustring YURL::unescape(ustring str) {
    if (0 <= str.indexOf('%')) {
        csmart nstr(new char[str.length()]);
        if (nstr == 0)
            return null;
        char *d = nstr;

        for (unsigned i = 0; i < str.length(); i++) {
            int c = str.charAt(i);

            if (c == '%') {
                if (i + 3 > str.length()) {
                    warn(_("Incomplete hex escape in URL at position %d."),
                            int(i + str.offset()));
                    return null;
                }
                int a = BinAscii::unhex(str.charAt(i + 1));
                int b = BinAscii::unhex(str.charAt(i + 2));
                if (a == -1 || b == -1) {
                    warn(_("Invalid hex escape in URL at position %d."),
                            int(i + str.offset()));
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
