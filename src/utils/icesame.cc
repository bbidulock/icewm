#include "config.h"
#include "yfull.h"
#include "ywindow.h"
#include "ylabel.h"
#include "ymenuitem.h"
#include "ymenu.h"
#include "yapp.h"
#include "yaction.h"
#include "ytopwindow.h"
#include "MwmUtil.h"

#define NO_KEYBIND
//#include "bindkey.h"
#include "prefs.h"
#define CFGDEF
//#include "bindkey.h"
#include "default.h"
#include "yconfig.h"


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define XCOUNT 15
#define YCOUNT 10
#define XSIZE 32
#define YSIZE 32
#define NCOLOR 4
#define FLAG 64

class IceSame: public YTopWindow, public YActionListener {
public:
    typedef YTopWindow inherited;

    IceSame(): YTopWindow() {
        srand(time(NULL) ^ getpid());
        score = 0;
        oldx = -1;
        oldy = -1;
        scoreLabel = new YLabel("0", this);
        markedLabel = new YLabel("0", this);

        c[0][0] = new YColor("rgb:00/00/00");
        c[0][1] = 0; //new YColor("rgb:00/00/00");
        c[1][0] = new YColor("rgb:FF/00/00");
        c[1][1] = new YColor("rgb:80/00/00");
        c[2][0] = new YColor("rgb:FF/FF/00");
        c[2][1] = new YColor("rgb:80/80/00");
        c[3][0] = new YColor("rgb:00/00/FF");
        c[3][1] = new YColor("rgb:00/00/80");

        setStyle(wsPointerMotion);
        setSize(XSIZE * XCOUNT, YSIZE * YCOUNT + scoreLabel->height());
        scoreLabel->setGeometry(0, YSIZE * YCOUNT, width() / 2, scoreLabel->height());
        scoreLabel->show();
        markedLabel->setGeometry(width() / 2, YSIZE * YCOUNT, width() - width() / 2, scoreLabel->height());
        markedLabel->show();

        // !!! keybindings, Menu, Shift+F10
        actionUndo = new YAction();
        actionNew = new YAction();
        actionRestart = new YAction();
        actionClose = new YAction();

        menu = new YMenu();
        menu->setActionListener(this);
        menu->addItem("Undo", 0, "Ctrl+Z", actionUndo);
        menu->addSeparator();
        menu->addItem("New", 0, "Ctrl+N", actionNew);
        menu->addItem("Restart", 0, "Ctrl+R", actionRestart);
        menu->addSeparator();
        menu->addItem("Close", 0, "Ctrl+Q", actionClose);

        setTitle("Same Game");
        // !!! fix
        //XStoreName(app->display(), handle(), );
        {
            MwmHints mwm;

            memset(&mwm, 0, sizeof(mwm));
            mwm.flags =
                MWM_HINTS_FUNCTIONS |
                MWM_HINTS_DECORATIONS;
            mwm.functions =
                MWM_FUNC_MOVE | MWM_FUNC_CLOSE | MWM_FUNC_MINIMIZE;
            mwm.decorations =
                MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE;

            setMwmHints(mwm);
#if 0
            XChangeProperty(app->display(), handle(),
                            _XATOM_MWM_HINTS, _XATOM_MWM_HINTS,
                            32, PropModeReplace,
                            (unsigned char *)&mwm, sizeof(mwm)/sizeof(long)); ///!!! ?????????
#endif
        }

        newGame();
    }
    virtual ~IceSame() {
        delete menu;
        delete actionUndo;
        delete actionNew;
        delete actionRestart;
        delete actionClose;
        delete scoreLabel;
        delete markedLabel;
        for (int i = 0; i < NCOLOR; i++) {
            delete c[i][0];
            delete c[i][1];
        }
    }

    void setScore(int s) {
        if (s != score) {
            score = s;
            char ss[24];
            sprintf(ss, "%d", score);
            ///!!! fix: center, no dynamic resize, add display for selected
            scoreLabel->setText(ss);
            scoreLabel->setSize(width() / 2, scoreLabel->height());
            scoreLabel->repaint();
        }
    }

    void setMarked(int s) {
        char ss[24];
        sprintf(ss, "%d", s);
        ///!!! fix: center, no dynamic resize, add display for selected
        markedLabel->setText(ss);
        markedLabel->setSize(width() - width() / 2, markedLabel->height());
        markedLabel->repaint();
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

    void newGame();

    void restartGame() {
        saveField();
        setScore(0);
        for (int x = 0; x < XCOUNT; x++)
            for (int y = 0; y < YCOUNT; y++)
                field[x][y] = restartField[x][y];
        repaint();
    }

    void paint(Graphics &g, int, int, unsigned int, unsigned int) {
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

    void saveField();

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
        if (ax < 0 || ay < 0 || ax >= XCOUNT * XSIZE || ay >= YCOUNT * YSIZE) {
            repaint();
            return ;
        }
        int x = ax / XSIZE;
        int y = ay / YSIZE;
        if (x == oldx && y == oldy && !clr)
            return ;
        oldx = x;
        oldy = y;

        release();
        int c;
        if ((c = mark(x, y)) <= 1) {
            field[x][y] &= ~FLAG;
            setMarked(0);
        } else if (clr) {
            saveField();
            clean();
            setScore(score + (c - 2) * (c - 2));
            setMarked(0);

            if ((c = mark(x, y)) <= 1)
                field[x][y] &= ~FLAG;
        } else {
            setMarked((c - 2) * (c - 2));
        }
        repaint();
    }

    void release();

    virtual void handleCrossing(const XCrossingEvent &crossing) {
        if (crossing.type == EnterNotify) {
            press(crossing.x, crossing.y, false);
        } else if (crossing.type == LeaveNotify) {
            release();
            repaint();
        }
        inherited::handleCrossing(crossing);
    }

    virtual void handleClick(const XButtonEvent &up, int count) {
        if (up.button == 3 && count == 1) {
            menu->popup(0, 0, up.x_root, up.y_root, -1, -1,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
            return ;
        } else
            press(up.x, up.y, true);
    }

    virtual void handleMotion(const XMotionEvent &motion) {
        press(motion.x, motion.y, false);
        inherited::handleMotion(motion);
    }

    virtual void actionPerformed(YAction *action, unsigned int /*modifiers*/) {
        if (action == actionNew)
            newGame();
        else if (action == actionRestart)
            restartGame();
        else if (action == actionUndo)
            undo();
        else if (action == actionClose)
            app->exit(0);
    }
    virtual void handleClose() {
        app->exit(0);
    }

    void configure(int x, int y, unsigned int width, unsigned int height) {
        YWindow::configure(x, y, width, height);
    }
private:
    int field[XCOUNT][YCOUNT];
    int undoField[XCOUNT][YCOUNT];
    int restartField[XCOUNT][YCOUNT];
    bool canUndo;
    int score, undoScore;
    YColor *c[NCOLOR][2];
    YMenu *menu;
    YLabel *scoreLabel;
    YLabel *markedLabel;
    YAction *actionUndo, *actionNew, *actionRestart, *actionClose;
    int oldx, oldy;
};

void IceSame::newGame() {
    saveField();
    setScore(0);
    for (int x = 0; x < XCOUNT; x++)
        for (int y = 0; y < YCOUNT; y++) {
            field[x][y] = randVal();
            restartField[x][y] = field[x][y];
        }
    repaint();
}
void IceSame::saveField() {
    undoScore = score;
    for (int x = 0; x < XCOUNT; x++)
        for (int y = 0; y < YCOUNT; y++)
            undoField[x][y] = field[x][y];
    canUndo = true;
}

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

void IceSame::release() {
    for (int x = 0; x < XCOUNT; x++)
        for (int y = 0; y < YCOUNT; y++)
            field[x][y] &= ~FLAG;
}

int main(int argc, char **argv) {
    YApplication app("icesame", &argc, &argv);

    //YPref x(0, "test.pref");
    //printf("%s='%s', %ld\n", x.getName(), x.getValue(), x.getNum(666));

    IceSame *game = new IceSame();

    game->show();

    int rc = app.mainLoop();

    delete game;

    return rc;
}
