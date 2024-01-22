/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ypaint.h"
#include "yxapp.h"
#include "yprefs.h"
#include "yfontbase.h"
#include "ascii.h"
#include "intl.h"
#include <stdlib.h>

#ifdef CONFIG_XFREETYPE
#include <X11/Xft/Xft.h>
#endif

#ifdef CONFIG_I18N
#include <wctype.h>
#endif

static inline Display* display()  { return xapp->display(); }

/******************************************************************************/

Graphics::Graphics(YWindow & window,
                   unsigned long vmask, XGCValues * gcv):
    fDrawable(window.handle()),
    fColor(), fFont(),
    fPicture(None),
    xOrigin(0), yOrigin(0)
{
    rWidth = window.width();
    rHeight = window.height();
    rDepth = (window.depth() ? window.depth() : xapp->depth());
    gc = XCreateGC(display(), drawable(), vmask, gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = nullptr;
#endif
}

Graphics::Graphics(YWindow & window):
    fDrawable(window.handle()),
    fColor(), fFont(),
    fPicture(None),
    xOrigin(0), yOrigin(0)
{
    rWidth = window.width();
    rHeight = window.height();
    rDepth = (window.depth() ? window.depth() : xapp->depth());
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(display(), drawable(), GCGraphicsExposures, &gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = nullptr;
#endif
}

Graphics::Graphics(ref<YPixmap> pixmap):
    fDrawable(pixmap->pixmap()),
    fColor(), fFont(),
    fPicture(None),
    xOrigin(0), yOrigin(0)
{
    rWidth = pixmap->width();
    rHeight = pixmap->height();
    rDepth = pixmap->depth();
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(display(), drawable(), GCGraphicsExposures, &gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = nullptr;
#endif
}

Graphics::Graphics(Drawable drawable, unsigned w, unsigned h, unsigned depth,
                   unsigned long vmask, XGCValues * gcv):
    fDrawable(drawable),
    fColor(), fFont(),
    fPicture(None),
    xOrigin(0), yOrigin(0),
    rWidth(w), rHeight(h), rDepth(depth)
{
    gc = XCreateGC(display(), drawable, vmask, gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = nullptr;
#endif
}

Graphics::Graphics(Drawable drawable, unsigned w, unsigned h, unsigned depth):
    fDrawable(drawable),
    fColor(), fFont(),
    fPicture(None),
    xOrigin(0), yOrigin(0),
    rWidth(w), rHeight(h), rDepth(depth)
{
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(display(), drawable, GCGraphicsExposures, &gcv);
#ifdef CONFIG_XFREETYPE
    fXftDraw = nullptr;
#endif
}

Graphics::~Graphics() {
    XFreeGC(display(), gc);
    gc = None;

    if (fPicture) {
        XRenderFreePicture(display(), fPicture);
        fPicture = None;
    }

#ifdef CONFIG_XFREETYPE
    if (fXftDraw) {
        XftDrawDestroy(fXftDraw);
        fXftDraw = nullptr;
    }
#endif
}

#ifdef CONFIG_XFREETYPE
XftDraw* Graphics::handleXft() {
    if (fXftDraw == nullptr) {
        fXftDraw = XftDrawCreate(display(), drawable(),
                                 xapp->visualForDepth(rdepth()),
                                 xapp->colormapForDepth(rdepth()));
    }
    return fXftDraw;
}
#endif

Picture Graphics::picture() {
    if (fPicture == None) {
        XRenderPictFormat* format = xapp->formatForDepth(rDepth);
        if (format) {
            XRenderPictureAttributes attr;
            unsigned long mask = None;
            attr.component_alpha = (rDepth == 32);
            mask |= CPComponentAlpha;
            fPicture = XRenderCreatePicture(display(), fDrawable,
                                            format, mask, &attr);
        }
    }
    return fPicture;
}

/******************************************************************************/

void Graphics::clear()
{
    clearArea(0, 0, rwidth(), rheight());
}

void Graphics::clearArea(int x, int y, unsigned w, unsigned h)
{
    setFunction(GXclear);
    XFillRectangle(display(), drawable(), gc, x, y, w, h);
    setFunction(GXcopy);
}

void Graphics::copyArea(int x, int y,
                        unsigned width, unsigned height,
                        int dx, int dy)
{
    XCopyArea(display(), drawable(), drawable(), gc,
              x - xOrigin, y - yOrigin, width, height,
              dx - xOrigin, dy - yOrigin);
}

void Graphics::copyDrawable(Drawable d,
                            int x, int y,
                            unsigned w, unsigned h,
                            int dx, int dy)
{
    if (d == None)
        return;

    XCopyArea(display(), d, drawable(), gc,
              x, y, w, h,
              dx - xOrigin, dy - yOrigin);
}

void Graphics::copyImage(ref<YImage> image, int x, int y) {
    ref<YPixmap> pixmap(image->renderToPixmap(rdepth()));
    if (pixmap != null)
        drawPixmap(pixmap, 0, 0, pixmap->width(), pixmap->height(), x, y);
}

void Graphics::copyPixmap(ref<YPixmap> p, int dx, int dy) {
    copyPixmap(p, 0, 0, p->width(), p->height(), dx, dy);
}

void Graphics::copyPixmap(ref<YPixmap> p,
                          int x, int y,
                          unsigned w, unsigned h,
                          int dx, int dy)
{
    if (p == null)
        return;
    Pixmap pixmap = p->pixmap(rdepth());
    if (pixmap) {
        copyDrawable(pixmap, x, y, w, h, dx, dy);
        return;
    }

    TLOG(("%s: attempt to copy pixmap 0x%lx of depth %d using gc of depth %d",
          __func__, pixmap, p->depth(), rdepth()));
#if defined(DEBUG) || defined(PRECON)
    if (xapp->synchronized())
        show_backtrace();
#endif
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

void Graphics::drawSegments(XSegment *segments, int n) {
    if (xOrigin | yOrigin) {
        for (int i = 0; i < n; i++) {
            segments[i].x1 -= xOrigin;
            segments[i].y1 -= yOrigin;
            segments[i].x2 -= xOrigin;
            segments[i].y2 -= yOrigin;
        }
    }
    XDrawSegments(display(), drawable(), gc, segments, n);
    if (xOrigin | yOrigin) {
        for (int i = 0; i < n; i++) {
            segments[i].x1 += xOrigin;
            segments[i].y1 += yOrigin;
            segments[i].x2 += xOrigin;
            segments[i].y2 += yOrigin;
        }
    }
}

void Graphics::drawRect(int x, int y, unsigned width, unsigned height) {
    XDrawRectangle(display(), drawable(), gc,
                   x - xOrigin, y - yOrigin, width, height);
}

void Graphics::drawRects(XRectangle *rects, unsigned n) {
    if (xOrigin | yOrigin) {
        for (unsigned i = 0; i < n; i++) {
            rects[i].x -= xOrigin;
            rects[i].y -= yOrigin;
        }
    }
    XDrawRectangles(display(), drawable(), gc, rects, int(n));
    if (xOrigin | yOrigin) {
        for (unsigned i = 0; i < n; i++) {
            rects[i].x += xOrigin;
            rects[i].y += yOrigin;
        }
    }
}

void Graphics::drawArc(int x, int y, unsigned width, unsigned height, int a1, int a2) {
    XDrawArc(display(), drawable(), gc,
             x - xOrigin, y - yOrigin, width, height, a1, a2);
}

/******************************************************************************/

void Graphics::drawChars(mstring s, int x, int y) {
    if (fFont != null && s != null) {
        fFont->drawGlyphs(*this, x, y, s.c_str(), s.length());
    }
}

void Graphics::drawChars(const char *data, int offset, int len, int x, int y) {
    if (fFont != null)
        fFont->drawGlyphs(*this, x, y, data + offset, len);
}

void Graphics::drawString(int x, int y, char const * str) {
    if (fFont != null)
        fFont->drawGlyphs(*this, x, y, str, int(strlen(str)));
}

void Graphics::drawChars(wchar_t* data, int offset, int len, int x, int y)
{
    if (fFont != null)
        fFont->drawGlyphs(*this, x, y, data + offset, len);
}

/**
 * Draw a string but limit its width. If overlong, truncate and show optional
 * ... character.
 */
void Graphics::drawStringEllipsis(int x, int y, const char *str, int maxWidth) {
    if (fFont != null)
        fFont->drawGlyphs(*this, x, y, str, int(strlen(str)), maxWidth);
}

void Graphics::drawCharUnderline(int x, int y, const char *str, int charPos) {
/// TODO #warning "FIXME: don't mess with multibyte here, use a wide char"
    int left = 0; //fFont ? fFont->textWidth(str, charPos) : 0;
    int right = 0; // fFont ? fFont->textWidth(str, charPos + 1) - 1 : 0;
    int len = int(strlen(str));
    int c = 0, cp = 0;

#ifdef CONFIG_I18N
    if (multiByte) mblen(nullptr, 0);
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
            int nc = mblen(str + c, size_t(len - c));
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

void Graphics::drawStringMultiline(const char* str, int x, int y, unsigned w) {
    const int tabpos = fFont->multilineTabPos(str);
    const int tabx = tabpos ? x + tabpos : x;

    while (*str) {
        const char* nln = strchr(str, '\n');
        const char* end = nln ? nln : str + strlen(str);
        const int len = int(end - str);
        const char* tab = static_cast<const char *>(memchr(str, '\t', len));

        if (leftToRight) {
            if (tab) {
                const char* ing = tab + 1;
                drawChars(str, 0, int(tab - str), x, y);
                drawChars(ing, 0, int(end - ing), tabx, y);
            } else {
                drawChars(str, 0, int(end - str), x, y);
            }
        } else if (rightToLeft) {
            if (tab) {
                const char* ing = tab + 1;
                int sw = fFont->textWidth(str, int(tab - str));
                int iw = fFont->textWidth(ing, int(end - ing));
                drawChars(str, 0, int(tab - str), x + w - sw, y);
                drawChars(ing, 0, int(end - ing), x + w - tabpos - iw, y);
            } else {
                int sw = fFont->textWidth(str, int(end - str));
                drawChars(str, 0, int(end - str), x + w - sw, y);
            }
        }

        y += fFont->height();
        str = *end ? end + 1 : end;
    }
}

/******************************************************************************/

void Graphics::fillRect(int x, int y, unsigned width, unsigned height) {
    XFillRectangle(display(), drawable(), gc,
                   x - xOrigin, y - yOrigin, width, height);
}

void Graphics::fillRect(int x, int y, unsigned width, unsigned height,
                        unsigned roundedCornerRadius)
{
    unsigned rounding = min(roundedCornerRadius, min(width, height) / 2);
    if (rounding == 0) {
        fillRect(x, y, width, height);
    } else {
        XArc arc[4];

        x -= xOrigin;
        y -= yOrigin;

        arc[0].x = x;
        arc[0].y = y;
        arc[0].width = 2 * rounding;
        arc[0].height = 2 * rounding;
        arc[0].angle1 = 90 * 64;
        arc[0].angle2 = 90 * 64;

        arc[1] = arc[0];
        arc[1].y = y + height - 2 * rounding - 1;
        arc[1].angle1 = 180 * 64;

        arc[2] = arc[1];
        arc[2].x = x + width - 2 * rounding - 1;
        arc[2].angle1 = 270 * 64;

        arc[3] = arc[2];
        arc[3].y = y;
        arc[3].angle1 = 0 * 64;

        XFillArcs(display(), drawable(), gc, arc, 4);

        XRectangle rec[3];
        int n = 0;
        int t1 = arc[0].x + rounding;
        int t2 = arc[2].x + rounding;
        if (t1 <= t2) {
            rec[n].x = t1;
            rec[n].width = t2 - t1 + 1;
            rec[n].y = arc[0].y;
            rec[n].height = height;
            ++n;
        }
        int u1 = arc[0].y + rounding;
        int u2 = arc[1].y + rounding;
        if (u1 <= u2) {
            rec[n].x = x;
            rec[n].width = rounding;
            rec[n].y = u1;
            rec[n].height = u2 - u1 + 1;
            ++n;
            rec[n] = rec[n - 1];
            rec[n].x += width - rounding;
            ++n;
        }
        XFillRectangles(display(), drawable(), gc, rec, n);
    }
}

void Graphics::fillRects(XRectangle *rects, int n) {
    if (xOrigin | yOrigin) {
        for (int i = 0; i < n; i++) {
            rects[i].x -= xOrigin;
            rects[i].y -= yOrigin;
        }
    }
    XFillRectangles(display(), drawable(), gc, rects, n);
    if (xOrigin | yOrigin) {
        for (int i = 0; i < n; i++) {
            rects[i].x += xOrigin;
            rects[i].y += yOrigin;
        }
    }
}

void Graphics::fillPolygon(XPoint *points, int n, int shape,
                           int mode)
{
    int n1 = (mode == CoordModeOrigin) ? n : 1;

    if (xOrigin | yOrigin) {
        for (int i = 0; i < n1; i++) {
            points[i].x -= xOrigin;
            points[i].y -= yOrigin;
        }
    }
    XFillPolygon(display(), drawable(), gc, points, n, shape, mode);
    if (xOrigin | yOrigin) {
        for (int i = 0; i < n1; i++) {
            points[i].x += xOrigin;
            points[i].y += yOrigin;
        }
    }
}

void Graphics::fillArc(int x, int y, unsigned width, unsigned height, int a1, int a2) {
    XFillArc(display(), drawable(), gc,
             x - xOrigin, y - yOrigin, width, height, a1, a2);
}

/******************************************************************************/

void Graphics::setColor(const YColor& aColor) {
    if (aColor) {
        fColor = aColor;
        unsigned long pixel = fColor.pixel();
        setColorPixel(pixel);
    }
}

void Graphics::setColorPixel(unsigned long pixel) {
    if (rdepth() == 32 && notbit(pixel, 0xFF000000) && xapp->alpha()) {
        pixel |= 0xFF000000;
    }
    XSetForeground(display(), gc, pixel);
}

void Graphics::setLineWidth(unsigned width) {
    XGCValues gcv;
    gcv.line_width = int(width);
    XChangeGC(display(), gc, GCLineWidth, &gcv);
}

void Graphics::setPenStyle(bool dotLine, int cap, int join) {
    XGCValues gcv;
    unsigned long mask = GCLineStyle | GCCapStyle | GCJoinStyle;
    gcv.line_style = dotLine ? LineOnOffDash : LineSolid;
    gcv.cap_style = cap;
    gcv.join_style = join;

    if (dotLine) {
        char dashes[] = { 1 };
        int num_dashes = int ACOUNT(dashes);
        int dash_offset = 0;
        XSetDashes(display(), gc, dash_offset, dashes, num_dashes);

        gcv.line_width = 1;
        mask |= GCLineWidth | GCCapStyle | GCJoinStyle;
    }

    XChangeGC(display(), gc, mask, &gcv);
}

void Graphics::setFunction(int function) {
    XSetFunction(display(), gc, function);
}

/******************************************************************************/

void Graphics::drawImage(ref<YImage> img, int x, int y) {
    drawImage(img, 0, 0, img->width(), img->height(), x, y);
}

void Graphics::drawImage(ref<YImage> img, int x, int y, unsigned w, unsigned h, int dx, int dy) {
    if (picture()) {
        unsigned depth = max(img->depth(), rdepth());
        ref<YPixmap> pix(img->renderToPixmap(depth, img->depth() == 32));
        if (pix != null) {
            Picture source = pix->picture();
            XRenderComposite(display(),
                             img->hasAlpha() ? PictOpOver : PictOpSrc,
                             source, None, picture(),
                             x, y, 0, 0, dx, dy, w, h);
            return;
        }
    }
    if (img->supportsDepth(rdepth())) {
        img->draw(*this, x, y, w, h, dx, dy);
    }
    else {
        ref<YPixmap> pix(img->renderToPixmap(rdepth()));
        if (pix != null) {
            drawPixmap(pix, x, y, w, h, dx, dy);
        }
    }
}

void Graphics::drawPixmap(ref<YPixmap> pix, int x, int y) {
    drawPixmap(pix, 0, 0, pix->width(), pix->height(), x, y);
}

void Graphics::drawPixmap(ref<YPixmap> pix, int sx, int sy,
                          unsigned w, unsigned h, int dx, int dy) {
    Picture source = pix->picture(), destin = picture();
    if (source && destin) {
        XRenderComposite(display(), PictOpOver, source, None, destin,
                         sx, sy, 0, 0, dx - xOrigin, dy - yOrigin, w, h);
        return;
    }

    Pixmap pixmap(pix->pixmap(rdepth()));
    if (pixmap == None) {
        tlog("Graphics::%s: attempt to draw pixmap 0x%lx of depth %d with gc of depth %d\n",
                __func__, pix->pixmap(), pix->depth(), rdepth());
        return;
    }
    if (pix->mask())
        drawClippedPixmap(pixmap,
                          pix->mask(),
                          sx, sy, w, h, dx, dy);
    else
        XCopyArea(display(), pixmap, drawable(), gc,
                  sx, sy, w, h, dx - xOrigin, dy - yOrigin);
}

void Graphics::drawMask(ref<YPixmap> pix, int x, int y) {
    if (pix->mask())
        XCopyArea(display(), pix->mask(), drawable(), gc,
                  0, 0, pix->width(), pix->height(), x - xOrigin, y - yOrigin);
}

void Graphics::drawClippedPixmap(Pixmap pix, Pixmap clip,
                                 int x, int y, unsigned w, unsigned h, int toX, int toY)
{
    unsigned long mask = GCFunction |
        GCGraphicsExposures | GCClipMask | GCClipXOrigin | GCClipYOrigin;
    XGCValues gcv;
    gcv.function = GXcopy;
    gcv.graphics_exposures = False;
    gcv.clip_mask = clip;
    gcv.clip_x_origin = toX - xOrigin;
    gcv.clip_y_origin = toY - yOrigin;
    GC clipPixmapGC = XCreateGC(display(), drawable(), mask, &gcv);
    XCopyArea(display(), pix, drawable(), clipPixmapGC,
              x, y, w, h, toX - xOrigin, toY - yOrigin);
    XFreeGC(display(), clipPixmapGC);
}

void Graphics::compositeImage(ref<YImage> img, int sx, int sy, unsigned w, unsigned h, int dx, int dy) {

    if (picture()) {
        int rx = dx;
        int ry = dy;
        int rw = int(w);
        int rh = int(h);

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

        unsigned depth = max(img->depth(), rdepth());
        ref<YPixmap> pix(img->renderToPixmap(depth, img->depth() == 32));
        if (pix != null) {
            Picture source = pix->picture();
            XRenderComposite(display(),
                             img->hasAlpha() ? PictOpOver : PictOpSrc,
                             source, None, picture(),
                             sx, sy, 0, 0, rx, ry,
                             unsigned(rw), unsigned(rh));
            return;
        }
    }

    // msg("call composite %ux%u:%u | %d %d %d %d | %d %d %d %d", rwidth(), rheight(), rdepth(), sx, sy, w, h, dx, dy, xOrigin, yOrigin);
    if (img->supportsDepth(rdepth())) {
        img->composite(*this, sx, sy, w, h, dx, dy);
    }
    else {
        ref<YPixmap> p(img->renderToPixmap(rdepth()));
        drawPixmap(p, sx, sy, w, h, dx, dy);
    }
}

/******************************************************************************/

void Graphics::draw3DRect(int x, int y, unsigned wid, unsigned hei, bool raised) {
    YColor back(color());
    YColor bright(back.brighter());
    YColor dark(back.darker());
    YColor t(raised ? bright : dark);
    YColor b(raised ? dark : bright);
    int w = int(wid), h = int(hei);

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

void Graphics::drawBorderW(int x, int y, unsigned wid, unsigned hei, bool raised) {
    YColor back(color());
    YColor bright(back.brighter());
    YColor dark(back.darker());
    int w = int(wid), h = int(hei);

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
void Graphics::drawBorderM(int x, int y, unsigned wid, unsigned hei, bool raised) {
    YColor back(color());
    YColor bright(back.brighter());
    YColor dark(back.darker());
    int w = int(wid), h = int(hei);

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

void Graphics::drawBorderG(int x, int y, unsigned wid, unsigned hei, bool raised) {
    YColor back(color());
    YColor bright(back.brighter());
    YColor dark(back.darker());
    int w = int(wid), h = int(hei);

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

void Graphics::drawLines(XPoint *points, int n, int mode) {
    int n1 = (mode == CoordModeOrigin) ? n : 1;
    if (xOrigin | yOrigin) {
        for (int i = 0; i < n1; i++) {
            points[i].x -= xOrigin;
            points[i].y -= yOrigin;
        }
    }
    XDrawLines(display(), drawable(), gc, points, n, mode);
    if (xOrigin | yOrigin) {
        for (int i = 0; i < n1; i++) {
            points[i].x += xOrigin;
            points[i].y += yOrigin;
        }
    }
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

    repHorz(p->pixmap(rdepth()), p->width(), p->height(), x, y, w);
}

void Graphics::repVert(ref<YPixmap> p, int x, int y, unsigned h) {
    if (p == null)
        return;

    repVert(p->pixmap(rdepth()), p->width(), p->height(), x, y, h);
}

void Graphics::fillPixmap(ref<YPixmap> pixmap, int x, int y,
                          unsigned w, unsigned h, int px, int py) {
    Pixmap xpixmap(pixmap->pixmap(rdepth()));
    int const pw(pixmap->width());
    int const ph(pixmap->height());
    if (xpixmap == None)
        return;

    px %= pw;
    py %= ph;
    if (px < 0)
        px += pw;
    if (py < 0)
        py += ph;
    const int pww(px ? pw - px : 0);
    const int phh(py ? ph - py : 0);

    if (0 < px) {
        if (0 < py)
            XCopyArea(display(), xpixmap, drawable(), gc,
                      px, py, pww, phh, x - xOrigin, y - yOrigin);

        for (int yy(y + phh), hh(h - phh); hh > 0; yy += ph, hh -= ph)
            XCopyArea(display(), xpixmap, drawable(), gc,
                      px, 0, pww, min(hh, ph), x - xOrigin, yy - yOrigin);
    }

    for (int xx(x + pww), ww(w - pww); ww > 0; xx += pw, ww -= pw) {
        int const www(min(ww, pw));

        if (0 < py)
            XCopyArea(display(), xpixmap, drawable(), gc,
                      0, py, www, phh, xx - xOrigin, y - yOrigin);

        for (int yy(y + phh), hh(h - phh); hh > 0; yy += ph, hh -= ph)
            XCopyArea(display(), xpixmap, drawable(), gc,
                      0, 0, www, min(hh, ph), xx - xOrigin, yy - yOrigin);
    }
}

void Graphics::drawSurface(YSurface const & surface, int x, int y, unsigned w, unsigned h,
                           int sx, int sy,
                           unsigned sw, unsigned sh
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

void Graphics::drawSurface(const YSurface& surf, int x, int y, unsigned w, unsigned h)
{
    if (surf.gradient != null)
        drawGradient(surf.gradient, x, y, w, h);
    else if (surf.pixmap != null)
        fillPixmap(surf.pixmap, x, y, w, h);
    else if (surf.color) {
        setColor(surf.color);
        fillRect(x, y, w, h);
    }
}

void Graphics::drawGradient(ref<YImage> gradient,
                            int x, int y, unsigned w, unsigned h,
                            int gx, int gy, unsigned gw, unsigned gh)
{
    ref<YImage> scaled = gradient->scale(gw, gh);
    if (scaled != null)
        scaled->draw(*this, gx, gy, w, h, x, y);
}

void Graphics::drawGradient(ref<YImage> gradient,
                            int x, int y, unsigned w, unsigned h)
{
    ref<YImage> scaled = gradient->scale(w, h);
    if (scaled != null)
        scaled->draw(*this, 0, 0, w, h, x, y);
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

unsigned long Graphics::getColorPixel() const {
    XGCValues values;
    XGetGCValues(display(), gc, GCForeground, &values);
    return values.foreground;
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
    XftDrawSetClip(handleXft(), nullptr);
#endif
}

void Graphics::maxOpacity() {
    if (rdepth() == 32 && xapp->alpha()) {
        setFunction(GXor);
        setColorPixel(0xFF000000);
        fillRect(0, 0, rWidth, rHeight);
        setFunction(GXcopy);
    }
}

/******************************************************************************/

void GraphicsBuffer::paint(Pixmap pixmap, const YRect& rect) {
    if (window()->handle() && window()->destroyed())
        return;

    bool clipping = false;
    const int x(rect.x());
    const int y(rect.y());
    const unsigned w(rect.width());
    const unsigned h(rect.height());
    const unsigned depth(window()->depth());

    if (x < 0 || y < 0 || int(w) <= 0 || int(h) <= 0 || pixmap == None) {
        return;
    }

    fNesting += 1;

    Graphics gfx(pixmap, w, h, depth);

    if (fNesting == 1) {
        if (fClipping || x || y ||
            w < window()->width() || h < window()->height())
        {
            XRectangle clip = { short(x), short(y),
                               (unsigned short)w, (unsigned short)h };
            gfx.setClipRectangles(&clip, 1);
            clipping = true;
        }

        gfx.clearArea(x, y, w, h);
    }

    window()->paint(gfx, rect);

    if (fNesting == 1) {
        if (pixmap == fPixmap && !window()->destroyed()) {
            window()->setBackgroundPixmap(pixmap);
            window()->clearArea(x, y, w, h);
        }
        if (clipping) {
            gfx.resetClip();
        }
    }

    fNesting -= 1;
}

void GraphicsBuffer::release() {
    if (fPixmap) {
        XFreePixmap(display(), fPixmap);
        fPixmap = None;
    }
}

GraphicsBuffer::~GraphicsBuffer() {
    release();
}

void GraphicsBuffer::paint(const YRect& rect) {
    if (0 < window()->width() && 0 < window()->height()) {
        GraphicsBuffer::paint(pixmap(), rect);
    }
}

void GraphicsBuffer::paint() {
    YRect rect(0, 0, window()->width(), window()->height());
    paint(rect);
}

Pixmap GraphicsBuffer::pixmap() {
    if (fPixmap == None || fDim != window()->dimension()) {
        if (fPixmap) XFreePixmap(display(), fPixmap);
        fPixmap = window()->createPixmap();
        fDim = window()->dimension();
    }
    return fPixmap;
}

void GraphicsBuffer::scroll(int dx, int dy) {
    if (fPixmap == None || fDim != window()->dimension()) {
        paint();
    }
    else if (dx == 0 && dy == 0) {
    }
    else if (abs(dx) >= int(window()->width())
          || abs(dy) >= int(window()->height()))
    {
        paint();
    }
    else {
        XGCValues gcv = {};
        unsigned long gcvflags = GCGraphicsExposures;
        gcv.graphics_exposures = False;
        GC scrollGC = XCreateGC(xapp->display(), fPixmap, gcvflags, &gcv);
        int ww = int(window()->width());
        int hh = int(window()->height());
        int sx, sy, px, py, pw, ph;
        if (dy > 0) {
            sy = dy; py = 0; ph = hh - dy;
        } else {
            sy = 0; py = -dy; ph = hh + dy;
        }
        if (dx > 0) {
            sx = dx; px = 0; pw = ww - dx;
        } else {
            sx = 0; px = -dx; pw = ww + dx;
        }
        XCopyArea(xapp->display(), fPixmap, fPixmap, scrollGC,
                  sx, sy, pw, ph, px, py);
        XFreeGC(xapp->display(), scrollGC);

        Graphics gfx(fPixmap, ww, hh, window()->depth());
        YRect rect[2];
        int count = 0;
        if (dy > 0) rect[count++] = { 0, ph, ww, dy };
        if (dy < 0) rect[count++] = { 0, 0, ww, -dy };
        if (dx > 0) rect[count++] = { pw, 0, dx, hh };
        if (dx < 0) rect[count++] = { 0, 0, -dx, hh };
        for (int i = 0; i < count; ++i) {
            XRectangle clip(rect[i]);
            gfx.setClipRectangles(&clip, 1);
            gfx.clearArea(clip.x, clip.y, clip.width, clip.height);
            window()->paint(gfx, YRect(clip));
        }
        gfx.resetClip();
        window()->setBackgroundPixmap(fPixmap);
        window()->clearArea(0, 0, ww, hh);
    }
}

// vim: set sw=4 ts=4 et:
