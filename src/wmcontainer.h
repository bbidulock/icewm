#ifndef WMCONTAINER_H
#define WMCONTAINER_H

#include "ywindow.h"

class YFrameWindow;

class YClientContainer: public YWindow {
public:
    YClientContainer(YFrameWindow* frame, int depth,
                     Visual* visual, Colormap colormap);
    virtual ~YClientContainer();

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);
    virtual void handleCrossing(const XCrossingEvent &crossing);
    virtual void handleExpose(const XExposeEvent &expose) {}

    void grabButtons();
    void releaseButtons();
    void grabActions();
    void regrabMouse();

    YFrameWindow* getFrame() const { return fFrame; }
    void setFrame(YFrameWindow* frame) { fFrame = frame; }

private:
    YFrameWindow* fFrame;
    bool fHaveGrab;
    bool fHaveActionGrab;
};

#endif

// vim: set sw=4 ts=4 et:
