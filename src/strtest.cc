#include "config.h"
#include "mstring.h"
#include "upath.h"
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>

char const *ApplicationName = "strtest";
static const char source[] = __FILE__;

#define equal(p, s)     (0 == strcmp(cstring(p), cstring(s)))

#define expect(u, s)    if (++testsrun, (u) == mstring(s) && equal(u, s)) \
        ++passed; else test_failed(cstring(u), cstring(s), __LINE__)

#define assert(u, b)    if (++testsrun, (b)) ++passed; else \
        test_failed(cstring(u), #b, __LINE__)

#define ispath(u, s)    if (++testsrun, (u) == upath(s) && equal(u, s)) \
        ++passed; else test_failed(cstring(u), cstring(s), __LINE__)

static int testsrun, passed, failed;
static const char *prog;

class strtest {
    const char *name;
public:
    strtest(const char *s) : name(s) {
        failed = passed = testsrun = 0;
    }
    ~strtest() {
        if (failed || passed != testsrun) {
            printf("%s: %7s: %d tests failed, %d tests passed of %d total\n",
                    prog, name, failed, passed, testsrun);
        }
        else {
            printf("%s: %7s: %2d/%d tests passed\n",
                    prog, name, passed, testsrun);
        }
    }
};

static void test_failed(const char *u, const char *s, int l)
{
    printf("%s: Test failed in %s:%d: u = \"%s\", s = \"%s\"\n",
            prog, source, l, u, s);
    ++failed;
}

static void test_mstring()
{
    strtest tester("mstring");

    mstring x("foo");
    expect(x, "foo");
    assert(x, x.length() == 3);

    mstring b("bar", 2);
    expect(b, "ba");
    assert(b, b.length() == 2);

    mstring e(null);
    expect(e, "");
    assert(e, e.length() == 0);
    assert(e, e == null);
    assert(e, e.indexOf(' ') == -1);

    mstring m("abc", 0);
    expect(m, "");
    assert(m, m.length() == 0);

    mstring n(0, 0);
    expect(n, "");
    assert(n, n.length() == 0);

    mstring y = x;
    expect(y, "foo");
    assert(y, y.length() == 3);

    mstring z = y.remove(1, 1);
    expect(z, "fo");
    assert(z, z.length() == 2);

    e += b;
    expect(e, "ba");
    assert(e, e.length() == 2);

    mstring c = y + e;
    expect(c, "fooba");
    assert(c, c.length() == 5);
    assert(c, c != y);
    assert(y, y != c);
    assert(c, c != e);
    assert(e, e != c);
    assert(c, c != y);
    assert(e, e != y);
    assert(x, x == y);

    mstring s = c.substring(2);
    expect(s, "oba");
    s = c.substring(2, 2);
    expect(s, "ob");

    assert(s, s.charAt(1) == 'b');
    assert(s, s.charAt(2) == -1);
    assert(s, s.indexOf('b') == 1);
    assert(s, s.indexOf('c') == -1);

    char buf[10];
    c.copyTo(buf, sizeof buf);
    expect(c, buf);

    for (int i = 0; i < 10000; i++) {
        z = x.replace(x.length(), 0, y);
    }
    expect(z, "foofoo");
    assert(z, z.length() == 6);

    assert(z, z.startsWith("foof"));
    assert(z, !z.startsWith("goof"));
    assert(z, !z.startsWith("foofoof"));
    assert(z, z.startsWith(""));
    assert(z, z.endsWith("foo"));
    assert(z, !z.endsWith("foof"));
    assert(z, !z.endsWith("foofoof"));
    assert(z, z.endsWith(""));

    mstring l(null), r(null);
    e = null;
    assert(e, e.split(0, &l, &r) == false);
    assert(z, z.split('o', &l, &r));
    expect(z, "foofoo");
    expect(l, "f");
    expect(r, "ofoo");
    assert(l, l.splitall('o', &l, &r));
    expect(l, "f");
    expect(r, "");

    mstring w = mstring(" \t\r\nabc\n\r\t ");
    mstring t = w.trim();
    expect(t, "abc");
    expect(w, " \t\r\nabc\n\r\t ");
    w = w.replace(0, 4, "_");
    w = w.replace(4, 4, ".");
    expect(w, "_abc.");
    t = w.remove(1, 3);
    expect(t, "_.");
    mstring k = t.insert(1, "xyz");
    expect(k, "_xyz.");
    mstring q = t.append("!?");
    expect(q, "_.!?");
}

static void test_upath()
{
    strtest tester("upath");

    upath r = upath::root();
    upath s = upath::sep();
    ispath(r, "/");
    ispath(s, "/");
    ispath(r + s, "/");
    assert(r, r != null);
    upath t = null;
    assert(t, t == null);
    upath u = null;
    assert(u, t == u);
    assert(r, r == s);
    assert(r, r != u);
    assert(r, u != r);

    upath e = "etc";
    ispath(e, "etc");
    upath re = r;
    re += e;
    ispath(re, "/etc");
    upath res = re;
    res += s;
    ispath(res, "/etc/");

    upath p = "passwd";
    ispath(p, "passwd");
    upath ep = r + e + p;
    ispath(ep, "/etc/passwd");
    ispath(ep.parent(), "/etc");
    expect(ep.name(), "passwd");
    ispath(ep, r + ep.parent().name() + ep.name());
    ispath(ep, r + ep.parent().name() + s + ep.name());

    assert(re, re.isAbsolute());
    assert(re, !re.isRelative());
    assert(re, re.dirExists());
    assert(re, re.isReadable());
    assert(re, !re.isWritable() || getuid() == 0);
    assert(re, re.isExecutable());
    assert(re, !re.fileExists());

    assert(ep, ep.isAbsolute());
    assert(ep, !ep.isRelative());
    assert(ep, ep.fileExists());
    assert(ep, ep.isReadable());
    assert(ep, !ep.isWritable() || getuid() == 0);
    assert(ep, !ep.isExecutable());
    assert(ep, !ep.dirExists());

    upath eps = ep + s;
    expect(eps, "/etc/passwd/");
    upath neps = eps.name();
    expect(neps, "");
    upath peps = eps.parent();
    expect(peps, "/etc");
}

int main(int argc, char **argv)
{
    prog = basename(argv[0]);

    test_mstring();
    test_upath();

    return 0;
}
