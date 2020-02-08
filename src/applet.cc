#include "config.h"
#include "ywindow.h"
#include "applet.h"
#include "yxapp.h"
#include "default.h"

Picturer::~Picturer()
{
}

IApplet::IApplet(Picturer *picturer, YWindow *parent) :
    YWindow(parent),
    isVisible(false),
    fPicturer(picturer),
    fPixmap(None)
{
    addStyle(wsNoExpose);
    addEventMask(VisibilityChangeMask);
}

IApplet::~IApplet()
{
    freePixmap();
}

void IApplet::freePixmap()
{
    if (fPixmap) {
        XFreePixmap(xapp->display(), fPixmap);
        fPixmap = None;
    }
}

void IApplet::handleVisibility(const XVisibilityEvent& visib)
{
    bool prev = isVisible;
    isVisible = (visib.state != VisibilityFullyObscured);
    if (prev < isVisible)
        repaint();
}

void IApplet::repaint()
{
    if (isVisible && visible() && fPicturer->picture())
        showPixmap();
}

Drawable IApplet::getPixmap()
{
    if (fPixmap == None)
        fPixmap = createPixmap();
    return fPixmap;
}

void IApplet::showPixmap() {
    Graphics g(fPixmap, width(), height(), depth());
    paint(g, YRect(0, 0, width(), height()));
    setBackgroundPixmap(fPixmap);
    clearWindow();
}

void IApplet::configure(const YRect2& r) {
    if (r.resized())
        freePixmap();
    if (None == fPixmap && 1 < r.width() && 1 < r.height())
        repaint();
}

