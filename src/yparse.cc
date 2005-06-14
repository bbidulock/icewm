#include "config.h"
#include "yparse.h"

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
