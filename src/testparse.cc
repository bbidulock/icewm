#include "yparse.h"

#include <stdio.h>
#include <unistd.h>

const char *ApplicationName = "testparse";

void dumpAttributes(ref<YAttribute> node) {
    while (node != null) {
        printf(" %s='%s'",
               cstring(node->name()).c_str(),
               cstring(node->getValue()).c_str());

        node = node->next();
    }
}

void dumpNodes(ref<YNode> node, int level) {
    int i;

    while (node != null) {
        ref<YElement> e = node->toElement();
        if (e != null) {
            if (e->firstChild() == null) {
                for (i = 0; i < level; i++)
                    printf("    ");
                printf("%s", cstring(e->name()).c_str());
                dumpAttributes(e->firstAttribute());
                printf(";\n");
            } else {
                for (i = 0; i < level; i++)
                    printf("    ");
                printf("%s", cstring(e->name()).c_str());
                dumpAttributes(e->firstAttribute());
                printf(" {\n");
                dumpNodes(e->firstChild(), level + 1);
                for (i = 0; i < level; i++)
                    printf("    ");
                printf("}\n");
            }
        } else {
            ref<YText> t = node->toText();
            if (t != null) {
                for (i = 0; i < level; i++)
                    printf("    ");
                printf("'%s';\n", cstring(t->text()).c_str());
            }
        }

        node = node->next();
    }
}

int write_fd(int *t, const char *buf, int len) {
    return write(*t, buf, len);
}

void test(const char *test) {
    int len = strlen(test);
    char *buf = new char[len];
    memcpy(buf, test, len + 1);
    YParseResult res;
    ref<YDocument> doc = YDocument::parse(buf, len, res);

    printf("---\n");
    if (doc != null) {
        dumpNodes(doc->firstChild(), 0);
        int fd = 1;
        int l, rc;

        printf("writing:\n");
        rc = doc->write(&fd, write_fd, &l);
        printf("wrote: %d %d\n", rc, len);
    } else {
        msg("error at %d:%d\n", res.row, res.col);
    }
}

int main(int argc, char **argv) {

    if (argc > 1) {
        static char buf[2 * 1024 * 1024];
        int len = read(0, buf, sizeof(buf));
        YParseResult res;
        ref<YDocument> doc = YDocument::parse(buf, len, res);
        if (doc != null) {
            msg("ok");
            //dumpNodes(doc->firstChild(), 0);
        } else {
            msg("error at %d:%d\n", res.row, res.col);
        }
    } else {
        test("test;");
        test("test n='1';");
        test("test n='1' b='true';");
        test("test n='1' b='true' { baz; }");
        test("test ; foo ; ");
        test("test {}");
        test("test { 'foo'; \"bar\"; }");
        test("test { foo; }");
        test("test; foo; bar;");
        test("a{b{c{d{e;}}}}");
        test("a{");
        test("b");
        test("b ( {} )");
        test("d a=");
        test("d a='");
        test("'");
        test("'\\");
    }
//    test("test xml:='www.w3.org/xml';");
//    test("test (foo='a') {}");
}
