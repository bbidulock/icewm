#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlocale.h>
#include "config.h"
#include "ylib.h"
#include "ypixbuf.h"
#include <X11/Xatom.h>
#include "ylistbox.h"
#include "yscrollview.h"
#include "ymenu.h"
#include "yxapp.h"
#include "sysdep.h"
#include "yaction.h"
#include "ymenuitem.h"
#include "ylocale.h"
#include "yrect.h"
#include "prefs.h"
#include "yicon.h"
#include "ascii.h"

#define DUMP
//#define TEXT

#include <unistd.h>

#include "intl.h"

#define LINE(c) ((c) == '\r' || (c) == '\n')
#define SPACE(c) ((c) == ' ' || (c) == '\t' || LINE(c))

char const * ApplicationName = "icehelp";

class HTListener {
public:
    virtual void activateURL(const char *url) = 0;
protected:
    virtual ~HTListener() {};
};

class text_node {
public:
    text_node(const char *t, int l, int _x, int _y, int _w, int _h) {
        text = t; len = l; next = 0; x = _x; y = _y; w = _w; h = _h;
    }

    int x, y;
    short w, h, len; // int?
    const char *text;
    text_node *next;
};

struct attribute {
    char *name;
    char *value;
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
        anchor,
        tt, dl, dd, dt,
        link, code, meta
    };

    node(node_type t) { next = 0; container = 0; type = t; wrap = 0; txt = 0; nattr = 0; attr = 0; }

    node_type type;
    node *next;
    node *container;
    char *txt;
    text_node *wrap;
    int xr, yr;

    int nattr;
    attribute *attr;
    //int width, height;

    static const char *to_string(node_type type);
};

node *root = NULL;

#define PRE    0x01
#define PRE1   0x02
#define BOLD   0x04
#define LINK   0x08

#define sfPar  0x01
#define sfText  0x02

node *add(node **first, node *last, node *n) {
    if (last)
        last->next = n;
    else
        *first = n;
    return n;
}

void add_attribute(node *n, char *abuf, char *vbuf) {
    //msg("[%s]=[%s]", abuf, vbuf ? vbuf : "");

    n->attr = (attribute *)realloc(n->attr, sizeof(attribute) * (n->nattr + 1));
    assert(n->attr != 0);

    n->attr[n->nattr].name = abuf;
    n->attr[n->nattr].value = vbuf;
    n->nattr++;
}

attribute *find_attribute(node *n, const char *name) {
    for (int i = 0; i < n->nattr; i++) {
        if (strcmp(name, n->attr[i].name) == 0)
            return n->attr + i;
    }
    return 0;
}

void dump_tree(int level, node *n);

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
        TS(tt);
        TS(dl);
        TS(dd);
        TS(dt);
        TS(link);
        TS(code);
        TS(meta);
    }
    return "??";
}

node::node_type get_type(const char *buf) {
    node::node_type type ;

    if (strcmp(buf, "HR") == 0)
        type = node::hrule;
    else if (strcmp(buf, "P") == 0)
        type = node::paragraph;
    else if (strcmp(buf, "BR") == 0)
        type = node::line;
    else if (strcmp(buf, "HTML") == 0)
        type = node::html;
    else if (strcmp(buf, "HEAD") == 0)
        type = node::head;
    else if (strcmp(buf, "TITLE") == 0)
        type = node::title;
    else if (strcmp(buf, "BODY") == 0)
        type = node::body;
    else if (strcmp(buf, "H1") == 0)
        type = node::h1;
    else if (strcmp(buf, "H2") == 0)
        type = node::h2;
    else if (strcmp(buf, "H3") == 0)
        type = node::h3;
    else if (strcmp(buf, "H4") == 0)
        type = node::h4;
    else if (strcmp(buf, "H5") == 0)
        type = node::h5;
    else if (strcmp(buf, "H6") == 0)
        type = node::h6;
    else if (strcmp(buf, "PRE") == 0)
        type = node::pre;
    else if (strcmp(buf, "FONT") == 0)
        type = node::font;
    else if (strcmp(buf, "TABLE") == 0)
        type = node::table;
    else if (strcmp(buf, "TR") == 0)
        type = node::tr;
    else if (strcmp(buf, "TD") == 0)
        type = node::td;
    else if (strcmp(buf, "DIV") == 0)
        type = node::div;
    else if (strcmp(buf, "UL") == 0)
        type = node::ul;
    else if (strcmp(buf, "OL") == 0)
        type = node::ol;
    else if (strcmp(buf, "A") == 0)
        type = node::anchor;
    else if (strcmp(buf, "B") == 0)
        type = node::bold;
    else if (strcmp(buf, "I") == 0)
        type = node::italic;
    else if (strcmp(buf, "CENTER") == 0)
        type = node::center;
    else if (strcmp(buf, "SCRIPT") == 0)
        type = node::script;
    else if (strcmp(buf, "A") == 0)
        type = node::anchor;
    else if (strcmp(buf, "TT") == 0)
        type = node::tt;
    else if (strcmp(buf, "DL") == 0)
        type = node::dl;
    else if (strcmp(buf, "DT") == 0)
        type = node::dt;
    else if (strcmp(buf, "LI") == 0)
        type = node::li;
    else if (strcmp(buf, "DD") == 0)
        type = node::dd;
    else if (strcmp(buf, "LINK") == 0)
        type = node::link;
    else if (strcmp(buf, "CODE") == 0)
        type = node::code;
    else if (strcmp(buf, "META") == 0)
        type = node::meta;
    else
        type = node::unknown;
    return type;
}

node *parse(FILE *fp, int flags, node *parent, node *&nextsub, node::node_type &close) {
    int c;
    node *f = NULL;
    node *l = NULL;

    //puts("<PARSE>");
    c = getc(fp);
    while (c != EOF) {
        switch (c) {
        case '<':
            c = getc(fp);
            if (c == '!') {
                c = getc(fp);
                if (c == '-') {
                    c = getc(fp);
                    if (c == '-') {
                        // comment
                        c = getc(fp);
                        if (c != EOF) do {
                            if (c == '-') {
                                c = getc(fp);
                                if (c == '-') {
                                    do { c = getc(fp); } while (c == '-');
                                    if (c == '>' || c == EOF)
                                        break;
                                }
                            }
                            if (c == EOF)
                                break;
                            c = getc(fp);
                        } while (c != EOF);
                    }
                }
                while (c != EOF && c != '>') { c = getc(fp); }
            } else if (c == '/') {
                // ignore </BR> </P> </LI> ...
                int len = 0;
                char *buf = 0;

                c = getc(fp);
                while (SPACE(c)) c = getc(fp);
                do {
                    buf = (char *)realloc(buf, ++len);
                    buf[len-1] = ASCII::toUpper(c);
                    c = getc(fp);
                } while (c != EOF && !SPACE(c) && c != '>');

                buf = (char *)realloc(buf, ++len);
                buf[len-1] = 0;

                node::node_type type = get_type(buf);

#if 1
                if (type == node::paragraph ||
                    type == node::line ||
                    type == node::hrule ||
                    type == node::link ||
                    type == node::meta)
                {
                } else {
                    if (parent) {
                        close = type;
                        //puts("</PARSE>");
                        return f;
                    }
                }
#endif
            } else {
                int len = 0;
                char *buf = 0;

                while (SPACE(c)) c = getc(fp);
                do {
                    buf = (char *)realloc(buf, ++len);
                    buf[len-1] = ASCII::toUpper(c);
                    c = getc(fp);
                } while (c != EOF && !SPACE(c) && c != '>');

                buf = (char *)realloc(buf, ++len);
                buf[len-1] = 0;

                node::node_type type = get_type(buf);
                node *n = new node(type);

                if (n == 0)
                    break;

#if 1
                while (SPACE(c)) c = getc(fp);

                while (c != '>') {
                    int alen = 0;
                    char *abuf = 0;

                    int vlen = 0;
                    char *vbuf = 0;

                    do {
                        abuf = (char *)realloc(abuf, ++alen + 1);
                        abuf[alen-1] = ASCII::toUpper(c);
                        abuf[alen] = 0;
                        c = getc(fp);
                    } while (c != EOF && !SPACE(c) && c != '=' && c != '>');

                    while (SPACE(c)) c = getc(fp);

                    if (c == '=') {
                        c = getc(fp);
                        while (SPACE(c)) c = getc(fp);
                        if (c == '"') {
                            c = getc(fp);
                            if (c != EOF && c != '"') do {
                                vbuf = (char *)realloc(vbuf, ++vlen + 1);
                                vbuf[vlen-1] = c;
                                vbuf[vlen] = 0;
                                c = getc(fp);
                            } while (c != EOF && c != '"');
                        } else {
                            if (c != EOF && c != '>') do {
                                vbuf = (char *)realloc(vbuf, ++vlen + 1);
                                vbuf[vlen-1] = c;
                                vbuf[vlen] = 0;
                                c = getc(fp);
                            } while (c != EOF && !SPACE(c) && c != '>');
                        }
                    }
                    add_attribute(n, abuf, vbuf);

                    while (SPACE(c)) c = getc(fp);

                }
#endif

                if (c != '>' && c != EOF)
                    do { c = getc(fp); } while (c != EOF && c != '>');

                if (type == node::line ||
                    type == node::hrule ||
                    type == node::paragraph||
                    type == node::link ||
                    type == node::meta)
                {
                    l = add(&f, l, n);
                } else {
                    node *container = 0;

                    if (type == node::li ||
                        type == node::dt ||
                        type == node::dd ||
                        type == node::paragraph ||
                        type == node::line
                       )
                    {
                        if (parent &&
                            (parent->type == type ||
                             type == node::dd && parent->type == node::dt ||
                             type == node::dt && parent->type == node::dd)
                           )
                        {
                            nextsub = n;
                            return f;
                        }
                    }

                    node *nextsub;
                    node::node_type close_type;
                    do {
                        nextsub = 0;
                        close_type = node::unknown;
                        container = parse(fp, flags | ((type == node::pre) ? PRE | PRE1 : 0), n, nextsub, close_type);
                        if (container)
                            n->container = container;
                        l = add(&f, l, n);

                        if (nextsub) {
                            //puts("CONTINUATION");
                            n = nextsub;
                        }
                        if (close_type != node::unknown) {
                            if (n->type != close_type)
                                return f;
                        }
                    } while (nextsub != 0);
                }
            }
            break;
        default:
            {
                int len = 0;
                char *buf = 0;

                do {
                    if (c == '&') {
                        char *entity = 0;
                        int elen = 0;

                        do {
                            entity = (char *)realloc(entity, ++elen + 1);
                            entity[elen - 1] = toupper(c);
                            c = getc(fp);
                        } while (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z');
                        entity[elen] = 0;
                        if (c != ';')
                            ungetc(c, fp);
                        if (strcmp(entity, "&AMP") == 0)
                            c = '&';
                        else if (strcmp(entity, "&LT") == 0)
                            c = '<';
                        else if (strcmp(entity, "&GT") == 0)
                            c = '>';
                        else if (strcmp(entity, "&NBSP") == 0)
                            c = 32+128;
                        free(entity);
                    }
                    if (c == '\r') {
                        c = getc(fp);
                        if (c != '\n')
                            ungetc(c, fp);
                        c = '\n';
                    }
                    if (!(flags & PRE))
                        if (SPACE(c))
                            c = ' ';
                    if ((flags & PRE1) && c == '\n')
                        ;
                    else
                        if ((flags & PRE) || c != ' ' || len == 0 || buf[len - 1] != ' ') {
                            buf = (char *)realloc(buf, ++len);
                            buf[len-1] = c;
                        }
                    flags &= ~PRE1;
                    c = getc(fp);
                } while (c != EOF && c != '<');

                if (c == '<') {
                    c = getc(fp);
                    if (c == '/') {

                        if (len && SPACE(buf[len - 1]))
                            len--;
                    }
                    ungetc(c, fp);
                    c = '<';
                }

                if (len) {
                    buf = (char *)realloc(buf, ++len);
                    buf[len-1] = 0;

                    node *n = new node(node::text);
                    n->txt = buf;
                    l = add(&f, l, n);
                }
                continue;
            }
        }
        c = getc(fp);
    }
    return f;
}

extern Atom _XA_WIN_ICONS;

class HTextView: public YWindow,
    public YScrollBarListener, public YScrollable, public YActionListener
{
public:
    HTextView(HTListener *fL, YScrollView *v, YWindow *parent);
    ~HTextView() {}

    void find_link(node *n);

    void setData(node *root) {
        fRoot = root;
        tx = ty = 0;
        layout();

        prevItem->setEnabled(false);
        nextItem->setEnabled(false);
        contentsItem->setEnabled(false);

        find_link(fRoot);
    }


    void par(int &state, int &x, int &y, int &h, const int left);
    void epar(int &state, int &x, int &y, int &h, const int left);
    void layout();
    void layout(node *parent, node *n1, int left, int right, int &x, int &y, int &w, int &h, int flags, int &state);
    void draw(Graphics &g, node *n1, bool href = false);
    node *find_node(node *n, int x, int y, node *&anchor, node::node_type type);

    virtual void paint(Graphics &g, const YRect &r) {
        g.setColor(bg);
        g.fillRect(r.x(), r.y(), r.width(), r.height());
        g.setColor(normalFg);
        g.setFont(font);

        draw(g, fRoot);
    }

    void resetScroll() {
        fVerticalScroll->setValues(ty, height(), 0, contentHeight());
        fVerticalScroll->setBlockIncrement(height());
        fVerticalScroll->setUnitIncrement(font->height());
        fHorizontalScroll->setValues(tx, width(), 0, contentWidth());
        fHorizontalScroll->setBlockIncrement(width());
        fHorizontalScroll->setUnitIncrement(20);
        if (view)
            view->layout();
    }

    void setPos(int x, int y) {
        if (x != tx || y != ty) {
            int dx = x - tx;
            int dy = y - ty;

            tx = x;
            ty = y;

            scrollWindow(dx, dy);
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

    int contentWidth() {
        return conWidth;
    }
    int contentHeight() {
        return conHeight;
    }
    YWindow *getWindow() { return this; }

    virtual void handleClick(const XButtonEvent &up, int /*count*/);

    virtual void actionPerformed(YAction *action, unsigned int /*modifiers*/) {
        if (action == actionClose)
            exit(0);
        if (action == actionNext)
            listener->activateURL(nextURL);
        if (action == actionPrev)
            listener->activateURL(prevURL);
        if (action == actionContents)
            listener->activateURL(contentsURL);
    }

    virtual void configure(const YRect &r, const bool resized) {
        YWindow::configure(r, resized);
        if (resized) layout();
    }

    bool handleKey(const XKeyEvent &key);
    void handleButton(const XButtonEvent &button);
private:
    node *fRoot;

    int tx, ty;
    int conWidth;
    int conHeight;

    ref<YFont> font;
    YColor *bg, *normalFg, *linkFg, *hrFg, *testBg;

    YScrollView *view;
    YScrollBar *fVerticalScroll;
    YScrollBar *fHorizontalScroll;

    YMenu *menu;
    YAction *actionClose;
    YAction *actionNone;
    HTListener *listener;

    char *prevURL;
    char *nextURL;
    char *contentsURL;

    YAction *actionPrev;
    YAction *actionNext;
    YAction *actionContents;

    YMenuItem *prevItem, *nextItem, *contentsItem;
};

HTextView::HTextView(HTListener *fL, YScrollView *v, YWindow *parent):
    YWindow(parent), fRoot(NULL), view(v), listener(fL) {
    view = v;
    fVerticalScroll = view->getVerticalScrollBar();
    fVerticalScroll->setScrollBarListener(this);
    fHorizontalScroll = view->getHorizontalScrollBar();
    fHorizontalScroll->setScrollBarListener(this);
    //setBitGravity(NorthWestGravity);
    tx = ty = 0;
    conWidth = conHeight = 0;

    prevURL = nextURL = contentsURL = 0;

    //font = YFont::getFont("9x15");
    //font = YFont::getFont("-adobe-helvetica-medium-r-normal--10-100-75-75-p-56-iso8859-1");
    //font = YFont::getFont("-adobe-helvetica-medium-r-normal--*-140-*-*-*-*-iso8859-1");
    //-adobe-helvetica-bold-o-normal--11-80-100-100-p-60-iso8859-1
    //font = YFont::getFont("-adobe-helvetica-medium-r-normal--8-80-75-75-p-46-iso8859-1");
    //font = YFont::getFont("-adobe-helvetica-medium-r-normal--24-240-75-75-p-130-iso8859-1");
    font = YFont::getFont("-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1", "sans-serif-12");
    //font = YFont::getFont("-adobe-helvetica-medium-r-normal--11-80-100-100-p-56-iso8859-1");
    //font = YFont::getFont("-adobe-helvetica-bold-r-normal--12-120-75-75-p-70-iso8859-1");
    normalFg = new YColor("rgb:00/00/00");
    hrFg = new YColor("rgb:80/80/80");
    linkFg = new YColor("rgb:00/00/CC");
    bg = new YColor("rgb:CC/CC/CC");
    testBg = new YColor("rgb:40/40/40");

    actionClose = new YAction();
    actionNone = new YAction();
    actionPrev = new YAction();
    actionNext = new YAction();
    actionContents = new YAction();

    menu = new YMenu();
    menu->setActionListener(this);
    menu->addItem(_("Back"), 0, _("Alt+Left"), actionNone)->setEnabled(false);
    menu->addItem(_("Forward"), 0, _("Alt+Right"), actionNone)->setEnabled(false);
    menu->addSeparator();
    prevItem = menu->addItem(_("Previous"), 0, "", actionPrev);
    nextItem = menu->addItem(_("Next"), 0, "", actionNext);
    menu->addSeparator();
    contentsItem = menu->addItem(_("Contents"), 0, "", actionContents);
    menu->addItem(_("Index"), 0, "", actionNone)->setEnabled(false);
    menu->addSeparator();
    menu->addItem(_("Close"), 0, _("Ctrl+Q"), actionClose);
}

node *HTextView::find_node(node *n, int x, int y, node *&anchor, node::node_type type) {

    while (n) {
        if (n->container) {
            node *f;

            if ((f = find_node(n->container, x, y, anchor, type)) != 0) {
                if (anchor == 0 && n->type == type)
                    anchor = n;
                return f;
            }
        }
        if (n->wrap) {
            text_node *t = n->wrap;
            while (t) {
                if (x >= t->x && x < t->x + t->w &&
                    y >= t->y && y < t->y + t->h)
                    return n;
                t = t->next;
            }
        }
        n = n->next;
    }
    return 0;
}

void HTextView::find_link(node *n) {
    while (n) {
        if (n->type == node::link) {
            attribute *rel = find_attribute(n, "REL");
            attribute *href = find_attribute(n, "HREF");
            if (rel && href && rel->value && href->value) {
                if (strcasecmp(rel->value, "previous") == 0) {
                    prevURL = newstr(href->value);
                    prevItem->setEnabled(true);
                }
                if (strcasecmp(rel->value, "next") == 0) {
                    nextURL = newstr(href->value);
                    nextItem->setEnabled(true);
                }
                if (strcasecmp(rel->value, "contents") == 0) {
                    contentsURL = newstr(href->value);
                    contentsItem->setEnabled(true);
                }
            }
        }
        if (n->container)
            find_link(n->container);
        n = n->next;
    }
}

void HTextView::layout() {
    int state = sfPar;
    int x = 0, y = 0;
    conWidth = conHeight = 0;
    layout(0, fRoot, 0, width(), x, y, conWidth, conHeight, 0, state);
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

void HTextView::par(int &state, int &x, int &y, int &h, const int left) {
    if (!(state & sfPar)) {
        h += font->height();
        x = left;
        y = h;
        addState(state, sfPar);
    }
}

void HTextView::epar(int &state, int &x, int &y, int &h, const int left) {
    if ((x > left) || ((state & (sfText | sfPar)) == sfText)) {
        h += font->height();
        x = left;
        y = h;
        removeState(state, sfText);
    }
    removeState(state, sfPar);
}

void HTextView::layout(node *parent, node *n1, int left, int right, int &x, int &y, int &w, int &h, int flags, int &state) {
    node *n = n1;

    ///puts("{");
    while (n) {
        n->xr = x;
        n->yr = y;
        switch (n->type) {
        case node::title:
            break;
        case node::script:
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
                layout(n, n->container, n->xr, right, x, y, w, h, flags, state);
            }
            x = left;
            y = h + font->height();
            addState(state, sfText);
            removeState(state, sfPar);
            break;
        case node::text:
            {
                char *c;

                while (n->wrap) {
                    text_node *t = n->wrap;
                    n->wrap = n->wrap->next;
                    delete t;
                }

                text_node **pwrap = &n->wrap;
                for (char *b = n->txt; *b; b = c) {
                    int wc = 0;

                    if (flags & PRE) {
                        c = b;
                        while (*c && *c != '\n')
                            c++;
                        wc = font->textWidth(b, c - b);
                    } else {
                        c = b;

                        if (x == left)
                            while (SPACE(*b))
                                b++;

                        c = b;

                        do {
                            char *d = c;

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

                    if (!(flags & PRE) && x == left) while (SPACE(*b)) b++;
                    if ((flags & PRE) || c - b > 0) {
#ifdef TEXT
                        {
                            char *f = b;
                            int ll;
                            msg("x=%d left=%d line: ", x, left);
                            ll = 0;
                            while (f < c) { putchar(*f); f++; ll++; }
                            msg("[len=%d]", ll);
                        }
#endif
                        par(state, x, y, h, left);
                        addState(state, sfText);

                        *pwrap = new text_node(b, c - b, x, y, wc, font->height());
                        pwrap = &((*pwrap)->next);
                        if (y + (int)font->height() > h)
                            h = y + font->height();

                        x += wc;
                        if (x > w)
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
            removeState(state, sfPar);
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
                layout(n, n->container, n->xr, right, x, y, w, h, flags, state);
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
                layout(n, n->container, x, right, x, y, w, h, flags | PRE, state);
            }
            y = h;
            x = left;
            removeState(state, sfPar);
            addState(state, sfText);
            //msg("set=%d", state);
            break;
        default:
            if (n->container)
                layout(n, n->container, left, right, x, y, w, h, flags, state);
            break;
        }
        n = n->next;
    }
    ///puts("}");
}

void HTextView::draw(Graphics &g, node *n1, bool href) {
    node *n = n1;

    while (n) {
        switch (n->type) {
        case node::title:
            break;
        case node::text:
            if (n->wrap) {
                text_node *t = n->wrap;

#if 0
                g.setColor(testBg);
                g.fillRect(t->x - tx, t->y - ty, t->w, t->h);
                g.setColor(normalFg);
#endif
                while (t) {
                    g.drawChars(t->text, 0, t->len, t->x - tx, t->y + font->ascent() - ty);
                    if (href) {
                        g.drawLine(t->x - tx, t->y - ty + font->ascent() + 1,
                                   t->x + t->w - tx, t->y - ty + font->ascent() + 1);
                    }

                    t = t->next;
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
                attribute *href = find_attribute(n, "HREF");
                if (href && href->value)
                    g.setColor(linkFg);
                if (n->container)
                    draw(g, n->container, href);
                if (href)
                    g.setColor(normalFg);
            }
            break;

        case node::li:
            g.fillArc(n->xr - tx, n->yr + (font->height() - 7) / 2 - ty, 7, 7, 0, 360 * 64);

        default:
            if (n->container)
                draw(g, n->container, href);
            break;
        }
        n = n->next;
    }
}

bool HTextView::handleKey(const XKeyEvent &key) {
    if (key.type == KeyPress) {

        if (fVerticalScroll->handleScrollKeys(key) == false
            && fHorizontalScroll->handleScrollKeys(key) == false)
        {
            return YWindow::handleKey(key);
        }
        return true;
    }
    return false;
}

void HTextView::handleButton(const XButtonEvent &button) {
    if (fVerticalScroll->handleScrollMouse(button) == false)
        YWindow::handleButton(button);
}

class FileView: public YWindow, public HTListener {
public:
    FileView(char *path);

    void loadFile();

    void activateURL(const char *url) {
        char link[1024];
        char *r;

        //msg("link: %s", url);

        strcpy(link, fPath);

        r = strrchr(link, '/');
        if (r)
            r[1] = 0;
        else
            link[0] = 0;

        strcat(link, url);

        r = strchr(link, '#');
        if (r)
            *r = 0;

#if 0
        FileView *view = new FileView(link);

        view->show();
#else
        free(fPath);
        fPath = strdup(link);
        loadFile();
        view->repaint();
#endif
    }

    virtual void configure(const YRect &r, const bool resized) {
        YWindow::configure(r, resized);
        if (resized) scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
    }

    virtual void handleClose() {
        delete this;
        app->exit(0);
    }

private:
    char *fPath;

    HTextView *view;
    YScrollView *scroll;
    ref<YPixmap> small_icon;
    ref<YPixmap> large_icon;
};

FileView::FileView(char *path) {
    setDND(true);
    fPath = newstr(path);

    scroll = new YScrollView(this);
    view = new HTextView(this, scroll, this);
    scroll->setView(view);

    view->show();
    scroll->show();

    setTitle(fPath);
    setClassHint("browser", "IceHelp");

    YIcon *file_icon = YIcon::getIcon("file");
    small_icon.init(new YPixmap(*file_icon->small()));
    large_icon.init(new YPixmap(*file_icon->large()));

    Pixmap icons[4] = {
        small_icon->pixmap(), small_icon->mask(),
        large_icon->pixmap(), large_icon->mask()
    };

    XChangeProperty(xapp->display(), handle(),
                    _XA_WIN_ICONS, XA_PIXMAP,
                    32, PropModeReplace,
                    (unsigned char *)icons, 4);

    setSize(640, 640);

    loadFile();
}

void HTextView::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        menu->popup(0, 0, 0, up.x_root, up.y_root,
                    YPopupWindow::pfCanFlipVertical |
                    YPopupWindow::pfCanFlipHorizontal /*|
                    YPopupWindow::pfPopupMenu*/);
        return ;
    } else if (up.button == 1) {
        node *anchor = 0;
        node *n = find_node(fRoot, up.x + tx, up.y + ty, anchor, node::anchor);

        if (n && anchor) {
            attribute *href = find_attribute(anchor, "HREF");


            if (href && href->value) {
                listener->activateURL(href->value);
            }
        }
    }
}

int main(int argc, char **argv) {
    YLocale locale;
    YXApplication app(&argc, &argv);

    if (argc == 2) {
        FileView *view = new FileView(argv[1]);
        view->show();

        return app.mainLoop();
    }

    printf(_("Usage: %s FILENAME\n\n"
             "A very simple HTML browser displaying the document specified "
             "by FILENAME.\n\n"),
           YApplication::Name);
    return 1;
}

void FileView::loadFile() {
    FILE *fp = fopen(fPath, "r");
    if (fp == 0) {
        warn(_("Invalid path: %s\n"), fPath);
        root = new node(node::div);
        node * last(NULL);

        node * txt(new node(node::text));
        txt->txt = _("Invalid path: ");
        last = add(&root, NULL, txt);

        txt = new node(node::text);
        txt->txt = fPath;
        last = add(&root, last, txt);

        view->setData(root);
        return ;
    }
    if (fp) {
        node *nextsub = 0;
        node::node_type close_type = node::unknown;
        root = parse(fp, 0, 0, nextsub, close_type);
        assert(nextsub == 0);
#ifdef DUMP
        dump_tree(0, root);
#endif
        view->setData(root);
    }
    fclose(fp);
}

void dump_tree(int level, node *n) {
    while (n) {
        printf("%*s<%s>\n", level, "", node::to_string(n->type));
        if (n->container) {
            dump_tree(level + 1, n->container);
            printf("%*s</%s>\n", level, "", node::to_string(n->type));
        }
        n = n->next;
    }
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
