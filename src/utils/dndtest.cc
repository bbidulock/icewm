#include "config.h"
#include "yxlib.h"
#include <X11/Xatom.h>
#include "ylistbox.h"
#include "yscrollview.h"
#include "ymenu.h"
#include "yapp.h"
#include "yaction.h"
#include "yinputline.h"
#include "sysdep.h"
#include "ypaint.h"
#include "base.h"

#include <string.h>

class TestWindow: public YWindow {
public:
    TestWindow(YWindow *parent): YWindow(parent) {
        setDND(true);
    }

    void handleClose() {
        app->exitLoop(0);
    }

    void paint(Graphics &g, const YRect &/*er*/) {
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
    YFont *textFont;

    YColor *c[256];

    DNDTarget(YWindow *parent): YWindow(parent) {
        px = py = -1;
        isInside = false;
        textFont = YFont::getFont("fixed");

        for (int i = 0; i < 256; i++) {
            c[i] = new YColor(0, 0, (i << 8) + i);
        }
    }

    void paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
        g.setColor(YColor::white);
        g.fillRect(0, 0, width(), height());
        for (int i = 0; i < 256; i++) {
            g.setColor(c[i]);
            g.drawLine(0, i, height(), i);
        }

        g.setColor(YColor::black);
        g.setFont(textFont);
        g.drawChars(sTarget, 0, strlen(sTarget), width() / 3, height() / 2);

        if (isInside) {
            g.drawRect(2, 2, width() - 4, height() - 4);
            if (px != -1 && py != -1) {
                g.drawLine(width() / 2, height() / 2, px, py);
            }
        }
    }
    virtual void handleDNDEnter(int nTypes, Atom *types) {
        warn("->enter");
        isInside = true;
        repaint();
        warn("DndEnter: Count=%d\n", nTypes);
        for (int i = 0; i < nTypes; i++) {
            warn("DndEnter: Type=%s\n", XGetAtomName(app->display(), types[i]));
        }
    }
    void handleDNDLeave() {
        warn("<-leave");
        isInside = false;
        repaint();
    }

    bool handleDNDPosition(int x, int y, Atom * /*action*/) {
        warn("  position %d %d\n", x, y);
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
    YFont *textFont;

    DNDSource(YWindow *parent): YWindow(parent) {
        textFont = YFont::getFont("fixed");
    }

    void paint(Graphics &g, int /*x*/, int /*y*/, unsigned int /*width*/, unsigned int /*height*/) {
        g.setColor(YColor::white);
        g.fillRect(0, 0, width(), height());

        g.setColor(YColor::black);
        g.setFont(textFont);
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
    s->setGeometry(0, 0, 256, 256);
    s->show();

    DNDTarget *t = new DNDTarget(w);
    t->setGeometry(260, 0, 256, 256);
    t->show();

    w->setSize(516, 256);
    w->show();

    return app.mainLoop();
}
