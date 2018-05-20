#ifndef __APPLET_H
#define __APPLET_H

class TrayPane;

class IAppletContainer {
public:
    virtual void relayout() = 0;
    virtual void contextMenu(int x_root, int y_root) = 0;
    virtual TrayPane* windowTrayPane() const = 0;
protected:
    virtual ~IAppletContainer() {}
};

class IApplet : public YWindow {
public:
    IApplet(YWindow *parent);
    virtual ~IApplet();

    virtual void handleExpose(const XExposeEvent &expose);
    virtual void handleMapNotify(const XMapEvent &map);
    virtual void handleUnmapNotify(const XUnmapEvent &map);
    virtual void handleVisibility(const XVisibilityEvent& visib);
    virtual void paint(Graphics &g, const YRect &r);
    virtual void repaint();
    virtual bool picture();

protected:
    void freePixmap();
    Drawable getPixmap();
    bool hasPixmap() const { return fPixmap != None; }

    bool isVisible;

private:
    bool isMapped;
    Drawable fPixmap;
};

extern YColorName taskBarBg;

#endif
