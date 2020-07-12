#include "config.h"
#include "yxapp.h"
#include "yinputline.h"
#include "ylabel.h"
#include "ybutton.h"
#include "prefs.h"
#include "yicon.h"
#include <X11/Xatom.h>
#include "ylocale.h"
#include "intl.h"

const char *ApplicationName = "icerun";

class InputRun : public YInputLine {
public:
    InputRun(YWindow *parent, YInputListener *listener) :
        YInputLine(parent, listener)
    {
    }

    virtual void handleClose() {
        xapp->exit(0);
    }

};

class IceRun : public YXApplication, public YInputListener {
    YInputLine *input;
    mstring cmdPrefix;
    ref<YPixmap> large;

public:
    IceRun(int* argc, char*** argv);
    ~IceRun() { delete input; }

    virtual void inputReturn(YInputLine* input) {
        mstring command(input->getText());
        if (command != cmdPrefix) {
            msg("%s", command.c_str());
            runCommand(command);
        }
        this->exit(0);
    }

    virtual void inputEscape(YInputLine* input) {
        this->exit(0);
    }

    virtual void inputLostFocus(YInputLine* input) {
    }

};

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    IceRun app(&argc, &argv);
    return app.mainLoop();
}

IceRun::IceRun(int* argc, char*** argv) :
    YXApplication(argc, argv),
    input(new InputRun(nullptr, this))
{
    int textWidth = input->getFont()->textWidth("M");
    int inputWidth = max(300, textWidth * 80);
    int textHeight = input->getFont()->height();
    int inputHeight = max(20, textHeight + 5);

    input->setSize(inputWidth, inputHeight);
    if (nonempty(terminalCommand)) {
        cmdPrefix = mstring(terminalCommand, " -e ");
    }
    input->setText(cmdPrefix, false);
    input->unselectAll();

    ref<YIcon> file = YIcon::getIcon("icewm");
    if (file != null) {
        unsigned depth = xapp->depth();
        large = YPixmap::createFromImage(file->large(), depth);
    }

    static char wm_clas[] = "IceWM";
    static char wm_name[] = "icehint";
    XClassHint class_hint = { wm_name, wm_clas };
    XSizeHints size_hints = { PSize, 0, 0, inputWidth, inputHeight, };
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
    Xutf8SetWMProperties(xapp->display(), input->handle(),
                         ApplicationName, "icewm", *argv, *argc,
                         &size_hints, &wmhints, &class_hint);

    input->setNetPid();
    input->show();
}

#if 0
    //YLabel *label = new YLabel("this is a string\na second line\n\nlast after empty");
    //label->show();

    {
        YButton *a = new YButton(0);
        a->setText("A");
        a->setSize(800, 600);

        YButton *b = new YButton(a);
        b->setText("B");
        b->setGeometry(50, 50, 150, 150);
        b->show();


        YButton *c = new YButton(a);
        c->setText("C");
        c->setGeometry(50, 300, 150, 150);
        c->show();

        YButton *d = new YButton(c);
        d->setText("D");
        d->setGeometry(50, 50, 50, 50);
        d->show();

        YButton *dd = new YButton(d);
        dd->setText("D");
        dd->setGeometry(20, 20, 20, 20);
        dd->show();

        YButton *e = new YButton(a);
        e->setText("C");
        e->setGeometry(300, 300, 250, 150);
        e->show();

        YButton *f = new YButton(e);
        f->setText("D");
        f->setGeometry(50, 50, 50, 50);
        f->show();

        YButton *g = new YButton(e);
        g->setText("D");
        g->setGeometry(120, 50, 50, 50);
        g->show();

        a->show();
        a->setToplevel(true);
    }
    {
        YButton *a = new YButton(0);
        a->setText("A");
        a->setSize(700, 700);

        YButton *c = new YButton(a);
        c->setText("C");
        c->setGeometry(50, 50, 250, 250);
        c->show();

        YButton *d = new YButton(c);
        d->setText("D");
        d->setGeometry(5, 5, 100, 100);
        d->show();

        YButton *dd = new YButton(d);
        dd->setText("E");
        dd->setGeometry(10, 10, 20, 20);
        dd->show();

        a->show();
        a->setToplevel(true);
    }
#endif

// vim: set sw=4 ts=4 et:
