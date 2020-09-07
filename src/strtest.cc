#include "config.h"
#include "mstring.h"
#include "upath.h"
#include "base.h"
#include "udir.h"
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <fnmatch.h>

char const *ApplicationName = "strtest";
static const char source[] = __FILE__;

#define equal(p, s)     (mstring(p) == mstring(s))

#define expect(u, s)    if (++testsrun, (u) == mstring(s) && equal(u, s)) \
        ++passed; else test_failed(u, s, __LINE__)

// XXX: those argument ordering and purpose := strange.
// Also name collision with regular assert macro.

#define assert(u, b)    if (++testsrun, (b)) ++passed; else \
        test_failed(mstring(u), #b, __LINE__)

#define ispath(u, s)    if (++testsrun, (u) == upath(s) && equal(u, s)) \
        ++passed; else test_failed(u, s, __LINE__)

#define sequal(u, s)    if (++testsrun, equal(u, s)) \
        ++passed; else test_failed(u, s, __LINE__)

#define ASSERT_EQ(l,r) if(++testsrun, (l) == (r)) ++passed; \
		else { test_failed(#l, #r, __LINE__); return; }
#define EXPECT_EQ(l,r) if(++testsrun, (l) == (r)) ++passed; \
        else test_failed(#l, #r, __LINE__);

static int testsrun, passed, failed;
static const char *prog;
static int total_failed = 0;

// basic detection of build daemon environment, where $HOME, $USER and
// ~ and ~<user> are not necessarily related.
bool isFakerootActive() { auto e=getenv("FAKED_MODE"); return e && *e; }

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

static void test_failed(mstring u, const char *s, int l)
{
    printf("%s: Test failed in %s:%d: u = \"%s\", s = \"%s\"\n",
            prog, source, l,
            u == null ? "NULL" : u.c_str(),
            s == nullptr ? "NULL" : s);
    ++failed;
    ++total_failed;
}

static void test_failed(mstring u, mstring s, int l)
{
    return test_failed(u, s == null ? "NULL" : s.c_str(), l);
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
    assert(e, e.find(null) == 0);
    assert(e, e.find(" ") == -1);
    assert(e, e.indexOf(' ') == -1);
    assert(e, e.lastIndexOf(' ') == -1);
    assert(e, e.count(' ') == 0);
    assert(e, e.substring(0) == null);
    assert(e, e.substring(0, 0) == null);

    mstring m("abc", (size_t) 0);
    expect(m, "");
    assert(m, m.length() == 0);

    mstring n(nullptr, (size_t) 0);
    expect(n, "");
    assert(n, n.length() == 0);

    mstring y = x;
    expect(y, "foo");
    assert(y, y.length() == 3);
    assert(y, y.compareTo(mstring("foo")) == 0);
    assert(y, y.compareTo(mstring("aaf")) > 0);
    assert(y, y.compareTo(mstring("zuu")) < 0);
    assert(y, y.compareTo(mstring(null)) > 0);

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
    assert(c, c.count('o') == 2);
    assert(c, c.count('f') == 1);
    assert(c, c.count('a') == 1);
    assert(c, c.count('?') == 0);
    expect(c.substring(5), "");
    expect(c.substring(3, 2), "ba");

    mstring s = c.substring(2);
    expect(s, "oba");
    s = c.substring(2, 2);
    expect(s, "ob");

    assert(s, s.charAt(1) == 'b');
    assert(s, s.charAt(2) == -1);
    assert(s, s.charAt(-1) == -1);
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
    assert(w, w.find("abc") == 4);
    assert(w, w.find("def") == -1);
    assert(w, w.find(" ") == 0);
    assert(w, w.find("") == 0);
    w = w.replace(0, 4, "_");
    w = w.replace(4, 4, ".");
    expect(w, "_abc.");
    t = w.remove(1, 3);
    expect(t, "_.");
    mstring k = t.insert(1, "xyz");
    expect(k, "_xyz.");
    mstring q = t.append("!?");
    expect(q, "_.!?");

    mstring ht = "http://www.icewm.org/";
    mstring hs = "https://www.icewm.org/";
    assert(ht, ht.find("://") == 4);
    assert(ht, ht.find(":///") == -1);
    assert(hs, ht.substring(ht.find("www")) == hs.substring(hs.find("www")));

    mstring sl = "/././././././././././";
    q = sl.searchAndReplaceAll("/./", "/");
    expect(q, "/");
    q = sl.searchAndReplaceAll("/./", "@");
    expect(q, "@.@.@.@.@./");
    q = sl.searchAndReplaceAll("/./", "/./");
    expect(q, sl);

    mstring ul = "aBcD.";
    q = ul.lower();
    expect(q, "abcd.");
    q = ul.upper();
    expect(q, "ABCD.");

    mstring u = nullptr;
    expect(u, "");
    u = mstring(nullptr) + "aha";
    expect(u, "aha");
    u = mstring("aha") + nullptr;
    expect(u, "aha");

    u = mstring("ab", "cd");
    expect(u, "abcd");
    u = mstring("ab", nullptr);
    expect(u, "ab");
    u = mstring(nullptr, "cd");
    expect(u, "cd");
    u = mstring(nullptr, nullptr);
    assert(u, u.isEmpty());

    u = mstring("ab", "cd", "ef");
    expect(u, "abcdef");
    u = mstring("ab", "cd", nullptr);
    expect(u, "abcd");
    u = mstring("ab", nullptr, "ef");
    expect(u, "abef");
    u = mstring(nullptr, "cd", "ef");
    expect(u, "cdef");
    u = mstring(nullptr, nullptr, nullptr);
    expect(u, "");

    u = mstring("#ffff").match("#fffff");
    expect(u, null);
    u = mstring("#fffff").match("#fffff");
    expect(u, "#fffff");
    u = mstring("f#ffffff").match("#fffff");
    expect(u, "#fffff");
    u = mstring("f#ffffff").match("#f{5}");
    expect(u, "#fffff");
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
    expect(u, t + u);
    expect(r, r + u);
    expect(s, t + s);

    upath e = "etc";
    ispath(e, "etc");
    upath re = r;
    re += e;
    ispath(re, "/etc");
    upath res = re;
    res += s;
    ispath(res, "/etc/");
    expect(e, t + e);
    expect(e, e + u);
    expect(re, t + re);
    expect(re, re + u);

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
    assert(neps, neps.length() == 0);
    upath peps = eps.parent();
    expect(peps, "/etc");

    upath fi = "file:///etc/passwd";
    upath ft = "ftp://www.icewm.org/";
    upath ht = "http://www.icewm.org/";
    upath hs = "https://www.icewm.org";
    upath np = "https:/www.icewm.org";
    upath nq = "https//www.icewm.org";
    assert(fi, fi.hasProtocol());
    assert(ft, ft.hasProtocol());
    assert(ht, ht.hasProtocol());
    assert(hs, hs.hasProtocol());
    assert(np, !np.hasProtocol());
    assert(nq, !nq.hasProtocol());
    assert(fi, !fi.isHttp());
    assert(ft, !ft.isHttp());
    assert(ht, ht.isHttp());
    assert(hs, hs.isHttp());
    assert(np, !np.isHttp());
    assert(nq, !nq.isHttp());

    upath a("abc.def");
    expect(a.getExtension(), ".def");
    a = "efg.";
    expect(a.getExtension(), null);
    a = "/hij.klm/nop";
    expect(a.getExtension(), null);
    a = a.path() + ".q";
    expect(a.getExtension(), ".q");
    a = "/stu/.vw";
    expect(a.getExtension(), null);
    if (!isFakerootActive()) {
        upath hm("~");
        expect(hm.expand(), getenv("HOME"));
        hm = "~/";
        assert(strlen(hm.expand()), 1 + strlen(getenv("HOME")));
    }
    upath hm = "$HOME";
    expect(hm.expand(), getenv("HOME"));
    hm = "$HOME/";
    assert(strlen(hm.expand()), 1 + strlen(getenv("HOME")));
    upath nothing("/else/matters");
    EXPECT_EQ(0, nothing.fnMatch("/else/ma*"));
    EXPECT_EQ(0, nothing.fnMatch("/el*/ma*"));
    EXPECT_EQ(FNM_NOMATCH, nothing.fnMatch("/else/mata*"));
}

static void test_strlc()
{
    strtest tester("strlc");
    char d[10] = "@";
    size_t n;
    n = strlcpy(d, "", 0);
    sequal(d, "@");
    assert(d, n == 0);

    n = strlcpy(d, "a", 0);
    sequal(d, "@");
    assert(d, n == 1);

    n = strlcpy(d, "", 1);
    sequal(d, "");
    assert(d, n == 0);

    n = strlcpy(d, "a", 1);
    sequal(d, "");
    assert(d, n == 1);

    n = strlcpy(d, "a", 2);
    sequal(d, "a");
    assert(d, n == 1);

    n = strlcpy(d, "ab", 2);
    sequal(d, "a");
    assert(d, n == 2);

    n = strlcpy(d, "ab", 3);
    sequal(d, "ab");
    assert(d, n == 2);

    n = strlcpy(d, "abc", sizeof d);
    sequal(d, "abc");
    assert(d, n == 3);

    n = strlcat(d, "def", 4);
    sequal(d, "abc");
    assert(d, n == 6);

    n = strlcat(d, "def", sizeof d);
    sequal(d, "abcdef");
    assert(d, n == 6);

    n = strlcat(d, "ghijkl", sizeof d);
    sequal(d, "abcdefghi");
    assert(d, n == 12);

    n = strlcpy(d, "123", sizeof d);
    sequal(d, "123");
    assert(d, n == 3);

    n = strlcpy(d, d + 1, sizeof d);
    sequal(d, "23");
    assert(d, n == 2);

    n = strlcpy(d, d + 1, sizeof d);
    sequal(d, "3");
    assert(d, n == 1);

    n = strlcpy(d, d + 1, sizeof d);
    sequal(d, "");
    assert(d, n == 0);
}

static void test_cdir()
{
    strtest tester("cdir");

    {
        cdir c("/etc");
        assert(c.path(), c.isOpen());
        int n = 0, p = 0, g = 0, r = 0;
        while (c.next()) {
            ++n;
            if (0 == strcmp(c.entry(), "passwd")) ++p;
            if (0 == strcmp(c.entry(), "group")) ++g;
            if (0 == strcmp(c.entry(), "resolv.conf")) ++r;
        }
        assert(c.path(), n >= 3);
        assert(c.path(), p == 1);
        assert(c.path(), g == 1);
        assert(c.path(), r == 1);

        c.open();
        n = 0;
        while (c.nextExt(".conf")) {
            ++n;
            const char *p = c.entry();
            assert(c.path(),
                    strlen(p) >= 5 &&
                    0 == strcmp(p + strlen(p) - 5, ".conf"));
        }
        assert(c.path(), n > 0);
    }
}

static void test_udir()
{
    strtest tester("udir");
    {
        udir u("/etc");
        assert(u.path(), u.isOpen());
        int n = 0, p = 0, g = 0, r = 0;
        while (u.next()) {
            ++n;
            if (u.entry() == "passwd") ++p;
            if (u.entry() == "group") ++g;
            if (u.entry() == "resolv.conf") ++r;
        }
        assert(u.path(), n >= 3);
        assert(u.path(), p == 1);
        assert(u.path(), g == 1);
        assert(u.path(), r == 1);

        u.open("/etc");
        n = 0;
        while (u.nextExt(".conf")) {
            ++n;
            assert(u.path(), u.entry().endsWith(".conf"));
        }
        assert(u.path(), n > 0);
    }
}

static void test_adir()
{
    strtest tester("adir");

    {
        adir a("/etc");
        assert(a.path(), a.isOpen());
        char buf[300] = "";
        while (a.next()) {
            const char *e = a.entry();
            assert(e, strcoll(buf, e) < 0);
            strlcpy(buf, e, sizeof buf);
        }
        assert(buf, strcoll(buf, "~~~~~~~~~") < 0);
    }

}

static void test_sdir()
{
    strtest tester("sdir");

    {
        sdir s("/etc");
        assert(s.path(), s.isOpen());
        char buf[300] = "";
        while (s.next()) {
            mstring c(s.entry());
            const char *e = c.c_str();
            assert(e, strcoll(buf, e) < 0);
            strlcpy(buf, e, sizeof buf);
        }
        assert(buf, strcoll(buf, "~~~~~~~~~") < 0);
    }

}

static void test_expand()
{
    strtest tester("expand");

    char* home = getenv("HOME");
    if (home) {
        char* tilde = tilde_expansion("~/");
        assert("~/", tilde != nullptr);
        if (tilde) {
            assert("~/", strncmp(tilde, home, strlen(home)) == 0);
            assert("~/", strlen(tilde) == strlen(home) + 1);
            delete[] tilde;
        }
    }

    if (!isFakerootActive()) {
        char* user = getenv("USER");
        if (user) {
            char buf[1024];
            snprintf(buf, sizeof buf, "~%s/", user);
            char* uhome = tilde_expansion(buf);
            assert(buf, uhome != nullptr);
            if (uhome) {
                assert(buf, strncmp(uhome, home, strlen(home)) == 0);
                assert(buf, strlen(uhome) == strlen(home) + 1);
                delete[] uhome;
            }
        }

        char rhome[] = "/root";
        if (access(rhome, 0) == 0) {
            char* tilde = tilde_expansion("~root/");
            assert("~root/", tilde != nullptr);
            if (tilde) {
                assert("~root/", strcmp(tilde, "/root/") == 0);
                delete[] tilde;
            }
        }
    }
    if (access("/usr/bin/printf", X_OK) == 0 &&
        access("/bin/printf", X_OK) != 0)
    {
        char* print = path_lookup("printf");
        assert("printf", print != nullptr);
        if (print) {
            assert("printf", strcmp(print, "/usr/bin/printf") == 0);
            delete[] print;
        }
    }

    home = getenv("HOME");
    if (home) {
        char dhome[] = "$HOME/";
        char buf[4096];
        snprintf(buf, sizeof buf, "%s/", home);
        char* expand = dollar_expansion(dhome);
        assert(dhome, expand != nullptr);
        if (expand) {
            assert(dhome, strcmp(expand, buf) == 0);
            delete[] expand;
        }
    }

    auto user = getenv("USER");
    if (user) {
        char duser[] = "${USER}/";
        char buf[4096];
        snprintf(buf, sizeof buf, "%s/", user);
        char* expand = dollar_expansion(duser);
        assert(duser, expand != nullptr);
        if (expand) {
            assert(duser, strcmp(expand, buf) == 0);
            delete[] expand;
        }
    }
}

int main(int argc, char **argv)
{
    prog = basename(argv[0]);

    test_mstring();
    test_expand();
    test_upath();
    test_strlc();
    test_cdir();
    test_udir();
    test_adir();
    test_sdir();

    return total_failed != 0;
}

// vim: set sw=4 ts=4 et:
