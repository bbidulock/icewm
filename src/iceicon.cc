#include "config.h"
#include "ylib.h"
#include <X11/Xatom.h>
#include "ywindow.h"
#include "yscrollbar.h"
#include "yscrollview.h"
#include "ymenu.h"
#include "yxapp.h"
#include "yaction.h"
#include "wmmgr.h"
#include "ypixbuf.h"
#include "yrect.h"
#include "sysdep.h"
#include "ylocale.h"
#include "yrect.h"
#include "yicon.h"
#include <dirent.h>
#include "intl.h"
#include "yprefs.h"

char const *ApplicationName = "iceicon";

class ObjectList;
class ObjectIconView;

ref<YIcon> folder;
ref<YIcon> file;

class YScrollView;

class YIconItem {
public:
    YIconItem();
    virtual ~YIconItem();

    YIconItem *getNext();
    YIconItem *getPrev();
    void setNext(YIconItem *next);
    void setPrev(YIconItem *prev);

    virtual const char *getText();
    virtual ref<YIcon> getIcon();

    int x, y, w, h;
    int ix;
    int iy;
    int tx;
    int ty;
private:
    YIconItem *fPrevItem, *fNextItem;
};

class YIconView: public YWindow, public YScrollBarListener, public YScrollable {
public:
    YIconView(YScrollView *view, YWindow *aParent);
    virtual ~YIconView();

    int addItem(YIconItem *item);
    //int addAfter(YIconItem *prev, YIconItem *item);
    //void removeItem(YIconItem *item);

    virtual void configure(const YRect &r, bool resized);

    virtual void paint(Graphics &g, const YRect &r);

    void setPos(int x, int y);
    virtual void scroll(YScrollBar *sb, int delta);
    virtual void move(YScrollBar *sb, int pos);

    void handleClick(const XButtonEvent &up, int count);

    YIconItem *getFirst() const { return fFirst; }
    YIconItem *getLast() const { return fLast; }

    int getItemCount();
    YIconItem *getItem(int item);
    YIconItem *findItemByPoint(int x, int y);
    int findItem(YIconItem *item);

    virtual int contentWidth();
    virtual int contentHeight();
    virtual YWindow *getWindow();

    virtual void activateItem(YIconItem *item);
    bool layout();

private:
    YScrollBar *fVerticalScroll;
    YScrollBar *fHorizontalScroll;
    YScrollView *fView;
    YIconItem *fFirst, *fLast;
    int fItemCount;
    YIconItem **fItems;

    int fOffsetX;
    int fOffsetY;

    int conWidth;
    int conHeight;

    void resetScrollBars();
    void freeItems();
    void updateItems();

    YColor *bg, *fg;
    ref<YFont> font;
    int fontWidth, fontHeight;
};

YIconItem::YIconItem() {
    fPrevItem = fNextItem = 0;
    x = y = w  = h = 0;
    tx = ty = ix = iy = 0;
}

YIconItem::~YIconItem() {
}

YIconItem *YIconItem::getNext() {
    return fNextItem;
}

YIconItem *YIconItem::getPrev() {
    return fPrevItem;
}

void YIconItem::setNext(YIconItem *next) {
    fNextItem = next;
}

void YIconItem::setPrev(YIconItem *prev) {
    fPrevItem = prev;
}


const char *YIconItem::getText() { return 0; }
ref<YIcon> YIconItem::getIcon() { return null; }

int YIconView::addItem(YIconItem *item) {
    PRECONDITION(item->getPrev() == 0);
    PRECONDITION(item->getNext() == 0);

    freeItems();

    item->setNext(0);
    item->setPrev(fLast);
    if (fLast)
        fLast->setNext(item);
    else
        fFirst = item;
    fLast = item;
    fItemCount++;
    return 1;
}

void YIconView::freeItems() {
    if (fItems) {
        delete fItems; fItems = 0;
    }
}

void YIconView::updateItems() {
    if (fItems == 0) {
        //fMaxWidth = 0;
        fItems = new YIconItem *[fItemCount];
        if (fItems) {
            YIconItem *a = getFirst();
            int n = 0;
            while (a) {
                fItems[n++] = a;

                /*int cw = 3 + 20 + a->getOffset();
                if (listBoxFont) {
                    const char *t = a->getText();
                    if (t)
                        cw += listBoxFont->textWidth(t) + 3;
                }
                if (cw > fMaxWidth)
                    fMaxWidth = cw;*/

                a = a->getNext();
            }
        }
    }
}

YIconView::YIconView(YScrollView *view, YWindow *aParent): YWindow(aParent) {
    fView = view;

    bg = new YColor("rgb:CC/CC/CC");
    fg = YColor::black; //new YColor("rgb:00/00/00");
    font = YFont::getFont("-b&h-lucida-medium-r-*-*-*-120-*-*-*-*-*-*", "monospace:size=10");
    fontWidth = font->textWidth("M");
    fontHeight = font->height();

    if (fView) {
        fVerticalScroll = view->getVerticalScrollBar();;
        fHorizontalScroll = view->getHorizontalScrollBar();
    } else {
        fHorizontalScroll = 0;
        fVerticalScroll = 0;
    }
    if (fVerticalScroll)
        fVerticalScroll->setScrollBarListener(this);
    if (fHorizontalScroll)
        fHorizontalScroll->setScrollBarListener(this);
    fOffsetX = fOffsetY = 0;
    fItems = 0;
    fItemCount = 0;
    fFirst = fLast = 0;
    setBitGravity(NorthWestGravity);
}

YIconView::~YIconView() {
}

void YIconView::activateItem(YIconItem */*item*/) {
}

void YIconView::configure(const YRect &r, const bool resized) {
    YWindow::configure(r, resized);

    if (resized && layout())
        repaint();
}

bool YIconView::layout() {
    int sw = this->width();

    int cx = 0;
    int cy = 0;
    int thisLine = 0;
    bool layoutChanged = false;

    conWidth = 0;
    conHeight = 0;
    thisLine = 0;
    YIconItem *icon = getFirst();
    while (icon) {
        const char *text = icon->getText();
        int tw = font->textWidth(text) + 4;
        int th = fontHeight + 2;
        ref<YImage> icn = icon->getIcon()->large();
        int iw = icn->width() + 4;
        int ih = icn->height() + 4;

        int tx, ty, ix, iy;

        ty = ih;
        int aw = tw;
        if (iw > aw) aw = iw;
        if (aw < 40) aw = 40;
        aw |= 0xF;
        ix = (aw - iw) / 2;
        tx = (aw - tw) / 2;
        iy = 0;
        int ah = ih + th;

        if ((cx + aw > sw) && (thisLine > 0)) {
            cx = 0;
            cy += ah;
            thisLine = 0;
        }

        if (icon->x != cx || icon->y != cy)
            layoutChanged = true;
        icon->x = cx;
        icon->y = cy;
        icon->w = aw;
        icon->h = ah;
        icon->tx = tx;
        icon->ty = ty;
        icon->ix = ix;
        icon->iy = iy;

        cx += aw;
        thisLine++;

        if (cx > conWidth)
            conWidth = cx;
        icon = icon->getNext();
        conHeight = cy + ah;
    }
    resetScrollBars();
    return layoutChanged;
}

void YIconView::paint(Graphics &g, const YRect &r) {
    int ex = r.x(), ey = r.y(), ew = r.width(), eh = r.height();
    g.setColor(bg);
    g.fillRect(ex, ey, ew, eh);
    g.setColor(fg);
    g.setFont(font);

    YIconItem *icon = getFirst();
    while (icon) {
        if ((icon->y + icon->h - fOffsetY) >= ey)
            break;
        icon = icon->getNext();
    }

    while (icon) {
        if ((icon->y - fOffsetY) > (ey + int(eh)))
            break;

        const char *text = icon->getText();
        ref<YImage> icn = icon->getIcon()->large();

        g.drawImage(icn,
                     icon->x - fOffsetX + icon->ix + 2,
                     icon->y - fOffsetY + icon->iy + 2);
        g.drawChars(text, 0, strlen(text),
                    icon->x - fOffsetX + icon->tx,
                    icon->y - fOffsetY + icon->ty + font->ascent() + 1);

        icon = icon->getNext();
    }
}

void YIconView::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 1 && count == 1) {
        YIconItem *i = findItemByPoint(up.x + fOffsetX, up.y + fOffsetY);

        if (i)
            activateItem(i);
    }
}

YIconItem *YIconView::findItemByPoint(int x, int y) {
    YIconItem *icon = getFirst();

    while (icon) {
        if (x >= icon->x && x < icon->x + icon->w &&
            y >= icon->y && y < icon->y + icon->h)
        {
            return icon;
        }
        icon = icon->getNext();
    }
    return 0;
}

void YIconView::setPos(int x, int y) {
    if (x != fOffsetX || y != fOffsetY) {
        int dx = x - fOffsetX;
        int dy = y - fOffsetY;

        fOffsetX = x;
        fOffsetY = y;

        scrollWindow(dx, dy);
    }
}

void YIconView::scroll(YScrollBar *sb, int delta) {
    if (sb == fHorizontalScroll)
        setPos(fOffsetX + delta, fOffsetY);
    else if (sb == fVerticalScroll)
        setPos(fOffsetX, fOffsetY + delta);
}

void YIconView::move(YScrollBar *sb, int pos) {
    if (sb == fHorizontalScroll)
        setPos(pos, fOffsetY);
    else if (sb == fVerticalScroll)
        setPos(fOffsetX, pos);
}

void YIconView::resetScrollBars() {
    fVerticalScroll->setValues(fOffsetY, height(), 0, conHeight);
    fVerticalScroll->setBlockIncrement(height());
    fVerticalScroll->setUnitIncrement(32);
    fHorizontalScroll->setValues(fOffsetX, width(), 0, conWidth);
    fHorizontalScroll->setBlockIncrement(width());
    fHorizontalScroll->setUnitIncrement(32);
    if (fView)
        fView->layout();
}

int YIconView::contentWidth() {
    return conWidth;
}

int YIconView::contentHeight() {
    return conHeight;
}

YWindow *YIconView::getWindow() {
    return this;
}

class ObjectIconItem: public YIconItem {
public:
    ObjectIconItem(char *container, char *name) {
        fContainer = container;
        fName = newstr(name);
        fFolder = false;

        struct stat sb;
        char *path = getLocation();
        if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
            fFolder = true;
        delete[] path;
    }
    virtual ~ObjectIconItem() { delete[] fName; fName = 0; }

    virtual const char *getText() { return fName; }
    bool isFolder() { return fFolder; }
    virtual ref<YIcon> getIcon() { return isFolder() ? folder : file; }


    char *getLocation();
private:
    char *fContainer;
    char *fName;
    bool fFolder;
};

char *ObjectIconItem::getLocation() {
    char *dir = fContainer;
    char *name = (char *)getText();
    int dlen;
    int nlen = (dlen = strlen(dir)) + 1 + strlen(name) + 1;
    char *npath;

    npath = new char[nlen];
    strcpy(npath, dir);
    if (dlen == 0 || dir[dlen - 1] != '/') {
        strcpy(npath + dlen, "/");
        dlen++;
    }
    strcpy(npath + dlen, name);
    return npath;
}

class ObjectIconView: public YIconView {
public:
    ObjectIconView(ObjectList *list, YScrollView *view, YWindow *aParent): YIconView(view, aParent) {
        fObjList = list;
    }

    virtual ~ObjectIconView() { }
    virtual void activateItem(YIconItem *item);

private:
    ObjectList *fObjList;
};

class ObjectList: public YWindow {
public:
    static int winCount;

    ObjectList(char *path) {
        setDND(true);
        fPath = newstr(path);
        scroll = new YScrollView(this);
        list = new ObjectIconView(this,
                                 scroll,
                                 scroll);
        scroll->setView(list);
        updateList();
        list->show();
        scroll->show();

        setTitle(fPath);

        int w = desktop->width();
        int h = desktop->height();

        setGeometry(YRect(w / 3, h / 3, w / 3, h / 3));

/// TODO         #warning boo!
/*
        Pixmap icons[4];
        icons[0] = folder->small()->pixmap();
        icons[1] = folder->small()->mask();
        icons[2] = folder->large()->pixmap();
        icons[3] = folder->large()->mask();
        XChangeProperty(app->display(), handle(),
                        _XA_WIN_ICONS, XA_PIXMAP,
                        32, PropModeReplace,
                        (unsigned char *)icons, 4);
*/
        winCount++;
    }

    ~ObjectList() { winCount--; }

    virtual void handleClose() {
        if (winCount == 1)
            app->exit(0);
        delete this;
    }

    void updateList();

    virtual void configure(const YRect &r, const bool resized) {
        YWindow::configure(r, resized);
        if (resized) scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
    }

    char *getPath() { return fPath; }

private:
    ObjectIconView *list;
    YScrollView *scroll;

    char *fPath;
};
int ObjectList::winCount = 0;

void ObjectList::updateList() {
    DIR *dir;

    if ((dir = opendir(fPath)) != NULL) {
        struct dirent *de;

        while ((de = readdir(dir)) != NULL) {
            char *n = de->d_name;

            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0)))
                ;
            else {
                ObjectIconItem *o = new ObjectIconItem(fPath, n);

                if (o)
                    list->addItem(o);
            }
        }
        closedir(dir);
    }
    if (list->layout())
        list->repaint();
}

void ObjectIconView::activateItem(YIconItem *item) {
    ObjectIconItem *obj = (ObjectIconItem *)item;
    char *path = obj->getLocation();

    if (obj->isFolder()) {
        //if (fork() == 0)
        //    execl("./icelist", "icelist", path, NULL);
        ObjectList *list = new ObjectList(path);
        list->show();
    } else {
        if (fork() == 0)
            execl("./iceview", "iceview", path, (void *)NULL);
    }
    delete path;

}

int main(int argc, char **argv) {
    YLocale locale;

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);
#endif

    YXApplication app(&argc, &argv);

    folder = YIcon::getIcon("folder");
    file = YIcon::getIcon("file");

    ObjectList *list = new ObjectList(argv[1] ? argv[1] : (char *)"/");
    list->show();

    return app.mainLoop();
}
