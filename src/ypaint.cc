/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ylib.h"
#include "ypixbuf.h"
#include "ypaint.h"

#include "yapp.h"
#include "sysdep.h"
#include "ystring.h"
#include "yprefs.h"
#include "prefs.h"
#include "stdio.h"

#include "intl.h"

/******************************************************************************/

/******************************************************************************/

YColor::YColor(unsigned short red, unsigned short green, unsigned short blue):
    fRed(red), fGreen(green), fBlue(blue),
    fDarker(NULL), fBrighter(NULL)
    INIT_XFREETYPE(xftColor, NULL) {
    alloc();
}

YColor::YColor(unsigned long pixel):
    fDarker(NULL), fBrighter(NULL)
    INIT_XFREETYPE(xftColor, NULL) {

    XColor color;
    color.pixel = pixel;
    XQueryColor(app->display(), app->colormap(), &color);

    fRed = color.red;
    fGreen = color.green;
    fBlue = color.blue;

    alloc();
}

YColor::YColor(const char *clr):
    fDarker(NULL), fBrighter(NULL)
    INIT_XFREETYPE(xftColor, NULL) {

    XColor color;
    XParseColor(app->display(), app->colormap(),
                clr ? clr : "rgb:00/00/00", &color);

    fRed = color.red;
    fGreen = color.green;
    fBlue = color.blue;

    alloc();
}

#ifdef CONFIG_XFREETYPE
YColor::~YColor() {
    if (NULL != xftColor) {
	XftColorFree (app->display (), app->visual (),
		      app->colormap(), xftColor);
	delete xftColor;
    }
}

YColor::operator XftColor * () {
    if (NULL == xftColor) {
	xftColor = new XftColor;

	XRenderColor color;
	color.red = fRed;
	color.green = fGreen;
	color.blue = fBlue;
	color.alpha = 0xffff;

	XftColorAllocValue(app->display(), app->visual(),
			   app->colormap(), &color, xftColor);
    }

    return xftColor;
}
#endif

void YColor::alloc() {
    XColor color;

    color.red = fRed;
    color.green = fGreen;
    color.blue = fBlue;
    color.flags = DoRed | DoGreen | DoBlue;

    if (Success == XAllocColor(app->display(), app->colormap(), &color))
    {
        int j, ncells;
        double long d = 65536. * 65536. * 65536. * 24;
        XColor clr;
        unsigned long pix;
        long d_red, d_green, d_blue;
        double long u_red, u_green, u_blue;

        pix = 0xFFFFFFFF;
        ncells = DisplayCells(app->display(), DefaultScreen(app->display()));
        for (j = 0; j < ncells; j++) {
            clr.pixel = j;
            XQueryColor(app->display(), app->colormap(), &clr);

            d_red   = color.red   - clr.red;
            d_green = color.green - clr.green;
            d_blue  = color.blue  - clr.blue;

            u_red   = 3UL * (d_red   * d_red);
            u_green = 4UL * (d_green * d_green);
            u_blue  = 2UL * (d_blue  * d_blue);

            double d1 = u_red + u_blue + u_green;

            if (pix == 0xFFFFFFFF || d1 < d) {
                pix = j;
                d = d1;
            }
        }
        if (pix != 0xFFFFFFFF) {
            clr.pixel = pix;
            XQueryColor(app->display(), app->colormap(), &clr);
            /*DBG(("color=%04X:%04X:%04X, match=%04X:%04X:%04X\n",
                   color.red, color.blue, color.green,
                   clr.red, clr.blue, clr.green));*/
            color = clr;
        }
        if (XAllocColor(app->display(), app->colormap(), &color) == 0)
            if (color.red + color.green + color.blue >= 32768)
                color.pixel = WhitePixel(app->display(),
                                         DefaultScreen(app->display()));
            else
                color.pixel = BlackPixel(app->display(),
                                         DefaultScreen(app->display()));
    }
    fRed = color.red;
    fGreen = color.green;
    fBlue = color.blue;
    fPixel = color.pixel;
}

YColor *YColor::darker() { // !!! fix
    if (fDarker == 0) {
        unsigned short red, blue, green;

        red = ((unsigned int)fRed) * 2 / 3;
        blue = ((unsigned int)fBlue) * 2 / 3;
        green = ((unsigned int)fGreen) * 2 / 3;
        fDarker = new YColor(red, green, blue);
    }
    return fDarker;
}

YColor *YColor::brighter() { // !!! fix
    if (fBrighter == 0) {
        unsigned short red, blue, green;

        red = min (((unsigned) fRed) * 4 / 3, 0xFFFFU);
        blue = min (((unsigned) fBlue) * 4 / 3, 0xFFFFU);
        green = min (((unsigned) fGreen) * 4 / 3, 0xFFFFU);

        fBrighter = new YColor(red, green, blue);
    }
    return fBrighter;
}

/******************************************************************************/

/******************************************************************************/

Graphics::Graphics(YWindow & window,
                   unsigned long vmask, XGCValues * gcv):
    fDisplay(app->display()),
    fDrawable(window.handle()),
    fColor(NULL), fFont(NULL),
    xOrigin(0), yOrigin(0)
{
    rWidth = window.width();
    rHeight = window.height();
    gc = XCreateGC(fDisplay, fDrawable, vmask, gcv);
}

Graphics::Graphics(YWindow & window):
    fDisplay(app->display()), fDrawable(window.handle()),
    fColor(NULL), fFont(NULL),
    xOrigin(0), yOrigin(0)
 {
    rWidth = window.width();
    rHeight = window.height();
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(fDisplay, fDrawable, GCGraphicsExposures, &gcv);
}

Graphics::Graphics(YPixmap const &pixmap, int x_org, int y_org):
    fDisplay(app->display()),
    fDrawable(pixmap.pixmap()),
    fColor(NULL), fFont(NULL),
    xOrigin(x_org), yOrigin(y_org)
 {
    rWidth = pixmap.width();
    rHeight = pixmap.height();
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(fDisplay, fDrawable, GCGraphicsExposures, &gcv);
}

Graphics::Graphics(Drawable drawable, int w, int h, unsigned long vmask, XGCValues * gcv):
    fDisplay(app->display()),
    fDrawable(drawable),
    fColor(NULL), fFont(NULL),
    xOrigin(0), yOrigin(0),
    rWidth(w), rHeight(h)
{
    gc = XCreateGC(fDisplay, fDrawable, vmask, gcv);
}

Graphics::Graphics(Drawable drawable, int w, int h):
    fDisplay(app->display()),
    fDrawable(drawable),
    fColor(NULL), fFont(NULL),
    xOrigin(0), yOrigin(0),
    rWidth(w), rHeight(h)
{
    XGCValues gcv; gcv.graphics_exposures = False;
    gc = XCreateGC(fDisplay, fDrawable, GCGraphicsExposures, &gcv);
}

Graphics::~Graphics() {
    XFreeGC(fDisplay, gc);
}

/******************************************************************************/

void Graphics::copyArea(const int x, const int y,
			const int width, const int height,
                        const int dx, const int dy)
{
    XCopyArea(fDisplay, fDrawable, fDrawable, gc,
              x - xOrigin, y - yOrigin, width, height,
              dx - xOrigin, dy - yOrigin);
}

void Graphics::copyDrawable(Drawable const d,
                            const int x, const int y, const int w, const int h,
                            const int dx, const int dy)
{
    XCopyArea(fDisplay, d, fDrawable, gc,
              x, y, w, h,
              dx - xOrigin, dy - yOrigin);
}

void Graphics::copyImage(XImage * image,
			 const int x, const int y, const int w, const int h,
                         const int dx, const int dy)
{
    XPutImage(fDisplay, fDrawable, gc, image,
              x, y,
              dx - xOrigin, dy - yOrigin, w, h);
}

#ifdef CONFIG_ANTIALIASING
void Graphics::copyPixbuf(YPixbuf & pixbuf,
			  const int x, const int y, const int w, const int h,
                          const int dx, const int dy, bool useAlpha)
{
    pixbuf.copyToDrawable(fDrawable, gc,
                          x, y, w, h,
                          dx - xOrigin, dy - yOrigin,
                          useAlpha);
}
void Graphics::copyAlphaMask(YPixbuf & pixbuf,
                             const int x, const int y, const int w, const int h,
                             const int dx, const int dy)
{
    pixbuf.copyAlphaToMask(fDrawable, gc,
                           x, y, w, h,
                           dx - xOrigin, dy - yOrigin);
}
#endif

/******************************************************************************/

void Graphics::drawPoint(int x, int y) {
    XDrawPoint(fDisplay, fDrawable, gc,
               x - xOrigin, y - yOrigin);
}

void Graphics::drawLine(int x1, int y1, int x2, int y2) {
    XDrawLine(fDisplay, fDrawable, gc,
              x1 - xOrigin, y1 - yOrigin,
              x2 - xOrigin, y2 - yOrigin);
}

void Graphics::drawLines(XPoint *points, int n, int mode) {
    int n1 = (mode == CoordModeOrigin) ? n : 1;
    for (int i = 0; i < n1; i++) {
        points[i].x -= xOrigin;
        points[i].y -= yOrigin;
    }
    XDrawLines(fDisplay, fDrawable, gc, points, n, mode);
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
    XDrawSegments(fDisplay, fDrawable, gc, segments, n);
    for (int i = 0; i < n; i++) {
        segments[i].x1 += xOrigin;
        segments[i].y1 += yOrigin;
        segments[i].x2 += xOrigin;
        segments[i].y2 += yOrigin;
    }
}

void Graphics::drawRect(int x, int y, int width, int height) {
    XDrawRectangle(fDisplay, fDrawable, gc,
                   x - xOrigin, y - yOrigin, width, height);
}

void Graphics::drawRects(XRectangle *rects, int n) {
    for (int i = 0; i < n; i++) {
        rects[i].x -= xOrigin;
        rects[i].y -= yOrigin;
    }
    XDrawRectangles(fDisplay, fDrawable, gc, rects, n);
    for (int i = 0; i < n; i++) {
        rects[i].x += xOrigin;
        rects[i].y += yOrigin;
    }
}

void Graphics::drawArc(int x, int y, int width, int height, int a1, int a2) {
    XDrawArc(fDisplay, fDrawable, gc,
             x - xOrigin, y - yOrigin, width, height, a1, a2);
}

/******************************************************************************/

void Graphics::drawChars(const char *data, int offset, int len, int x, int y) {
    if (NULL != fFont)
        fFont->drawGlyphs(*this, x, y, data + offset, len);
}

void Graphics::drawString(int x, int y, char const * str) {
    drawChars(str, 0, strlen(str), x, y);
}

void Graphics::drawStringEllipsis(int x, int y, const char *str, int maxWidth) {
    int const len(strlen(str));
    int const w(fFont ? fFont->textWidth(str, len) : 0);

    if (fFont == 0 || w <= maxWidth) {
        drawChars(str, 0, len, x, y);
    } else {
        int const maxW(maxWidth - fFont->textWidth("...", 3));
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

	l-= sl;
	w-= sw;

        if (l > 0)
	    drawChars(str, 0, l, x, y);
        if (l < len)
            drawChars("...", 0, 3, x + w, y);
    }
}

void Graphics::drawCharUnderline(int x, int y, const char *str, int charPos) {
#warning "FIXME: don't mess with multibyte here, use a wide char"
    int left = 0; //fFont ? fFont->textWidth(str, charPos) : 0;
    int right = 0; // fFont ? fFont->textWidth(str, charPos + 1) - 1 : 0;
    int len = strlen(str);
    int c = 0, cp = 0;

#ifdef CONFIG_I18N
    if (multiByte) mblen(NULL, 0);
#endif
    while (c <= len && cp <= charPos + 1) {
        if (charPos == cp) {
            left = fFont ? fFont->textWidth(str, c) : 0;
//            msg("l: %d %d %d %d %d", c, cp, charPos, left, right);
        } else if (charPos + 1 == cp) {
            right = fFont ? fFont->textWidth(str, c) - 1: 0;
//            msg("l: %d %d %d %d %d", c, cp, charPos, left, right);
            break;
        }
        if (c >= len || str[c] == '\0')
            break;
#ifdef CONFIG_I18N
        if (multiByte) {
            int nc = mblen(str + c, len - c);
            if (nc < 1) // bad things
                c++;
            else
                c += nc;
        } else
#endif
            c++;
        cp++;
    }
//    msg("%d %d %d %d %d", c, cp, charPos, left, right);

    drawLine(x + left, y + 2, x + right, y + 2);
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

struct YRotated {
    struct R90 {
	static int xOffset(YFont const * font) { return -font->descent(); }
	static int yOffset(YFont const * /*font*/) { return 0; }

	template <class T>
	static T width(T const & /*w*/, T const & h) { return h; }
	template <class T>
	static T height(T const & w, T const & /*h*/) { return w; }


	static void rotate(XImage * src, XImage * dst) {
	    for (int sy = src->height - 1, dx = 0; sy >= 0; --sy, ++dx)
		for (int sx = src->width - 1, &dy = sx; sx >= 0; --sx)
		    XPutPixel(dst, dx, dy, XGetPixel(src, sx, sy));
	}
    };

    struct R270 {
	static int xOffset(YFont const * font) { return -font->descent(); }
	static int yOffset(YFont const * /*font*/) { return 0; }

	template <class T>
	static T width(T const & /*w*/, T const & h) { return h; }
	template <class T>
	static T height(T const & w, T const & /*h*/) { return w; }

	static void rotate(XImage * src, XImage * dst) {
	    for (int sy = src->height - 1, &dx = sy; sy >= 0; --sy)
	        for (int sx = src->width - 1, dy = 0; sx >= 0; --sx, ++dy)
		    XPutPixel(dst, dx, dy, XGetPixel(src, sx, sy));
	}
    };
};

#ifdef CONFIG_MOVESIZE_FX
template <class Rt>
void Graphics::drawStringRotated(int x, int y, char const * str) {
    int const l(strlen(str));
    int const w(fFont->textWidth(str, l));
    int const h(fFont->ascent() + fFont->descent());

    Pixmap pixmap = YPixmap::createPixmap(w, h, 1);
    Graphics canvas(pixmap, w, h);
//    GraphicsCanvas canvas(w, h, 1);
    if (None == canvas.drawable()) {
	warn(_("Resource allocation for rotated string \"%s\" (%dx%d px) "
	       "failed"), str, w, h);
	return;
    }

    canvas.fillRect(0, 0, w, h);
    canvas.setFont(fFont);
    canvas.setColor(YColor::white);
    canvas.drawChars(str, 0, l, 0, fFont->ascent());

    XImage * horizontal(XGetImage(fDisplay, canvas.drawable(),
				  0, 0, w, h, 1, XYPixmap));
    if (NULL == horizontal) {
	warn(_("Resource allocation for rotated string \"%s\" (%dx%d px) "
	       "failed"), str, w, h);
	return;
    }

    int const bpl(((Rt::width(w, h) >> 3) + 3) & ~3);

    XImage * rotated(XCreateImage(fDisplay, app->visual(), 1, XYPixmap,
				  0, new char[bpl * Rt::height(w, h)],
				  Rt::width(w, h), Rt::height(w, h), 32, bpl));

    if (NULL == rotated) {
	warn(_("Resource allocation for rotated string \"%s\" (%dx%d px) "
	       "failed"), str, Rt::width(w, h), Rt::height(w, h));
	return;
    }

    Rt::rotate(horizontal, rotated);
    XDestroyImage(horizontal);

    Pixmap mask_pixmap = YPixmap::createPixmap(Rt::width(w, h), Rt::height(w, h), 1);
    //GraphicsCanvas mask(Rt::width(w, h), Rt::height(w, h), 1);
    Graphics mask(mask_pixmap, Rt::width(w, h), Rt::height(w, h));
    if (None == mask.drawable()) {
	warn(_("Resource allocation for rotated string \"%s\" (%dx%d px) "
	       "failed"), str, Rt::width(w, h), Rt::height(w, h));
	return;
    }

    mask.copyImage(rotated, 0, 0);
    XDestroyImage(rotated);

    x += Rt::xOffset(fFont);
    y += Rt::yOffset(fFont);

#warning "this is completely broken, because it interferes with repaint clipping"
    setClipMask(mask.drawable());
    setClipOrigin(x, y);

    fillRect(x, y, Rt::width(w, h), Rt::height(w, h));

#warning "and this"
    setClipOrigin(0, 0);
    setClipMask(None);

    XFreePixmap(app->display(), mask_pixmap);
    XFreePixmap(app->display(), pixmap);
}

void Graphics::drawString90(int x, int y, char const * str) {
    drawStringRotated<YRotated::R90>(x, y, str);
}
/*
void Graphics::drawString180(int x, int y, char const * str) {
    drawStringRotated<YRotated::R180>(x, y, str);
}
*/
void Graphics::drawString270(int x, int y, char const * str) {
    drawStringRotated<YRotated::R270>(x, y, str);
}
#endif

/******************************************************************************/

void Graphics::fillRect(int x, int y, int width, int height) {
    XFillRectangle(fDisplay, fDrawable, gc,
                   x - xOrigin, y - yOrigin, width, height);
}

void Graphics::fillRects(XRectangle *rects, int n) {
    for (int i = 0; i < n; i++) {
        rects[i].x -= xOrigin;
        rects[i].y -= yOrigin;
    }
    XFillRectangles(fDisplay, fDrawable, gc, rects, n);
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
    XFillPolygon(fDisplay, fDrawable, gc, points, n, shape, mode);
    for (int i = 0; i < n1; i++) {
        points[i].x += xOrigin;
        points[i].y += yOrigin;
    }
}

void Graphics::fillArc(int x, int y, int width, int height, int a1, int a2) {
    XFillArc(fDisplay, fDrawable, gc,
             x - xOrigin, y - yOrigin, width, height, a1, a2);
}

/******************************************************************************/

void Graphics::setColor(YColor * aColor) {
    fColor = aColor;
    XSetForeground(fDisplay, gc, fColor->pixel());
}

void Graphics::setFont(YFont * aFont) {
    fFont = aFont;
}

void Graphics::setLineWidth(int width) {
    XGCValues gcv;
    gcv.line_width = width;
    XChangeGC(fDisplay, gc, GCLineWidth, &gcv);
}

void Graphics::setPenStyle(bool dotLine) {
    XGCValues gcv;

    if (dotLine) {
        char c = 1;
        gcv.line_style = LineOnOffDash;
        XSetDashes(fDisplay, gc, 0, &c, 1);
    } else {
        gcv.line_style = LineSolid;
    }

    XChangeGC(fDisplay, gc, GCLineStyle, &gcv);
}

void Graphics::setFunction(int function) {
    XSetFunction(fDisplay, gc, function);
}

void Graphics::setClipRects(int x, int y, XRectangle rectangles[], int n,
			    int ordering) {
    XSetClipRectangles(fDisplay, gc, x - xOrigin, y - yOrigin, rectangles, n, ordering);
}

void Graphics::setClipMask(Pixmap mask) {
    XSetClipMask(fDisplay, gc, mask);
}

void Graphics::setClipOrigin(int x, int y) {
    XSetClipOrigin(fDisplay, gc, x - xOrigin, y - yOrigin);
}

/******************************************************************************/

void Graphics::drawImage(YIcon::Image * image, int const x, int const y) {
#ifdef CONFIG_ANTIALIASING
    int dx = x;
    int dy = y;
    int dw = image->width();
    int dh = image->height();

    if (dx < xorigin()) {
        dw -= xorigin() - dx;
        dx = xorigin();
    }
    if (dy < yorigin()) {
        dh -= yorigin() - dy;
        dy = yorigin();
    }
    if (dx + dw > xorigin() + rWidth) {
        dw = xorigin() + rWidth - dx;
    }
    if (dy + dh > yorigin() + rHeight) {
        dh = yorigin() + rHeight - dy;
    }

    MSG(("drawImage %d %d %d %d %dx%d | %d %d | %d %d | %d %d | %d %d",
         x, y, dx, dy, dw, dh, xorigin(), yorigin(), x, y,
         dx - x, dy - y, dx - xOrigin, dy - yOrigin));
    if (dw <= 0 || dh <= 0)
        return;
    YPixbuf bg(fDrawable, None, dw, dh, dx - xOrigin, dy - yOrigin);
    bg.copyArea(*image, dx - x, dy - y, dw, dh, 0, 0);
    bg.copyToDrawable(fDrawable, gc, 0, 0, dw, dh, dx - xOrigin, dy - yOrigin);
#else
    drawPixmap(image, x, y);
#endif
}

void Graphics::drawPixmap(YPixmap const * pix, int const x, int const y) {
    if (pix->mask())
        drawClippedPixmap(pix->pixmap(),
                          pix->mask(),
                          0, 0, pix->width(), pix->height(), x, y);
    else
        XCopyArea(fDisplay, pix->pixmap(), fDrawable, gc,
                  0, 0, pix->width(), pix->height(), x - xOrigin, y - yOrigin);
}

void Graphics::drawMask(YPixmap const * pix, int const x, int const y) {
    if (pix->mask())
        XCopyArea(fDisplay, pix->mask(), fDrawable, gc,
                  0, 0, pix->width(), pix->height(), x - xOrigin, y - yOrigin);
}

void Graphics::drawClippedPixmap(Pixmap pix, Pixmap clip,
                                 int x, int y, int w, int h, int toX, int toY)
{
    static GC clipPixmapGC = None;
    XGCValues gcv;

    if (clipPixmapGC == None) {
        gcv.graphics_exposures = False;
        clipPixmapGC = XCreateGC(app->display(), desktop->handle(),
                                 GCGraphicsExposures,
                                 &gcv);
    }

    gcv.clip_mask = clip;
    gcv.clip_x_origin = toX - xOrigin;
    gcv.clip_y_origin = toY - yOrigin;
    XChangeGC(fDisplay, clipPixmapGC,
              GCClipMask|GCClipXOrigin|GCClipYOrigin, &gcv);
    XCopyArea(fDisplay, pix, fDrawable, clipPixmapGC,
              x, y, w, h, toX - xOrigin, toY - yOrigin);
    gcv.clip_mask = None;
    XChangeGC(fDisplay, clipPixmapGC, GCClipMask, &gcv);
}

/******************************************************************************/

void Graphics::draw3DRect(int x, int y, int w, int h, bool raised) {
    YColor *back(color());
    YColor *bright(back->brighter());
    YColor *dark(back->darker());
    YColor *t(raised ? bright : dark);
    YColor *b(raised ? dark : bright);

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

void Graphics::drawBorderW(int x, int y, int w, int h, bool raised) {
    YColor *back(color());
    YColor *bright(back->brighter());
    YColor *dark(back->darker());

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
void Graphics::drawBorderM(int x, int y, int w, int h, bool raised) {
    YColor *back(color());
    YColor *bright(back->brighter());
    YColor *dark(back->darker());

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

void Graphics::drawBorderG(int x, int y, int w, int h, bool raised) {
    YColor *back(color());
    YColor *bright(back->brighter());
    YColor *dark(back->darker());

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

void Graphics::drawCenteredPixmap(int x, int y, int w, int h, YPixmap *pixmap) {
    int r = x + w;
    int b = y + h;
    int pw = pixmap->width();
    int ph = pixmap->height();

    drawOutline(x, y, r, b, pw, ph);
    drawPixmap(pixmap, x + w / 2 - pw / 2, y + h / 2 - ph / 2);
}

void Graphics::drawOutline(int l, int t, int r, int b, int iw, int ih) {
    if (l + iw >= r && t + ih >= b)
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

void Graphics::repHorz(Drawable d, int pw, int ph, int x, int y, int w) {
    while (w > 0) {
        XCopyArea(fDisplay, d, fDrawable, gc, 0, 0, min(w, pw), ph, x - xOrigin, y - yOrigin);
        x += pw;
        w -= pw;
    }
}

void Graphics::repVert(Drawable d, int pw, int ph, int x, int y, int h) {
    while (h > 0) {
        XCopyArea(fDisplay, d, fDrawable, gc, 0, 0, pw, min(h, ph), x - xOrigin, y - yOrigin);
        y += ph;
        h -= ph;
    }
}

void Graphics::fillPixmap(YPixmap const * pixmap, int const x, int const y,
			  int const w, int const h, int px, int py) {
    int const pw(pixmap->width());
    int const ph(pixmap->height());

    px%= pw; const int pww(px ? pw - px : 0);
    py%= ph; const int phh(py ? ph - py : 0);

    if (px) {
	if (py)
            XCopyArea(fDisplay, pixmap->pixmap(), fDrawable, gc,
                      px, py, pww, phh, x - xOrigin, y - yOrigin);

        for (int yy(y + phh), hh(h - phh); hh > 0; yy += ph, hh -= ph)
            XCopyArea(fDisplay, pixmap->pixmap(), fDrawable, gc,
                      px, 0, pww, min(hh, ph), x - xOrigin, yy - yOrigin);
    }

    for (int xx(x + pww), ww(w - pww); ww > 0; xx+= pw, ww-= pw) {
	int const www(min(ww, pw));

	if (py)
            XCopyArea(fDisplay, pixmap->pixmap(), fDrawable, gc,
                      0, py, www, phh, xx - xOrigin, y - yOrigin);

        for (int yy(y + phh), hh(h - phh); hh > 0; yy += ph, hh -= ph)
            XCopyArea(fDisplay, pixmap->pixmap(), fDrawable, gc,
                      0, 0, www, min(hh, ph), xx - xOrigin, yy - yOrigin);
    }
}

void Graphics::drawSurface(YSurface const & surface, int x, int y, int w, int h,
			   int const sx, int const sy,
#ifdef CONFIG_GRADIENTS
			   const int sw, const int sh) {
    if (surface.gradient)
	drawGradient(*surface.gradient, x, y, w, h, sx, sy, sw, sh);
    else
#else
			   const int /*sw*/, const int /*sh*/) {
#endif
    if (surface.pixmap)
	fillPixmap(surface.pixmap, x, y, w, h, sx, sy);
    else if (surface.color) {
	setColor(surface.color);
	fillRect(x, y, w, h);
    }
}

#ifdef CONFIG_GRADIENTS
void Graphics::drawGradient(const class YPixbuf & pixbuf,
			    int const x, int const y, const int w, const int h,
			    int const gx, int const gy, const int gw, const int gh) {
    YPixbuf(pixbuf, gw, gh).copyToDrawable(fDrawable, gc, gx, gy, w, h, x - xOrigin, y - yOrigin);
}
#endif

/******************************************************************************/

void Graphics::drawArrow(YDirection direction, int x, int y, int size,
			 bool pressed) {
    YColor *nc(color());
    YColor *oca = pressed ? nc->darker() : nc->brighter(),
	   *ica = pressed ? YColor::black : nc,
    	   *ocb = pressed ? wmLook == lookGtk ? nc : nc->brighter()
			: nc->darker(),
	   *icb = pressed ? nc->brighter() : YColor::black;

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
    XGetGCValues(fDisplay, gc, GCFunction, &values);
    return values.function;
}

/******************************************************************************/
/******************************************************************************/
