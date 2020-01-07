#include "config.h"
#include "mstring.h"
#include "yarray.h"
#include "ypointer.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

char const *ApplicationName("testarray");
bool multiByte(true);

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
    printf("%s: count=%d, capacity=%d\n  content={",
           label, array.getCount(), array.getCapacity());

    if (array.nonempty()) {
        printf(" %d", array[0]);

        for (YArray<int>::SizeType i = 1; i < array.getCount(); ++i)
            printf(", %d", array[i]);
    }

    puts(" }");
}

static void dump(const char *label, const YArray<const char *> &array) {
    printf("%s: count=%d, capacity=%d\n  content={",
           label, array.getCount(), array.getCapacity());

    for (YArray<const char *>::SizeType i = 0; i < array.getCount(); ++i)
        printf(" %s", array[i]);

    puts(" }");
}

static void dump(const char *label, const YStringArray &array) {
    printf("%s: count=%d, capacity=%d\n  content={",
           label, array.getCount(), array.getCapacity());

    for (YStringArray::SizeType i = 0; i < array.getCount(); ++i)
        printf(" %s", array[i]);

    puts(" }");
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

    puts("testing append YArray<int>");

    for (int i = 0; i < 13; ++i) {
        dump("Array<int>", a); a.append(i);
    }
    assert(a.getCount() == 13);
    assert(a[1] = 1);
    assert(a[12] = 12);

    dump("Array<int>", a);

    puts("");
    puts("testing insert for YArray<int>");

    a.insert(5, -1); dump("Array<int>", a);
    a.insert(13, -2); dump("Array<int>", a);
    a.insert(15, -3); dump("Array<int>", a);
    a.insert(16, -4); dump("Array<int>", a);
    //    a.insert(42, -5); dump("Array<int>", a);
    //    a.insert(160, -6); dump("Array<int>", a);
    a.insert(0, -7); dump("Array<int>", a);
    assert(a.getCount() == 18);
    assert(a[6] == -1);
    assert(a[17] == -4);

    puts("");
    puts("testing clear for YArray<int>");
    a.clear(); dump("Array<int>", a);
    assert(a.isEmpty());

    puts("");
    puts("another insertion test for YArray<int>");
    a.insert(0, 1); dump("Array<int>: inserted 1@0", a);
    a.insert(1, 2); dump("Array<int>: inserted 2@1", a);
    a.insert(1, 3); dump("Array<int>: inserted 3@1", a);
    a.insert(0, 4); dump("Array<int>: inserted 4@0", a);
    a.insert(0, 5); dump("Array<int>: inserted 5@0", a);
    a.insert(0, 6); dump("Array<int>: inserted 6@0", a);
    assert(a.getCount() == 6);
    assert(a[5] == 2);

    puts("");
}

static void test_cptr() {
    puts("testing append for YArray<const char *>");

    YArray<const char *> b;
    dump("Array<const char *>", b); b.append("this");
    dump("Array<const char *>", b); b.append("is");
    dump("Array<const char *>", b); b.append("the");
    dump("Array<const char *>", b); b.append("stinking");
    dump("Array<const char *>", b); b.append("foo");
    dump("Array<const char *>", b); b.append("bar");
    dump("Array<const char *>", b);

    puts("");
}

static void test_str() {
    puts("testing append for YStringArray");

    YStringArray c;
    dump("YStringArray", c); c.append("this");
    dump("YStringArray", c); c.append("is");
    dump("YStringArray", c); c.append("the");
    dump("YStringArray", c); c.append("stinking");
    dump("YStringArray", c); c.append("foo");
    dump("YStringArray", c); c.append("bar");
    dump("YStringArray", c);

    puts("");

    puts("copy constructors for YStringArray");

    YStringArray orig;
    orig.append("check");
    orig.append("this");
    orig.append("out");
    dump("orig", orig);

    const YStringArray copy(orig);
    const YStringArray copy2(copy);

    dump("orig", orig);
    dump("copy", copy);
    dump("copy2", copy2);

    printf("orig vs. copy: %s\n", cmp(orig, copy));
    printf("orig vs. copy2: %s\n", cmp(orig, copy2));
    printf("copy vs. copy2: %s\n", cmp(copy, copy2));

    for (int i = 0; i < copy.getCount(); ++i) {
        printf("C copy[%d] = %s.\n", i, copy.getCArray()[i]);
    }
    for (int i = 0; i < copy2.getCount(); ++i) {
        printf("C copy2[%d] = %s.\n", i, copy2.getCArray()[i]);
    }

    puts("");
}

static void test_mstr() {
    typedef MStringArray::IterType iter_t;

    watch mark;

    char buf[24];
    const int N = 12345;
    asmart<mstring> ms(new mstring[N]);
    for (int i = 0; i < N; ++i) {
        snprintf(buf, sizeof buf, "%d", i);
        ms[i] = mstring(buf);
    }
    MStringArray ma;
    for (int i = 0; i < N; ++i) {
        ma.append(ms[i]);
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

    printf("tested MStringArray OK (%s)\n\n", mark.report());
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

    printf("tested refstring array OK (%s)\n\n", mark.report());
}

int main() {
    test_int();
    test_cptr();
    test_str();
    test_mstr();
    test_refstr();

    return 0;
}

// vim: set sw=4 ts=4 et:
