#ifndef LOGEVENT_H
#define LOGEVENT_H

#if !LOGEVENTS && DEBUG
#define LOGEVENTS 1
#endif

extern bool loggingEvents;

#if LOGEVENTS

void logAny(const XAnyEvent& xev);
void logButton(const XButtonEvent& xev);
void logColormap(const XColormapEvent& xev);
void logConfigureNotify(const XConfigureEvent& xev);
void logConfigureRequest(const XConfigureRequestEvent& xev);
void logCreate(const XCreateWindowEvent& xev);
void logCrossing(const XCrossingEvent& xev);
void logDestroy(const XDestroyWindowEvent& xev);
void logExpose(const XExposeEvent& xev);
void logFocus(const XFocusChangeEvent& xev);
void logGravity(const XGravityEvent& xev);
void logKey(const XKeyEvent& xev);
void logMapRequest(const XMapRequestEvent& xev);
void logMapNotify(const XMapEvent& xev);
void logUnmap(const XUnmapEvent& xev);
void logMotion(const XMotionEvent& xev);
void logProperty(const XPropertyEvent& xev);
void logReparent(const XReparentEvent& xev);
void logVisibility(const XVisibilityEvent& xev);

const char* getAtomName(unsigned long atom);
void setLogEvent(int evtype, bool enable);

void logEvent(const XEvent& xev);
#ifdef CONFIG_SHAPE
void logShape(const XEvent& xev);
#endif
#ifdef CONFIG_XRANDR
void logRandrScreen(const XEvent& xev);
void logRandrNotify(const XEvent& xev);
#endif

#endif

typedef const char* (*AtomNameFunc)(unsigned long atom);
void setAtomName(AtomNameFunc atomNameFunc);
void logClientMessage(const XClientMessageEvent& xev);
bool toggleLogEvents();
bool initLogEvents();

#endif
