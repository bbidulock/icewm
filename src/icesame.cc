#include "config.h"
#include "yfull.h"
#include "ywindow.h"
#include "ylabel.h"
#include "ymenuitem.h"
#include "ymenu.h"
#include "yxapp.h"
#include "yaction.h"
#include "MwmUtil.h"
#include "yrect.h"
#include "ylocale.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "intl.h"

#define XCOUNT 15
#define YCOUNT 10
#define XSIZE 32
#define YSIZE 32
#define NCOLOR 4
#define FLAG 64

char const *ApplicationName = "icesame";

ref<YPixmap> dialogbackPixmap;

class IceSame: public YWindow, public YActionListener {
public:
    IceSame(YWindow *parent): YWindow(parent) {
        srand(time(NULL) ^ getpid());
        score = 0;
        scoreLabel = new YLabel("0", this);

        c[0][0] = "rgb:00/00/00";
        c[0][1] = "rgb:00/00/00";
        c[1][0] = "rgb:FF/00/00";
        c[1][1] = "rgb:80/00/00";
        c[2][0] = "rgb:FF/FF/00";
        c[2][1] = "rgb:80/80/00";
        c[3][0] = "rgb:00/00/FF";
        c[3][1] = "rgb:00/00/80";

        setStyle(wsPointerMotion);
        setSize(XSIZE * XCOUNT, YSIZE * YCOUNT + scoreLabel->height());
        scoreLabel->setGeometry(YRect(0, YSIZE * YCOUNT, width(), scoreLabel->height()));
        scoreLabel->show();

        // !!! keybindings, Menu, Shift+F10

        menu = new YMenu();
        menu->setActionListener(this);
        menu->addItem(_("Undo"), 0, _("Ctrl+Z"), actionUndo);
        menu->addSeparator();
        menu->addItem(_("New"), 0, _("Ctrl+N"), actionNew);
        menu->addItem(_("Restart"), 0, _("Ctrl+R"), actionRestart);
        menu->addSeparator();
        menu->addItem(_("Close"), 0, _("Ctrl+Q"), actionClose);

        // !!! fix
        setTitle(_("Same Game"));

        MwmHints mwm(
                MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
                MWM_FUNC_MOVE | MWM_FUNC_CLOSE | MWM_FUNC_MINIMIZE,
                MWM_DECOR_TITLE | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE,
                0,
                0);

        XChangeProperty(xapp->display(), handle(),
                        _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                        32, PropModeReplace,
                        (unsigned char *)&mwm, PROP_MWM_HINTS_ELEMENTS);

        char res_name[] = "icesame";
        char res_class[] = "IceSame";
        XClassHint ch;
        ch.res_name = res_name;
        ch.res_class = res_class;
        XSetClassHint(xapp->display(), handle(), &ch);

        newGame();
    }
    virtual ~IceSame() {
    }

    void setScore(int s) {
        if (s != score) {
            score = s;
            char ss[24];
            sprintf(ss, "%d", score);
            ///!!! fix: center, no dynamic resize, add display for selected
            scoreLabel->setText(ss);
            scoreLabel->setToolTip(ss);
            scoreLabel->setGeometry(YRect(0, YSIZE * YCOUNT, width(), scoreLabel->height()));
            scoreLabel->repaint();
        }
    }

    int randVal() {
        int r = rand();

        if (r < RAND_MAX / 3)
            return 1;
        else if (r < RAND_MAX / 3 * 2)
            return 2;
        else
            return 3;
    }

    void newGame() {
        saveField();
        setScore(0);
        for (int x = 0; x < XCOUNT; x++)
            for (int y = 0; y < YCOUNT; y++) {
                field[x][y] = randVal();
                restartField[x][y] = field[x][y];
            }
        repaint();
    }

    void restartGame() {
        saveField();
        setScore(0);
        for (int x = 0; x < XCOUNT; x++)
            for (int y = 0; y < YCOUNT; y++)
                field[x][y] = restartField[x][y];
        repaint();
    }

    void paint(Graphics &g, const YRect &/*r*/) {
        for (int x = 0; x < XCOUNT; x++)
            for (int y = 0; y < YCOUNT; y++) {
                int v = field[x][y];

                if (v > FLAG)
                    g.setColor(c[v & ~FLAG][1]);
                else
                    g.setColor(c[v][0]);

                g.fillRect(x * XSIZE, y * YSIZE, XSIZE, YSIZE);
            }
    }

    void saveField() {
        undoScore = score;
        for (int x = 0; x < XCOUNT; x++)
            for (int y = 0; y < YCOUNT; y++)
                undoField[x][y] = field[x][y];
        canUndo = true;
    }

    void undo() { //!!! unlimited undo
        if (!canUndo)
            return ;
        setScore(undoScore);
        for (int x = 0; x < XCOUNT; x++)
            for (int y = 0; y < YCOUNT; y++)
                field[x][y] = undoField[x][y];
        repaint();
        canUndo = false;
    }

    int mark(int x, int y);

    void clean() {
        int total = 0;
        for (int x = XCOUNT - 1; x >= 0; x--) {
            int vert = 0;
            for (int y = 0; y < YCOUNT; y++) {
                if (field[x][y] > 0 && field[x][y] < FLAG)
                    vert++;
                else {
                    for (int j = y; j > 0; j--)
                        field[x][j] = field[x][j - 1];
                    field[x][0] = 0;
                    //y--;
                }
            }
            total += vert;
            if (vert == 0) {
                for (int i = x; i < XCOUNT - 1; i++)
                    for (int j = 0; j < YCOUNT; j++)
                        field[i][j] = field[i + 1][j];
                for (int j = 0; j < YCOUNT; j++)
                    field[XCOUNT - 1][j] = 0;
            }
        }
        if (total == 0) {
            setScore(score + 1000);
        }
    }

    void press(int ax, int ay, bool clr) {
        release();
        if (ax < 0 || ay < 0 || ax >= XCOUNT * XSIZE || ay >= YCOUNT * YSIZE) {
            repaint();
            return ;
        }
        int x = ax / XSIZE;
        int y = ay / YSIZE;
        int c;
        if ((c = mark(x, y)) <= 1)
            field[x][y] &= ~FLAG;
        else if (clr) {
            saveField();
            clean();
            setScore(score + (c - 2) * (c - 2));

            if ((c = mark(x, y)) <= 1)
                field[x][y] &= ~FLAG;
        }
        repaint();
    }

    void release() {
        for (int x = 0; x < XCOUNT; x++)
            for (int y = 0; y < YCOUNT; y++)
                field[x][y] &= ~FLAG;
    }

    virtual void handleCrossing(const XCrossingEvent &crossing) {
        if (crossing.type == EnterNotify) {
            press(crossing.x, crossing.y, false);
        } else if (crossing.type == LeaveNotify) {
            release();
            repaint();
        }
        YWindow::handleCrossing(crossing);
    }
    virtual void handleClick(const XButtonEvent &up, int /*count*/) {
        if (up.button == 2) {
            sleep(5);
            {
                XWMHints wmh;

                memset(&wmh, 0, sizeof(wmh));
                wmh.flags = InputHint | XUrgencyHint;
                wmh.input = False;
                //wmh.

                XSetWMHints(xapp->display(), handle(), &wmh);
            }
        }
        if (up.button == 3) {
            menu->popup(this, 0, 0, up.x_root, up.y_root, //-1, -1, -1,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
            return ;
        } else
            press(up.x, up.y, true);
    }

    virtual void handleMotion(const XMotionEvent &motion) {
        press(motion.x, motion.y, false);
        YWindow::handleMotion(motion);
    }

    virtual void actionPerformed(YAction action, unsigned int /*modifiers*/) {
        if (action == actionNew)
            newGame();
        else if (action == actionRestart)
            restartGame();
        else if (action == actionUndo)
            undo();
        else if (action == actionClose)
            exit(0);
    }
    virtual void handleClose() {
        exit(0);
    }
private:
    int field[XCOUNT][YCOUNT];
    int undoField[XCOUNT][YCOUNT];
    int restartField[XCOUNT][YCOUNT];
    bool canUndo;
    int score, undoScore;
    YColorName c[NCOLOR][2];
    YMenu *menu;
    YLabel *scoreLabel;
    YAction actionUndo, actionNew, actionRestart, actionClose;
};

int IceSame::mark(int x, int y) {
    int c = field[x][y];
    if (c == 0)
        return 0;

    field[x][y] |= FLAG;
    int count = 1;

    if (x > 0 && field[x - 1][y] == c) count += mark(x - 1, y);
    if (y > 0 && field[x][y - 1] == c) count += mark(x, y - 1);
    if (x < XCOUNT - 1 && field[x + 1][y] == c) count += mark(x + 1, y);
    if (y < YCOUNT - 1 && field[x][y + 1] == c) count += mark(x, y + 1);
    return count;
}

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    YXApplication app(&argc, &argv);

    IceSame *game = new IceSame(0);

    game->show();

    return app.mainLoop();
}

// vim: set sw=4 ts=4 et:
