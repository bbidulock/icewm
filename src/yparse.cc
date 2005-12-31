#include "config.h"
#include "yparse.h"
#include "sysdep.h"

YNode::~YNode() {
}

YDocument::YDocument() {
}

void YDocument::freeNodes(ref<YNode> n) {
    ref<YNode> n1;
    while (n !=  null) {
        n1 = n->fNext;
        freeNode(n);
        n = n1;
    }
}

void YDocument::freeNode(ref<YNode> &n) {
    freeNodes(n->fFirstChild);
    n->fFirstChild = n->fLastChild = null;
    n->fParent = null;
    n->fPrev = null;
    n->fNext = null;
    ref<YElement> e = n->toElement();
    if (e != null) {
        ref<YAttribute> a = e->fFirstAttribute, a1;
        e->fLastAttribute = null;
        while (a != null) {
            a1 = a->fNext;
            a->fPrev = null;
            a->fNext = null;
            a->fParent = null;
            a = a1;
        }
       e-> fFirstAttribute = null;
    }
}

YDocument::~YDocument() {
    fLastChild = null;
    freeNodes(fFirstChild);
    fFirstChild = null;
}

void YDocument::addChild(ref<YNode> child) {
    if (fLastChild == null) {
        fFirstChild = child;
        fLastChild = child;
    } else {
        fLastChild->fNext = child;
        child->fPrev = fLastChild;
        fLastChild = child;
    }
}

void YNode::addChild(ref<YNode> child) {
    child->fParent.init(this);
    if (fLastChild == null) {
        fFirstChild = child;
        fLastChild = child;
    } else {
        fLastChild->fNext = child;
        child->fPrev = fLastChild;
        fLastChild = child;
    }
}

ref<YElement> YDocument::createElement(ustring element, ustring ns) {
    ref<YElement> e;
    e.init(new YElement(element, ns));
    return e;
}

ref<YElement> YElement::toElement(ustring name) {
    if (fName != null && fName.equals(name))
        return ref<YElement>(this);
    return null;
}

ref<YAttribute> YDocument::createAttribute(ustring element, ustring ns) {
    ref<YAttribute> a;
    a.init(new YAttribute(element, ns, null));
    return a;
}

ref<YAttribute> YDocument::createAttribute(ustring element) {
    ref<YAttribute> a;
    a.init(new YAttribute(element, null, null));
    return a;
}

ref<YText> YDocument::createText(ustring text) {
    ref<YText> t;
    t.init(new YText(text));
    return t;
}

#if 0
void YElement::addChild(ref<YNode> child) {
    if (fLastChild == null) {
        fFirstChild = child;
        fLastChild = child;
    } else {
        fLastChild->fNext = child;
        child->fPrev = fLastChild;
        fLastChild = child;
    }
}
#endif

ustring YElement::getAttribute(ustring name) {
    ref<YAttribute> a = fFirstAttribute;
    while (a != null) {
        if (a->name().equals(name))
            return a->getValue();
        a = a->next();
    }
    return null;
}

void YElement::setAttribute(ustring name, ustring ns, ustring value) {
    ref<YAttribute> attr(new YAttribute(name, ns, value));

    attr->fParent.init(this);
    if (fLastAttribute == null) {
        fFirstAttribute = fLastAttribute = attr;
    } else {
        fLastAttribute->fNext = attr;
        attr->fPrev = fLastAttribute;
        fLastAttribute = attr;
    }
}

#if 0
void YElement::setAttribute(ustring name, ustring value) {
}
#endif

static bool isWhitespaceChar(char c) {
    return
        c == ' ' ||
        c == '\t' ||
        c == '\n' /*||
        c == '\r' ||
        c == '\f'*/;
}

static bool isElementChar(char c) {
    return
        c >= 'a' && c <= 'z' ||
        c >= 'A' && c <= 'Z' ||
        c >= '0' && c <= '9' ||
        c == '_' ||
        c == '-' ||
        c == '.' ||
        c == ':';
}

static void skipWhitespace(char *&p, char *e, YParseResult &res) {
    while (p < e && isWhitespaceChar(*p)) {
        if (*p == '\n') {
            res.row++;
            res.col = 0;
        } else {
            res.col++;
        }
        p++;
    }
}

static ustring getName(char *&p, char *e, YParseResult &res) {
    char *b = p;

    while (p < e && isElementChar(*p)) {
        p++;
        res.col++;
    }
    ustring txt(b, p - b);

    skipWhitespace(p, e, res);
    return txt;
}

static ustring getString(char *&p, char *e, YParseResult &res) {
    char c = *p++;
    res.col++;

    if (c != '\'' && c != '"')
        return null;

    char *b = p;
    char *d = p;

    while (p < e && *p != c) {
        if (*p == '\\') {
            p++;
            res.col++;
            if (!(p < e))
                return null;
        }
        if (*p == '\n') {
            res.row++;
            res.col = 0;
        } else {
            res.col++;
        }
        *d++ = *p++;
    }
    if (p < e && *p == c) {
        p++;
        res.col++;
    } else
        return null;
    skipWhitespace(p, e, res);

    ustring s(b, d - b);
    return s;
}

ref<YDocument> YDocument::parse(char *buf, int len, YParseResult &res) {
    res.col = 1;
    res.row = 1;

    ref<YDocument> doc(new YDocument());
    char *p = buf;
    char *e = buf + len;
    ref<YNode> root = null;
    if (doc == null)
        return null;
    while (p < e) {
        if (*p == '\n') {
            p++;
            res.col = 1;
            res.row++;
        } else if (isWhitespaceChar(*p)) {
            p++; res.col++;
        } else if (*p == '}') {
            p++; res.col++;
            root = root->parent();
        } else if (*p == '\'' || *p == '"') {
            ustring s = getString(p, e, res);

            if (p < e && *p == ';') {
                p++;
                res.col++;
            } else
                return null;
            if (s == null)
                return null;
            ref<YText> text(new YText(s));
            if (text == null)
                return null;
            if (root != null)
                root->addChild(text);
            else
                doc->addChild(text);
        } else if (isElementChar(*p)) {
            ustring txt = getName(p, e, res);
            ustring ns(null);

            int colonPos = txt.indexOf(':');
            if (colonPos != -1) {
                ns = txt.substring(0, colonPos);
                txt = txt.substring(colonPos + 1);
            }

            if (txt.length() == 0)
                return null;
            if (ns != null && ns.length() == 0)
                return null;

            ref<YElement> element(new YElement(txt, ns));

            if (root != null)
                root->addChild(element);
            else
                doc->addChild(element);

            while (p < e && isElementChar(*p)) {
                ustring txt = getName(p, e, res);
                ustring ns(null);

                int colonPos = txt.indexOf(':');
                if (colonPos != -1) {
                    ns = txt.substring(0, colonPos);
                    txt = txt.substring(colonPos + 1);
                }

                if (p < e && *p == '=') {
                    p++;
                    res.col++;
                } else
                    return null;

                if (txt.length() == 0)
                    return null;
                if (ns != null && ns.length() == 0)
                    return null;

                ustring value = getString(p, e, res);
                if (value == null)
                    return null;

                element->setAttribute(txt, ns, value);
            }

            if (p < e && *p == ';') {
                p++;
                res.col++;
            } else if (p < e && *p == '{') {
                p++;
                res.col++;

                root = element;
            } else
                return null;
        } else
            return null;
    }
    if (root != null)
        return null;
    return doc;
}

int YDocument::writeRaw(const char *s, int l, void *t, int (*writer)(void *t, const char *buf, int len), int *len) {
    int rc = writer(t, s, l);
    if (rc == l)
        *len += rc;
    else
        return -1;
    return 0;
}

int YDocument::writeEscaped(const char *s, int l, void *t, int (*writer)(void *t, const char *buf, int len), int *len) {
    int rc;

    for (int i = 0; i < l; s++, i++) {
        if (*s == '"' /*|| *s == '\''*/ || *s == '\\') {
            rc = writeRaw("\\", 1, t, writer, len);
            if (rc < 0)
                return rc;
        }
        rc = writeRaw(s, 1, t, writer, len);
        if (rc < 0)
            return rc;
    }
    return 0;
}

int YDocument::writeAttributes(ref<YAttribute> node, void *t, int (*writer)(void *t, const char *buf, int len), int *len) {
    int rc;
    while (node != null) {
        rc = writeRaw(" ", 1, t, writer, len);
        if (rc < 0)
            return rc;

        cstring name(node->name());
        rc = writeRaw(name.c_str(), name.c_str_len(), t, writer, len);
        if (rc < 0)
            return rc;

        rc = writeRaw("=\"", 2, t, writer, len);
        if (rc < 0)
            return rc;

        cstring value(node->getValue());
        rc = writeEscaped(value.c_str(), value.c_str_len(), t, writer, len);

        rc = writeRaw("\"", 2, t, writer, len);
        if (rc < 0)
            return rc;

        node = node->next();
    }
    return 0;
}

int YDocument::writeNodes(ref<YNode> node, void *t, int (*writer)(void *t, const char *buf, int len), int *len) {
    int rc;

    while (node != null) {
        ref<YElement> e = node->toElement();
        if (e != null) {
            if (e->firstChild() == null) {
                cstring name(e->name());
                rc = writeRaw(name.c_str(), name.c_str_len(), t, writer, len);
                if (rc < 0)
                    return rc;
                rc = writeAttributes(e->firstAttribute(), t, writer, len);
                if (rc < 0)
                    return rc;
                rc = writeRaw(";\n", 2, t, writer, len);
                if (rc < 0)
                    return rc;
            } else {
                cstring name(e->name());
                rc = writeRaw(name.c_str(), name.c_str_len(), t, writer, len);
                if (rc < 0)
                    return rc;
                rc = writeAttributes(e->firstAttribute(), t, writer, len);
                if (rc < 0)
                    return rc;
                rc = writeRaw(" {\n", 3, t, writer, len);
                if (rc < 0)
                    return rc;

                writeNodes(e->firstChild(), t, writer, len);
                rc = writeRaw("}\n", 2, t, writer, len);
                if (rc < 0)
                    return rc;
            }
        } else {
            ref<YText> n = node->toText();
            if (n != null) {
                rc = writeRaw("\"", 1, t, writer, len);
                if (rc < 0)
                    return rc;

                cstring text(n->text());
                rc = writeEscaped(text.c_str(), text.c_str_len(), t, writer, len);

                rc = writeRaw("\";\n", 3, t, writer, len);
                if (rc < 0)
                    return rc;
            }
        }
        node = node->next();
    }
    return 0;
}

int YDocument::write(void *t, int (*writer)(void *t, const char *buf, int len), int *len) {
    *len = 0;

    int rc = writeNodes(firstChild(), t, writer, len);
    return rc;
}

ref<YDocument> YDocument::loadFile(mstring filename) {
    cstring cs(filename);
    int fd = open(cs.c_str(), O_RDONLY | O_TEXT);

    if (fd == -1)
        return null;

    struct stat sb;

    if (fstat(fd, &sb) == -1)
        return null;

    int len = sb.st_size;

    char *buf = new char[len + 1];
    if (buf == 0)
        return null;

    if ((len = read(fd, buf, len)) < 0) {
        delete[] buf;
        return null;
    }

    buf[len] = 0;
    close(fd);

    YParseResult res;
    ref<YDocument> doc = YDocument::parse(buf, len, res);
    if (doc == null) {
        msg("parse error at %s:%d:%d\n",
            cstring(filename).c_str(),
            res.row, res.col);
    }

    delete[] buf;
    return doc;
}
