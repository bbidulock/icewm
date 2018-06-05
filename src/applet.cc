#include "ywindow.h"
#include "applet.h"
#include "yxapp.h"

Picturer::~Picturer()
{
}

IApplet::IApplet(Picturer *picturer, YWindow *parent) :
    YWindow(parent),
    isVisible(false),
    isMapped(false),
    fPicturer(picturer),
    fPixmap(None)
{
    addEventMask(VisibilityChangeMask | StructureNotifyMask);
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

void IApplet::handleExpose(const XExposeEvent &e)
{
    if (fPixmap || fPicturer->picture())
        paint(getGraphics(), YRect(e.x, e.y, e.width, e.height));
}

void IApplet::handleMapNotify(const XMapEvent &map)
{
    isMapped = true;
}

void IApplet::handleUnmapNotify(const XUnmapEvent &map)
{
    isMapped = false;
}

void IApplet::handleVisibility(const XVisibilityEvent& visib)
{
    isVisible = (visib.state != VisibilityFullyObscured);
}

void IApplet::paint(Graphics &g, const YRect& r) {
    if (fPixmap)
        g.copyDrawable(fPixmap, r.x(), r.y(), r.width(), r.height(), r.x(), r.y());
}

void IApplet::repaint()
{
    if (isMapped && isVisible && fPicturer->picture())
        paint(getGraphics(), YRect(0, 0, width(), height()));
}

Drawable IApplet::getPixmap()
{
    if (fPixmap == None)
        fPixmap = XCreatePixmap(xapp->display(), handle(),
                                width(), height(), depth());
    return fPixmap;
}



