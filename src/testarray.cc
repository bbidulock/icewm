#include "config.h"
#include "mstring.h"
#include "yarray.h"
#include "ypointer.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define assert(a) if ((a) != 0) okays++; else bad(#a, __LINE__)

char const *ApplicationName("testarray");
bool multiByte(true);
static bool test_time(false);
static bool test_dump(false);
static int fails;
static int okays;
static int total;

static void bad(const char* str, int line) {
    fails++;
    printf("%s: test failed at line %d: %s\n", ApplicationName, line, str);
}

static void report(const char* mod) {
    int done = fails + okays;
    if (fails) {
        printf("%s: %6s: %d/%d tests failed, %d/%d tests succeeded\n",
                ApplicationName, mod+5, fails, done, okays, done);
    } else {
        printf("%s: %6s: %d/%d tests succeeded\n",
                ApplicationName, mod+5, okays, done);
    }
    total += fails;
    fails = okays = 0;
}

class watch {
    double start;
    char buf[42];
public:
    double time() const {
        timeval now;
        gettimeofday(&now, 0);
        return now.tv_sec + 1e-6 * now.tv_usec;
    }
    watch() : start(time()) {}
    double delta() const { return time() - start; }
    const char* report() {
        snprintf(buf, sizeof buf, "%.6f seconds", delta());
        return buf;
    }
};

int strnullcmp(const char *a, const char *b) {
    return a ? (b ? strcmp(a, b) : 1) : (b ? -1 : 0);
}

static void dump(const char *label, const YArray<int> &array) {
    if (test_dump) {
        printf("%s: count=%d, capacity=%d\n  content={",
               label, array.getCount(), array.getCapacity());

        if (array.nonempty()) {
            printf(" %d", array[0]);

            for (YArray<int>::SizeType i = 1; i < array.getCount(); ++i)
                printf(", %d", array[i]);
        }

        puts(" }");
    }
}

static void dump(const char *label, const YArray<const char *> &array) {
    if (test_dump) {
        printf("%s: count=%d, capacity=%d\n  content={",
               label, array.getCount(), array.getCapacity());

        for (YArray<const char *>::SizeType i = 0; i < array.getCount(); ++i)
            printf(" %s", array[i]);

        puts(" }");
    }
}

static void dump(const char *label, const YStringArray &array) {
    if (test_dump) {
        printf("%s: count=%d, capacity=%d\n  content={",
               label, array.getCount(), array.getCapacity());

        for (YStringArray::SizeType i = 0; i < array.getCount(); ++i)
            printf(" %s", array[i]);

        puts(" }");
    }
}

static const char *cmp(const YStringArray &a, const YStringArray &b) {
    static char buf[999];
    if (a.getCount() != b.getCount()) {
        snprintf(buf, sizeof buf,
                 "size differs: %d != %d", a.getCount(), b.getCount());
        return buf;
    }

    for (YStringArray::SizeType i = 0; i < a.getCount(); ++i)
        if (strnullcmp(a[i], b[i])) {
            snprintf(buf, sizeof buf,
                     "values differ at %d: '%s' != '%s'",
                    i, a[i], b[i]);
            return buf;
        }

    for (YStringArray::SizeType i = 0; i < a.getCount(); ++i)
        if (a[i] != b[i]) {
            snprintf(buf, sizeof buf,
                     "pointers differ at %d: %p != %p",
                    i, a[i], b[i]);
            return buf;
        }

    return "equal - MUST BE AN ERROR";
}

static void test_int() {
    YArray<int> a;

    for (int i = 0; i < 13; ++i) {
        dump("Array<int>", a); a.append(i);
        assert(a.getCount() == i+1 && a[i] == i);
    }
    assert(a.getCount() == 13);
    assert(a[1] = 1);
    assert(a[12] = 12);

    dump("Array<int>", a);

    a.insert(5, -1); dump("Array<int>", a);
    assert(a.getCount() == 14 && a[5] == -1);
    a.insert(13, -2); dump("Array<int>", a);
    assert(a.getCount() == 15 && a[13] == -2);
    a.insert(15, -3); dump("Array<int>", a);
    assert(a.getCount() == 16 && a[15] == -3);
    a.insert(16, -4); dump("Array<int>", a);
    assert(a.getCount() == 17 && a[16] == -4);
    //    a.insert(42, -5); dump("Array<int>", a);
    //    a.insert(160, -6); dump("Array<int>", a);
    a.insert(0, -7); dump("Array<int>", a);
    assert(a.getCount() == 18 && a[0] == -7);
    assert(a[6] == -1);
    assert(a[17] == -4);
    a.remove(13);
    a.remove(13);
    a.remove(13);
    assert(a[13] == -3);

    a.clear(); dump("Array<int>", a);
    assert(a.isEmpty());

    a.insert(0, 1); dump("Array<int>: inserted 1@0", a);
    a.insert(1, 2); dump("Array<int>: inserted 2@1", a);
    a.insert(1, 3); dump("Array<int>: inserted 3@1", a);
    a.insert(0, 4); dump("Array<int>: inserted 4@0", a);
    a.insert(0, 5); dump("Array<int>: inserted 5@0", a);
    a.insert(0, 6); dump("Array<int>: inserted 6@0", a);
    assert(a.getCount() == 6);
    assert(a[5] == 2);
    assert(a[4] == 3);
    assert(a[3] == 1);
    assert(a[2] == 4);
    assert(a[1] == 5);
    assert(a[0] == 6);

    a.moveto(0, 5);
    a.moveto(2, 1);
    a.moveto(4, 2);
    a.moveto(4, 3);
    a.moveto(0, 4);
    assert(a[5] == 6);
    assert(a[4] == 5);
    assert(a[3] == 4);
    assert(a[2] == 3);
    assert(a[1] == 2);
    assert(a[0] == 1);

    report(__func__);
}

static void test_cptr() {
    YArray<const char *> b;
    assert(b.getCount() == 0);
    dump("Array<const char *>", b); b.append("this");
    assert(b.getCount() == 1 && !strcmp(b[0], "this"));
    dump("Array<const char *>", b); b.append("is");
    assert(b.getCount() == 2 && !strcmp(b[0], "this"));
    assert(b.getCount() == 2 && !strcmp(b[1], "is"));
    dump("Array<const char *>", b); b.append("the");
    assert(b.getCount() == 3 && !strcmp(b[0], "this"));
    assert(b.getCount() == 3 && !strcmp(b[1], "is"));
    assert(b.getCount() == 3 && !strcmp(b[2], "the"));
    dump("Array<const char *>", b); b.append("stinking");
    assert(b.getCount() == 4 && !strcmp(b[0], "this"));
    assert(b.getCount() == 4 && !strcmp(b[1], "is"));
    assert(b.getCount() == 4 && !strcmp(b[2], "the"));
    assert(b.getCount() == 4 && !strcmp(b[3], "stinking"));
    dump("Array<const char *>", b); b.append("foo");
    assert(b.getCount() == 5 && !strcmp(b[0], "this"));
    assert(b.getCount() == 5 && !strcmp(b[1], "is"));
    assert(b.getCount() == 5 && !strcmp(b[2], "the"));
    assert(b.getCount() == 5 && !strcmp(b[3], "stinking"));
    assert(b.getCount() == 5 && !strcmp(b[4], "foo"));
    dump("Array<const char *>", b); b.append("bar");
    assert(b.getCount() == 6 && !strcmp(b[0], "this"));
    assert(b.getCount() == 6 && !strcmp(b[1], "is"));
    assert(b.getCount() == 6 && !strcmp(b[2], "the"));
    assert(b.getCount() == 6 && !strcmp(b[3], "stinking"));
    assert(b.getCount() == 6 && !strcmp(b[4], "foo"));
    assert(b.getCount() == 6 && !strcmp(b[5], "bar"));
    dump("Array<const char *>", b);

    report(__func__);
}

static void test_str() {
    YStringArray c;
    dump("YStringArray", c); c.append("this");
    assert(c.getCount() == 1 && !strcmp(c[0], "this"));
    dump("YStringArray", c); c.append("is");
    assert(c.getCount() == 2 && !strcmp(c[1], "is"));
    dump("YStringArray", c); c.append("the");
    assert(c.getCount() == 3 && !strcmp(c[2], "the"));
    dump("YStringArray", c); c.append("stinking");
    assert(c.getCount() == 4 && !strcmp(c[3], "stinking"));
    dump("YStringArray", c); c.append("foo");
    assert(c.getCount() == 5 && !strcmp(c[4], "foo"));
    dump("YStringArray", c); c.append("bar");
    assert(c.getCount() == 6 && !strcmp(c[5], "bar"));
    dump("YStringArray", c);

    YStringArray orig;
    orig.append("check");
    orig.append("this");
    orig.append("out");
    dump("orig", orig);
    assert(orig.getCount() == 3);

    const YStringArray copy(orig);
    assert(orig.getCount() == 0);
    assert(copy.getCount() == 3);
    assert(copy[0] && !strcmp(copy[0], "check"));
    assert(copy[1] && !strcmp(copy[1], "this"));
    assert(copy[2] && !strcmp(copy[2], "out"));
    const YStringArray copy2(copy);
    assert(orig.getCount() == 0);
    assert(copy.getCount() == 3);
    assert(copy2.getCount() == 3);
    assert(copy2[0] && !strcmp(copy2[0], "check"));
    assert(copy2[1] && !strcmp(copy2[1], "this"));
    assert(copy2[2] && !strcmp(copy2[2], "out"));

    dump("orig", orig);
    dump("copy", copy);
    dump("copy2", copy2);

    if (orig.getCount() || copy.getCount() != 3)
        printf("orig vs. copy: %s\n", cmp(orig, copy));
    if (orig.getCount() || copy2.getCount() != 3)
        printf("orig vs. copy2: %s\n", cmp(orig, copy2));
    if (copy.getCount() != 3 || copy2.getCount() != 3)
        printf("copy vs. copy2: %s\n", cmp(copy, copy2));

    report(__func__);
}

static void test_mstr() {
    typedef MStringArray::IterType iter_t;

    watch mark;

    char buf[24];
    const int N = 1234;
    asmart<mstring> ms(new mstring[N]);
    for (int i = 0; i < N; ++i) {
        snprintf(buf, sizeof buf, "%d", i);
        ms[i] = mstring(buf);
    }
    MStringArray ma;
    for (int i = 0; i < N; ++i) {
        ma.append(ms[i]);
        assert(find(ma, ms[i]) == i);
    }
    long n = 0L;
    for (mstring& m : ma) {
        assert(m == mstring(n));
        n++;
    }

    int c = 0;
    for (iter_t iter = ma.iterator(); ++iter; ) {
        assert(iter.where() == c);
        assert(*iter == ms[c]);
        ++c;
    }
    assert(c == N);
    for (iter_t iter = ma.reverseIterator(); ++iter; ) {
        --c;
        assert(iter.where() == c);
        assert(*iter == ms[c]);
    }
    assert(c == 0);

    for (int i = 0; i < N; ++i) {
        int k = i;
        while (k & 1)
            k >>= 1;
        if (k != i) {
            ma.remove(i);
            ma.insert(i, ma[k]);
        }
    }
    for (int i = 0; i < N; ++i) {
        int k = i;
        while (k & 1)
            k >>= 1;
        assert(ma[i] == ms[k]);
    }

    if (test_time)
        printf("tested MStringArray OK (%s)\n\n", mark.report());
    report(__func__);
}

/*
 * A very simple sharable string class for use with ref and YRefArray.
 * The copy semantics are thanks to ref; use YRefArray for aggregation.
 */
class stringcounted : public refcounted {
public:
    stringcounted() : ptr(0) {}         // remain null forever
    stringcounted(const char* str);     // new[] allocate copy
    virtual ~stringcounted();           // delete[] string ptr
    operator const char *() const { return ptr; }
    char operator[](int i) const { return ptr[i]; }
private:
    stringcounted(const stringcounted&);        // use ref
    void operator=(const stringcounted&);       // use ref
    const char* ptr;
};

stringcounted::stringcounted(const char* str):
    ptr(str ? strdup(str) : 0)
{
}

stringcounted::~stringcounted() {
    free((void *) ptr);
}

typedef ref<stringcounted> refstring;

inline refstring newRefString(const char *str) {
    return refstring(new stringcounted(str));
}

static void test_refstr() {
    typedef YRefArray<stringcounted> array_t;

    watch mark;

    char buf[24];
    const int N = 12345;
    asmart<csmart> ms(new csmart[N]);
    for (int i = 0; i < N; ++i) {
        snprintf(buf, sizeof buf, "%d", i);
        ms[i] = newstr(buf);
    }
    array_t ma;
    for (int i = 0; i < N; ++i) {
        ma.append(newRefString(ms[i]));
    }

    for (int i = 0; i < N; ++i) {
        assert(!strcmp(*ma[i], ms[i]));
    }

    for (int i = 0; i < N; ++i) {
        int k = i;
        while (k & 1)
            k >>= 1;
        if (k != i) {
            ma.remove(i);
            ma.insert(i, ma[k]);
        }
    }
    for (int i = 0; i < N; ++i) {
        int k = i;
        while (k & 1)
            k >>= 1;
        assert(!strcmp(*ma[i], ms[k]));
    }

    if (test_time)
        printf("tested refstring array OK (%s)\n\n", mark.report());
    report(__func__);
}

static void test_options(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        char* s = argv[i];
        if (!strcmp(s, "-t") || !strcmp(s, "--time")) {
            test_time = true;
        }
        else if (!strcmp(s, "-d") || !strcmp(s, "--dump")) {
            test_dump = true;
        }
        else {
            printf("invalid option: %s\n", s);
        }
    }
}

int main(int argc, char** argv) {
    test_options(argc, argv);
    test_int();
    test_cptr();
    test_str();
    test_mstr();
    test_refstr();

    return total != 0;
}

// vim: set sw=4 ts=4 et:
