#include "config.h"
#include <X11/Xatom.h>
#include "ylistbox.h"
#include "ymenu.h"
#include "yxapp.h"
#include "sysdep.h"
#include "yaction.h"
#include "yinputline.h"
#include "ymenuitem.h"
#include "ylocale.h"
#include "yrect.h"
#include "ascii.h"
#include "intl.h"
#include "ykey.h"
#include "yfontname.h"
#include "yfontbase.h"
#include <strings.h>
#include <X11/Xutil.h>

#define ICEWM_SITE      "https://ice-wm.org/"
#define ICEWM_FAQ       "https://ice-wm.org/FAQ/"
#define THEME_HOWTO     "https://ice-wm.org/themes/"
#define ICEGIT_SITE     "https://github.com/bbidulock/icewm/"
#define ICEWM_1         DOCDIR "/icewm.1.html"
#define ICEWMBG_1       DOCDIR "/icewmbg.1.html"
#define ICESOUND_1      DOCDIR "/icesound.1.html"
#define BACKGROUND      "rgb:E0/E0/E0"
#define FOREGROUND      "rgb:00/00/00"
#define LINK_COLOR      "rgb:06/45/AD"
#define RULE_COLOR      "rgb:80/80/80"

#ifdef DEBUG
#define DUMP
//#define TEXT
#endif

#define SPACE(c)  ASCII::isWhiteSpace(c)

enum ViewerDimensions {
    ViewerWidth      = 700,
    ViewerHeight     = 700,
    ViewerLeftMargin =  20,
    ViewerTopMargin  =  10,
};

char const * ApplicationName = "icehelp";

static bool verbose, complain, ignoreClose, noFreeType, reverseVideo;

class cbuffer {
    size_t cap, ins;
    char *ptr;
public:
    cbuffer() : cap(0), ins(0), ptr(nullptr) {
    }
    cbuffer(const char *s) : cap(1 + strlen(s)), ins(cap - 1),
        ptr((char *)malloc(cap))
    {
        memcpy(ptr, s, cap);
    }
    cbuffer(const cbuffer& c) :
        cap(c.cap), ins(c.ins), ptr(nullptr) {
        if (c.ptr) {
            ptr = (char *)malloc(cap);
            memcpy(ptr, c.ptr, cap);
        }
    }
    cbuffer& operator=(const char *c) {
        if (c != ptr) {
            if (c) {
                ins = strlen(c);
                cap = 1 + ins;
                ptr = (char *)realloc(ptr, cap);
                memcpy(ptr, c, cap);
            } else if (ptr) {
                cap = ins = 0;
                free(ptr); ptr = nullptr;
            }
        }
        return *this;
    }
    cbuffer& operator=(const cbuffer& c) {
        if (&c != this) {
            if (c.ptr) {
                ins = c.ins;
                cap = 1 + ins;
                ptr = (char *)realloc(ptr, cap);
                memcpy(ptr, c.ptr, cap);
            } else if (ptr) {
                cap = ins = 0;
                free(ptr); ptr = nullptr;
            }
        }
        return *this;
    }
    cbuffer& operator+=(const char *c) {
        if (c) {
            size_t siz = 1 + strlen(c);
            if (cap < ins + siz) {
                cap = ins + siz;
                ptr = (char *)realloc(ptr, cap);
            }
            memcpy(ptr + ins, c, siz);
            ins += siz - 1;
        }
        return *this;
    }
    int len() const {
        return (int) ins;
    }
    void push(int c) {
        if (cap < 2 + ins) {
            cap = 8 + 3 * cap / 2;
            ptr = (char *)realloc(ptr, cap);
        }
        ptr[ins+1] = 0;
        ptr[ins++] = (char) c;
    }
    int pop() {
        int result = 0;
        if (ins > 0) {
            result = ptr[--ins];
            ptr[ins] = 0;
        }
        return result;
    }
    char *peek() {
        return ptr;
    }
    char *release() {
        char *result = ptr;
        cap = ins = 0;
        ptr = nullptr;
        return result;
    }
    ~cbuffer() {
        if (ptr) free(ptr);
    }
    operator const char *() const {
        return ptr;
    }
    int last() const {
        return ins > 0 ? ptr[ins-1] : 0;
    }
    bool isEmpty() const {
        return ins == 0;
    }
    bool nonempty() const {
        return ins > 0;
    }
};

class lowbuffer : public cbuffer {
public:
    void push(int c) {
        cbuffer::push(ASCII::toLower(c));
    }
    lowbuffer& operator+=(const char *c) {
        if (c) {
            while (*c) push(*c++);
        }
        return *this;
    }
};

// singly linked list of nodes which have a 'next' pointer.
template <class T>
class flist {
    flist(const flist&);
    flist& operator=(const flist&);
    T *head, *tail;
public:
    flist() : head(nullptr), tail(nullptr) {}
    flist(T *t) : head(t), tail(t) { t->next = nullptr; }
    void add(T *t) {
        t->next = nullptr;
        if (head) {
            tail = tail->next = t;
        } else {
            tail = head = t;
        }
    }
    T *first() const { return head; }
    T *last() const { return tail; }
    void destroy() {
        for (; head; head = tail) {
            tail = head->next;
            delete head;
        }
    }
    bool nonempty() const { return head != 0; }
    operator T *() const { return head; }
};
// like flist, but delete all nodes when done.
template <class T>
class nlist : public flist<T> {
    typedef flist<T> super;
public:
    nlist() : super() {}
    nlist(T *t) : super(t) {}
    ~nlist() { super::destroy(); }
};

class text_node {
public:
    text_node(const char *t, int l, int f, int _x, int _y, int _w, int _h) :
        text(t), len(l), fl(f), x(_x), y(_y), tw(_w), th(_h), next(nullptr)
    {
    }

    const char *text;
    int len, fl;
    int x, y;
    int tw, th;
    text_node *next;
};

class attr {
public:
    enum attr_type {
        noattr = 0,
        href = 1,
        id = 2,
        name = 4,
        rel = 8,
        maxattr = rel
    } atype;
    mstring value;
    attr *next;
    attr() : atype(noattr), value(null), next(nullptr) {}
    attr(attr_type t, const mstring& s) : atype(t), value(s), next(nullptr) {}
    attr(const attr& a) : atype(a.atype), value(a.value), next(nullptr) {}
    attr& operator=(const attr& a) {
        if (&a != this) {
            atype = a.atype;
            value = a.value;
        }
        return *this;
    }
    operator bool() const { return atype > noattr; }
    static attr_type get_type(const char *s) {
        if (0 == strcmp(s, "href")) return href;
        if (0 == strcmp(s, "id")) return id;
        if (0 == strcmp(s, "name")) return name;
        if (0 == strcmp(s, "rel")) return rel;
        return noattr;
    }
};

class attr_list {
private:
    attr_list(const attr_list&);
    attr_list& operator=(const attr_list&);
    nlist<attr> list;
public:
    attr_list() : list() {}
    void add(attr::attr_type typ, const mstring& s) {
        if (typ > 0) {
            list.add(new attr(typ, s));
        }
    }
    bool get(int mask, attr *found) const {
        for (attr *a = list.first(); a; a = a->next) {
            if ((mask & a->atype) != 0) {
                *found = *a;
                return true;
            }
        }
        return false;
    }
    bool get(int mask, const char *value, attr *found) const {
        for (attr *a = list.first(); a; a = a->next) {
            if ((mask & a->atype) != 0 &&
                0 == strcmp(value, a->value))
            {
                *found = *a;
                return true;
            }
        }
        return false;
    }
};

class node {
public:
    enum node_type {
        unknown,
        html, head, title,
        body,
        text, pre, line, paragraph, hrule,
        h1, h2, h3, h4, h5, h6,
        table, tr, td,
        div,
        ul, ol, li,
        bold, font, italic,
        center,
        script,
        style,
        anchor, img,
        tt, dl, dd, dt,
        thead, tbody, tfoot,
        link, code, meta, form, input,
        section, figure, aside, footer, main,
    };

    node(node_type t) :
        type(t),
        next(nullptr),
        container(nullptr),
        txt(nullptr),
        wrap(),
        xr(0),
        yr(0),
        attrs()
    {
    }
    ~node();

    node_type type;
    node *next;
    node *container;
    const char *txt;
    typedef nlist<text_node> text_list;
    text_list wrap;
    int xr, yr;
    attr_list attrs;

    void add_attribute(attr::attr_type name, const char *value) {
        if (name > attr::noattr) {
            attrs.add(name, value);
        }
    }
    void add_attribute(const char *name, const char *value) {
        add_attribute(attr::get_type(name), value);
    }
    bool get_attribute(int mask, attr *found) const {
        return attrs.get(mask, found);
    }
    bool get_attribute(const char *name, attr *found) const {
        return attrs.get(attr::get_type(name), found);
    }
    node* find_attr(int mask, const char *value);

    static const char *to_string(node_type type);
    static node_type get_type(const char *buf);
};

node::~node() {
    delete next;
    delete container;
    free((void *)txt);
}

node* node::find_attr(int mask, const char *value) {
    for (node *n = this; n; n = n->next) {
        attr got;
        if (n->attrs.get(mask, value, &got)) {
            return n;
        }
        if (n->container) {
            node *found = n->container->find_attr(mask, value);
            if (found) return found;
        }
    }
    return nullptr;
}


#define PRE    0x01
#define PRE1   0x02
#define BOLD   0x04
#define LINK   0x08
#define BIG    0x10
#define ITAL   0x20
#define MONO   0x40

#define sfPar  0x01
#define sfText  0x02

static void dump_tree(int level, node *n);

#define TS(x) case x : return #x

const char *node::to_string(node_type type) {
    switch (type) {
        TS(unknown);
        TS(html);
        TS(head);
        TS(title);
        TS(body);
        TS(text);
        TS(pre);
        TS(line);
        TS(paragraph);
        TS(hrule);
        TS(anchor);
        TS(h1);
        TS(h2);
        TS(h3);
        TS(h4);
        TS(h5);
        TS(h6);
        TS(table);
        TS(tr);
        TS(td);
        TS(div);
        TS(ul);
        TS(ol);
        TS(li);
        TS(bold);
        TS(font);
        TS(italic);
        TS(center);
        TS(script);
        TS(style);
        TS(tt);
        TS(dl);
        TS(dd);
        TS(dt);
        TS(link);
        TS(code);
        TS(meta);
        TS(thead);
        TS(tbody);
        TS(tfoot);
        TS(img);
        TS(form);
        TS(input);
        TS(section);
        TS(figure);
        TS(aside);
        TS(footer);
        TS(main);
    }
    if (complain) {
        tlog("Unknown node_type %d, after %s, before %s", type,
                type > unknown ? to_string((node_type)(type - 1)) : "",
                type < input ? to_string((node_type)(type + 1)) : "");
    }

    return "??";
}

#undef TS
#define TS(x,y) if (0 == strcmp(buf, #x)) return node::y

node::node_type node::get_type(const char *buf)
{
    TS(html, html);
    TS(head, head);
    TS(title, title);
    TS(body, body);
    TS(text, text);
    TS(pre, pre);
    TS(address, pre);
    TS(br, line);
    TS(p, paragraph);
    TS(header, paragraph);
    TS(article, paragraph);
    TS(nav, paragraph);
    TS(hr, hrule);
    TS(a, anchor);
    TS(h1, h1);
    TS(h2, h2);
    TS(h3, h3);
    TS(h4, h4);
    TS(h5, h5);
    TS(h6, h6);
    TS(table, table);
    TS(tr, tr);
    TS(td, td);
    TS(div, div);
    TS(span, div);
    TS(ul, ul);
    TS(ol, ol);
    TS(li, li);
    TS(b, bold);
    TS(strong, bold);
    TS(font, font);
    TS(i, italic);
    TS(em, italic);
    TS(small, italic);
    TS(center, center);
    TS(script, script);
    TS(svg, script);
    TS(button, script);
    TS(path, script);
    TS(style, style);
    TS(tt, tt);
    TS(dl, dl);
    TS(dd, dd);
    TS(dt, dt);
    TS(link, link);
    TS(code, code);
    TS(samp, code);
    TS(kbd, code);
    TS(var, code);
    TS(option, code);
    TS(label, code);
    TS(blockquote, code);
    TS(meta, meta);
    TS(thead, thead);
    TS(tbody, tbody);
    TS(tfoot, tfoot);
    TS(img, img);
    TS(form, form);
    TS(input, input);
    TS(section, section);
    TS(figure, figure);
    TS(aside, aside);
    TS(footer, footer);
    TS(main, main);
    if (*buf && buf[strlen(buf)-1] == '/') {
        cbuffer cbuf(buf);
        cbuf.pop();
        return get_type(cbuf);
    }
    static const char* ignored[] = {
        "abbr",
        "g",
        "g-emoji",
        "include-fragment",
        "rect",
        "relative-time",
        "th",
        "time",
        "time-ago",
    };
    for (unsigned i = 0; i < ACOUNT(ignored); ++i) {
        if (0 == strcmp(buf, ignored[i]))
            return node::unknown;
    }
    if (complain) {
        tlog("unknown tag %s", buf);
    }
    return node::unknown;
}

static int ignore_comments(FILE *fp) {
    int c = getc(fp);
    if (c == '-') {
        c = getc(fp);
        if (c == '-') {
            // comment
            int n = 0;
            while ((c = getc(fp)) != EOF) {
                if (c == '>' && n >= 2) {
                    break;
                }
                n = (c == '-') ? (n + 1) : 0;
            }
        }
    }
    else if (c == '[' && (c = getc(fp)) == 'C') {
        /* skip over <![CDATA[...]]> */
        int n = 0;
        while ((c = getc(fp)) != EOF) {
            if (c == '>' && n >= 2) {
                break;
            }
            n = (c == ']') ? (n + 1) : 0;
        }
    }
    while (c != EOF && c != '>') {
        c = getc(fp);
    }
    return c;
}

static int non_space(int c, FILE *fp) {
    while (c != EOF && ASCII::isWhiteSpace(c)) {
        c = getc(fp);
    }
    return c;
}

static int getc_non_space(FILE *fp) {
    return non_space(getc(fp), fp);
}

static node *parse(FILE *fp, int flags, node *parent, node *&nextsub, node::node_type &close) {
    flist<node> nodes;

    for (int c = getc(fp); c != EOF; c = (c == '<') ? c : getc(fp)) {
        if (c == '<') {
            c = getc(fp);
            if (c == '!') {
                c = ignore_comments(fp);
            } else if (c == '/') {
                // ignore </BR> </P> </LI> ...
                lowbuffer buf;

                c = getc_non_space(fp);
                while (c != EOF && !SPACE(c) && c != '>') {
                    buf.push(c);
                    c = getc(fp);
                }

                node::node_type type = node::get_type(buf);
                if (type == node::paragraph ||
                    type == node::line ||
                    type == node::hrule ||
                    type == node::link ||
                    type == node::unknown ||
                    (type == node::form && (!parent || parent->type != type)) ||
                    type == node::meta)
                {
                }
                else if (parent) {
                    close = type;
                    break;
                }
            } else {
                lowbuffer buf;

                c = non_space(c, fp);
                while (c != EOF && !SPACE(c) && c != '>') {
                    buf.push(c);
                    c = getc(fp);
                }

                node::node_type type = buf.nonempty()
                                     ? node::get_type(buf) : node::unknown;
                node *n = new node(type);
                if (n == nullptr)
                    break;
                while ((c = non_space(c, fp)) != '>' && c != EOF) {
                    lowbuffer abuf;

                    while (c != EOF && !SPACE(c) && c != '=' && c != '>') {
                        abuf.push(c);
                        c = getc(fp);
                    }
                    c = non_space(c, fp);
                    if (c == '=') {
                        cbuffer vbuf;
                        c = getc_non_space(fp);
                        if (c == '"') {
                            while ((c = getc(fp)) != EOF && c != '"') {
                                vbuf.push(c);
                            }
                        } else {
                            while (c != EOF && !SPACE(c) && c != '>') {
                                vbuf.push(c);
                                c = getc(fp);
                            }
                        }
                        if (abuf.nonempty())
                            n->add_attribute(abuf, vbuf);
                    }
                }

                if (type == node::line ||
                    type == node::hrule ||
                    type == node::paragraph||
                    type == node::link ||
                    type == node::meta)
                {
                    nodes.add(n);
                } else {
                    node *container = nullptr;

                    if (type == node::li ||
                        type == node::dt ||
                        type == node::dd ||
                        type == node::paragraph ||
                        type == node::line
                       )
                    {
                        if (parent &&
                            (parent->type == type ||
                             (type == node::dd && parent->type == node::dt) ||
                             (type == node::dt && parent->type == node::dd))
                           )
                        {
                            nextsub = n;
                            break;
                        }
                    }

                    node *nextsub;
                    node::node_type close_type;
                    do {
                        nextsub = nullptr;
                        close_type = node::unknown;
                        int fl = flags;
                        if (type == node::pre) fl |= PRE | PRE1;
                        container = parse(fp, fl, n, nextsub, close_type);
                        if (container)
                            n->container = container;
                        nodes.add(n);

                        if (nextsub) {
                            n = nextsub;
                        }
                        if (close_type != node::unknown) {
                            if (n->type != close_type)
                                return nodes;
                        }
                    } while (nextsub != nullptr);
                }
            }
        }
        else if (c != '<') {
            cbuffer buf;

            do {
                if (c == '&') {
                    lowbuffer entity;

                    do {
                        entity.push(c);
                        c = getc(fp);
                    } while (ASCII::isAlnum(c) || c == '#');
                    if (c != ';')
                        ungetc(c, fp);
                    if (strcmp(entity, "&amp") == 0)
                        c = '&';
                    else if (strcmp(entity, "&lt") == 0
                          || strcmp(entity, "&#062") == 0)
                        c = '<';
                    else if (strcmp(entity, "&gt") == 0
                          || strcmp(entity, "&#060") == 0)
                        c = '>';
                    else if (strcmp(entity, "&le") == 0
                          || strcmp(entity, "&#8804") == 0) {
                        buf += "<=";
                        continue;
                    }
                    else if (strcmp(entity, "&ge") == 0
                          || strcmp(entity, "&#8805") == 0) {
                        buf += ">=";
                        continue;
                    }
                    else if (strcmp(entity, "&ne") == 0) {
                        buf += "!=";
                        continue;
                    }
                    else if (strcmp(entity, "&quot") == 0)
                        c = '"';
                    else if (strcmp(entity, "&nbsp") == 0)
                        c = ' ';    // 32+128;
                    else if (strcmp(entity, "&#160") == 0)
                        c = ' ';    // 32+128;
                    else if (strcmp(entity, "&#8203") == 0)
                        c = ' ';    // 32+128;  zero width space
                    else if (strcmp(entity, "&#8211") == 0
                          || strcmp(entity, "&ndash") == 0)
                        c = '-';    // en dash
                    else if (strcmp(entity, "&#8212") == 0
                          || strcmp(entity, "&mdash") == 0)
                        c = '-';    // em dash
                    else if (strcmp(entity, "&#8216") == 0
                          || strcmp(entity, "&lsquo") == 0)
                        c = '`';   // left single quote
                    else if (strcmp(entity, "&#8217") == 0
                          || strcmp(entity, "&rsquo") == 0)
                        c = '\'';   // right single quote
                    else if (strcmp(entity, "&#8220") == 0
                          || strcmp(entity, "&ldquo") == 0) {
                        buf += "``"; // left double quotes
                        continue;
                    }
                    else if (strcmp(entity, "&#8221") == 0
                          || strcmp(entity, "&rdquo") == 0) {
                        buf += "''"; // right double quotes
                        continue;
                    }
                    else if (strcmp(entity, "&#8226") == 0
                          || strcmp(entity, "&middot") == 0)
                        c = '*';   // bullet
                    else if (strcmp(entity, "&#8230") == 0
                          || strcmp(entity, "&hellip") == 0) {
                        buf += "..."; // horizontal ellipsis
                        continue;
                    }
                    else if (strcmp(entity, "&#8594") == 0) {
                        buf += "->"; // rightwards arrow
                        continue;
                    }
                    else if (strcmp(entity, "&copy") == 0) {
                        buf += "(c)"; // copyright symbol
                        continue;
                    }
                    else if (strcmp(entity, "&reg") == 0) {
                        buf += "(R)"; // registered sign
                        continue;
                    }
                    else if (entity.len() > 2 && !strcmp(entity + 2, "tilde")) {
                        c = entity[1];
                    }
                    else if (entity.len() > 2 && !strcmp(entity + 2, "acute")) {
                        c = entity[1];
                    }
                    else if (entity.len() > 2 && !strcmp(entity + 2, "uml")) {
                        c = entity[1];
                    }
                    else {
                        unsigned special = 0;
                        int len = 0;
                        if (1 == sscanf(entity, "&#%u%n", &special, &len)
                                && len == entity.len()
                                && inrange(special, 32U, 126U))
                        {
                            c = (char) special;
                        }
                        else {
                            if (complain) {
                                tlog("unknown special '%s'", entity.peek());
                            }
                            c = ' ';
                        }
                    }
                }
                if (c == '\r') {
                    c = getc(fp);
                    if (c != '\n')
                        ungetc(c, fp);
                    c = '\n';
                }
                if (!(flags & PRE)) {
                    if (SPACE(c))
                        c = ' ';
                }
                if ((flags & PRE1) && c == '\n')
                    ;
                else if (c != ' ' || (flags & PRE) ||
                        buf.isEmpty() || buf.last() != ' ')
                {
                    buf.push(c);
                }
                flags &= ~PRE1;
            } while ((c = getc(fp)) != EOF && c != '<');

            if (c == '<') {
                int k = getc(fp);
                if (k == '/') {
                    if (SPACE(buf.last()))
                        buf.pop();
                }
                ungetc(k, fp);
            }

            if (buf.nonempty()) {
                node *n = new node(node::text);
                n->txt = buf.release();
                nodes.add(n);
            }
        }
    }
    return nodes;
}

class History {
private:
    MStringArray array;
    int where;
public:
    History() : where(-1) { }
    bool empty() const { return array.isEmpty(); }
    int size() const { return array.getCount(); }
    const char* get(int i) const { return array[i]; }
    void push(const mstring& s) {
        if (where == -1 || (s.nonempty() && s != get(where))) {
            for (int k = size() - 1; k > where; --k) {
                array.remove(k);
            }
            array.insert(++where, s);
        }
    }
    mstring& current() {
        if (empty()) array.insert(++where, null);
        return array[where];
    }
    bool hasLeft() const { return where > 0; }
    bool left() {
        return hasLeft() ? (--where, true) : false;
    }
    bool hasRight() const { return where + 1 < size(); }
    bool right() {
        return hasRight() ? (++where, true) : false;
    }
    bool hasFirst() const { return 0 < size(); }
    bool first() {
        return hasFirst() ? (where = 0, true) : false;
    }
};

class FontEntry {
public:
    int size;
    int flag;
    const char *core;
    const char *xeft;
    YFont font;
};
class FontTable {
public:
    static FontEntry table[];
    static YFont get(int size, int flags);
    static void reset() {
        for (int k = 0; table[k].size; ++k) {
            table[k].font = null;
        }
    }
};
FontEntry FontTable::table[] = {
    { 12, 0,
        "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1",
        "Snap:size=10,sans-serif:size=10" },
    { 14, 0,
        "-adobe-helvetica-medium-r-normal--14-140-75-75-p-77-iso8859-1",
        "Snap:size=12,sans-serif:size=12" },
    { 18, 0,
        "-adobe-helvetica-medium-r-normal--18-180-75-75-p-98-iso8859-1",
        "Snap:size=14,sans-serif:size=14" },
    { 12, BOLD,
        "-adobe-helvetica-bold-r-normal--12-120-75-75-p-70-iso8859-1",
        "Snap:size=10:bold,sans-serif:size=10:bold" },
    { 14, BOLD,
        "-adobe-helvetica-bold-r-normal--14-140-75-75-p-82-iso8859-1",
        "Snap:size=12:bold,sans-serif:size=12:bold" },
    { 18, BOLD,
        "-adobe-helvetica-bold-r-normal--18-180-75-75-p-103-iso8859-1",
        "Snap:size=14:bold,sans-serif:size=14:bold" },
    { 12, MONO,
        "-adobe-courier-medium-r-normal--12-120-75-75-m-70-iso8859-1",
        "courier:size=10,monospace:size=10" },
    { 17, MONO | BOLD,
        "-adobe-courier-bold-r-normal--17-120-100-100-m-100-iso8859-1",
        "courier:size=14,monospace:size=14" },
    { 12, ITAL,
        "-adobe-helvetica-medium-o-normal--12-120-75-75-p-67-iso8859-1",
        "sans-serif:size=10:oblique" },
    { 14, ITAL,
        "-adobe-helvetica-medium-o-normal--14-140-75-75-p-78-iso8859-1",
        "sans-serif:size=12:oblique" },
    { 0, 0, nullptr, nullptr },
};
YFont FontTable::get(int size, int flags) {
    int best = 0;
    int diff = abs(size - table[best].size);
    for (int i = 1; table[i].size > 0; ++i) {
        int delt = abs(size - table[i].size);
        if (delt < diff) {
            best = i;
            diff = delt;
        }
        else if (diff == delt) {
            int bflag = table[best].flag;
            int iflag = table[i].flag;
            if ((bflag & BOLD) != (iflag & BOLD)) {
                if ((iflag & BOLD) == (flags & BOLD)) {
                    best = i;
                }
            }
            else if ((bflag & MONO) != (iflag & MONO)) {
                if ((iflag & MONO) == (flags & MONO)) {
                    best = i;
                }
            }
            else if ((bflag & ITAL) != (iflag & ITAL)) {
                if ((iflag & ITAL) == (flags & ITAL)) {
                    best = i;
                }
            }
        }
    }
    if (table[best].font == null) {
        const char* xeft = noFreeType ? nullptr : table[best].xeft;
        YFontName fontName(&table[best].core, &xeft);
        table[best].font = fontName;
    }
    return table[best].font;
}

class HTListener {
public:
    virtual void activateURL(mstring url, bool relative = false) = 0;
    virtual void openBrowser(mstring url) = 0;
    virtual void handleClose() = 0;
protected:
    virtual ~HTListener() {}
};

class ActionItem : public YAction {
private:
    YMenuItem   *item;
    ActionItem(const ActionItem&);
    ActionItem& operator=(const ActionItem&);
public:
    ActionItem() : item(nullptr) {}
    ~ActionItem() { }
    void operator=(YMenuItem* menuItem) { item = menuItem; }
    YMenuItem* operator->() { return item; }
};

class HTextView: public YWindow,
    public YScrollBarListener,
    public YScrollable,
    public YActionListener,
    public YInputListener
{
public:
    HTextView(HTListener *fL, YScrollView *v, YWindow *parent);
    ~HTextView();

    void find_link(node *n);

    void setData(node *root) {
        delete fRoot;
        fRoot = root;
        tx = ty = 0;
        layout();

        actionPrev->setEnabled(false);
        actionNext->setEnabled(false);
        actionContents->setEnabled(false);

        nextURL = null;
        prevURL = null;
        contentsURL = null;

        find_link(fRoot);
        if (contentsURL == null) {
            node *n = fRoot->find_attr(attr::id | attr::name, "toc");
            if (n) {
                contentsURL = "#toc";
                actionContents->setEnabled(true);
            }
        }

        actionIndex->setEnabled(history.hasFirst());
        actionLeft->setEnabled(history.hasLeft());
        actionRight->setEnabled(history.hasRight());
    }


    void par(int &state, int &x, int &y, unsigned &h, const int left);
    void epar(int &state, int &x, int &y, unsigned &h, const int left);
    void layout();
    void layout(node *parent, node *n1, int left, int right, int &x, int &y, unsigned &w, unsigned &h, int flags, int &state);
    void draw(Graphics &g, node *n1, bool isHref = false);
    node *find_node(node *n, int x, int y, node *&anchor, node::node_type type);
    void find_fragment(const char *frag);
    bool findNext(node* n1);
    void startSearch();

    virtual void paint(Graphics &g, const YRect &r) {
        g.setColor(bg);
        g.fillRect(r.x(), r.y(), r.width(), r.height());

        if (ty < 20) {
            int sz = 19;
            g.setColor(reverseVideo ? normalFg.darker().reverse() : bg.darker());
            g.fillRect(width() - 20, 20 - ty - sz, sz, sz, 1);
            g.setColor(bg.brighter());
            for (int i = 0; i <= 2; ++i) {
                g.drawLine(width() - 20 +  2, 20 - ty - 15 + i * 5,
                           width() - 20 + 16, 20 - ty - 15 + i * 5);
            }
        }

        g.setColor(normalFg);
        g.setFont(font);

        draw(g, fRoot);
    }
    virtual void handleExpose(const XExposeEvent& expose) {
    }

    bool isFocusTraversable() {
        return true;
    }

    virtual void repaint() {
        graphicsBuffer.paint();
    }

    virtual void configure(const YRect2& r) {
        if (r.resized()) {
            resetScroll();
            repaint();
        }
    }

    void resetScroll() {
        fVerticalScroll->setValues(ty, height(), 0, contentHeight());
        fVerticalScroll->setBlockIncrement(max(40U, height()) / 2);
        fVerticalScroll->setUnitIncrement(font->height());
        fHorizontalScroll->setValues(tx, width(), 0, contentWidth());
        fHorizontalScroll->setBlockIncrement(max(40U, width()) / 2);
        fHorizontalScroll->setUnitIncrement(20);
        if (fScrollView)
            fScrollView->layout();
    }

    void setPos(int x, int y) {
        if (x != tx || y != ty) {
            int dx = x - tx;
            int dy = y - ty;

            tx = x;
            ty = y;

            graphicsBuffer.scroll(dx, dy);
        }
    }

    virtual void scroll(YScrollBar *sb, int delta) {
        if (sb == fHorizontalScroll)
            setPos(tx + delta, ty);
        else if (sb == fVerticalScroll)
            setPos(tx, ty + delta);
    }
    virtual void move(YScrollBar *sb, int pos) {
        if (sb == fHorizontalScroll)
            setPos(pos, ty);
        else if (sb == fVerticalScroll)
            setPos(tx, pos);
    }

    unsigned contentWidth() {
        return conWidth;
    }
    unsigned contentHeight() {
        return conHeight;
    }

    virtual void handleClick(const XButtonEvent &up, int /*count*/);

    virtual void actionPerformed(YAction action, unsigned int modifiers = 0) {
        if (action == actionClose) {
            listener->handleClose();
        }
        else if (action == actionBrowser) {
            listener->openBrowser(history.current());
        }
        else if (action == actionReload) {
            listener->activateURL(history.current(), false);
        }
        else if (action == actionNext) {
            if (actionNext->isEnabled())
                listener->activateURL(nextURL, true);
        }
        else if (action == actionPrev) {
            if (actionPrev->isEnabled())
                listener->activateURL(prevURL, true);
        }
        else if (action == actionContents) {
            if (actionContents->isEnabled())
                listener->activateURL(contentsURL, true);
        }
        else if (action == actionIndex) {
            if (actionIndex->isEnabled() && history.first())
                listener->activateURL(history.current());
        }
        else if (action == actionLeft) {
            if (actionLeft->isEnabled() && history.left())
                listener->activateURL(history.current());
        }
        else if (action == actionRight) {
            if (actionRight->isEnabled() && history.right())
                listener->activateURL(history.current());
        }
        else if (action == actionLink[0]) {
            if (actionLink[0]->isEnabled())
                listener->activateURL(ICEWM_1);
        }
        else if (action == actionLink[1]) {
            if (actionLink[1]->isEnabled())
                listener->activateURL(ICEWMBG_1);
        }
        else if (action == actionLink[2]) {
            if (actionLink[2]->isEnabled())
                listener->activateURL(ICESOUND_1);
        }
        else if (action == actionLink[3]) {
            if (actionLink[3]->isEnabled())
                listener->activateURL(ICEWM_FAQ);
        }
        else if (action == actionLink[4]) {
            if (actionLink[4]->isEnabled())
                listener->activateURL(ICEHELPIDX);
        }
        else if (action == actionLink[5]) {
            if (actionLink[5]->isEnabled())
                listener->activateURL(PACKAGE_BUGREPORT);
        }
        else if (action == actionLink[6]) {
            if (actionLink[6]->isEnabled())
                listener->activateURL(THEME_HOWTO);
        }
        else if (action == actionLink[7]) {
            if (actionLink[7]->isEnabled())
                listener->activateURL(ICEWM_SITE);
        }
        else if (action == actionLink[8]) {
            if (actionLink[8]->isEnabled())
                listener->activateURL(ICEGIT_SITE);
        }
#ifdef CONFIG_XFREETYPE
        else if (action == actionFreeType) {
            noFreeType = !noFreeType;
            actionFreeType->setChecked(!noFreeType);
            FontTable::reset();
            actionPerformed(actionReload);
        }
#endif
        else if (action == actionReversed) {
            reverseVideo = !reverseVideo;
            actionReversed->setChecked(reverseVideo);
            if (reverseVideo) {
                bg = FOREGROUND;
                normalFg = BACKGROUND;
                linkFg = LINK_COLOR;
                linkFg.reverse();
            } else {
                bg = BACKGROUND;
                normalFg = FOREGROUND;
                linkFg = LINK_COLOR;
            }
            repaint();
            YScrollBar::reverseVideo();
            if (fHorizontalScroll->visible())
                fHorizontalScroll->repaint();
            if (fVerticalScroll->visible())
                fVerticalScroll->repaint();
        }
        else if (action == actionFind) {
            if (input == nullptr) {
                input = new YInputLine(this, this);
            }
            if (input) {
                input->setBorderWidth(2);
                input->setGeometry(YRect(1, 1, 250, 25));
                input->show();
                xapp->sync();
                setFocus(input);
                input->requestFocus(true);
                input->gotFocus();
            }
        }
        else if (action == actionSearch) {
            startSearch();
        }
    }

    bool handleKey(const XKeyEvent &key);
    void handleButton(const XButtonEvent &button);
    void addHistory(const mstring& path);
private:
    node *fRoot;

    int tx, ty;
    unsigned conWidth;
    unsigned conHeight;

    YFont font;
    int fontFlag, fontSize;
    YColorName bg, normalFg, linkFg, hrFg;

    YScrollView *fScrollView;
    YScrollBar *fVerticalScroll;
    YScrollBar *fHorizontalScroll;

    YMenu *menu;
    ActionItem actionClose;
    ActionItem actionLeft;
    ActionItem actionRight;
    ActionItem actionIndex;
    ActionItem actionFind;
    ActionItem actionSearch;
    ActionItem actionPrev;
    ActionItem actionNext;
    ActionItem actionReload;
    ActionItem actionBrowser;
    ActionItem actionContents;
    ActionItem actionLink[10];
    ActionItem actionFreeType;
    ActionItem actionReversed;
    HTListener *listener;

    mstring search;
    mstring prevURL;
    mstring nextURL;
    mstring contentsURL;
    History history;
    GraphicsBuffer graphicsBuffer;

    void flagFont(int n) {
        int mask = (n & (MONO | BOLD | ITAL)) | ((n & PRE) ? MONO : 0);
        int size = (n & BIG) ? 18 : 14;
        if (mask != fontFlag || size != fontSize) {
            font = FontTable::get(size, mask);
            fontSize = size;
            fontFlag = mask;
        }
    }
    class Flags {
    public:
        HTextView *view;
        const int oldf, newf;
        Flags(HTextView *v, int o, int n) : view(v), oldf(o), newf(n) {
            view->flagFont(newf);
        }
        ~Flags() { view->flagFont(oldf); }
        operator int() const { return newf; }
    };

    void inputReturn(YInputLine* inputline) {
        search = input->getText();
        delete input; input = nullptr;
        startSearch();
    }
    void inputEscape(YInputLine* inputline) {
        delete input; input = nullptr;
    }
    void inputLostFocus(YInputLine* inputline) {
        delete input; input = nullptr;
    }
    YInputLine* input;
};

HTextView::HTextView(HTListener *fL, YScrollView *v, YWindow *parent):
    YWindow(parent), fRoot(nullptr),
    bg(BACKGROUND),
    normalFg(FOREGROUND),
    linkFg(LINK_COLOR),
    hrFg(RULE_COLOR),
    fScrollView(v),
    fVerticalScroll(v->getVerticalScrollBar()),
    fHorizontalScroll(v->getHorizontalScrollBar()),
    listener(fL),
    graphicsBuffer(this),
    input(nullptr)
{
    fScrollView->setListener(this);
    tx = ty = 0;
    conWidth = conHeight = 0;
    fontFlag = 0;
    fontSize = 0;
    flagFont(0);
    setTitle("HTextView");

    menu = new YMenu();
    menu->setActionListener(this);
    actionLeft = menu->addItem(_("Back"), 0, _("Alt+Left"), actionLeft);
    actionLeft->setEnabled(false);
    actionRight = menu->addItem(_("Forward"), 0, _("Alt+Right"), actionRight);
    actionRight->setEnabled(false);
    menu->addSeparator();
    actionPrev = menu->addItem(_("Previous"), 0, null, actionPrev);
    actionNext = menu->addItem(_("Next"), 0, null, actionNext);
    menu->addSeparator();
    actionContents = menu->addItem(_("Contents"), 0, null, actionContents);
    actionIndex = menu->addItem(_("Index"), 0, null, actionIndex);
    actionIndex->setEnabled(false);
    actionFind = menu->addItem(_("Find..."), 0, _("Ctrl+F"), actionFind);
    actionSearch = menu->addItem(_("Find next"), 0, _("F3"), actionSearch);
    menu->addSeparator();
    actionBrowser = menu->addItem(_("Open in Browser"), 0, _("Ctrl+B"),
                                  actionBrowser);
    actionReload = menu->addItem(_("Reload"), 0, _("Ctrl+R"), actionReload);
    actionClose = menu->addItem(_("Close"), 0, _("Ctrl+Q"), actionClose);
    menu->addSeparator();

    int k = -1;
    k++;
    actionLink[k] = menu->addItem(_("Icewm(1)"), 2, _("Ctrl+I"), actionLink[k]);
    k++;
    actionLink[k] = menu->addItem(_("Icewmbg(1)"), 5, null, actionLink[k]);
    k++;
    actionLink[k] = menu->addItem(_("Icesound(1)"), 3, null, actionLink[k]);
    k++;
    actionLink[k] = menu->addItem(_("FAQ"), 0, null, actionLink[k]);
    k++;
    actionLink[k] = menu->addItem(_("Manual"), 0, _("Ctrl+M"), actionLink[k]);
    k++;
    actionLink[k] = menu->addItem(_("Support"), 0, null, actionLink[k]);
    k++;
    actionLink[k] = menu->addItem(_("Theme Howto"), 0, _("Ctrl+T"), actionLink[k]);
    k++;
    actionLink[k] = menu->addItem(_("Website"), 0, _("Ctrl+W"), actionLink[k]);
    k++;
    actionLink[k] = menu->addItem(_("Github"), 0, null, actionLink[k]);

    menu->addSeparator();
#ifdef CONFIG_XFREETYPE
    actionFreeType = menu->addItem("FreeType fonts", -1, _("Ctrl+X"), actionFreeType);
    actionFreeType->setChecked(!noFreeType);
#else
    noFreeType = true;
#endif
    actionReversed = menu->addItem("Reverse video", -1, _("Ctrl+V"), actionReversed);
    actionReversed->setChecked(reverseVideo);
}

HTextView::~HTextView() {
    delete fRoot;
    delete input;
    delete menu;
}

void HTextView::addHistory(const mstring& path) {
    history.push(path);
    actionLeft->setEnabled(history.hasLeft());
    actionRight->setEnabled(history.hasRight());
    actionIndex->setEnabled(history.hasFirst());
}

node *HTextView::find_node(node *n, int x, int y, node *&anchor, node::node_type type) {

    for (; n; n = n->next) {
        if (n->container) {
            node *f = find_node(n->container, x, y, anchor, type);
            if (f) {
                if (anchor == nullptr && n->type == type)
                    anchor = n;
                return f;
            }
        }
        for (text_node *t = n->wrap; t; t = t->next) {
            if (x >= t->x && x < t->x + t->tw &&
                y < t->y && y > t->y - t->th)
                return n;
        }
    }
    return nullptr;
}

void HTextView::find_link(node *n) {
    for (; n; n = n->next) {
        if (n->type == node::link) {
            attr rel, href;
            if (n->get_attribute("rel", &rel) &&
                n->get_attribute("href", &href))
            {
                if (strcasecmp(rel.value, "previous") == 0) {
                    prevURL = href.value;
                    actionPrev->setEnabled(true);
                }
                if (strcasecmp(rel.value, "next") == 0) {
                    nextURL = href.value;
                    actionNext->setEnabled(true);
                }
                if (strcasecmp(rel.value, "contents") == 0) {
                    contentsURL = href.value;
                    actionContents->setEnabled(true);
                }
            }
        }
        if (n->container)
            find_link(n->container);
    }
}

void HTextView::find_fragment(const char *frag) {
    node *n = fRoot->find_attr(attr::id | attr::name, frag);
    if (n) {
        int y = max(0, min(n->yr, (int) contentHeight() - (int) height()));
        setPos(0, y);
        fVerticalScroll->setValue(y);
        fHorizontalScroll->setValue(0);
    }
}

void HTextView::layout() {
    int state = sfPar;
    int x = ViewerLeftMargin, y = ViewerTopMargin;
    int left = x, right = width() - x;
    conWidth = conHeight = 0;
    layout(nullptr, fRoot, left, right, x, y, conWidth, conHeight, 0, state);
    conHeight += font->height();
    resetScroll();
}

void addState(int &state, int value) {
    state |= value;
    //msg("addState=%d %d", state, value);
}

void removeState(int &state, int value) {
    state &= ~value;
    //msg("removeState=%d %d", state, value);
}

void HTextView::par(int &state, int &x, int &y, unsigned &h, const int left) {
    if (!(state & sfPar)) {
        h += font->height();
        x = left;
        y = h;
        addState(state, sfPar);
    }
}

void HTextView::epar(int &state, int &x, int &y, unsigned &h, const int left) {
    if ((x > left) || ((state & (sfText | sfPar)) == sfText)) {
        h += font->height();
        x = left;
        y = h;
        removeState(state, sfText);
    }
    removeState(state, sfPar);
}

void HTextView::layout(
        node *parent, node *n1, int left, int right,
        int &x, int &y, unsigned &w, unsigned &h,
        int flags, int &state)
{
    ///puts("{");
    for (node *n = n1; n; n = n->next) {
        n->xr = x;
        n->yr = y;
        switch (n->type) {
        case node::head:
        case node::meta:
        case node::title:
        case node::script:
        case node::style:
            break;
        case node::h1:
        case node::h2:
        case node::h3:
        case node::h4:
        case node::h5:
        case node::h6:
            if (x > left) {
                x = left;
                y = h + font->height();
            }
            removeState(state, sfPar);
            x = left;
            n->xr = x;
            n->yr = y;
            if (n->container) {
                int h1bold = n->type == node::h1 ? BOLD : 0;
                Flags fl(this, flags, flags | BIG | h1bold);
                layout(n, n->container, n->xr, right, x, y, w, h, fl, state);
            }
            x = left;
            y = h + font->height();
            addState(state, sfText);
            removeState(state, sfPar);
            break;
        case node::text:
            n->wrap.destroy();
            for (const char *b = n->txt, *c; *b; b = c) {
                int wc = 0;

                if (flags & PRE) {
                    c = b;
                    while (*c && *c != '\n')
                        c++;
                    wc = font->textWidth(b, c - b);
                } else {
                    if (x == left)
                        while (SPACE(*b))
                            b++;

                    c = b;

                    do {
                        const char *d = c;

                        if (SPACE(*d))
                            while (SPACE(*d))
                                d++;
                        else
                            while (*d && !SPACE(*d))
                                d++;

                        int w1 = font->textWidth(b, d - b);

                        if (x + w1 < right) {
                            wc = w1;
                            c = d;
                        } else if (x == left && wc == 0) {
                            wc = w1;
                            c = d;
                            break;
                        } else if (wc == 0) {
                            x = left;
                            y = h;
                            while (SPACE(*c))
                                c++;
                            b = c;
                        } else
                            break;
                        ///msg("width=%d %d / %d / %d", wc, x, left, right);
                    } while (*c);
                }

                if (!(flags & PRE) && x == left) {
                    while (SPACE(*b)) b++;
                }
                if ((flags & PRE) || c - b > 0) {
                    par(state, x, y, h, left);
                    addState(state, sfText);

                    n->wrap.add(new text_node(b, c - b, flags,
                                x, y + 12, wc, font->height()));
                    if (y + (int)font->height() > (int)h)
                        h = y + font->height();

                    x += wc;
                    if (x > (int)w)
                        w = x;

                    if ((flags & PRE)) {
                        if (*c == '\n') {
                            c++;
                            x = left;
                            y = h;
                        }
                    }
                }
                //msg("len=%d x=%d left=%d %s", c - b, x, left, b);
            }
            break;
        case node::line:
            //puts("<BR>");
            if (x != left) {
                y = h;
                //h += font->height();
            } else {
                h += font->height();
                y = h;
            }
            x = left;
            break;
        case node::paragraph:
            if (parent) {
                if (parent->type != node::dd && parent->type != node::li) {
                    removeState(state, sfPar);
                }
            }
            //puts("<P>");
            //par(state, x, y, h, left);
            x = left;
            y = h;
            n->xr = x;
            n->yr = y;
            break;
        case node::hrule:
            //puts("<HR>");
            //epar(state, x, y, h, left);
            if ((state & sfText) && !(state & sfPar))
                h += font->height();
            x = left;
            n->xr = x;
            n->yr = h;
            h += 10;
            y = h;
            addState(state, sfPar);
            break;
        case node::center:
            x = left;
            y = h;
            if (n->container) {
                layout(n, n->container, n->xr, right, x, y, w, h, flags, state);
                // !!! center
            }
            h += font->height();
            x = left;
            y = h;
            break;
        case node::anchor:
            if (n->container) {
                layout(n, n->container, left, right, x, y, w, h, flags, state);
            }
            break;
        case node::ul:
        case node::ol:
            //puts("<UL>");
            //epar(state, x, y, h, left);
            if (!(parent && parent->type == node::li)) {
                removeState(state, sfPar);
            }
            y = h;
            x = left;
            n->xr = x;
            n->yr = y;
            x += 40;
            if (n->container) {
                layout(n, n->container, x, right, x, y, w, h, flags, state);
            }
            addState(state, sfText);
            x = left;
            y = h;
            //state &= ~sfPar;
            //par(state, x, y, h, left);
            //x = left;
            //y = h + font->height();
            removeState(state, sfPar);
            addState(state, sfText);
            break;
        case node::dl:
            //puts("<UL>");
            epar(state, x, y, h, left);
            n->xr = x;
            n->yr = y;
            if (parent && parent->type == node::dt)
                x += 40;
            if (n->container) {
                //puts("<DL>\n");
                layout(n, n->container, x, right, x, y, w, h, flags, state);
                //puts("</DL>\n");
            }
            addState(state, sfText);
            x = left;
            y = h;
            removeState(state, sfPar);
            addState(state, sfText);
            //state &= ~sfPar;
            //par(state, x, y, h, left);
            //x = left;
            //y = h + font->height();
            break;
        case node::dt:
            //puts("<LI>");
            //if (state & sfText)
            y = h;
            x = left;
            n->xr = x;
            n->yr = y;

            addState(state, sfPar);
            if (n->container) {
                layout(n, n->container, left, right, x, y, w, h, flags, state);
            }
            y = h;
            x = left;
            //removeState(state, sfPar | sfText);
            break;
        case node::dd:
            //puts("<DT>");
            //if (state & sfText)
            epar(state, x, y, h, left);
            y = h;
            x = left + 40;
            n->xr = x;
            n->yr = y;

            addState(state, sfPar);
            if (n->container) {
                //puts("<DD>\n");
                layout(n, n->container, x, right, x, y, w, h, flags, state);
                //puts("</DD>\n");
                h += font->height();
            }
            y = h;
            x = left;
            //removeState(state, sfPar | sfText);
            break;

        case node::li:
            //puts("<LI>");

            //msg("state=%d", state);
            epar(state, x, y, h, left);
            //if (state & sfText)
            y = h;
            n->xr = x - 12;
            n->yr = y;

            addState(state, sfPar);
            if (n->container) {
                layout(n, n->container, left, right, x, y, w, h, flags, state);
            }
            y = h;
            x = left;
            //removeState(state, sfPar | sfText);
            break;
        case node::pre:
            if (x > left) {
                x = left;
                h += font->height();
                y = h;
            }
            if (n->container) {
                Flags fl(this, flags, flags | PRE);
                layout(n, n->container, x, right, x, y, w, h, fl, state);
            }
            y = h;
            x = left;
            removeState(state, sfPar);
            addState(state, sfText);
            //msg("set=%d", state);
            break;
        case node::bold:
            if (n->container) {
                Flags fl(this, flags, flags | BOLD);
                layout(n, n->container, left, right, x, y, w, h, fl, state);
            }
            break;
        case node::italic:
            if (n->container) {
                Flags fl(this, flags, flags | ITAL);
                layout(n, n->container, left, right, x, y, w, h, fl, state);
            }
            break;
        case node::tt:
        case node::code:
            if (n->container) {
                Flags fl(this, flags, flags | MONO);
                layout(n, n->container, left, right, x, y, w, h, fl, state);
            }
            break;
        case node::html:
        case node::body:
        case node::div:
        case node::table:
        case node::tr:
        case node::td:
        case node::thead:
        case node::tbody:
        case node::tfoot:
        case node::img:
        case node::unknown:
        case node::link:
        case node::form:
        case node::input:
        case node::section:
        case node::figure:
        case node::aside:
        case node::footer:
        case node::main:
            if (n->container) {
                layout(n, n->container, left, right, x, y, w, h, flags, state);
            }
            break;
        default:
            if (complain) {
                tlog("default layout for node type %s", node::to_string(n->type));
            }
            if (n->container)
                layout(n, n->container, left, right, x, y, w, h, flags, state);
            break;
        }
    }
    ///puts("}");
}

void HTextView::draw(Graphics &g, node *n1, bool isHref) {
    for (node *n = n1; n; n = n->next) {
        switch (n->type) {
        case node::head:
        case node::title:
        case node::script:
        case node::style:
            break;

        case node::text:
            for (text_node *t = n->wrap; t; t = t->next) {
                flagFont(t->fl);
                g.setFont(font);
                int y = t->y + t->th - font->height() - ty - 5;
                g.drawChars(t->text, 0, t->len, t->x - tx, y);
                if (isHref) {
                    g.drawLine(t->x - tx, y + 1, t->x + t->tw - tx, y + 1);
                }
            }
            break;

        case node::hrule:
            g.setColor(hrFg);
            g.drawLine(0 + n->xr - tx, n->yr + 4 - ty, width() - 1 - tx, n->yr + 4 - ty);
            g.drawLine(0 + n->xr - tx, n->yr + 5 - ty, width() - 1 - tx, n->yr + 5 - ty);
            g.setColor(normalFg);
            break;

        case node::anchor:
            {
                attr href;
                bool found = n->get_attribute("href", &href);
                if (found && href.value.length())
                    g.setColor(linkFg);
                if (n->container)
                    draw(g, n->container, found || isHref);
                if (found && isHref == false)
                    g.setColor(normalFg);
            }
            break;

        case node::li:
            g.fillArc(n->xr - tx, n->yr - ty - 2, 7, 7, 0, 360 * 64);
            if (n->container)
                draw(g, n->container, isHref);
            break;

        default:
            if (n->container)
                draw(g, n->container, isHref);
            break;
        }
    }
}

bool HTextView::findNext(node *n1) {
    for (node *n = n1; n; n = n->next) {
        if (n->container) {
            if (findNext(n->container))
                return true;
        }
        for (text_node *tn = n->wrap; tn; tn = tn->next) {
            if (ty < tn->y - tn->th) {
                const char* found = strstr(tn->text, search.c_str());
                if (found && found < tn->text + tn->len) {
                    setPos(0, tn->y - tn->th);
                    return true;
                }
            }
        }
    }
    return false;
}

void HTextView::startSearch() {
    while (search.nonempty() && 0 < search[0] && search[0] < ' ') {
        search = search.substring(1);
    }
    if (search.nonempty()) {
        if (findNext(fRoot)) {
            resetScroll();
        }
    }
}

bool HTextView::handleKey(const XKeyEvent &key) {
    if (fScrollView->handleScrollKeys(key)) {
        return true;
    }
    if (input && input->visible()) {
        return input->handleKey(key);
    }

    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);
        if ((m & ControlMask) != 0 && (m & ~ControlMask) == 0) {
            if (k == XK_q) {
                actionPerformed(actionClose);
                return true;
            }
            if (k == XK_b) {
                actionPerformed(actionBrowser);
                return true;
            }
            if (k == XK_r) {
                actionPerformed(actionReload);
                return true;
            }
            if (k == XK_i) {
                actionPerformed(actionLink[0]);
                return true;
            }
            if (k == XK_m) {
                actionPerformed(actionLink[4]);
                return true;
            }
            if (k == XK_t) {
                actionPerformed(actionLink[6]);
                return true;
            }
            if (k == XK_w) {
                actionPerformed(actionLink[7]);
                return true;
            }
            if (k == XK_v) {
                actionPerformed(actionReversed);
                return true;
            }
            if (k == XK_x) {
                actionPerformed(actionFreeType);
                return true;
            }
            if (k == XK_f) {
                actionPerformed(actionFind);
                return true;
            }
        }
        if ((m & xapp->AltMask) != 0 && (m & ~xapp->AltMask) == 0) {
            if (k == XK_Left || k == XK_KP_Left) {
                actionPerformed(actionLeft);
                return true;
            }
            if (k == XK_Right || k == XK_KP_Right) {
                actionPerformed(actionRight);
                return true;
            }
        }
        if (notbit(m, ControlMask | xapp->AltMask | ShiftMask)) {
            if (k == XK_F3) {
                startSearch();
                return true;
            }
        }
    }
    return false;
}

void HTextView::handleButton(const XButtonEvent &button) {
    if (fVerticalScroll->handleScrollMouse(button) == false)
        YWindow::handleButton(button);
}

void HTextView::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == Button3) {
        menu->popup(nullptr, nullptr, nullptr, up.x_root, up.y_root,
                    YPopupWindow::pfCanFlipVertical |
                    YPopupWindow::pfCanFlipHorizontal /*|
                    YPopupWindow::pfPopupMenu*/);
    }
    else if (up.button == Button1 && up.x >= int(width()) - 20 && up.y <= 20 - ty) {
        menu->popup(this, nullptr, nullptr,
                    up.x_root - up.x + int(width()) - 1,
                    up.y_root - up.y,
                    YPopupWindow::pfFlipHorizontal);
    }
    else if (up.button == Button1) {
        node *anchor = nullptr;
        node *n = find_node(fRoot, up.x + tx, up.y + ty, anchor, node::anchor);
        if (n && anchor) {
            attr href;
            if (anchor->get_attribute("href", &href) && href.value) {
                listener->activateURL(href.value, true);
            }
        }
    }
}

class FileView: public YDndWindow, public HTListener {
public:
    FileView(YApplication *app, int argc, char **argv);
    ~FileView() {
        delete view;
        delete scroll;
        FontTable::reset();
        extern void clearFontCache();
        clearFontCache();
    }

    void activateURL(mstring url, bool relative = false) override;
    void openBrowser(mstring url) override;

    void configure(const YRect2& r) override {
        if (r.resized()) {
            scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
        }
    }

    void handleClose() override {
        if (ignoreClose == false) {
            app->exitLoop(0);
        }
    }
    void handleExpose(const XExposeEvent& expose) override {
    }

    bool isFocusTraversable() override {
        return true;
    }

private:
    bool loadFile(upath path);
    bool loadHttp(upath path);
    void invalidPath(upath path, const char *reason);
    void run(const char* path, const char* arg1 = nullptr,
             const char* arg2 = nullptr, const char* arg3 = nullptr);

    bool handleKey(const XKeyEvent &key) override {
        return view->handleKey(key);
    }

    upath fPath;
    YApplication *app;

    HTextView *view;
    YScrollView *scroll;
    ref<YPixmap> small_icon;
    ref<YPixmap> large_icon;
};

FileView::FileView(YApplication *iapp, int argc, char **argv)
    : fPath(), app(iapp), view(nullptr), scroll(nullptr)
{
    setDND(true);
    setStyle(wsNoExpose);
    setTitle("IceHelp");

    scroll = new YScrollView(this);
    view = new HTextView(this, scroll, scroll);
    scroll->setView(view);

    view->show();
    scroll->show();

    setSize(ViewerWidth, ViewerHeight);

    ref<YIcon> file_icon = YIcon::getIcon("file");
    if (file_icon != null) {
        Pixmap icons[4];
        int count = 0;
        if (file_icon->small() != null) {
            small_icon = YPixmap::createFromImage(file_icon->small(), xapp->depth());
            if (small_icon != null) {
                icons[count++] = small_icon->pixmap();
                icons[count++] = small_icon->mask();
            }
        }
        if (file_icon->large() != null) {
            large_icon = YPixmap::createFromImage(file_icon->large(), xapp->depth());
            if (large_icon != null) {
                icons[count++] = large_icon->pixmap();
                icons[count++] = large_icon->mask();
            }
        }
        if (count > 0) {
            extern Atom _XA_WIN_ICONS;
            setProperty(_XA_WIN_ICONS, XA_PIXMAP, icons, count);
        }
    }
    setNetPid();

    XWMHints wmhints = {};
    wmhints.flags = InputHint | StateHint;
    wmhints.input = True;
    wmhints.initial_state = NormalState;
    if (large_icon != null) {
        wmhints.icon_pixmap = large_icon->pixmap();
        wmhints.icon_mask = large_icon->mask();
        wmhints.flags |= IconPixmapHint | IconMaskHint;
    }
    XSetWMHints(xapp->display(), handle(), &wmhints);

    YTextProperty name("icehelp");
    YTextProperty icon("icehelp");
    XSizeHints size = { PSize, 0, 0,
                        ViewerWidth, ViewerHeight,
                      };
    XClassHint klas = { const_cast<char *>("icehelp"),
                        const_cast<char *>("IceWM") };
    XSetWMProperties(xapp->display(), handle(),
                     &name, &icon, argv, argc,
                     &size, &wmhints, &klas);
}

void FileView::activateURL(mstring url, bool relative) {
    if (verbose) {
        tlog("activateURL('%s', %s)", url.c_str(),
                relative ? "relative" : "not-relative");
    }

    /*
     * Differentiate:
     * - has (only) a fragment (#).
     * - url is local/remote.
     * - if local: absolute or relative to previous path.
     */

    mstring path, frag;
    if (url.splitall('#', &path, &frag) == false ||
        path.length() + frag.length() == 0) {
        return; // empty
    }

    if (relative && path.nonempty() && false == upath(path).hasProtocol()) {
        if (upath(path).isRelative()) {
            int k = fPath.path().lastIndexOf('/');
            if (k >= 0) {
                path = fPath.path().substring(0, k + 1) + path;
            }
        }
        else if (fPath.hasProtocol()) {
            int k = fPath.path().find("://");
            if (k > 0) {
                int i = fPath.path().substring(k + 3).indexOf('/');
                if (i > 0) {
                    path = fPath.path().substring(0, k + 3 + i) + path;
                }
            }
        }
    }
    path = path.searchAndReplaceAll("/./", "/");

    if (path.length() > 0) {
        if (upath(path).hasProtocol()) {
            if (path.startsWith("file:///")) {
                path = path.substring((int)strlen("file://"));
            }
        }
        if (upath(path).hasProtocol()) {
            if (upath(path).isHttp()) {
                if (path.count('/') == 2) {
                    path += "/";
                }
                if (loadHttp(path) == false) {
                    return;
                }
            }
            else {
                return invalidPath(path, _("Unsupported protocol."));
            }
        }
        else if (loadFile(path) == false) {
            return;
        }
    }
    if (frag.length() > 0 && view->contentHeight() > view->height()) {
        // search
        view->find_fragment(frag);
    }
    view->repaint();
    if (frag.length() > 0) {
        path = path + "#" + frag;
    }
    if (relative && path.charAt(0) == '#') {
        path = fPath.path() + path;
    }
    view->addHistory(path);
    fPath = path;
    setNetName(path);
}

void FileView::openBrowser(mstring url) {
    char* env = getenv("BROWSER");
    mstring brow(nonempty(env) && !strchr(env, '/')
                 ? mstring("/usr/bin/") + env : env);
    if (brow != null && upath(brow).isExecutable()) {
        run(brow, url);
    }
    else if (upath("/usr/bin/xdg-open").isExecutable()) {
        run("/usr/bin/xdg-open", url);
    }
    else if (upath("/usr/bin/gnome-open").isExecutable()) {
        run("/usr/bin/gnome-open", url);
    }
    else if (upath("/usr/bin/kde-open").isExecutable()) {
        run("/usr/bin/kde-open", url);
    }
    else if (upath("/usr/bin/gio").isExecutable()) {
        run("/usr/bin/gio", "open", url);
    }
    else if (upath("/usr/bin/python3").isExecutable()) {
        run("/usr/bin/python3", "-m", "webbrowser", url);
    }
    else if (upath("/usr/bin/python").isExecutable()) {
        run("/usr/bin/python", "-m", "webbrowser", url);
    }
    else if (upath("/usr/bin/sensible-browser").isExecutable()) {
        run("/usr/bin/sensible-browser", url);
    }
}

void FileView::run(const char* path, const char* arg1,
                   const char* arg2, const char* arg3)
{
    char* args[] = {
        const_cast<char*>(path),
        const_cast<char*>(arg1),
        const_cast<char*>(arg2),
        const_cast<char*>(arg3),
        nullptr
    };
    switch (fork()) {
        case -1:
            fail("fork");
            break;
        case 0:
            if (execv(path, args) == -1) {
                fail("exec %s", path);
            }
            break;
        default:
            break;
    }
}

void FileView::invalidPath(upath path, const char *reason) {
    const char *cstr = path.string();
    const char *cfmt = _("Invalid path: %s\n");
    const char *crea = reason;
    tlog(cfmt, cstr);
    tlog("%s", crea);

    const size_t size = 9 + strlen(cfmt) + strlen(cstr) + strlen(crea);
    char *cbuf = (char *)malloc(size);
    snprintf(cbuf, size, cfmt, cstr);
    strlcat(cbuf, ":\n ", size);
    strlcat(cbuf, crea, size);

    node *root = new node(node::div);
    flist<node> nodes(root);

    node *txt = new node(node::text);
    txt->txt = cbuf;
    nodes.add(txt);

    view->setData(root);
    view->repaint();
}

bool FileView::loadFile(upath path) {
    if (path.fileExists() == false) {
        invalidPath(path, _("Path does not refer to a file."));
        return false;
    }
    FILE *fp = fopen(path.string(), "r");
    if (fp == nullptr) {
        invalidPath(path, _("Failed to open file for reading."));
        return false;
    }
    node *nextsub = nullptr;
    node::node_type close_type = node::unknown;
    node *root = parse(fp, 0, nullptr, nextsub, close_type);
    assert(nextsub == 0);
    fclose(fp);
    dump_tree(0, root);
    view->setData(root);
    return true;
}

class temp_file {
private:
    FILE *fp;
    cbuffer cbuf;
    bool init(const char *tdir) {
        cbuf = (upath(tdir) + "iceXXXXXX").string();
        int fd = mkstemp(cbuf.peek());
        if (fd >= 0) fp = fdopen(fd, "w+b");
        return fp;
    }
    temp_file(const temp_file&);
    temp_file& operator=(const temp_file&);
public:
    temp_file() : fp(nullptr) {
        const char *tenv = getenv("TMPDIR");
        if ((tenv && init(tenv)) || init("/tmp") || init("/var/tmp")) ;
        else tlog(_("Failed to create a temporary file"));
    }
    ~temp_file() {
        unlink(cbuf.peek());
        if (fp) fclose(fp);
    }
    operator bool() const { return fp != nullptr; }
    int fildes() const { return fileno(fp); }
    FILE *filep() const { return fp; }
    const char *path() const { return cbuf; }
    int size() const {
        struct stat b;
        return fstat(fildes(), &b) >= 0 ? (int) b.st_size : 0;
    }
    void rewind() const { ::rewind(fp); }
};

class downloader {
private:
    mstring curl, wget, gzip, cat;
    void test(mstring *mst, const mstring& dir, const char *exe) {
        if (mst->isEmpty()) {
            upath bin = upath(dir) + exe;
            if (bin.isExecutable() && bin.fileExists()) {
                *mst = bin;
            }
        }
    }
    bool empty() const {
        return curl.isEmpty() || wget.isEmpty()
            || gzip.isEmpty() || cat.isEmpty();
    }
    void init() {
        const char defp[] = "/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin";
        const char *penv = getenv("PATH");
        mstring mpth(penv ? penv : defp), mdir;
        while (empty() && mpth.splitall(':', &mdir, &mpth)) {
            if (mdir.isEmpty())
                continue;
            test(&curl, mdir, "curl");
            test(&wget, mdir, "wget");
            test(&gzip, mdir, "gzip");
            test(&cat, mdir, "cat");
        }
    }
    downloader(const downloader&);
    downloader& operator=(const downloader&);
public:
    downloader() { init(); }
    operator bool() const {
        return curl.nonempty() || wget.nonempty();
    }
    bool downloadTo(const char *remote, const temp_file& local) {
        mstring cmd;
        if (curl.nonempty()) {
            cmd = curl + " --compressed --location -s -o ";
        }
        else if (wget.nonempty()) {
            cmd = wget + " -q -k -O ";
        }
        else {
            return false;
        }
        cmd = cmd + "'" + local.path() + "' '" + remote + "'";
        if (!command(cmd)) {
            return false;
        }
        local.rewind();
        if (is_compressed(local.fildes())) {
            if (decompress(local) == false) {
                return false;
            }
        }
        local.rewind();
        return true;
    }
    static bool is_safe(const char *url) {
        for (const char *p = url; *p; ++p) {
            if (!ASCII::isAlnum(*p) && !strchr(":/.+-_@%?&=", *p)) {
                return false;
            }
        }
        return true;
    }
    static bool is_compressed(int fd) {
        char b[3];
        return filereader(fd, false).read_all(BUFNSIZE(b)) >= 2
            && b[0] == '\x1F' && b[1] == '\x8B';
    }
    static bool command(mstring mcmd) {
        const char *cmd = mcmd;
        int xit = ::system(cmd);
        if (xit) {
            tlog(_("Failed to execute system(%s) (%d)"), cmd, xit);
            return false;
        }
        // tlog("Executed system(%s) OK!", cmd);
        return true;
    }
    bool decompress(const temp_file& local) {
        if (gzip.nonempty() && cat.nonempty()) {
            temp_file temp;
            if (temp) {
                mstring cmd = gzip + " -d -c <'";
                cmd = cmd + local.path() + "' >'";
                cmd = cmd + temp.path() + "'";
                if (command(cmd)) {
                    cmd = cat + "<'" + temp.path() + "' >'";
                    cmd = cmd + local.path() + "'";
                    if (command(cmd)) {
                        local.rewind();
                        return true;
                    }
                }
            }
        }
        tlog(_("Failed to decompress %s"), local.path());
        return false;
    }
};

bool FileView::loadHttp(upath path) {
    downloader loader;
    if (!loader) {
        invalidPath(path, _("Could not locate curl or wget in PATH"));
        return false;
    }
    if (!loader.is_safe(path.string())) {
        invalidPath(path, _("Unsafe characters in URL"));
        return false;
    }
    temp_file temp;
    if (!temp) {
        return false;
    }
    if (loader.downloadTo(path.string(), temp)) {
        return loadFile(temp.path());
    }
    return false;
}

static int handler(Display *display, XErrorEvent *xev) {
    if (true) {
        char message[80], req[80], number[80];

        snprintf(number, 80, "%d", xev->request_code);
        XGetErrorDatabaseText(display, "XRequest",
                              number, "",
                              req, sizeof(req));
        if (req[0] == 0)
            snprintf(req, 80, "[request_code=%d]", xev->request_code);

        if (XGetErrorText(display, xev->error_code, message, 80) != Success)
            *message = '\0';

        tlog("X error %s(0x%lX): %s. #%lu, +%lu, -%lu.",
             req, xev->resourceid, message, xev->serial,
             NextRequest(display), LastKnownRequestProcessed(display));
    }
    return 0;
}

static void print_help()
{
    printf(_(
    "Usage: %s [OPTIONS] [ FILENAME | URL ]\n\n"
    "IceHelp is a very simple HTML browser for the IceWM window manager.\n"
    "It can display a HTML document from file, or browse a website.\n"
    "It remembers visited pages in a history, which is navigable\n"
    "by key bindings and a context menu (right mouse click).\n"
    "It neither supports rendering of images nor JavaScript.\n"
    "If no file or URL is given it will display the IceWM Manual\n"
    "from %s.\n"
    "\n"
    "Options:\n"
    "  -d, --display=NAME  NAME of the X server to use.\n"
    "  --sync              Synchronize X11 commands.\n"
    "\n"
    "  -B                  Display the IceWM icewmbg manpage.\n"
    "  -b, --bugs          Display the IceWM bug reports (primitively).\n"
    "  -f, --faq           Display the IceWM FAQ and Howto.\n"
    "  -g                  Display the IceWM Github website.\n"
    "  -i, --icewm         Display the IceWM icewm manpage.\n"
    "  -m, --manual        Display the IceWM Manual (default).\n"
    "  -s                  Display the IceWM icesound manpage.\n"
    "  -t, --theme         Display the IceWM themes Howto.\n"
    "  -w, --website       Display the IceWM website.\n"
    "\n"
    "  -V, --version       Prints version information and exits.\n"
    "  -h, --help          Prints this usage screen and exits.\n"
    "\n"
    "Environment variables:\n"
    "  DISPLAY=NAME        Name of the X server to use.\n"
    "\n"
    "To report bugs, support requests, comments please visit:\n"
    "%s\n"
    "\n"),
        ApplicationName,
        ICEHELPIDX,
        PACKAGE_BUGREPORT);
    exit(0);
}

int main(int argc, char **argv) {
    YLocale locale;
    const char *helpfile(nullptr);
    bool nodelete = false, netping = false;

    for (char **arg = 1 + argv; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            if (is_switch(*arg, "b", "bugs"))
                helpfile = PACKAGE_BUGREPORT;
            else if (is_switch(*arg, "f", "faq"))
                helpfile = ICEWM_FAQ;
            else if (is_short_switch(*arg, "B"))
                helpfile = ICEWMBG_1;
            else if (is_short_switch(*arg, "g"))
                helpfile = ICEGIT_SITE;
            else if (is_switch(*arg, "i", "icewm"))
                helpfile = ICEWM_1;
            else if (is_switch(*arg, "m", "manual"))
                helpfile = ICEHELPIDX;
            else if (is_short_switch(*arg, "s"))
                helpfile = ICESOUND_1;
            else if (is_switch(*arg, "t", "themes"))
                helpfile = THEME_HOWTO;
            else if (is_switch(*arg, "w", "website"))
                helpfile = ICEWM_SITE;
            else if (is_help_switch(*arg))
                print_help();
            else if (is_version_switch(*arg))
                print_version_exit(VERSION);
            else if (is_copying_switch(*arg))
                print_copying_exit();
            else if (is_long_switch(*arg, "noclose"))
                ignoreClose = true;
            else if (is_long_switch(*arg, "nodelete"))
                nodelete = true;
            else if (is_long_switch(*arg, "noxft"))
                noFreeType = true;
            else if (is_long_switch(*arg, "reverse"))
                reverseVideo = true;
            else if (is_long_switch(*arg, "netping"))
                netping = true;
            else if (is_long_switch(*arg, "verbose"))
                verbose = true;
            else if (is_long_switch(*arg, "sync")) {
                YXApplication::synchronizeX11 = true; }
            else if (is_long_switch(*arg, "logevents"))
                toggleLogEvents();
            else {
                char *dummy(nullptr);
                if (GetArgument(dummy, "d", "display", arg, argv + argc)) {
                    /*ignore*/; }
                else
                    warn(_("Ignoring option '%s'"), *arg);
            }
        }
        else {
            helpfile = *arg;
        }
    }
#ifdef PRECON
    complain = true;
#elif DEBUG
    complain = (debug | verbose);
#else
    complain = verbose;
#endif

    if (helpfile == nullptr) {
        helpfile = ICEHELPIDX;
    }

    XSetErrorHandler(handler);
    YXApplication app(&argc, &argv);
    FileView view(&app, argc, argv);
    if (nodelete) {
        XDeleteProperty(app.display(), view.handle(), _XA_WM_PROTOCOLS);
    } else {
        extern Atom _XA_NET_WM_PING;
        Atom prot[] = {
            _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS, _XA_NET_WM_PING
        };
        XSetWMProtocols(app.display(), view.handle(), prot, 2 + netping);
    }
    view.activateURL(helpfile);
    view.show();
    app.mainLoop();

    return 0;
}

static void dump_tree(int level, node *n) {
#ifdef DUMP
    while (n) {
        printf("%*s<%s>\n", level, "", node::to_string(n->type));
        if (n->container) {
            dump_tree(level + 1, n->container);
            printf("%*s</%s>\n", level, "", node::to_string(n->type));
        }
        n = n->next;
    }
#endif
}

#if 0

#define S_LINK "\x1B[04m"
#define E_LINK "\x1B[0m"
#define S_HEAD "\x1B[01;31m"
#define E_HEAD "\x1B[0m"
#define S_BOLD "\x1B[01m"
#define E_BOLD "\x1B[0m"
#define S_ITALIC "\x1B[33m"
#define E_ITALIC "\x1B[0m"
#define S_DEBUG "\x1B[07;31m"
#define E_DEBUG "\x1B[0m"

int pos = 0;
int width = 80;

void clear_line() {
    if (pos != 0)
        puts("");
    pos = 0;
}

void new_line(int count = 1) {
    while (count-- > 0)
        putchar('\n');
    pos = 0;
}

void blank_line(int count = 1) {
    clear_line();
    new_line(count);
}

void out(char *text) {
    printf("%s", text);
    pos += strlen(text);
}

void tab(int len) {
    pos += len;
    while (len-- > 0) putchar(' ');
}

void pre(int left, char *text) {
    char *p = text;

    clear_line();
    while (*p) {
        tab(left);
        while (*p && *p != '\n') {
            putchar(*p);
            pos++;
            p++;
        }
        if (*p == '\n')
            p++;
        new_line();
    }
}

void wrap(int left, char *text) {
    //putchar('[');
    while (*text) {
        int chr = 0;
        int sp = 0;
        int col = pos;
        char *p = text;

        //printf("[%d:%s]", pos, text);
        while (*p && col <= width) {
            if (*p == ' ')
                sp = chr;
            chr++;
            col++;
            p++;
        }
        if (col <= width)
            sp = chr;
        if (sp == 0)
            sp = chr;
        //printf("(%d)", col);

        while (sp > 0) {
            putchar(*text);
            pos++;
            text++;
            sp--;
        }
        if (*text == ' ')
            text++;
        if (*text) {
            //if (pos != width)
                puts("");
            pos = 0;
            tab(left);
        }
    }
    //putchar(']');
}

node *blank(node *n) {
    if (n->next && n->next->type == node::text) {
        if (n->next->content.text[0] == ' ' &&
            n->next->content.text[1] == 0) {
            if (n->next->next &&
                (n->next->next->type == node::h1 ||
                 n->next->next->type == node::h2))
                return n->next;
            blank_line();
            return n->next;
        }
    }
    blank_line();
    return n;
}

void dump(int flags, int left, node *n, node *up) {
    while (n) {
        switch (n->type) {
        case node::html:
            dump(flags, left, n->container, n);
            break;
        case node::head:
            dump(flags, left, n->container, n);
            break;
        case node::body:
        case node::table:
        case node::font:
        case node::tr:
        case node::td:
        case node::div:
            dump(flags, left, n->container, n);
            break;
        case node::ul:
        case node::ol:
            dump(flags, left, n->content.container, n);
            if (up && up->type != node::li) {
                n = blank(n);
            }
            break;
        case node::li:
            clear_line();
            tab(left);
            out("  * ");
            dump(flags, left + 4, n->content.container, n);
            break;
        case node::bold:
            printf(S_BOLD);
            dump(flags | BOLD, left, n->content.container, n);
            printf(E_BOLD);
            break;
        case node::italic:
            printf(S_ITALIC);
            dump(flags | BOLD, left, n->content.container, n);
            printf(E_ITALIC);
            break;
        case node::h1:
        case node::h2:
        case node::h3:
        case node::h4:
        case node::h5:
        case node::h6:
            blank_line();
            printf(S_HEAD);
            dump(flags, left, n->content.container, n);
            printf(E_HEAD);
            n = blank(n);
            break;
        case node::pre:
            dump(flags | PRE, left + 4, n->content.container, n);
            break;
        case node::link:
            printf(S_LINK);
            dump(flags | LINK, left, n->content.container, n);
            printf(E_LINK);
            break;
        case node::hrule:
            clear_line();
            for (int i = 0; i < 72; i++)
                pos += printf("-");
            clear_line();
            break;
        case node::paragraph:
            clear_line();
            new_line(1);
            break;
        case node::line:
            new_line(1);
            break;
        case node::text:
            if (flags & PRE)
                pre(left, n->content.text);
            else {
                wrap(left, n->content.text);
            }
            break;
        default:
            printf(S_DEBUG "?" E_DEBUG);
        }
        n = n->next;
    }
}

int main(int argc, char **argv) {
    FILE *fp = fopen(argv[1], "r");
    if (fp == 0)
        fp = stdin;
    char *e = getenv("COLUMNS");
    if (e) width = atoi(e);
    if (fp) {
        root = parse(fp, 0);
        dump(0, 0, root, 0);
        clear_line();
    }
}
#endif

// vim: set sw=4 ts=4 et:
