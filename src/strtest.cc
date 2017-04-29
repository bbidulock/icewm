#include "config.h"
#include "mstring.h"
#include <stdio.h>
#include <libgen.h>

char const *ApplicationName = "strtest";

#define expect(u, s)    if (++testsrun, (u) == mstring(s) && 0 == strcmp(cstring(u).c_str(), s)) ++passed; else test_failed(u, s, __FILE__, __LINE__)

#define assert(u, b)    if (++testsrun, (b)) ++passed; else test_failed(u, #b, __FILE__, __LINE__)

static int testsrun, passed, failed;
static const char *prog;

static void test_failed(const mstring& u, const char *s, const char *f, int l)
{
    printf("%s: Test failed: %s.%d: u = \"%s\", s = \"%s\"\n",
            prog, f, l, cstring(u).c_str(), s);
    ++failed;
}

int main(int argc, char **argv)
{
    prog = basename(argv[0]);

    mstring x("foo");
    expect(x, "foo");
    assert(x, x.length() == 3);

    mstring b("bar", 2);
    expect(b, "ba");
    assert(b, b.length() == 2);

    mstring e(null);
    expect(e, "");
    assert(e, e.length() == 0);

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
    assert(w, w == " \t\r\nabc\n\r\t ");
    w = w.replace(0, 4, "_");
    w = w.replace(4, 4, ".");
    expect(w, "_abc.");
    t = w.remove(1, 3);
    expect(t, "_.");
    mstring k = t.insert(1, "xyz");
    expect(k, "_xyz.");
    mstring q = t.append("!?");
    expect(q, "_.!?");

    if (failed || passed != testsrun) {
        printf("%s: %d tests failed, %d tests passed of %d total\n",
                prog, failed, passed, testsrun);
    }
    else {
        printf("%s: %d/%d tests passed\n",
                prog, passed, testsrun);
    }
    return 0;
}
