#include "config.h"
#include "ylib.h"
#include <X11/Xatom.h>
#include "ylistbox.h"
#include "yscrollview.h"
#include "ymenu.h"
#include "yapp.h"
#include "yaction.h"
#include "yinputline.h"
//#include "wmmgr.h"
#include "sysdep.h"
#include <dirent.h>

#include "MwmUtil.h"

#define NO_KEYBIND
//#include "bindkey.h"
#include "prefs.h"
#define CFGDEF
//#include "bindkey.h"
#include "default.h"

class TestWindow: public YWindow {
public:
    TestWindow(YWindow *parent): YWindow(parent) {
        setDND(true);
    }

    void handleClose() {
        app->exitLoop(0);
    }

    void paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
        g.setColor(YColor::black);
        g.fillRect(0, 0, width(), height());
    }
};

class DNDTarget: public YWindow {
public:
    int px;
    int py;
    bool isInside;

    static const char *sTarget;

    DNDTarget(YWindow *parent): YWindow(parent) {
        px = py = -1;
        isInside = false;
    }

    void paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
        g.setColor(YColor::white);
        g.fillRect(0, 0, width(), height());

        g.setColor(YColor::black);
        g.drawChars(sTarget, 0, strlen(sTarget), width() / 3, height() / 2);

        if (isInside) {
            g.drawRect(2, 2, width() - 4, height() - 4);
            if (px != -1 && py != -1) {
                g.drawLine(width() / 2, height() / 2, px, py);
            }
        }
    }
    virtual void handleDNDEnter(int nTypes, Atom *types) {
        puts("->enter");
        isInside = true;
        repaint();
        printf("DndEnter: Count=%d\n", nTypes);
        for (int i = 0; i < nTypes; i++) {
            printf("DndEnter: Type=%s\n", XGetAtomName(app->display(), types[i]));
        }
    }
    void handleDNDLeave() {
        puts("<-leave");
        isInside = false;
        repaint();
    }

    bool handleDNDPosition(int x, int y, Atom * /*action*/) {
        printf("  position %d %d\n", x, y);
        px = x;
        py = y;
        repaint();
        return false;
    }
};
const char *DNDTarget::sTarget = "target";

class DNDSource: public YWindow {
public:
    static const char *sSource;

    DNDSource(YWindow *parent): YWindow(parent) {
    }

    void paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
        g.setColor(YColor::white);
        g.fillRect(0, 0, width(), height());

        g.setColor(YColor::black);
        g.drawChars(sSource, 0, strlen(sSource), width() / 3, height() / 2);
    }

    void handleButton(const XButtonEvent &button) {
        if (button.button == 3) {
            if (button.type == ButtonPress)
                startDrag(0, 0);
            else
                endDrag(true);
        }
    }
};

const char *DNDSource::sSource = "source";

int main(int argc, char **argv) {
    YApplication app("dndtest", &argc, &argv);
    TestWindow *w;

    w = new TestWindow(0);

    DNDSource *s = new DNDSource(w);
    s->setGeometry(0, 0, 200, 200);
    s->show();

    DNDTarget *t = new DNDTarget(w);
    t->setGeometry(210, 0, 200, 200);
    t->show();

    w->setSize(410, 200);
    w->show();

    return app.mainLoop();
}
