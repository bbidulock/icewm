#include "ypointer.h"
#include <string.h>
#include <stdio.h>

// this is best run with valgrind --leak-check=full.

#define expect(x,y) if ((x) == (y)) ok(); else nex(#x, #y, __LINE__)
#define assert(x)   if ((x)) ok(); else bad(#x, __LINE__)

static int succ, fail, done;

static void ok()
{
    ++succ, ++done;
}

static void nex(const char *x, const char *y, int l)
{
    ++fail, ++done;
    printf("%s:%d: expected %s == %s\n",
            __FILE__, l, x, y);
}

static void bad(const char *x, int l)
{
    ++fail, ++done;
    printf("%s:%d: assertion failed %s\n",
            __FILE__, l, x);
}

void test_ypointer() {
    osmart<char> m(new char('m'));
    osmart<char> n(new char('n'));

    osmart<char> k(new char('k'));
    osmart<char> l(new char('l'));

    osmart<char> i(new char('i'));
    osmart<char> j(new char('j'));

    osmart<char> p(new char('p'));
    osmart<char> q(new char('q'));
    expect(*p, 'p');
    expect(*q, 'q');
    *q = '2';

    *p = '1';
    assert(*p == '1');
    const osmart<char> r(new char('r'));

    assert(*r == 'r');

    asmart<char> a(new char[1]);
    asmart<char> b(new char[1]);
    a[0] = 'a';
    b[0] = 'b';
    expect(*a, 'a');
    expect(*b, 'b');

    *a = 'c';

    assert(*a == 'c');

    csmart s(new char[1]);
    csmart t(new char[1]);

    *s = 's';
    *t = 't';
    expect(s[0], 's');
    expect(t[0], 't');

    *s = 'u';

    fsmart<char> f(strdup("f"));
    fsmart<char> g(strdup("g"));
    assert(!strcmp(f, "f"));
    assert(!strcmp(g, "g"));

    f[0] = 'h';
}

static void report()
{
    if (fail) {
        printf("%s: %d/%d tests failed, %d/%d tests succeeded\n",
                __FILE__, fail, done, succ, done);
    } else {
        printf("%s: %d/%d tests succeeded\n",
                __FILE__, succ, done);
    }
}

int main(int argc, char **argv) {
    test_ypointer();
    report();
    return !fail;
}
