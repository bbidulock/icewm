#include "config.h"
#include "ylib.h"
#include <X11/Xatom.h>
#include "ywindow.h"
#include "ytopwindow.h"
#include "yscrollbar.h"
#include "yscrollview.h"
#include "ymenu.h"
#include "yapp.h"
#include "yaction.h"
//#include "wmmgr.h"
#include "sysdep.h"
#include <dirent.h>

#include "MwmUtil.h"
#include "WinMgr.h"

#define NO_KEYBIND
//#include "bindkey.h"
#include "prefs.h"
#define CFGDEF
//#include "bindkey.h"
#include "default.h"

class ObjectList;
class ObjectIconView;

YIcon *folder = 0;
YIcon *file = 0;

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
    virtual YIcon *getIcon();

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

    virtual void configure(int x, int y, unsigned int width, unsigned int height);

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);

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
    YFont *font;
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
YIcon *YIconItem::getIcon() { return 0; }

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
        fItems = new (YIconItem *)[fItemCount];
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
    font = YFont::getFont("-b&h-lucida-medium-r-*-*-*-120-*-*-*-*-*-*");
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
    YIconItem *a = fFirst, *n;
    while (a) {
        n = a->getNext();
        delete a;
        a = n;
    }
    delete bg; bg = 0;
}

void YIconView::activateItem(YIconItem * /*item*/) {
}

void YIconView::configure(int x, int y, unsigned int width, unsigned int height) {
    YWindow::configure(x, y, width, height);

    if (layout())
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
        YPixmap *icn = icon->getIcon()->large();
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

void YIconView::paint(Graphics &g, int ex, int ey, unsigned int ew, unsigned int eh) {
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
        YPixmap *icn = icon->getIcon()->large();

        g.drawPixmap(icn,
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
        delete path;
    }
    virtual ~ObjectIconItem() {
        delete [] fName; fName = 0;
    }

    virtual const char *getText() { return fName; }
    bool isFolder() { return fFolder; }
    virtual YIcon *getIcon() { return isFolder() ? folder : file; }


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

class ObjectList: public YTopWindow {
public:
    static int winCount;

    ObjectList(const char *path) {
        setDND(true);
        fIsDesktop = false;
        fPath = newstr(path);
        scroll = new YScrollView(this);
        list = new ObjectIconView(this,
                                 scroll,
                                 scroll);
        scroll->setView(list);
        updateList();
        list->show();
        scroll->show();

        XStoreName(app->display(), handle(), fPath);

        int w = desktop->width();
        int h = desktop->height();

        setGeometry(w / 3, h / 3, w / 3, h / 3);

#if 0
        Pixmap icons[4];
        icons[0] = folder->small()->pixmap();
        icons[1] = folder->small()->mask();
        icons[2] = folder->large()->pixmap();
        icons[3] = folder->large()->mask();
        XChangeProperty(app->display(), handle(),
                        _XA_WIN_ICONS, XA_PIXMAP,
                        32, PropModeReplace,
                        (unsigned char *)icons, 4);
#endif
        winCount++;
    }

    ~ObjectList() {
        delete list; list = 0;
        delete scroll; scroll = 0;
        delete [] fPath; fPath = 0;
        winCount--;
    }

    virtual void handleClose() {
        if (winCount == 1) {
            delete this;
            app->exit(0);
        } else
            delete this;

    }

    void setDesktop(bool isDesktop);
    void updateList();

    virtual void configure(int x, int y, unsigned int width, unsigned int height) {
        YWindow::configure(x, y, width, height);
        scroll->setGeometry(0, 0, width, height);
    }

    char *getPath() { return fPath; }

private:
    ObjectIconView *list;
    YScrollView *scroll;

    char *fPath;
    bool fIsDesktop;
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
        //    execl("./icelist", "icelist", path, 0);
        ObjectList *list = new ObjectList(path);
        list->show();
    } else {
        if (fork() == 0) {
            if (access(path, X_OK) == 0)
                execl(path, path, 0);
            else
                execl("./iceview", "iceview", path, 0);
            exit(1);
        }
    }
    delete path;
}

void ObjectList::setDesktop(bool isDesktop) { // before mapping only!!!
    if (fIsDesktop != isDesktop) {
        fIsDesktop = isDesktop;

        static Atom xa_win_layer = None;
        static Atom xa_win_state = None;

        if (xa_win_layer == None)
            xa_win_layer = XInternAtom(app->display(), XA_WIN_LAYER, False);
        if (xa_win_state == None)
            xa_win_state = XInternAtom(app->display(), XA_WIN_STATE, False);

        if (fIsDesktop) {
            unsigned long layer = WinLayerDesktop;
            unsigned long state = WinStateAllWorkspaces | WinStateMaximizedVert | WinStateMaximizedHoriz;

            XChangeProperty(app->display(),
                            handle(),
                            xa_win_layer,
                            XA_CARDINAL,
                            32, PropModeReplace,
                            (unsigned char *)&layer, 1);

            XChangeProperty(app->display(),
                            handle(),
                            xa_win_state,
                            XA_CARDINAL,
                            32, PropModeReplace,
                            (unsigned char *)&state, 1);

            MwmHints mwm;

            memset(&mwm, 0, sizeof(mwm));
            mwm.flags =
                MWM_HINTS_FUNCTIONS |
                MWM_HINTS_DECORATIONS;
            mwm.functions = 0; //MWM_FUNC_CLOSE;
            mwm.decorations = 0;
                //MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE;

            setMwmHints(mwm);
        } else {
            //XDeleteProperty(app->display(), handle(), xa_win_layer);
            //XDeleteProperty(app->display(), handle(), xa_win_layer);
        }
    }
}

void usage() {
    fprintf(stderr, "iceicon [--desktop] directory\n");
    exit(1);
}

int main(int argc, char **argv) {
    YApplication app("iceicon", &argc, &argv);
    bool isDesktop = false;
    const char *dir = 0;

    for (int a = 1; a < argc; a++)
        if (strcmp(argv[a], "--desktop") == 0)
            isDesktop = true;
        else if (strcmp(argv[a], "--help") == 0)
            usage();
        else if (dir == 0)
            dir = argv[a];

    folder = app.getIcon("folder");
    file = app.getIcon("file");

    if (dir == 0)
        dir = getenv("HOME");
    if (dir == 0)
        dir = "/";

    ObjectList *list = new ObjectList(dir);
    list->setDesktop(isDesktop);
    list->show();

    int rc = app.mainLoop();

    //delete folder;
    //delete file;

    return rc;
}
