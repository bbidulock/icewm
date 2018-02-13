#include "config.h"
#include "yxapp.h"
#include "yinputline.h"
#include "ylabel.h"
#include "ybutton.h"
#include "prefs.h"
#include "ylocale.h"
#include "intl.h"

const char *ApplicationName = "icerun";

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    YXApplication app(&argc, &argv);

    YInputLine *input = new YInputLine();
    input->setSize(100, 20);
    input->setText(terminalCommand);
    input->show();

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
    return app.mainLoop();
}

// vim: set sw=4 ts=4 et:
