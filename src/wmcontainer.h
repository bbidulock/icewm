#ifndef __WMCONTAINER_H
#define __WMCONTAINER_H

#include "ywindow.h"

class YFrameWindow;

class YClientContainer: public YWindow {
public:
    YClientContainer(YWindow *parent, YFrameWindow *frame);
    virtual ~YClientContainer();

    virtual void handleButton(const XButtonEvent &button);
    virtual void handleConfigureRequest(const XConfigureRequestEvent &configureRequest);
    virtual void handleMapRequest(const XMapRequestEvent &mapRequest);
    virtual void handleCrossing(const XCrossingEvent &crossing);

    void grabButtons();
    void releaseButtons();
    void grabActions();
    void regrabMouse();

    YFrameWindow *getFrame() const { return fFrame; };
private:
    YFrameWindow *fFrame;
    bool fHaveGrab;
    bool fHaveActionGrab;
};

#endif

// vim: set sw=4 ts=4 et:
