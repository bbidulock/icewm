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

class Picturer {
public:
    virtual ~Picturer() = 0;
    virtual bool picture() = 0;
};

class IApplet : public YWindow {
public:
    IApplet(Picturer *picturer, YWindow *parent);
    virtual ~IApplet();

    virtual void handleExpose(const XExposeEvent &expose) {}
    virtual void handleVisibility(const XVisibilityEvent& visib);
    virtual void paint(Graphics &g, const YRect &r) {}
    virtual void repaint();

protected:
    void freePixmap();
    Drawable getPixmap();
    bool hasPixmap() const { return fPixmap != None; }

    bool isVisible;

private:
    virtual void configure(const YRect2& r);
    void showPixmap();

    Picturer* fPicturer;
    Drawable fPixmap;
};

extern YColorName taskBarBg;

#endif
