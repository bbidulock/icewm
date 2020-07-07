#include "config.h"
#include "yfull.h"
#include <X11/Xatom.h>
#include "ylistbox.h"
#include "yscrollview.h"
#include "ymenu.h"
#include "yxapp.h"
#include "yaction.h"
#include "yinputline.h"
#include "wmmgr.h"
#include "yrect.h"
#include "ypaint.h"
#include "sysdep.h"
#include "yicon.h"
#include "udir.h"
#include "ylocale.h"
#include <dirent.h>

#include "intl.h"

const char *ApplicationName = "icelist";
static ref<YPixmap> listbackPixmap;
static ref<YImage> listbackPixbuf;

class ObjectList;
class ObjectListBox;

static ref<YIcon> folder;
static ref<YIcon> file;

class ObjectListItem: public YListItem {
public:
    ObjectListItem(char *container, mstring name):
        fPath(upath(container) + name),
        fName(name),
        fFolder(fPath.dirExists())
    {
    }
    virtual ~ObjectListItem() { }

    virtual mstring getText() { return fName; }
    bool isFolder() { return fFolder; }
    virtual ref<YIcon> getIcon() { return isFolder() ? folder : file; }

    const char* getLocation() { return fPath.string(); }
private:
    upath fPath;
    mstring fName;
    bool fFolder;
};

class ObjectListBox: public YListBox, public YActionListener {
public:
    ObjectListBox(ObjectList *list, YScrollView *view, YWindow *aParent):
        YListBox(view, aParent)
    {
        fObjList = list;

        YMenu *openMenu = new YMenu();
        openMenu->addItem(_("List View"), 0, null, actionOpenList);
        openMenu->addItem(_("Icon View"), 0, null, actionOpenIcon);

        folderMenu = new YMenu();
        folderMenu->setActionListener(this);
        folderMenu->addItem(_("Open"), 0, actionOpenList, openMenu);
    }

    virtual ~ObjectListBox();

    virtual bool handleKey(const XKeyEvent &key) {
        return YListBox::handleKey(key);
    }

    virtual void handleClick(const XButtonEvent &up, int count) {
        if (up.button == 3 && count == 1) {
            YMenu *menu = folderMenu;
            menu->popup(this, 0, 0, up.x_root, up.y_root,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
            return ;
        } else
            return YListBox::handleClick(up, count);
    }

    virtual void activateItem(YListItem *item);

    virtual void actionPerformed(YAction action, unsigned int /*modifiers*/) {
        if (action == actionOpenList) {
        }
    }
private:
    ObjectList *fObjList;
    YMenu *folderMenu;
    YAction actionClose;
    YAction actionOpen;
    YAction actionOpenList;
    YAction actionOpenIcon;
};

class ObjectList: public YWindow {
public:
    static int winCount;

    ObjectList(const char *path, YWindow *aParent): YWindow(aParent) {
        setDND(true);
        fPath = newstr(path);
        scroll = new YScrollView(this);
        list = new ObjectListBox(this, scroll, scroll);
        scroll->setView(list);
        updateList();
        list->show();
        scroll->show();

        setTitle(fPath);

        int w = desktop->width();
        int h = desktop->height();

        setGeometry(YRect(w / 3, h / 3, w / 3, h / 3));

/*
        Pixmap icons[4];
        icons[0] = folder->small()->pixmap();
        icons[1] = folder->small()->mask();
        icons[2] = folder->large()->pixmap();
        icons[3] = folder->large()->mask();
        XChangeProperty(xapp->display(), handle(),
                        _XA_WIN_ICONS, XA_PIXMAP,
                        32, PropModeReplace,
                        (unsigned char *)icons, 4);
*/
        winCount++;

    }

    ~ObjectList() {
        winCount--;

        while (list->getFirst()) {
            ObjectListItem* item =
                static_cast<ObjectListItem *>(list->getFirst());
            list->removeItem(item);
            delete item;
        }
        delete list;
        delete scroll;
        delete[] fPath;
    }

    virtual void handleClose() {
        if (winCount == 1)
            xapp->exit(0);
        delete this;
    }

    void updateList();

    virtual void configure(const YRect &r) {
        YWindow::configure(r);
        scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
    }

    char *getPath() { return fPath; }

private:
    ObjectListBox *list;
    YScrollView *scroll;

    char *fPath;
};
int ObjectList::winCount = 0;

void ObjectList::updateList() {
    adir dir(fPath);
    YArray<ObjectListItem *> file;
    while (dir.next()) {
        ObjectListItem *o = new ObjectListItem(fPath, dir.entry());
        if (o) {
            if (o->isFolder())
                list->addItem(o);
            else
                file += o;
        }
    }
    YArray<ObjectListItem *>::IterType o = file.iterator();
    while (++o) {
        list->addItem(o);
    }
}

ObjectListBox::~ObjectListBox() {
    delete folderMenu;
}

void ObjectListBox::activateItem(YListItem *item) {
    ObjectListItem *obj = (ObjectListItem *)item;
    const char* path = obj->getLocation();

    if (obj->isFolder()) {
        //if (fork() == 0)
        //    execl("./icelist", "icelist", path, NULL);
        ObjectList *list = new ObjectList(path, 0);
        list->show();
    } else {
        if (fork() == 0)
            execl("./iceview", "iceview", path, (void *)NULL);
    }
}

class Panes;

#define NPANES 4
#define TH 20

class Pane: public YWindow {
public:
    Pane(const char *title, const char *path, Panes *aParent);
    ~Pane() {
        delete list;
        free(title);
    }

    virtual void paint(Graphics &g, const YRect &r);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);

    virtual void configure(const YRect &r) {
        YWindow::configure(r);
        list->setGeometry(YRect(0, TH, r.width(), r.height() - TH));
    }
private:
    YColorName titleBg;
    YColorName titleFg;
    YColorName bg;

    char *title;
    int dragY;
    ObjectList *list;
    Panes *owner;
};

class Panes: public YWindow {
public:
    Pane *panes[NPANES];

    Panes(YWindow *aParent = 0);

    ~Panes() {
        for (int i = 0; i < NPANES; i++)
            delete panes[i];
    }

    virtual void handleClose() {
        //if (winCount == 1)
        xapp->exit(0);
    }

    virtual void configure(const YRect &r) {
        YWindow::configure(r);

        for (int i = 0; i < NPANES; i++)
            panes[i]->setSize(r.width(), panes[i]->height());
        panes[NPANES - 1]->setSize(r.width(),
                                   r.height() - panes[NPANES - 1]->y());
    }

    void movePane(Pane *pane, int delta);
};


Pane::Pane(const char *atitle, const char *path, Panes *aParent):
    YWindow(aParent),
    titleBg("#6666CC"),
    titleFg("#FFFFFF"),
    bg("#CCCCCC")
{
    title = strdup(atitle);
    list = new ObjectList(path, this);
    list->show();
    owner = aParent;
}

void Pane::paint(Graphics &g, const YRect &/*r*/) {
    g.setColor(titleBg);
    g.fillRect(0, 0, width(), TH);
    g.setColor(titleFg);
/// !!!    g.drawPixmap(folder->small(), 2, 4);
    g.drawChars(title, 0, strlen(title), 20, 17);
    //g.setColor(bg);
    //g.fillRect(0, TH, width(), height() - TH);
}



void Pane::handleButton(const XButtonEvent &button) {
    dragY = button.y_root - y();
}

void Pane::handleMotion(const XMotionEvent &motion) {
    owner->movePane(this, motion.y_root - dragY);
}

Panes::Panes(YWindow *aParent): YWindow(aParent) {
    panes[0] = new Pane("/", "/", this);
    panes[1] = new Pane("Home", getenv("HOME"), this);
    panes[2] = new Pane("Windows", "wmwinlist:", this);
    panes[3] = new Pane("tmp", "/tmp", this);

    int w = 200, h = 0;
    int height = 600, h1 = height / (NPANES + 1);
    for (int i = 0; i < NPANES; i++) {
        panes[i]->setGeometry(YRect(0, h, w, h1));
        h += h1;
        panes[i]->show();
    }
}

void Panes::movePane(Pane *pane, int delta) {
    for (int i = 1; i < NPANES; i++) {
        int oldY = pane->y();
        if (panes[i] == pane) {
            int min = TH * i;
            int max = height() - TH * (NPANES - i - 1);

            if (delta < min)
                delta = min;
            if (delta > max - TH)
                delta = max - TH;
            int ob = pane->y() + pane->height();
            int b = ob - delta;
            if (b < TH)
                b = TH;
            pane->setGeometry(YRect(pane->x(), delta, pane->width(), b));
        }
        if (pane->y() < oldY) {
            int bottom = pane->y();
            int n = i - 1;
            do {
                if (panes[n]->y() + TH > bottom) {
                    panes[n]->setGeometry(YRect(panes[n]->x(),
                                                bottom - TH,
                                                panes[n]->width(),
                                                TH));
                    bottom -= TH;
                } else {
                    panes[n]->setSize(panes[n]->width(),
                                      bottom - panes[n]->y());
                    break;
                }
                n--;
            } while (n > 0);
        } else if (pane->y() > oldY) {
            int top = pane->y() + pane->height();
            int n = i + 1;

            panes[i - 1]->setSize(panes[i - 1]->width(),
                                  pane->y() - panes[i - 1]->y());
            while (n < NPANES) {
                if (panes[n]->y() < top && panes[n]->height() == TH) {
                    panes[n]->setPosition(panes[n]->x(),
                                          top);
                    top += TH;
                } else if (panes[n]->y() < top) {
                    panes[n]->setGeometry(
                        YRect(panes[n]->x(),
                              top,
                              panes[n]->width(),
                              panes[n]->y() + panes[n]->height() - top));
                    break;
                }
                n++;
            }
        }
    }
}

class IceList : public YWindow {
private:
    Panes* p;
    YInputLine *input;
    ref<YPixmap> large;
public:
    IceList() : YWindow() {
        p = new Panes(this);
        p->setGeometry(YRect(0, 0, 300, 800));
        p->show();

        input = new YInputLine(this);
        input->setGeometry(YRect(0, p->height(), p->width(), 25));
        input->setText("http://slashdot.org/", false);
        input->show();

        setGeometry(YRect(0, 0, p->width(), input->y() + input->height()));

        ref<YIcon> file = YIcon::getIcon("icewm");
        if (file != null) {
            unsigned depth = xapp->depth();
            large = YPixmap::createFromImage(file->large(), depth);
        }

        static char wm_clas[] = "IceWM";
        static char wm_name[] = "icelist";
        XClassHint class_hint = { wm_name, wm_clas };
        XSizeHints size_hints = { PSize,
            0, 0, int(width()), int(height())
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
                             wm_name, ApplicationName, nullptr, 0,
                             &size_hints, &wmhints, &class_hint);

        setNetPid();
        show();
    }
    ~IceList() {
        delete p;
        delete input;
    }

    virtual bool handleKey(const XKeyEvent &key) {
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

    virtual void handleClose() {
        xapp->exit(0);
    }

};

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    YXApplication app(&argc, &argv);

    folder = YIcon::getIcon("folder");
    file = YIcon::getIcon("file");

    //ObjectList *list = new ObjectList(argv[1] ? argv[1] : (char *)"/", 0);
    //list->show();

    IceList icelist;

    return app.mainLoop();
}

// vim: set sw=4 ts=4 et:
