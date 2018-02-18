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
#include "ylocale.h"
#include <dirent.h>

#include "intl.h"

const char *ApplicationName = "icelist";
ref<YPixmap> listbackPixmap;
ref<YImage> listbackPixbuf;

class ObjectList;
class ObjectListBox;

ref<YIcon> folder;
ref<YIcon> file;

class ObjectListItem: public YListItem {
public:
    ObjectListItem(char *container, ustring name): fName(name) {
        fContainer = container;
        fFolder = false;

        struct stat sb;
        char *path = getLocation();
        if (lstat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
            fFolder = true;
        delete[] path;
    }
    virtual ~ObjectListItem() { }

    virtual ustring getText() { return fName; }
    bool isFolder() { return fFolder; }
    virtual ref<YIcon> getIcon() { return isFolder() ? folder : file; }


    char *getLocation();
private:
    char *fContainer;
    ustring fName;
    bool fFolder;
};

char *ObjectListItem::getLocation() {
    char *dir = fContainer;
    ustring name = getText();
    int dlen;
    int nlen = (dlen = strlen(dir)) + 1 + name.length() + 1;
    char *npath;

    npath = new char[nlen];
    memcpy(npath, dir, dlen + 1);
    if (dlen == 0 || dir[dlen - 1] != '/') {
        strcpy(npath + dlen, "/");
        dlen++;
    }
    cstring cs(name);
    strcpy(npath + dlen, cs.c_str());
    return npath;
}

class ObjectListBox: public YListBox, public YActionListener {
public:
    ObjectListBox(ObjectList *list, YScrollView *view, YWindow *aParent): YListBox(view, aParent) {
        fObjList = list;

        YMenu *openMenu = new YMenu();
        openMenu->addItem(_("List View"), 0, null, actionOpenList);
        openMenu->addItem(_("Icon View"), 0, null, actionOpenIcon);

        folderMenu = new YMenu();
        folderMenu->setActionListener(this);
        folderMenu->addItem(_("Open"), 0, actionOpenList, openMenu);
    }

    virtual ~ObjectListBox() { }

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
        list = new ObjectListBox(this,
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

    ~ObjectList() { winCount--; }

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
    DIR *dir;

    if ((dir = opendir(fPath)) != NULL) {
        struct dirent *de;

        while ((de = readdir(dir)) != NULL) {
            char *n = de->d_name;

            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0)))
                ;
            else {
                ObjectListItem *o = new ObjectListItem(fPath, n);

                if (o)
                    list->addItem(o);
            }
        }
        closedir(dir);
    }
}

void ObjectListBox::activateItem(YListItem *item) {
    ObjectListItem *obj = (ObjectListItem *)item;
    char *path = obj->getLocation();

    if (obj->isFolder()) {
        //if (fork() == 0)
        //    execl("./icelist", "icelist", path, NULL);
        ObjectList *list = new ObjectList(path, 0);
        list->show();
    } else {
        if (fork() == 0)
            execl("./iceview", "iceview", path, (void *)NULL);
    }
    delete path;
}

class Panes;

#define NPANES 4
#define TH 20

class Pane: public YWindow {
public:
    Pane(const char *title, const char *path, Panes *aParent);

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

    virtual void handleClose() {
        //if (winCount == 1)
        xapp->exit(0);
        delete this;
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

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    YXApplication app(&argc, &argv);
    YWindow *w;

    folder = YIcon::getIcon("folder");
    file = YIcon::getIcon("file");

    //ObjectList *list = new ObjectList(argv[1] ? argv[1] : (char *)"/", 0);
    //list->show();

    w = new YWindow();

    Panes *p = new Panes(w);
    p->setGeometry(YRect(0, 0, 200, 700));
    p->show();


    YInputLine *input = new YInputLine(w);
    input->setGeometry(YRect(0, 700, 200, 20));
    input->setText("http://slashdot.org/");
    input->show();

    w->setGeometry(YRect(0, 0, 200, 720));
    w->show();

    return app.mainLoop();
}

// vim: set sw=4 ts=4 et:
