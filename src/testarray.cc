#include "yarray.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

char const *ApplicationName("testarray");
bool multiByte(true);

static void dump(const char *label, const YArray<int> &array) {
    printf("%s: count=%ld, capacity=%ld\n  content={",
           label, array.getCount(), array.getCapacity());

    if (array.getCount() > 0) {
        printf(" %d", array[0]);

        for (YArray<int>::SizeType i = 1; i < array.getCount(); ++i)
            printf(", %d", array[i]);
    }

    puts(" }");
}

static void dump(const char *label, const YArray<const char *> &array) {
    printf("%s: count=%ld, capacity=%ld\n  content={",
           label, array.getCount(), array.getCapacity());

    for (YArray<const char *>::SizeType i = 0; i < array.getCount(); ++i)
        printf(" %s", array[i]);

    puts(" }");
}

static void dump(const char *label, const YStringArray &array) {
    printf("%s: count=%ld, capacity=%ld\n  content={",
           label, array.getCount(), array.getCapacity());

    for (YStringArray::SizeType i = 0; i < array.getCount(); ++i)
        printf(" %s", array[i]);

    puts(" }");
}

static const char *cmp(const YStringArray &a, const YStringArray &b) {
    if (a.getCount() != b.getCount()) return "size differs";

    for (YStringArray::SizeType i = 0; i < a.getCount(); ++i)
        if (strnullcmp(a[i], b[i])) return "values differ";

    for (YStringArray::SizeType i = 0; i < a.getCount(); ++i)
        if (a[i] != b[i]) return "pointers differ";

    return "equal - MUST BE AN ERROR";
}

int main() {
    YArray<int> a;

    puts("testing append YArray<int>");

    for (int i = 0; i < 13; ++i) {
        dump("Array<int>", a); a.append(i);
    }
    assert(a.getCount() == 13);
    assert(a[1] = 1);
    assert(a[12] = 12);

    dump("Array<int>", a);

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

    puts("testing clear for YArray<int>");
    a.clear(); dump("Array<int>", a);
    assert(a.getCount() == 0);

    puts("another insertion test for YArray<int>");
    a.insert(0, 1); dump("Array<int>: inserted 1@0", a);
    a.insert(1, 2); dump("Array<int>: inserted 2@1", a);
    a.insert(1, 3); dump("Array<int>: inserted 3@1", a);
    a.insert(0, 4); dump("Array<int>: inserted 4@0", a);
    a.insert(0, 5); dump("Array<int>: inserted 5@0", a);
    a.insert(0, 6); dump("Array<int>: inserted 6@0", a);
    assert(a.getCount() == 6);
    assert(a[5] == 2);

    puts("testing append for YArray<const char *>");

    YArray<const char *> b;
    dump("Array<const char *>", b); b.append("this");
    dump("Array<const char *>", b); b.append("is");
    dump("Array<const char *>", b); b.append("the");
    dump("Array<const char *>", b); b.append("stinking");
    dump("Array<const char *>", b); b.append("foo");
    dump("Array<const char *>", b); b.append("bar");
    dump("Array<const char *>", b);

    puts("testing append for YStringArray");

    YStringArray c;
    dump("YStringArray", c); c.append("this");
    dump("YStringArray", c); c.append("is");
    dump("YStringArray", c); c.append("the");
    dump("YStringArray", c); c.append("stinking");
    dump("YStringArray", c); c.append("foo");
    dump("YStringArray", c); c.append("bar");
    dump("YStringArray", c);

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

    return 0;
}
