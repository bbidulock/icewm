#include "config.h"
#include "ylib.h"
#include <X11/Xatom.h>
#include "ylistbox.h"
#include "yscrollview.h"
#include "ymenu.h"
#include "yapp.h"
#include "yaction.h"
#include "yinputline.h"
#include "sysdep.h"
#include <dirent.h>
#include "ycstring.h"

#include "MwmUtil.h"

#include "default.h"
#define CFGDEF
#include "default.h"

class ObjectList;
class ObjectListBox;

YIcon *folder = 0;
YIcon *file = 0;

class ObjectListItem: public YListItem {
public:
    ObjectListItem(char *container, char *name) {
        fContainer = container;
        fName = CStr::newstr(name);
        fFolder = false;

        struct stat sb;
        char *path = getLocation();
        if (lstat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
            fFolder = true;
        delete path;
    }
    virtual ~ObjectListItem() { delete fName; fName = 0; }

    virtual const CStr *getText() { return fName; }
    bool isFolder() { return fFolder; }
    virtual YIcon *getIcon() { return isFolder() ? folder : file; }


    char *getLocation();
private:
    char *fContainer;
    CStr *fName;
    bool fFolder;
};

char *ObjectListItem::getLocation() {
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

class ObjectListBox: public YListBox, public YActionListener {
public:
    ObjectListBox(ObjectList *list, YScrollView *view, YWindow *aParent): YListBox(view, aParent) {
        fObjList = list;

        actionOpenList = new YAction();
        actionOpenIcon = new YAction();
        actionOpen = new YAction();
        actionClose = new YAction();

        YMenu *openMenu = new YMenu();
        openMenu->addItem("List View", 0, 0, actionOpenList);
        openMenu->addItem("Icon View", 0, 0, actionOpenIcon);

        folderMenu = new YMenu();
        folderMenu->setActionListener(this);
        folderMenu->addItem("Open", 0, actionOpenList, openMenu);
    }

    virtual ~ObjectListBox() { }

    virtual bool handleKeySym(const XKeyEvent &key, KeySym ksym, int vmod) {
        return YListBox::handleKeySym(key, ksym, vmod);
    }

    virtual void handleClick(const XButtonEvent &up, int count) {
        if (up.button == 3 && count == 1) {
            YMenu *menu = folderMenu;
            menu->popup(0, 0, up.x_root, up.y_root, -1, -1,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
            return ;
        } else
            return YListBox::handleClick(up, count);
    }

    virtual void activateItem(YListItem *item);

    virtual void actionPerformed(YAction *action, unsigned int /*modifiers*/) {
        if (action == actionOpenList) {
        }
    }
private:
    ObjectList *fObjList;
    YMenu *folderMenu;
    YAction *actionClose;
    YAction *actionOpen;
    YAction *actionOpenList;
    YAction *actionOpenIcon;
};

class ObjectList: public YWindow {
public:
    static int winCount;

    ObjectList(const char *path, YWindow *aParent): YWindow(aParent) {
        fPath = newstr(path);
        scroll = new YScrollView(this);
        list = new ObjectListBox(this,
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

    ~ObjectList() { winCount--; }

    virtual void handleClose() {
        if (winCount == 1)
            app->exit(0);
        delete this;
    }

    void updateList();

    virtual void configure(int x, int y, unsigned int width, unsigned int height) {
        YWindow::configure(x, y, width, height);
        scroll->setGeometry(0, 0, width, height);
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
        //    execl("./icelist", "icelist", path, 0);
        ObjectList *list = new ObjectList(path, 0);
        list->show();
    } else {
        if (fork() == 0) {
            execl("./iceview", "iceview", path, 0);
            exit(1);
        }
    }
    delete path;
}

class Panes;

#define NPANES 4
#define TH 20

class Pane: public YWindow {
public:
    Pane(const char *title, const char *path, Panes *aParent);

    virtual void paint(Graphics &g, int x, int y, unsigned int width, unsigned int height);
    virtual void handleButton(const XButtonEvent &button);
    virtual void handleMotion(const XMotionEvent &motion);

    virtual void configure(int x, int y, unsigned int width, unsigned int height) {
        YWindow::configure(x, y, width, height);
        list->setGeometry(0, TH, width, height - TH);
    }
    bool isOpen;
    int oldSize;
private:
    YColor *titleBg;
    YColor *titleFg;

    YColor *bg;
    char *title;
    int dragY;
    ObjectList *list;
    Panes *owner;
    bool moving;
};

class Panes: public YWindow {
public:
    Pane *panes[NPANES];

    Panes(YWindow *aParent = 0);

    virtual void handleClose() {
        //if (winCount == 1)
        app->exit(0);
        delete this;
    }

    void toggleOpen(Pane *pane);

    virtual void configure(int x, int y, unsigned int width, unsigned int height) {
        Pane *last;

        YWindow::configure(x, y, width, height);
        for (int i = 0; i < NPANES; i++)
            panes[i]->setSize(width, panes[i]->height());

        last = panes[NPANES - 1];

        if (last->y() + 20 <= (int)height) {
            last->setGeometry(0, last->y(), width, height - last->y());
        } else {
            last->setSize(width, 20);
            movePane(last, height - 20);
        }
    }

    void movePane(Pane *pane, int delta);
};


Pane::Pane(const char *atitle, const char *path, Panes *aParent): YWindow(aParent) {
    title = strdup(atitle);
    titleBg = new YColor("#6666CC");
    titleFg = YColor::white;
    bg = new YColor("#CCCCCC");
    list = new ObjectList(path, this);
    list->show();
    owner = aParent;
    isOpen = true;
    dragY = 0;
    moving = false;
}

void Pane::paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
    g.setColor(titleBg);
    g.fillRect(0, 0, width(), TH);
    g.setColor(titleFg);
    if (folder && folder->small())
        g.drawPixmap(folder->small(), 2, 4);
    g.drawChars(title, 0, strlen(title), 20, 17);
    //g.setColor(bg);
    //g.fillRect(0, TH, width(), height() - TH);
}



void Pane::handleButton(const XButtonEvent &button) {
    if (button.button == 1) {
        if ((button.state & ControlMask) && button.type == ButtonPress)
            owner->toggleOpen(this);
        else {
            if (button.type == ButtonPress)
                moving = true;
            else
                moving = false;
            dragY = button.y_root - y();
        }
    } else if (button.button == 3) {
        if (button.type == ButtonPress)
            startDrag(0, NULL);
        else
            endDrag(false);
    }
}

void Pane::handleMotion(const XMotionEvent &motion) {
    if (moving)
        if (motion.state & Button1Mask)
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
        panes[i]->setGeometry(0, h, w, h1); h += h1;
        panes[i]->show();
    }
}

void Panes::toggleOpen(Pane *pane) {
    Pane *next;

    for (int i = 0; i < NPANES - 1; i++) {
        next = panes[i + 1];
        if (panes[i] == pane) {
            if (pane->isOpen) {
                pane->oldSize = pane->height();
                pane->setSize(width(), TH);
                pane->isOpen = false;
                next->setGeometry(0,
                                  pane->y() + TH,
                                  width(),
                                  next->y() + next->height() -
                                  (pane->y() + pane->height()));
            } else {
                int maxH = pane->height() + next->height() - TH;
                if (pane->oldSize > maxH)
                    pane->oldSize = maxH;

                pane->setSize(width(), pane->oldSize);
                next->setGeometry(0,
                                  pane->y() + pane->height(),
                                  width(),
                                  next->y() + next->height() -
                                  (pane->y() + pane->height()));
                pane->oldSize = 0;
                pane->isOpen = true;
            }
        }
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
            if (!pane->isOpen)
                b = TH;
            if (delta < i * TH)
                delta = i * TH;
            pane->setGeometry(pane->x(), delta, pane->width(), b);
        }
        if (pane->y() < oldY) {
            int bottom = pane->y();
            int n = i - 1;
            do {
                if (panes[n]->y() + TH > bottom || !panes[n]->isOpen) {
                    panes[n]->setGeometry(panes[n]->x(),
                                          bottom - TH,
                                          panes[n]->width(),
                                          TH);
                    bottom -= TH;
                } else {
                    panes[n]->setSize(panes[n]->width(),
                                      bottom - panes[n]->y());
                    break;
                }
                n--;
            } while (n > 0);
        } else if (pane->y() > oldY) {
            Pane *prev = 0;

            int j = i - 1;
            while (j >= 0) {
                if (panes[j]->isOpen) {
                    prev = panes[j];
                    i = j;
                    break;
                }
                j--;
            }
            if (prev) {
                int top = pane->y() + pane->height();
                int n = i + 1;

                prev->setSize(prev->width(),
                              panes[n]->y() - prev->y());

                while (n < NPANES) {
                    if (panes[n]->y() < top && panes[n]->height() == TH) {
                        panes[n]->setPosition(panes[n]->x(),
                                              top);
                        top += TH;
                    } else if (panes[n]->y() < top) {
                        if (panes[n]->isOpen) {
                            panes[n]->setGeometry(panes[n]->x(),
                                                  top,
                                                  panes[n]->width(),
                                                  panes[n]->y() +
                                              panes[n]->height() - top);
                            break;
                        } else {
                            panes[n]->setGeometry(panes[n]->x(), top,
                                                  panes[n]->width(), TH);
                            top += TH;
                        }
                    }
                    n++;
                }
            }
        }
    }
}

class YDockWindow: public YWindow {
public:
    YDockWindow(YWindow *aParent = 0): YWindow(aParent) {
        count = 0;
        setDND(true);
    }

    virtual void configure(int x, int y, unsigned int width, unsigned int height) {
        YWindow::configure(x, y, width, height);
        w[0]->setGeometry(0, 0, width, height - w[1]->height());
        w[1]->setGeometry(0, height - w[1]->height(), width, w[1]->height());
    }
    void add(YWindow *ww) {
        w[count++] = ww;
    }
    virtual void handleClose() {
        delete this;
        app->exit(0);
    }
private:
    int count;
    YWindow *w[10];
};

int main(int argc, char **argv) {
    YApplication app("icelist", &argc, &argv);
    YDockWindow *w;

    folder = YIcon::getIcon("folder");
    file = YIcon::getIcon("file");

    //ObjectList *list = new ObjectList(argv[1] ? argv[1] : (char *)"/", 0);
    //list->show();
    //debug = 0;

    w = new YDockWindow();

    Panes *p = new Panes(w);
    p->show();


    YInputLine *input = new YInputLine(w);
    input->setText("http://slashdot.org/");
    input->selectAll();
    input->show();

    w->add(p);
    w->add(input);

    w->setGeometry(0, 0, 200, 720);
    w->show();

    return app.mainLoop();
}
