#ifndef __YPARSE_H
#define __YPARSE_H

#include "mstring.h"

class YNode;
class YDocument;
class YElement;
class YAttribute;
class YText;

struct YParseResult {
    int row;
    int col;
};

class YDocument: public refcounted {
public:
    YDocument();
    ~YDocument();

    ref<YElement> createElement(ustring element, ustring ns);
    ref<YAttribute> createAttribute(ustring attribute, ustring ns);
    ref<YAttribute> createAttribute(ustring attribute);
    ref<YText> createText(ustring text);

    void addChild(ref<YNode> child);
    void removeChild(ref<YNode> child);
    ref<YNode> firstChild() { return fFirstChild; }
    ref<YNode> lastChild() { return fLastChild; }

    static ref<YDocument> parse(char *buf, int len, YParseResult &res);
    static ref<YDocument> loadFile(mstring filename);
    int write(void *t, int (*writer)(void *t, const char *buf, int len), int *len);
private:
    ref<YNode> fFirstChild;
    ref<YNode> fLastChild;

    void freeNode(ref<YNode> &n);
    void freeNodes(ref<YNode> n);

    int writeRaw(const char *s, int l, void *t, int (*writer)(void *t, const char *buf, int len), int *len);
    int writeEscaped(const char *s, int l, void *t, int (*writer)(void *t, const char *buf, int len), int *len);
    int writeAttributes(ref<YAttribute> node, void *t, int (*writer)(void *t, const char *buf, int len), int *len);
    int writeNodes(ref<YNode> node, void *t, int (*writer)(void *t, const char *buf, int len), int *len);
};

class YNode: public refcounted {
public:
    virtual ~YNode();

    enum NodeType {
        Document,
        Element,
        Attribute,
        Text
    };
    NodeType getType() { return fNodeType; }

    virtual ref<YDocument> toDocument() { return null; }
    virtual ref<YElement> toElement() { return null; }
    virtual ref<YElement> toElement(ustring) { return null; }
    virtual ref<YAttribute> toAttribute() { return null; }
    virtual ref<YText> toText() { return null; }

    void addChild(ref<YNode> child);
    void removeChild(ref<YNode> child);
    ref<YNode> firstChild() { return fFirstChild; }
    ref<YNode> lastChild() { return fLastChild; }
    ref<YNode> parent() { return fParent; }

    ref<YNode> next() { return fNext; }
    ref<YNode> prev() { return fPrev; }
private:
    friend class YDocument;
    friend class YElement;
    friend class YAttribute;
    friend class YText;

    YNode(NodeType nodeType): fNodeType(nodeType) {}

    NodeType fNodeType;

    ref<YNode> fParent;
    ref<YNode> fNext;
    ref<YNode> fPrev;
    ref<YNode> fFirstChild;
    ref<YNode> fLastChild;
};

class YElement: public YNode {
    friend class YDocument;
    friend class YAttribute;

    YElement(ustring name, ustring ns): YNode(Element), fName(name), fNamespace(ns) {}
public:
    ustring getAttribute(ustring name);
    void setAttribute(ustring name, ustring ns, ustring value);
//    void setAttribute(ustring name, ustring value);
//    void addAttribute(ref<YAttribute> attr);
    void removeAttribute(ustring name, ustring ns);

    virtual ref<YElement> toElement(ustring name);
    virtual ref<YElement> toElement() { return ref<YElement>(this); }

    ustring getValue();

    ustring name() { return fName; }
    ref<YNode> firstAttribute() { return fFirstAttribute; }

private:
    ustring fName;
    ustring fNamespace;

    ref<YAttribute> fFirstAttribute;
    ref<YAttribute> fLastAttribute;
};

class YAttribute: public YNode {
    friend class YDocument;
    friend class YElement;

    YAttribute(ustring name, ustring ns, ustring value): YNode(Attribute), fName(name), fNamespace(ns), fValue(value) {};
public:
    void setValue(ustring value) { fValue = value; }
    ustring getValue() { return fValue; }

    ustring name() { return fName; }

    virtual ref<YAttribute> toAttribute() { return ref<YAttribute>(this); }

private:
    ustring fName;
    ustring fNamespace;
    ustring fValue;
};

class YText: public YNode {
public:
    YText(): YNode(Text), fText(null) {};
    YText(ustring text): YNode(Text), fText(text) {};

    virtual ref<YText> toText() { return ref<YText>(this); }

    ustring text() { return fText; }
private:
    ustring fText;
};

#endif
