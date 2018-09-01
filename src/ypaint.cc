/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ypaint.h"
#include "yxapp.h"
#include "yprefs.h"
#include "intl.h"
#include <ctype.h>

#ifdef CONFIG_XFREETYPE
#include <X11/Xft/Xft.h>
#endif

#ifdef DEBUG
/* since recently sometimes copy area for NULL pixmap is done: */
#define XCopyArea(a,b,c,d,e,f,g,h,i,j) \
    do { Drawable B(b),C(c); PRECONDITION(B); PRECONDITION(C); ::XCopyArea(a,B,C,d,e,f,g,h,i,j); } while (0)
#endif

static inline Display* display()  { return xapp->display(); }
static inline Colormap colormap() { return xapp->colormap(); }
static inline Visual*  visual()   { return xapp->visual(); }

/******************************************************************************/

Graphics::Graphics(YWindow & window,
                   unsigned long vmask, XGCValues * gcv):
    fDrawable(window.handle()),
    fColor(), fFont(null),
    xOrigin(0), yOrigin(0)
{
    rWidth = window.width();
    rHeight = window.height();
    rDepth = (window.depth() ? window.depth() : xapp->depth());
    gc = XCreateGC(display(), drawable(), vmask, gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = 0;
#endif
}

Graphics::Graphics(YWindow & window):
    fDrawable(window.handle()),
    fColor(), fFont(null),
    xOrigin(0), yOrigin(0)
 {
    rWidth = window.width();
    rHeight = window.height();
    rDepth = (window.depth() ? window.depth() : xapp->depth());
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(display(), drawable(), GCGraphicsExposures, &gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = 0;
#endif
}

Graphics::Graphics(ref<YPixmap> pixmap, int x_org, int y_org):
    fDrawable(pixmap->pixmap()),
    fColor(), fFont(null),
    xOrigin(x_org), yOrigin(y_org)
 {
    rWidth = pixmap->width();
    rHeight = pixmap->height();
    rDepth = pixmap->depth();
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(display(), drawable(), GCGraphicsExposures, &gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = 0;
#endif
}

Graphics::Graphics(Drawable drawable, unsigned w, unsigned h, unsigned depth,
                   unsigned long vmask, XGCValues * gcv):
    fDrawable(drawable),
    fColor(), fFont(null),
    xOrigin(0), yOrigin(0),
    rWidth(w), rHeight(h), rDepth(depth)
{
    gc = XCreateGC(display(), drawable, vmask, gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = 0;
#endif
}

Graphics::Graphics(Drawable drawable, unsigned w, unsigned h, unsigned depth):
    fDrawable(drawable),
    fColor(), fFont(null),
    xOrigin(0), yOrigin(0),
    rWidth(w), rHeight(h), rDepth(depth)
{
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(display(), drawable, GCGraphicsExposures, &gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = 0;
#endif
}

Graphics::~Graphics() {
    XFreeGC(display(), gc);
    gc = None;

#ifdef CONFIG_XFREETYPE
    if (fXftDraw) {
        XftDrawDestroy(fXftDraw);
        fXftDraw = 0;
    }
#endif
}

#ifdef CONFIG_XFREETYPE
XftDraw* Graphics::handleXft() {
    if (fXftDraw == 0) {
        fXftDraw = XftDrawCreate(display(), drawable(),
                    visual(), colormap());
    }
    return fXftDraw;
}
#endif

/******************************************************************************/

void Graphics::copyArea(const int x, const int y,
                        const unsigned width, const unsigned height,
                        const int dx, const int dy)
{
    XCopyArea(display(), drawable(), drawable(), gc,
              x - xOrigin, y - yOrigin, width, height,
              dx - xOrigin, dy - yOrigin);
}

void Graphics::copyDrawable(Drawable const d,
                            const int x, const int y,
                            const unsigned w, const unsigned h,
                            const int dx, const int dy)
{
    if (d == None)
        return;

    XCopyArea(display(), d, drawable(), gc,
              x, y, w, h,
              dx - xOrigin, dy - yOrigin);
}

void Graphics::copyPixmap(ref<YPixmap> p,
                          const int x, const int y,
                          const unsigned w, const unsigned h,
                          const int dx, const int dy)
{
    if (p == null)
        return;
    if (p->depth() == rdepth()) {
        copyDrawable(p->pixmap(), x, y, w, h, dx, dy);
        return;
    }

    if (32 == rdepth()) {
        Pixmap pixmap32 = p->pixmap32();
        if (pixmap32) {
            copyDrawable(pixmap32, x, y, w, h, dx, dy);
            return;
        }
    }

    tlog("%s:%d:Graphics::%s: attempt to copy pixmap 0x%lx of depth %d using gc of depth %d",
            __FILE__, __LINE__, __func__, p->pixmap(), p->depth(), rdepth());
}

/******************************************************************************/

void Graphics::drawPoint(int x, int y) {
    XDrawPoint(display(), drawable(), gc,
               x - xOrigin, y - yOrigin);
}

void Graphics::drawLine(int x1, int y1, int x2, int y2) {
    XDrawLine(display(), drawable(), gc,
              x1 - xOrigin, y1 - yOrigin,
              x2 - xOrigin, y2 - yOrigin);
}

void Graphics::drawLines(XPoint *points, int n, int mode) {
    int n1 = (mode == CoordModeOrigin) ? n : 1;
    for (int i = 0; i < n1; i++) {
        points[i].x -= xOrigin;
        points[i].y -= yOrigin;
    }
    XDrawLines(display(), drawable(), gc, points, n, mode);
    for (int i = 0; i < n1; i++) {
        points[i].x += xOrigin;
        points[i].y += yOrigin;
    }
}

void Graphics::drawSegments(XSegment *segments, int n) {
    for (int i = 0; i < n; i++) {
        segments[i].x1 -= xOrigin;
        segments[i].y1 -= yOrigin;
        segments[i].x2 -= xOrigin;
        segments[i].y2 -= yOrigin;
    }
    XDrawSegments(display(), drawable(), gc, segments, n);
    for (int i = 0; i < n; i++) {
        segments[i].x1 += xOrigin;
        segments[i].y1 += yOrigin;
        segments[i].x2 += xOrigin;
        segments[i].y2 += yOrigin;
    }
}

void Graphics::drawRect(int x, int y, unsigned width, unsigned height) {
    XDrawRectangle(display(), drawable(), gc,
                   x - xOrigin, y - yOrigin, width, height);
}

void Graphics::drawRects(XRectangle *rects, unsigned n) {
    for (unsigned i = 0; i < n; i++) {
        rects[i].x -= xOrigin;
        rects[i].y -= yOrigin;
    }
    XDrawRectangles(display(), drawable(), gc, rects, n);
    for (unsigned i = 0; i < n; i++) {
        rects[i].x += xOrigin;
        rects[i].y += yOrigin;
    }
}

void Graphics::drawArc(int x, int y, unsigned width, unsigned height, int a1, int a2) {
    XDrawArc(display(), drawable(), gc,
             x - xOrigin, y - yOrigin, width, height, a1, a2);
}

/******************************************************************************/

void Graphics::drawChars(const ustring &s, int x, int y) {
    if (fFont != null && s != null) {
        cstring cs(s);
        fFont->drawGlyphs(*this, x, y, cs.c_str(), cs.c_str_len());
    }
}

void Graphics::drawChars(const char *data, int offset, int len, int x, int y) {
    if (fFont != null)
        fFont->drawGlyphs(*this, x, y, data + offset, len);
}

void Graphics::drawString(int x, int y, char const * str) {
    drawChars(str, 0, strlen(str), x, y);
}

void Graphics::drawStringEllipsis(int x, int y, const char *str, int maxWidth) {
    int const len(strlen(str));
    int const w = (fFont != null) ? fFont->textWidth(str, len) : 0;

    if (fFont == null || w <= maxWidth) {
        drawChars(str, 0, len, x, y);
    } else {
        int maxW = 0;
        if (!showEllipsis)
            maxW = (maxWidth);
        else
            maxW = (maxWidth - fFont->textWidth("...", 3));

        int l(0), w(0);
        int sl(0), sw(0);

#ifdef CONFIG_I18N
        if (multiByte) mblen(NULL, 0);
#endif

        if (maxW > 0) {
            while (l < len) {
                int nc, wc;
#ifdef CONFIG_I18N
                if (multiByte) {
                    nc = mblen(str + l, len - l);
                    if (nc < 1) { // bad things
                        l++;
                        continue;
                    }
                    wc = fFont->textWidth(str + l, nc);
                } else
#endif
                {
                    nc = 1;
                    wc = fFont->textWidth(str + l, 1);
                }

                if (w + wc < maxW) {
                    if (1 == nc && isspace (str[l]))
                    {
                        sl+= nc;
                        sw+= wc;
                    } else {
                        sl =
                            sw = 0;
                    }

                    l+= nc;
                    w+= wc;
                } else
                    break;
            }
        }

        l -= sl;
        w -= sw;

        if (l > 0)
            drawChars(str, 0, l, x, y);
        if (showEllipsis) {
            if (l < len)
                drawChars("...", 0, 3, x + w, y);
        }
    }
}

void Graphics::drawStringEllipsis(int x, int y, const ustring &str, int maxWidth) {
    cstring cs(str);
    return drawStringEllipsis(x, y, cs.c_str(), maxWidth);
}

void Graphics::drawCharUnderline(int x, int y, const char *str, int charPos) {
/// TODO #warning "FIXME: don't mess with multibyte here, use a wide char"
    int left = 0; //fFont ? fFont->textWidth(str, charPos) : 0;
    int right = 0; // fFont ? fFont->textWidth(str, charPos + 1) - 1 : 0;
    int len = strlen(str);
    int c = 0, cp = 0;

#ifdef CONFIG_I18N
    if (multiByte) mblen(NULL, 0);
#endif
    while (c <= len && cp <= charPos + 1) {
        if (charPos == cp) {
            left = (fFont != null) ? fFont->textWidth(str, c) : 0;
            //            msg("l: %d %d %d %d %d", c, cp, charPos, left, right);
        } else if (charPos + 1 == cp) {
            right = (fFont != null) ? fFont->textWidth(str, c) - 1: 0;
            //            msg("l: %d %d %d %d %d", c, cp, charPos, left, right);
            break;
        }
        if (c >= len || str[c] == '\0')
            break;
#ifdef CONFIG_I18N
        if (multiByte) {
            int nc = mblen(str + c, len - c);
            if (nc < 1) { // bad things
                c++;
                cp++;
            } else {
                c += nc;
                cp += nc;
            }
        } else
#endif
        {
            c++;
            cp++;
        }
    }
    //    msg("%d %d %d %d %d", c, cp, charPos, left, right);

    if (left < right)
        drawLine(x + left, y + 2, x + right, y + 2);
}

void Graphics::drawCharUnderline(int x, int y, const ustring &str, int charPos) {
    cstring cs(str);
    return drawCharUnderline(x, y, cs.c_str(), charPos);
}

void Graphics::drawStringMultiline(int x, int y, const char *str) {
    unsigned const tx(x + fFont->multilineTabPos(str));

    for (const char * end(strchr(str, '\n')); end;
         str = end + 1, end = strchr(str, '\n')) {
        int const len(end - str);
        const char * tab((const char *) memchr(str, '\t', len));

        if (tab) {
            drawChars(str, 0, tab - str, x, y);
            drawChars(tab + 1, 0, end - tab - 1, tx, y);
        }
        else
            drawChars(str, 0, end - str, x, y);

        y+= fFont->height();
    }

    const char * tab(strchr(str, '\t'));

    if (tab) {
        drawChars(str, 0, tab - str, x, y);
        drawChars(tab + 1, 0, strlen(tab + 1), tx, y);
    }
    else
        drawChars(str, 0, strlen(str), x, y);
}

void Graphics::drawStringMultiline(int x, int y, const ustring &str) {
    cstring cs(str);
    return drawStringMultiline(x, y, cs.c_str());
}

/******************************************************************************/

void Graphics::fillRect(int x, int y, unsigned width, unsigned height) {
    XFillRectangle(display(), drawable(), gc,
                   x - xOrigin, y - yOrigin, width, height);
}

void Graphics::fillRects(XRectangle *rects, int n) {
    for (int i = 0; i < n; i++) {
        rects[i].x -= xOrigin;
        rects[i].y -= yOrigin;
    }
    XFillRectangles(display(), drawable(), gc, rects, n);
    for (int i = 0; i < n; i++) {
        rects[i].x += xOrigin;
        rects[i].y += yOrigin;
    }
}

void Graphics::fillPolygon(XPoint *points, int const n, int const shape,
                           int const mode)
{
    int n1 = (mode == CoordModeOrigin) ? n : 1;

    for (int i = 0; i < n1; i++) {
        points[i].x -= xOrigin;
        points[i].y -= yOrigin;
    }
    XFillPolygon(display(), drawable(), gc, points, n, shape, mode);
    for (int i = 0; i < n1; i++) {
        points[i].x += xOrigin;
        points[i].y += yOrigin;
    }
}

void Graphics::fillArc(int x, int y, unsigned width, unsigned height, int a1, int a2) {
    XFillArc(display(), drawable(), gc,
             x - xOrigin, y - yOrigin, width, height, a1, a2);
}

/******************************************************************************/

void Graphics::setColor(YColor aColor) {
    fColor = aColor;
    setColorPixel(fColor.pixel());
}

void Graphics::setColorPixel(unsigned long pixel) {
    if (rdepth() == 32)
        pixel |= 0xff000000;
    XSetForeground(display(), gc, pixel);
}

void Graphics::setFont(ref<YFont> aFont) {
    fFont = aFont;
}

void Graphics::setLineWidth(unsigned width) {
    XGCValues gcv;
    gcv.line_width = width;
    XChangeGC(display(), gc, GCLineWidth, &gcv);
}

void Graphics::setPenStyle(bool dotLine) {
    XGCValues gcv;

    if (dotLine) {
        char c = 1;
        gcv.line_style = LineOnOffDash;
        XSetDashes(display(), gc, 0, &c, 1);
    } else {
        gcv.line_style = LineSolid;
    }

    XChangeGC(display(), gc, GCLineStyle, &gcv);
}

void Graphics::setFunction(int function) {
    XSetFunction(display(), gc, function);
}

/******************************************************************************/

void Graphics::drawImage(ref<YImage> pix, int const x, int const y) {
    pix->draw(*this, x, y);
}

void Graphics::drawImage(ref<YImage> pix, int x, int y, unsigned w, unsigned h, int dx, int dy) {
    pix->draw(*this, x, y, w, h, dx, dy);
}

void Graphics::drawPixmap(ref<YPixmap> pix, int const x, int const y) {
    if (pix->depth() != rdepth()) {
        tlog("Graphics::%s: attempt to draw pixmap 0x%lx of depth %d with gc of depth %d\n",
                __func__, pix->pixmap(), pix->depth(), rdepth());
        return;
    }
    if (pix->mask())
        drawClippedPixmap(pix->pixmap(),
                          pix->mask(),
                          0, 0, pix->width(), pix->height(), x, y);
    else
        XCopyArea(display(), pix->pixmap(), drawable(), gc,
                  0, 0, pix->width(), pix->height(), x - xOrigin, y - yOrigin);
}

void Graphics::drawPixmap(ref<YPixmap> pix, int const sx, int const sy,
        const unsigned w, const unsigned h, const int dx, const int dy) {
    if (pix->depth() != rdepth()) {
        tlog("Graphics::%s: attempt to draw pixmap 0x%lx of depth %d with gc of depth %d\n",
                __func__, pix->pixmap(), pix->depth(), rdepth());
        return;
    }
    if (pix->mask())
        drawClippedPixmap(pix->pixmap(),
                          pix->mask(),
                          sx, sy, w, h, dx, dy);
    else
        XCopyArea(display(), pix->pixmap(), drawable(), gc,
                  sx, sy, w, h, dx - xOrigin, dy - yOrigin);
}

void Graphics::drawMask(ref<YPixmap> pix, int const x, int const y) {
    if (pix->mask())
        XCopyArea(display(), pix->mask(), drawable(), gc,
                  0, 0, pix->width(), pix->height(), x - xOrigin, y - yOrigin);
}

void Graphics::drawClippedPixmap(Pixmap pix, Pixmap clip,
                                 int x, int y, unsigned w, unsigned h, int toX, int toY)
{
    static GC clipPixmapGC = None;
    XGCValues gcv;

    if (clipPixmapGC == None) {
        gcv.graphics_exposures = False;
        clipPixmapGC = XCreateGC(display(), desktop->handle(),
                                 GCGraphicsExposures,
                                 &gcv);
    }

    gcv.clip_mask = clip;
    gcv.clip_x_origin = toX - xOrigin;
    gcv.clip_y_origin = toY - yOrigin;
    XChangeGC(display(), clipPixmapGC,
              GCClipMask|GCClipXOrigin|GCClipYOrigin, &gcv);
    XCopyArea(display(), pix, drawable(), clipPixmapGC,
              x, y, w, h, toX - xOrigin, toY - yOrigin);
    gcv.clip_mask = None;
    XChangeGC(display(), clipPixmapGC, GCClipMask, &gcv);
}

void Graphics::compositeImage(ref<YImage> img, int const sx, int const sy, unsigned w, unsigned h, int dx, int dy) {
    if (img != null) {
        int rx = dx;
        int ry = dy;
        int rw = w;
        int rh = h;

#if 0
        if (rx < xOrigin) {
            rw -= xOrigin - rx;
            rx = xOrigin;
        }
        if (ry < yOrigin) {
            rh -= yOrigin - ry;
            ry = yOrigin;
        }
        if (rx + rw > xOrigin + rWidth) {
            rw = xOrigin + rWidth - rx;
        }
        if (ry + rh > yOrigin + rHeight) {
            rh = yOrigin + rHeight - ry;
        }
#endif

#if 0
        msg("drawImage %d %d %d %d %dx%d | %d %d | %d %d | %d %d | %d %d",
            sx, sy, dx, dy, dw, dh, xorigin(), yorigin(), sx, sy,
            dx - x, dy - y, dx - xOrigin, dy - yOrigin);
#endif
        if (rw <= 0 || rh <= 0)
            return;
        //msg("call composite %d %d %d %d | %d %d %d %d", dx, dy, dw, dh, x, y, xOrigin, yOrigin);
        img->composite(*this, sx, sy, rw, rh, rx, ry);
    }
}

/******************************************************************************/

void Graphics::draw3DRect(int x, int y, unsigned w, unsigned h, bool raised) {
    YColor back(color());
    YColor bright(back.brighter());
    YColor dark(back.darker());
    YColor t(raised ? bright : dark);
    YColor b(raised ? dark : bright);

    setColor(t);
    drawLine(x, y, x + w, y);
    drawLine(x, y, x, y + h);
    setColor(b);
    drawLine(x, y + h, x + w, y + h);
    drawLine(x + w, y, x + w, y + h);
    setColor(back);
    drawPoint(x + w, y);
    drawPoint(x, y + h);
}

void Graphics::drawBorderW(int x, int y, unsigned w, unsigned h, bool raised) {
    YColor back(color());
    YColor bright(back.brighter());
    YColor dark(back.darker());

    if (raised) {
        setColor(bright);
        drawLine(x, y, x + w - 1, y);
        drawLine(x, y, x, y + h - 1);
        setColor(YColor::black);
        drawLine(x, y + h, x + w, y + h);
        drawLine(x + w, y, x + w, y + h);
        setColor(dark);
        drawLine(x + 1, y + h - 1, x + w - 1, y + h - 1);
        drawLine(x + w - 1, y + 1, x + w - 1, y + h - 1);
    } else {
        setColor(bright);
        drawLine(x + 1, y + h, x + w, y + h);
        drawLine(x + w, y + 1, x + w, y + h);
        setColor(YColor::black);
        drawLine(x, y, x + w, y);
        drawLine(x, y, x, y + h);
        setColor(dark);
        drawLine(x + 1, y + 1, x + w - 1, y + 1);
        drawLine(x + 1, y + 1, x + 1, y + h - 1);
    }
    setColor(back);
}

// doesn't move... needs two pixels on all sides for up and down
// position.
void Graphics::drawBorderM(int x, int y, unsigned w, unsigned h, bool raised) {
    YColor back(color());
    YColor bright(back.brighter());
    YColor dark(back.darker());

    if (raised) {
        setColor(bright);
        drawLine(x + 1, y + 1, x + w, y + 1);
        drawLine(x + 1, y + 1, x + 1, y + h);
        drawLine(x + 1, y + h, x + w, y + h);
        drawLine(x + w, y + 1, x + w, y + h);

        setColor(dark);
        drawLine(x, y, x + w - 1, y);
        drawLine(x, y, x, y + h);
        drawLine(x, y + h - 1, x + w - 1, y + h - 1);
        drawLine(x + w - 1, y, x + w - 1, y + h - 1);

        ///  how to get the color of the taskbar??
        setColor(back);

        drawPoint(x, y + h);
        drawPoint(x + w, y);
    } else {
        setColor(dark);
        drawLine(x, y, x + w - 1, y);
        drawLine(x, y, x, y + h - 1);

        setColor(bright);
        drawLine(x + 1, y + h, x + w, y + h);
        drawLine(x + w, y + 1, x + w, y + h);

        setColor(back);
        drawRect(x + 1, y + 1, x + w - 2, y + h - 2);
        //drawLine(x + 1, y + 1, x + w - 1, y + 1);
        //drawLine(x + 1, y + 1, x + 1, y + h - 1);
        //drawLine(x, y + h - 1, x + w - 1, y + h - 1);
        //drawLine(x + w - 1, y, x + w - 1, y + h - 1);

        ///  how to get the color of the taskbar??
        drawPoint(x, y + h);
        drawPoint(x + w, y);
    }
}

void Graphics::drawBorderG(int x, int y, unsigned w, unsigned h, bool raised) {
    YColor back(color());
    YColor bright(back.brighter());
    YColor dark(back.darker());

    if (raised) {
        setColor(bright);
        drawLine(x, y, x + w - 1, y);
        drawLine(x, y, x, y + h - 1);
        setColor(dark);
        drawLine(x, y + h, x + w, y + h);
        drawLine(x + w, y, x + w, y + h);
        setColor(YColor::black);
        drawLine(x + 1, y + h - 1, x + w - 1, y + h - 1);
        drawLine(x + w - 1, y + 1, x + w - 1, y + h - 1);
    } else {
        setColor(bright);
        drawLine(x + 1, y + h, x + w, y + h);
        drawLine(x + w, y + 1, x + w, y + h);
        setColor(YColor::black);
        drawLine(x, y, x + w, y);
        drawLine(x, y, x, y + h);
        setColor(dark);
        drawLine(x + 1, y + 1, x + w - 1, y + 1);
        drawLine(x + 1, y + 1, x + 1, y + h - 1);
    }
    setColor(back);
}

void Graphics::drawCenteredPixmap(int x, int y, unsigned w, unsigned h, ref<YPixmap> pixmap) {
    int r = x + w;
    int b = y + h;
    int pw = pixmap->width();
    int ph = pixmap->height();

    drawOutline(x, y, r, b, pw, ph);
    drawPixmap(pixmap, x + w / 2 - pw / 2, y + h / 2 - ph / 2);
}

void Graphics::drawOutline(int l, int t, int r, int b, unsigned iw, unsigned ih) {
    if (l + (int)iw >= r && t + (int)ih >= b)
        return ;

    int li = (l + r) / 2 - iw / 2;
    int ri = (l + r) / 2 + iw / 2;
    int ti = (t + b) / 2 - ih / 2;
    int bi = (t + b) / 2 + ih / 2;

    if (l < li)
        fillRect(l, t, li - l, b - t);

    if (ri < r)
        fillRect(ri, t, r - ri, b - t);

    if (t < ti)
        if (li < ri)
            fillRect(li, t, ri - li, ti - t);

    if (bi < b)
        if (li < ri)
            fillRect(li, bi, ri - li, b - bi);
}

void Graphics::repHorz(Drawable d, unsigned pw, unsigned ph, int x, int y, unsigned w) {
    if (d == None)
        return;
#if 1
    XSetTile(display(), gc, d);
    XSetTSOrigin(display(), gc, x - xOrigin, y - yOrigin);
    XSetFillStyle(display(), gc, FillTiled);
    XFillRectangle(display(), drawable(), gc, x - xOrigin, y - yOrigin, w, ph);
    XSetFillStyle(display(), gc, FillSolid);
#else
    while (w > 0) {
        XCopyArea(display(), d, drawable(), gc, 0, 0, min(w, pw), ph, x - xOrigin, y - yOrigin);
        x += pw;
        w -= pw;
    }
#endif
}

void Graphics::repVert(Drawable d, unsigned pw, unsigned ph, int x, int y, unsigned h) {
    if (d == None)
        return;
#if 1
    XSetTile(display(), gc, d);
    XSetTSOrigin(display(), gc, x - xOrigin, y - yOrigin);
    XSetFillStyle(display(), gc, FillTiled);
    XFillRectangle(display(), drawable(), gc, x - xOrigin, y - yOrigin, pw, h);
    XSetFillStyle(display(), gc, FillSolid);
#else
    while (h > 0) {
        XCopyArea(display(), d, drawable(), gc, 0, 0, pw, min(h, ph), x - xOrigin, y - yOrigin);
        y += ph;
        h -= ph;
    }
#endif
}

void Graphics::repHorz(ref<YPixmap> p, int x, int y, unsigned w) {
    if (p == null)
        return;

    if (p->depth() == rdepth()) {
        repHorz(p->pixmap(), p->width(), p->height(), x, y, w);
        return;
    }

    if (32 == rdepth()) {
        Pixmap pixmap = p->pixmap32();
        if (pixmap) {
            repHorz(pixmap, p->width(), p->height(), x, y, w);
            return;
        }
    }
}

void Graphics::repVert(ref<YPixmap> p, int x, int y, unsigned h) {
    if (p == null)
        return;

    if (p->depth() == rdepth()) {
        repVert(p->pixmap(), p->width(), p->height(), x, y, h);
        return;
    }

    if (32 == rdepth()) {
        Pixmap pixmap32 = p->pixmap32();
        if (pixmap32) {
            repVert(pixmap32, p->width(), p->height(), x, y, h);
            return;
        }
    }
}

void Graphics::fillPixmap(ref<YPixmap> pixmap, int x, int y,
                          unsigned w, unsigned h, int px, int py) {
    int const pw(pixmap->width());
    int const ph(pixmap->height());

    px%= pw; const int pww(px ? pw - px : 0);
    py%= ph; const int phh(py ? ph - py : 0);

    if (px) {
        if (py)
            XCopyArea(display(), pixmap->pixmap(), drawable(), gc,
                      px, py, pww, phh, x - xOrigin, y - yOrigin);

        for (int yy(y + phh), hh(h - phh); hh > 0; yy += ph, hh -= ph)
            XCopyArea(display(), pixmap->pixmap(), drawable(), gc,
                      px, 0, pww, min(hh, ph), x - xOrigin, yy - yOrigin);
    }

    for (int xx(x + pww), ww(w - pww); ww > 0; xx+= pw, ww-= pw) {
        int const www(min(ww, pw));

        if (py)
            XCopyArea(display(), pixmap->pixmap(), drawable(), gc,
                      0, py, www, phh, xx - xOrigin, y - yOrigin);

        for (int yy(y + phh), hh(h - phh); hh > 0; yy += ph, hh -= ph)
            XCopyArea(display(), pixmap->pixmap(), drawable(), gc,
                      0, 0, www, min(hh, ph), xx - xOrigin, yy - yOrigin);
    }
}

void Graphics::drawSurface(YSurface const & surface, int x, int y, unsigned w, unsigned h,
                           int const sx, int const sy,
                           const unsigned sw, const unsigned sh
) {
    if (surface.gradient != null)
        drawGradient(surface.gradient, x, y, w, h, sx, sy, sw, sh);
    else
    if (surface.pixmap != null)
        fillPixmap(surface.pixmap, x, y, w, h, sx, sy);
    else if (surface.color) {
        setColor(surface.color);
        fillRect(x, y, w, h);
    }
}

void Graphics::drawGradient(ref<YImage> gradient,
                            int const x, int const y, const unsigned w, const unsigned h,
                            int const gx, int const gy, const unsigned gw, const unsigned gh)
{
    ref<YImage> scaled = gradient->scale(gw, gh);
    scaled->draw(*this, gx, gy, w, h, x, y);
}

/******************************************************************************/

void Graphics::drawArrow(YDirection direction, int x, int y, unsigned size,
                         bool pressed)
{
    YColor nc(color());
    YColor oca = pressed ? nc.darker() : nc.brighter(),
        ica = pressed ? YColor::black : nc,
        ocb = pressed ? wmLook == lookGtk ? nc : nc.brighter() : nc.darker(),
        icb = pressed ? nc.brighter() : YColor::black;

    XPoint points[3];

    short const am(size / 2);
    short const ah(wmLook == lookGtk ||
                   wmLook == lookMotif ? size : size / 2);
    short const aw(wmLook == lookGtk ||
                   wmLook == lookMotif ? size : size - (size & 1));

    switch (direction) {
    case Up:
        points[0].x = x;
        points[0].y = y + ah;
        points[1].x = x + am;
        points[1].y = y;
        points[2].x = x + aw;
        points[2].y = y + ah;
        break;

    case Down:
        points[0].x = x;
        points[0].y = y;
        points[1].x = x + am;
        points[1].y = y + ah;
        points[2].x = x + aw;
        points[2].y = y;
        break;

    case Left:
        points[0].x = x + ah;
        points[0].y = y;
        points[1].x = x;
        points[1].y = y + am;
        points[2].x = x + ah;
        points[2].y = y + aw;
        break;

    case Right:
        points[0].x = x;
        points[0].y = y;
        points[1].x = x + ah;
        points[1].y = y + am;
        points[2].x = x;
        points[2].y = y + aw;
        break;

    default:
        return ;
    }

    short const dx0(direction == Up || direction == Down ? 1 : 0);
    short const dy0(direction == Up || direction == Down ? 0 : 1);
    short const dx1(direction == Up || direction == Left ? dy0 : -dy0);
    short const dy1(direction == Up || direction == Left ? dx0 : -dx0);

    //    setWideLines(); // --------------------- render slow, but accurate lines ---
    // ============================================================= inner bevel ===
    if (wmLook == lookGtk || wmLook == lookMotif) {
        setColor(ocb);
        drawLine(points[2].x - dx0 - (size & 1 ? 0 : dx1),
                 points[2].y - (size & 1 ? 0 : dy1) - dy0,
                 points[1].x + 2 * dx1, points[1].y + 2 * dy1);

        setColor(wmLook == lookMotif ? oca : ica);
        drawLine(points[0].x + dx0 - (size & 1 ? dx1 : 0),
                 points[0].y + dy0 - (size & 1 ? dy1 : 0),
                 points[1].x + (size & 1 ? dx0 : 0)
                 + (size & 1 ? dx1 : dx1 + dx1),
                 points[1].y + (size & 1 ? dy0 : 0)
                 + (size & 1 ? dy1 : dy1 + dy1));

        if ((direction == Up || direction == Left)) setColor(ocb);
        drawLine(points[0].x + dx0 - dx1, points[0].y + dy0 - dy1,
                 points[2].x - dx0 - dx1, points[2].y - dy0 - dy1);
    } else if (wmLook == lookWarp3) {
        drawLine(points[0].x + dx0, points[0].y + dy0,
                 points[1].x + dx0, points[1].y + dy0);
        drawLine(points[2].x + dx0, points[2].y + dy0,
                 points[1].x + dx0, points[1].y + dy0);
    } else
        fillPolygon(points, 3, Convex, CoordModeOrigin);

    // ============================================================= outer bevel ===
    if (wmLook == lookMotif || wmLook == lookGtk) {
        setColor(wmLook == lookMotif ? ocb : icb);

        drawLine(points[2].x, points[2].y, points[1].x, points[1].y);

        if (wmLook == lookGtk || wmLook == lookMotif) setColor(oca);
        drawLine(points[0].x, points[0].y, points[1].x, points[1].y);

        if ((direction == Up || direction == Left))
            setColor(wmLook == lookMotif ? ocb : icb);

    } else
        drawLines(points, 3, CoordModeOrigin);

    if (wmLook != lookWarp3)
        drawLine(points[0].x, points[0].y, points[2].x, points[2].y);

    //    setThinLines(); // ---------- render fast, but possibly inaccurate lines ---
    setColor(nc);
}

int Graphics::function() const {
    XGCValues values;
    XGetGCValues(display(), gc, GCFunction, &values);
    return values.function;
}

void Graphics::setClipRectangles(XRectangle *rect, int count) {
    XSetClipRectangles(display(), gc,
                       -xOrigin, -yOrigin, rect, count, Unsorted);
#ifdef CONFIG_XFREETYPE
    XftDrawSetClipRectangles(handleXft(), -xOrigin, -yOrigin, rect, count);
#endif
}

void Graphics::setClipMask(Pixmap mask) {
    XSetClipMask(display(), gc, mask);
}

void Graphics::resetClip() {
    XSetClipMask(display(), gc, None);
#ifdef CONFIG_XFREETYPE
    XftDrawSetClip(handleXft(), 0);
#endif
}

/******************************************************************************/
/******************************************************************************/

// vim: set sw=4 ts=4 et:
