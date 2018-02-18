#include "config.h"
#include "ylib.h"
#include <X11/Xatom.h>
#include "ylistbox.h"
#include "yscrollview.h"
#include "ymenu.h"
#include "yxapp.h"
#include "sysdep.h"
#include "yaction.h"
#include "yrect.h"
#include "ylocale.h"
#include "prefs.h"
#include "yicon.h"

#include <unistd.h>
extern "C" {
#include <sys/mman.h>
}

#include "intl.h"

char const *ApplicationName = "iceview";
ref<YIcon> file;

extern Atom _XA_WIN_ICONS;

class TextView: public YWindow,
    public YScrollBarListener, public YScrollable, public YActionListener
{
public:
    TextView(YScrollView *v, YWindow *parent):
        YWindow(parent),
        bg("rgb:C0/C0/C0"),
        fg(YColor::black)
    {
        expandTabs = true;
        hexView = false;
        wrapLines = true;

        view = v;
        fVerticalScroll = view->getVerticalScrollBar();
        fVerticalScroll->setScrollBarListener(this);
        fHorizontalScroll = view->getHorizontalScrollBar();
        fHorizontalScroll->setScrollBarListener(this);
        setBitGravity(NorthWestGravity);
        maxWidth = 0;
        tx = ty = 0;

        buf = 0;
        bufLen = 0;

        lineCount = 0;
        linePos = 0;
        lineWCount = 0;
        lineWPos = 0;
        maxLineLen = 80; // for buffer
        fmt = 0;
        wrapWidth = 0;
        fWidth = 0;
        fHeight = 0;

        font = YFont::getFont("-adobe-courier-medium-r-*-*-*-100-*-*-*-*-*-*", "monospace:size=10");
        fontWidth = font->textWidth("M");
        fontHeight = font->height();

        menu = new YMenu();
        menu->setActionListener(this);
        //menu->addItem(_("Find..."), 0, _("Ctrl+F"), actionFind);
        menu->addSeparator();
        menu->addItem(_("Hex View"), 0, _("Ctrl+H"), actionToggleHexView);
        menu->addItem(_("Expand Tabs"), 0, _("Ctrl+T"), actionToggleExpandTabs);
        menu->addItem(_("Wrap Lines"), 0, _("Ctrl+W"), actionToggleWrapLines);
        menu->addSeparator();
        menu->addItem(_("Close"), 0, _("Ctrl+Q"), actionClose);
    }

    ~TextView() {
    }
    int nextTab(int n) {
        return (n / 8 + 1) * 8;
    }

    char *line(int l) {
        PRECONDITION(l >= 0 && l < lineCount);
        return buf + linePos[2 * l];
    }

    int lineChars(int l) {
        PRECONDITION(l >= 0 && l < lineCount);
        return linePos[2 * (l + 1)] - linePos[2 * l];
    }

    int lineLen(int l) {
        PRECONDITION(l >= 0 && l < lineCount);
        return linePos[2 * l + 1];
    }

    int getLen(int l) {
        int n = lineChars(l);
        if (!expandTabs)
            return n;
        char *p = line(l);
        int len = 0;

        while (n > 0) {
            if (*p == '\t' && expandTabs)
                len = nextTab(len);
            else
                len++;
            n--;
            p++;
        }
        return len;
    }

    void addLinePos(int p, bool r) {
        linePos = (int *)realloc(linePos, 2 * (lineCount + 1) * sizeof(int *));
        linePos[2 * lineCount] = p;
        linePos[2 * lineCount + 1] = 0;
        if (r) {
            lineCount++;

            if (lineCount >= 2) {
                int len = getLen(lineCount - 2);
                linePos[2 * (lineCount - 2) + 1] = len;
                if (len > maxLineLen)
                    maxLineLen = len;
                int w = fontWidth * len;
                if (w > maxWidth)
                    maxWidth = w;
            }
        }
    }

    void setData(char *d, int len) {
        buf = d;
        bufLen = len;

        chunkCount = bufLen / 16;
        if (bufLen % 16)
            chunkCount++;

        findLines();
    }

    void findLines() {
        free(linePos);
        lineCount = 0;
        linePos = 0;

        char *p;
        char *e = buf + bufLen;

        addLinePos(0, true);
        for (p = buf; p < e; p++) {
            if (*p == '\n')
                addLinePos(p - buf + 1, true);
        }
        addLinePos(bufLen, false);
        delete fmt;
        fmt = new char[maxLineLen];
        findWLines(width() / fontWidth);
    }

    char *lineW(int l) {
        PRECONDITION(l >= 0 && l < lineWCount);
        return buf + lineWPos[l];
    }

    int lineWChars(int l) {
        PRECONDITION(l >= 0 && l < lineWCount);
        return lineWPos[l + 1] - lineWPos[l];
    }

    void findWLines(int wrap) {
        if (!wrapLines) {
            delete [] lineWPos; lineWPos = 0;
            lineWCount = 0;
        }
        int nw = 0;
        if (wrap < 16)
            wrap = 16;

        wrapWidth = wrap;

        for (int i = 0; i < lineCount; i++) {
            if (expandTabs) {
                int l = lineChars(i);
                char *p = line(i);
                int len = 0;
                int w = 0;

                nw++;
                while (len < l) {
                    if (*p == '\t') {
                        w = nextTab(w);
                    } else
                        w++;

                    if (w > wrap) {
                        nw++;
                        w = 0;
                        if (*p == '\t')
                            continue;
                    }
                    p++;
                    len++;
                }
            } else {
                int l = lineLen(i);
                nw += 1 + l / wrap;
                if ((l % wrap) == 0 && l > 0)
                    nw--;
            }
        }

        if (nw != lineWCount) {
            delete [] lineWPos;

            lineWPos = new int[nw + 1];
            if (lineWPos == 0)
                return ;
            lineWCount = nw;

            nw = 0;
            for (int i = 0; i < lineCount; i++) {
                lineWPos[nw++] = linePos[2 * i];
                if (expandTabs) {

                    int l = lineChars(i);
                    char *p = line(i);
                    int len = 0;
                    int w = 0;

                    while (len < l) {
                        if (*p == '\t') {
                            w = nextTab(w);
                        } else
                            w++;

                        if (w > wrap) {
                            lineWPos[nw] = p - buf;
                            nw++;
                            w = 0;
                            if (*p == '\t')
                                continue;
                        }
                        len++;
                        p++;
                    }
                } else {
                    int l = lineChars(i);
                    if (l > wrap) {
                        while (l > wrap) {
                            lineWPos[nw] = lineWPos[nw - 1] + wrap;
                            nw++;
                            l -= wrap;
                        }
                    }
                }
            }
            lineWPos[nw] = bufLen;
            assert(nw == lineWCount);
        }
    }

    int format(char *p, int len) {
        int n = 0;

        if (hexView) {
            static char hex[] = "0123456789ABCDEF";
            char *e = buf + bufLen;
            char *d = fmt;
            int i;
            int o = p - buf;

            *d++ = hex[(o >> 28) & 0xF];
            *d++ = hex[(o >> 24) & 0xF];
            *d++ = hex[(o >> 20) & 0xF];
            *d++ = hex[(o >> 16) & 0xF];
            *d++ = hex[(o >> 12) & 0xF];
            *d++ = hex[(o >> 8) & 0xF];
            *d++ = hex[(o >> 4) & 0xF];
            *d++ = hex[(o >> 0) & 0xF];

            *d++ = ' ';
            *d++ = ' ';
            for (i = 0; i < 16; i++) {
                if (p + i < e) {
                    unsigned char u = p[i];
                    *d++ = hex[(u >> 4) & 0x0F];
                    *d++ = hex[u & 0x0F];
                } else {
                    *d++ = ' ';
                    *d++ = ' ';
                }
                //if ((i % 4) == 3)
                    *d++ = ' ';
            }
#if 0
            *d++ = ' ';
            for (i = 0; i < 16; i++) {
                if (p + i < e) {
                    unsigned char u = p[i];
                    *d++ = u;
                }
            }
#endif
            n = d - fmt;
        } else {
            int n1;

            while (len > 0) {
                if (*p == '\t' && expandTabs) {
                    n1 = nextTab(n);
                    while (n < n1) {
                        fmt[n] = ' ';
                        n++;
                    }
                } else {
                    fmt[n++] = *p;
                }
                p++;
                len--;
            }
        }
        return n;
    }

    virtual void paint(Graphics &g, const YRect &r) {
        int wx = r.x(), wy = r.y(), wwidth = r.width(), wheight = r.height();
        g.setColor(bg);
        g.fillRect(wx, wy, wwidth, wheight);
        g.setFont(font);
        g.setColor(fg);
        int l1 = (ty + wy) / fontHeight;
        int l2 = (ty + wy + wheight) / fontHeight;
        if ((ty + wy + wheight) % fontHeight)
            l2++;
        int y = l1 * fontHeight - ty;
        for (int l = l1; l < l2; l++) {
            if (hexView) {
                if (l >= chunkCount)
                    break;
            } else if (wrapLines) {
                if (l >= lineWCount)
                    break;
            } else {
                if (l >= lineCount)
                    break;
            }
            int n = 0;

            if (hexView) {
                char *p = buf + l * 16;
                n = format(p, 16);
            } else if (wrapLines) {
                char *p = lineW(l);
                if (p) {
                    int len = lineWChars(l);

                    if (len > 0 && p[len - 1] == '\n')
                        len--;
                    n = format(p, len);
                }
            } else {
                char *p = line(l);
                if (p) {
                    int len = lineChars(l);

                    if (len > 0 && p[len - 1] == '\n')
                        len--;
                    n = format(p, len);
                }
            }

            int o = tx/fontWidth;
            int r = width()/fontWidth + 1;
            if (o < n) {
                n -= o;
                if (n > r)
                    n = r;
                g.drawChars(fmt + o, 0, n,
                            1 - tx + o * fontWidth,
                            1 + y + font->ascent());
            }
            y += fontHeight;
        }

        resetScroll();
    }

    void resetScroll() {
        fVerticalScroll->setValues(ty, height(), 0, contentHeight());
        fVerticalScroll->setBlockIncrement(height());
        fVerticalScroll->setUnitIncrement(fontHeight);
        fHorizontalScroll->setValues(tx, width(), 0, contentWidth());
        fHorizontalScroll->setBlockIncrement(width());
        fHorizontalScroll->setUnitIncrement(fontWidth);
        if (view)
            view->layout();
    }

    void setPos(int x, int y) {
        if (x != tx || y != ty) {
            int dx = x - tx;
            int dy = y - ty;

            tx = x;
            ty = y;

            scrollWindow(dx, dy);
        }
    }

    virtual void scroll(YScrollBar *sb, int delta) {
        if (sb == fHorizontalScroll)
            setPos(tx + delta, ty);
        else if (sb == fVerticalScroll)
            setPos(tx, ty + delta);
    }
    virtual void move(YScrollBar *sb, int pos) {
        if (sb == fHorizontalScroll)
            setPos(pos, ty);
        else if (sb == fVerticalScroll)
            setPos(tx, pos);
    }

    unsigned contentWidth() {
        if (hexView)
            return 78 * fontWidth + 2;
        else if (wrapLines)
            return wrapWidth * fontWidth;
        else
            return maxWidth + 2;
    }
    unsigned contentHeight() {
        int n;
        if (hexView)
            n = chunkCount;
        if (wrapLines)
            n = lineWCount;
        else
            n = lineCount;
        return n * fontHeight + 2; // for 1 pixel spacing
    }
    YWindow *getWindow() { return this; }

    int getFontWidth() { return fontWidth; }
    int getFontHeight() { return fontHeight; }

    virtual void handleClick(const XButtonEvent &up, int /*count*/) {
        if (up.button == 3) {
            menu->popup(this, 0, 0, up.x_root, up.y_root,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
            return ;
        }
    }

    virtual void actionPerformed(YAction action, unsigned int /*modifiers*/) {
        if (action == actionToggleHexView) {
            hexView = hexView ? false : true;
            repaint();
        } else if (action == actionToggleExpandTabs) {
            expandTabs = expandTabs ? false : true;
            repaint();
        } else if (action == actionToggleWrapLines) {
            wrapLines = wrapLines ? false : true;
            findWLines(width() / fontWidth);
            repaint();
        } else if (action == actionClose)
            exit(0);
    }

    virtual void configure(const YRect &r) {
        YWindow::configure(r);
        if (fWidth != int(r.width()) || fHeight != int(r.height())) {
            fWidth = int(r.width());
            fHeight = int(r.height());
            if (wrapLines) {
                int nw = lineWCount;
                findWLines(r.width() / fontWidth);
                if (lineWCount != nw)
                    repaint();
            }
            resetScroll();
        }
   }
private:
    int bufLen;
    char *buf;
    int lineCount;
    int *linePos;
    int lineWCount;
    int *lineWPos;

    int fWidth;
    int fHeight;

    int chunkCount;
    int maxLineLen;
    char *fmt;
    int maxWidth;
    int tx, ty;
    int fontWidth, fontHeight;
    int wrapWidth;

    YColorName bg, fg;
    ref<YFont> font;
    YScrollView *view;
    YScrollBar *fVerticalScroll;
    YScrollBar *fHorizontalScroll;

    bool expandTabs;
    bool hexView;
    bool wrapLines;

    YMenu *menu;
    YAction actionClose;
    YAction actionToggleExpandTabs, actionToggleWrapLines, actionToggleHexView;
};

class FileView: public YWindow {
public:
    FileView(char *path) {
        setDND(true);
        fPath = newstr(path);

        scroll = new YScrollView(this);
        view = new TextView(scroll, this);
        scroll->setView(view);

        view->show();
        scroll->show();

        setTitle(fPath);
        file = YIcon::getIcon("file");
#if 0
        Pixmap icons[4];
        icons[0] = file->small()->pixmap();
        icons[1] = file->small()->mask();
        icons[2] = file->large()->pixmap();
        icons[3] = file->large()->mask();
        XChangeProperty(xapp->display(), handle(),
                        _XA_WIN_ICONS, XA_PIXMAP,
                        32, PropModeReplace,
                        (unsigned char *)icons, 4);
#endif
        int x = 80 * view->getFontWidth();
        int y = 30 * view->getFontHeight();

        setSize(x, y);

        loadFile();

        printf("x:y = %d:%d\n", view->contentWidth(), view->contentHeight());

        if (int(view->contentWidth()) < x)
            x = int(view->contentWidth());
        if (int(view->contentHeight()) < y)
            y = int(view->contentHeight());

        if (x < 200)
            x = 200;
        if (y < 150)
            y = 150;

        setSize(x, y);
    }

    void loadFile() {
        struct stat sb;

        int fd = open(fPath, O_RDONLY);
        if (fd == -1)
            return;
        if (fstat(fd, &sb) != 0) {
            close(fd);
            return;
        }
        int len = sb.st_size;
        char *buf;

        if ((buf = (char *)mmap(0, len, PROT_READ, MAP_SHARED, fd, 0)) == 0) {
            buf = (char *)malloc(len);
            if (buf == 0) {
                close(fd);
                return;
            }
            if ((len = read(fd, buf, len)) < 0) {
                free(buf);
                close(fd);
                return;
            }
        }


        view->setData(buf, len);
        close(fd);
    }

    virtual void configure(const YRect &r) {
        YWindow::configure(r);
        scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
    }

    virtual void handleClose() {
        delete this;
        xapp->exit(0);
    }

private:
    char *fPath;

    TextView *view;
    YScrollView *scroll;
};

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    YXApplication app(&argc, &argv);

    if (argc > 1) {
        FileView *view = new FileView(argv[1]);
        view->show();

        return app.mainLoop();
    }
}

// vim: set sw=4 ts=4 et:
