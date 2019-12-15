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
#include "yrect.h"
#include "sysdep.h"
#include "upath.h"
#include "udir.h"
#include "yarray.h"
#include "ylocale.h"
#include "yrect.h"
#include "yicon.h"
#include <dirent.h>
#include "intl.h"
#include "yprefs.h"

char const *ApplicationName = "iceicon";

class ObjectList;
class ObjectIconView;

static ref<YIcon> folder;
static ref<YIcon> file;
static int textLimit;
static bool hugeIcons;

class YIconItem : public YListNode<YIconItem> {
public:
    YIconItem();
    virtual ~YIconItem() {}

    virtual const char *getText() = 0;
    virtual ref<YIcon> getIcon() = 0;
    virtual bool isFolder() = 0;

    int x, y, w, h;
    int ix;
    int iy;
    int tx;
    int ty;
};

class YIconView: public YWindow, public YScrollBarListener, public YScrollable {
public:
    YIconView(YScrollView *view, YWindow *aParent);
    virtual ~YIconView();

    int addItem(YIconItem *item);
    //int addAfter(YIconItem *prev, YIconItem *item);
    //void removeItem(YIconItem *item);

    virtual void configure(const YRect &r);

    virtual void paint(Graphics &g, const YRect &r);

    void setPos(int x, int y);
    virtual void scroll(YScrollBar *sb, int delta);
    virtual void move(YScrollBar *sb, int pos);

    void handleButton(const XButtonEvent& up);
    void handleClick(const XButtonEvent& up, int count);
    bool handleKey(const XKeyEvent& key);

    int getItemCount();
    YIconItem *getItem(int item);
    YIconItem *findItemByPoint(int x, int y);
    int findItem(YIconItem *item);

    virtual unsigned contentWidth();
    virtual unsigned contentHeight();
    virtual YWindow *getWindow();

    virtual void activateItem(YIconItem *item);
    bool layout();

private:
    YScrollBar *fVerticalScroll;
    YScrollBar *fHorizontalScroll;
    YScrollView *fView;
    YList<YIconItem> fList;
    YIconItem **fItems;

    int fOffsetX;
    int fOffsetY;

    unsigned conWidth;
    unsigned conHeight;

    void resetScrollBars();
    void freeItems();
    void updateItems();

    YColorName bg, fg;
    ref<YFont> font;
    int fontWidth, fontHeight;
};

YIconItem::YIconItem() {
    x = y = w  = h = 0;
    tx = ty = ix = iy = 0;
}

int YIconView::addItem(YIconItem *item) {
    freeItems();
    fList.append(item);
    return 1;
}

void YIconView::freeItems() {
    if (fItems) {
        delete[] fItems;
        fItems = nullptr;
    }
}

void YIconView::updateItems() {
    if (fItems == nullptr) {
        //fMaxWidth = 0;
        int fItemCount = fList.count();
        fItems = new YIconItem *[fItemCount];
        if (fItems) {
            for (int k = 0, n = 0; k < 2 && n < fItemCount; ++k) {
                YListIter<YIconItem> iter = fList.iterator();
                while (++iter) {
                    if (k == 0 && iter->isFolder()) {
                        fItems[n++] = iter;
                    }
                    if (k == 1 && !iter->isFolder()) {
                        fItems[n++] = iter;
                    }

                    /*int cw = 3 + 20 + a->getOffset();
                    if (listBoxFont) {
                        const char *t = a->getText();
                        if (t)
                            cw += listBoxFont->textWidth(t) + 3;
                    }
                    if (cw > fMaxWidth)
                        fMaxWidth = cw;*/
                }
            }
        }
    }
}

YIconView::YIconView(YScrollView *view, YWindow *aParent):
    YWindow(aParent),
    bg("rgb:CC/CC/CC"),
    fg(YColor::black)
{
    fView = view;

    font = YFont::getFont("-b&h-lucida-medium-r-*-*-*-120-*-*-*-*-*-*", "monospace:size=10");
    fontWidth = font->textWidth("M");
    fontHeight = font->height();

    if (fView) {
        fVerticalScroll = view->getVerticalScrollBar();
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
    setBitGravity(NorthWestGravity);
    setDoubleBuffer(true);
}

YIconView::~YIconView() {
    freeItems();
    while (fList) {
        YIconItem* item = fList.front();
        fList.remove(item);
        delete item;
    }
}

void YIconView::activateItem(YIconItem */*item*/) {
}

void YIconView::configure(const YRect &r) {
    YWindow::configure(r);

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

    YListIter<YIconItem> icon = fList.iterator();
    while (++icon) {
        const char *text = icon->getText();
        if (0 < textLimit && textLimit < int(strlen(text))) {
            text += strlen(text) - textLimit;
        }
        int tw = font->textWidth(text) + 4;
        int th = fontHeight + 2;
        ref<YImage> icn =
            hugeIcons ? icon->getIcon()->huge() : icon->getIcon()->large();
        if (icn == null)
            continue;
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

        if (cx > int(conWidth))
            conWidth = unsigned(cx);
        conHeight = cy + ah;
    }
    resetScrollBars();
    return layoutChanged;
}

void YIconView::paint(Graphics &g, const YRect &r) {
    int ex = r.x(), ey = r.y(), ew = int(r.width()), eh = int(r.height());
    g.setColor(bg);
    g.fillRect(ex, ey, ew, eh);
    g.setColor(fg);
    g.drawRect(ex + 1, ey + 1, ew - 2, eh - 2);
    g.setFont(font);

    msg("fOffsetY %d, value %d, amount %d\n", fOffsetY,
            fVerticalScroll->getValue(),
            fVerticalScroll->getVisibleAmount());

    YListIter<YIconItem> icon = fList.iterator();
    while (++icon) {
        if ((icon->y + icon->h - fOffsetY) < ey)
            continue;
        if ((icon->y - fOffsetY) > ey + eh)
            break;

        const char *text = icon->getText();
        if (0 < textLimit && textLimit < int(strlen(text))) {
            text += strlen(text) - textLimit;
        }
        ref<YImage> icn =
            hugeIcons ? icon->getIcon()->huge() : icon->getIcon()->large();
        if (icn == null)
            continue;

        g.drawImage(icn,
                    icon->x - fOffsetX + icon->ix + 2,
                    icon->y - fOffsetY + icon->iy + 2);
        g.drawChars(text, 0, strlen(text),
                    icon->x - fOffsetX + icon->tx,
                    icon->y - fOffsetY + icon->ty + font->ascent() + 1);
    }
}

void YIconView::handleButton(const XButtonEvent& button) {
    if (button.button == Button4 || button.button == Button5) {
        if (fVerticalScroll->handleScrollMouse(button)) {
            return;
        }
    }
    else {
        YWindow::handleButton(button);
    }
}

void YIconView::handleClick(const XButtonEvent &up, int count) {
    if (up.button == Button1 && count == 1) {
        YIconItem *i = findItemByPoint(up.x + fOffsetX, up.y + fOffsetY);
        if (i)
            activateItem(i);
    }
    else if (up.button == Button2) {
        delete fView->parent();
    }
    else if (up.button == Button3) {
        xapp->exit(0);
    }
}

bool YIconView::handleKey(const XKeyEvent& key) {
    if (fVerticalScroll->handleScrollKeys(key) == true) {
        return true;
    }
    if (key.type == KeyPress) {
        KeySym k = keyCodeToKeySym(key.keycode);
        int m = KEY_MODMASK(key.state);
        if (k == XK_Escape) {
            xapp->exit(0);
            return true;
        }
        else if (k == XK_q && hasbit(m, ControlMask)) {
            xapp->exit(0);
            return true;
        }
    }
    return YWindow::handleKey(key);
}

YIconItem *YIconView::findItemByPoint(int x, int y) {
    YListIter<YIconItem> icon = fList.iterator();
    while (++icon) {
        if (x >= icon->x && x < icon->x + icon->w &&
            y >= icon->y && y < icon->y + icon->h)
        {
            return icon;
        }
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

unsigned YIconView::contentWidth() {
    return conWidth;
}

unsigned YIconView::contentHeight() {
    return conHeight;
}

YWindow *YIconView::getWindow() {
    return this;
}

class ObjectIconItem: public YIconItem {
public:
    ObjectIconItem(const char *container, const char *name) :
        fPath(upath(container) + name),
        fName(newstr(name)),
        fFolder(fPath.dirExists())
    {
    }
    virtual ~ObjectIconItem() {
        delete[] fName;
        fName = nullptr;
    }

    virtual const char *getText() { return fName; }
    const char* getLocation() { return fPath.string(); }
    bool isFolder() { return fFolder; }
    virtual ref<YIcon> getIcon();
private:
    upath fPath;
    char *fName;
    bool fFolder;
};

ref<YIcon> ObjectIconItem::getIcon() {
    if (isFolder()) {
        return folder;
    }
    else {
        pstring ext(fPath.getExtension().lower());
        if (ext == ".xpm" || ext == ".png" || ext == ".svg") {
            ref<YIcon> icon = YIcon::getIcon(fPath.string());
            if (icon != null) {
                if (hugeIcons ? icon->huge() != null : icon->large() != null)
                    return icon;
            }
        }
        else if (ext == ".jpg" || ext == ".jpeg") {
            ref<YIcon> icon = YIcon::getIcon(fPath.string());
            if (icon != null) {
                if (hugeIcons ? icon->huge() != null : icon->large() != null)
                    return icon;
            }
        }
        return file;
    }
}

class ObjectIconView: public YIconView {
public:
    ObjectIconView(ObjectList *list, YScrollView *view, YWindow *aParent): YIconView(view, aParent) {
        fObjList = list;
    }

    virtual ~ObjectIconView();
    virtual void activateItem(YIconItem *item);

private:
    ObjectList *fObjList;
    YArray<pid_t> pids;
};

ObjectIconView::~ObjectIconView() {
    YArray<pid_t>::IterType iter = pids.iterator();
    while (++iter) {
        kill(*iter, SIGTERM);
    }
}

class ObjectList: public YWindow {
public:
    static int winCount;

    ObjectList(const char *path) {
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

        YRect geo(w / 3, h / 3, w / 3, h / 3);
        setGeometry(geo);

        ref<YIcon> file = YIcon::getIcon("icewm");
        if (file != null) {
            unsigned depth = xapp->depth();
            large = YPixmap::createFromImage(file->large(), depth);
        }

        winCount++;

        static char wm_clas[] = "IceWM";
        static char wm_name[] = "iceicon";
        XClassHint class_hint = { wm_name, wm_clas };
        XSizeHints size_hints = { PSize,
            0, 0, int(geo.width()), int(geo.height())
        };
        XWMHints wmhints = {
            InputHint | StateHint,
            True,
            NormalState,
            large != null ? large->pixmap() : None, None, 0, 0,
            large != null ? large->mask() : None,
            None
        };
        if (wmhints.icon_pixmap)
            wmhints.flags |= IconPixmapHint;
        if (wmhints.icon_mask)
            wmhints.flags |= IconMaskHint;
        Xutf8SetWMProperties(xapp->display(), handle(),
                             fPath, ApplicationName, nullptr, 0,
                             &size_hints, &wmhints, &class_hint);

        setNetPid();
    }

    ~ObjectList() {
        delete list;
        delete scroll;
        if (--winCount <= 0)
            xapp->exit(0);
    }

    virtual void handleClose() {
        delete this;
    }

    void updateList();

    virtual void configure(const YRect &r) {
        YWindow::configure(r);
        scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
    }

    char *getPath() { return fPath; }

private:
    ObjectIconView *list;
    YScrollView *scroll;

    char *fPath;

    ref<YPixmap> large;
};
int ObjectList::winCount = 0;

void ObjectList::updateList() {
    adir dir(fPath);
    YArray<ObjectIconItem *> file;
    while (dir.next()) {
        ObjectIconItem *o = new ObjectIconItem(dir.path(), dir.entry());
        if (o) {
            if (o->isFolder())
                list->addItem(o);
            else
                file += o;
        }
    }
    YArray<ObjectIconItem *>::IterType o = file.iterator();
    while (++o) {
        list->addItem(o);
    }
    if (list->layout())
        list->repaint();
}

void ObjectIconView::activateItem(YIconItem *item) {
    ObjectIconItem *obj = (ObjectIconItem *)item;
    const char* path = obj->getLocation();

    if (obj->isFolder()) {
        //if (fork() == 0)
        //    execl("./icelist", "icelist", path, NULL);
        ObjectList *list = new ObjectList(path);
        list->show();
    } else {
        pid_t pid = fork();
        if (pid == 0)
            execl("./iceview", "iceview", path, nullptr);
        else if (pid > 1)
            pids += pid;
    }
}

static const char* help() {
    static const char help[] =
    "  -l, --limit=LENGTH  Limit length of filenames to LENGTH.\n"
    "  -H, --Huge          Use huge icon size (48x48).\n"
    "  -L, --Large         Use large icon size (32x32).\n"
    ;
    return help;
}

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    check_argv(argc, argv, help(), VERSION);

    YStringArray names;
    YXApplication app(&argc, &argv);
    for (char ** arg = argv + 1; arg < argv + argc; ++arg) {
        if (**arg == '-') {
            char *value(0);
            if (GetArgument(value, "l", "limit", arg, argv+argc))
                textLimit = strtol(value, nullptr, 0);
            else if (is_switch(*arg, "H", "Huge"))
                hugeIcons = true;
            else if (is_switch(*arg, "L", "Large"))
                hugeIcons = false;
        }
        else {
            names += *arg;
        }
    }

    folder = YIcon::getIcon("folder");
    file = YIcon::getIcon("file");

    if (names.isEmpty())
        names += "/";

    YStringArray::IterType iter = names.iterator();
    while (++iter) {
        if (upath(*iter).isReadable()) {
            ObjectList* list = new ObjectList(*iter);
            if (list) {
                list->show();
            }
        }
    }

    return app.mainLoop();
}

// vim: set sw=4 ts=4 et:
