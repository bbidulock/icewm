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
#include "upath.h"
#include "yimage.h"
#include "ylocale.h"
#include "prefs.h"
#include "yicon.h"
#include "intl.h"

char const *ApplicationName = "iceview";

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
        menu->addItem(_("Hex View"), 0, _("Ctrl+H"), actionToggleHexView);
        menu->addItem(_("Expand Tabs"), 0, _("Ctrl+T"), actionToggleExpandTabs);
        menu->addItem(_("Wrap Lines"), 0, _("Ctrl+W"), actionToggleWrapLines);
        menu->addSeparator();
        menu->addItem(_("Close"), 0, _("Ctrl+Q"), actionClose);
    }

    ~TextView() {
        delete menu;
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

    void setImage(ref<YImage> image) {
        fImage = image;
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
        int wx = r.x();
        int wy = r.y();
        int wwidth = int(r.width());
        int wheight = int(r.height());

        g.setColor(bg);
        g.setFont(font);

        if (fImage != null) {
            int ix = tx;
            int iy = ty;
            int iw = min(wx + wwidth, ix + int(fImage->width()));
            int ih = min(wy + wheight, iy + int(fImage->height()));
            if (wx < iw && wy < ih) {
                g.drawImage(fImage, ix + wx, iy + wy, iw - wx, ih - wy, wx, wy);
            }
            if (wx + wwidth > int(fImage->width())) {
                g.fillRect(fImage->width(), wy, wx + wwidth - fImage->width(), wheight);
            }
            if (wy + wheight > int(fImage->height())) {
                g.fillRect(wx, fImage->height(), wwidth, wy + wheight - fImage->height());
            }
            return;
        }

        g.fillRect(wx, wy, wwidth, wheight);
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
        if (fImage != null)
            return fImage->width();
        else if (hexView)
            return 78 * fontWidth + 2;
        else if (wrapLines)
            return wrapWidth * fontWidth;
        else
            return maxWidth + 2;
    }
    unsigned contentHeight() {
        if (fImage != null)
            return fImage->height();
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

    virtual void handleButton(const XButtonEvent &up) {
        if (up.button == Button4 || up.button == Button5) {
            fVerticalScroll->handleScrollMouse(up);
        }
    }

    virtual void handleClick(const XButtonEvent &up, int /*count*/) {
        if (up.button == 3) {
            menu->popup(this, 0, 0, up.x_root, up.y_root,
                        YPopupWindow::pfCanFlipVertical |
                        YPopupWindow::pfCanFlipHorizontal |
                        YPopupWindow::pfPopupMenu);
            return ;
        }
    }

    virtual bool handleKey(const XKeyEvent& key) {
        if (fVerticalScroll->handleScrollKeys(key) == true) {
            return true;
        }
        if (fHorizontalScroll->handleScrollKeys(key) == true) {
            return true;
        }
        if (key.type == KeyPress) {
            KeySym k = keyCodeToKeySym(key.keycode);
            int m = KEY_MODMASK(key.state);
            if (k == XK_Escape) {
                actionPerformed(actionClose, m);
                return true;
            }
            else if (k == XK_q && hasbit(m, ControlMask)) {
                actionPerformed(actionClose, m);
                return true;
            }
        }
        return YWindow::handleKey(key);
    }

    virtual void actionPerformed(YAction action, unsigned int modifiers) {
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
        } else if (action == actionClose) {
            if (hasbit(modifiers, ControlMask)) {
                xapp->exit(0);
            } else {
                parent()->handleClose();
            }
        }
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
    ref<YImage> fImage;
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
    FileView(char *path) :
        YWindow(desktop, None),
        fPath(newstr(path)),
        scroll(new YScrollView(this)),
        view(new TextView(scroll, this))
    {
        scroll->setView(view);
        view->show();
        scroll->show();

        int x = max(200, 80 * view->getFontWidth());
        int y = max(150, 30 * view->getFontHeight());

        loadFile();

        x = min(max(x, int(view->contentWidth())),
                xapp->displayWidth() - 100);
        y = min(max(y, int(view->contentHeight())),
                xapp->displayHeight() - 100);

        setSize(x, y);

        if (window_group == None) {
            window_group = XCreateSimpleWindow(
                    xapp->display(),
                    xapp->root(),
                    0, 0, 1, 1, 0,
                    xapp->black(),
                    xapp->black());
        }

        ref<YIcon> file = YIcon::getIcon("icewm");
        if (file != null) {
            unsigned depth = xapp->depth();
            large = YPixmap::createFromImage(file->large(), depth);
        }

        static char wm_clas[] = "IceWM";
        static char wm_name[] = "iceview";
        XClassHint class_hint = { wm_name, wm_clas };
        XSizeHints size_hints = { PSize, 0, 0, x, y };
        XWMHints wmhints = {
            InputHint | StateHint | WindowGroupHint,
            True,
            NormalState,
            large != null ? large->pixmap() : None, None, 0, 0,
            large != null ? large->mask() : None,
            window_group
        };
        if (wmhints.icon_pixmap)
            wmhints.flags |= IconPixmapHint;
        if (wmhints.icon_mask)
            wmhints.flags |= IconMaskHint;
        Xutf8SetWMProperties(xapp->display(), handle(),
                             fPath, ApplicationName, nullptr, 0,
                             &size_hints, &wmhints, &class_hint);

        setNetPid();
        setDND(true);

        views += this;
    }

    ~FileView() {
        delete scroll;
        delete view;
    }

    void loadFile() {
        upath path(fPath);
        mstring ext(path.getExtension().lower());
        if (ext == ".xpm" || ext == ".png" || ext == ".svg" ||
            ext == ".jpg" || ext == ".jpeg")
        {
            ref<YImage> image = YImage::load(path);
            if (image != null) {
                view->setImage(image);
                if (width() < image->width() ||
                    height() < image->height())
                {
                    setSize(max(width(), image->width()),
                            max(height(), image->height()));
                }
            }
        }
        else
        {
            char* buf = path.loadText();
            if (buf) {
                int len = strlen(buf);
                view->setData(buf, len);
            }
        }
    }

    virtual void configure(const YRect &r) {
        YWindow::configure(r);
        scroll->setGeometry(YRect(0, 0, r.width(), r.height()));
    }

    static int getCount() {
        return views.getCount();
    }

    static void remove(FileView* view) {
        findRemove(views, view);
        if (views.isEmpty()) {
            xapp->exit(0);
        }
    }

    virtual void handleClose() {
        remove(this);
    }

    static void closeAll() {
        for (int i = views.getCount(); i-- > 0; ) {
            views.remove(i);
        }
        if (window_group) {
            XDestroyWindow(xapp->display(), window_group);
            window_group = None;
        }
    }

private:
    char *fPath;

    YScrollView *scroll;
    TextView *view;

    ref<YPixmap> large;

    static Window window_group;
    static YObjectArray<FileView> views;
};

Window FileView::window_group;
YObjectArray<FileView> FileView::views;

int main(int argc, char **argv) {
    YLocale locale;

    bindtextdomain(PACKAGE, LOCDIR);
    textdomain(PACKAGE);

    YXApplication app(&argc, &argv);
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            continue;
        }
        else {
            FileView* view = new FileView(argv[i]);
            view->show();
        }
    }
    if (FileView::getCount() > 0) {
        app.mainLoop();
        FileView::closeAll();
    }
    return app.exitCode();
}

// vim: set sw=4 ts=4 et:
