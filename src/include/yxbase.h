#ifndef __YBASE_H
#define __YBASE_H

#define __YIMP_X__

#ifndef __YIMP_XLIB__
typedef struct _XDisplay Display;
typedef struct _XGC *GC;
typedef union _XEvent XEvent;
struct XFontStruct;
#ifdef CONFIG_I18N
typedef struct _XOC *XFontSet;
#endif
struct XPoint;
struct XExposeEvent;
struct XGraphicsExposeEvent;
struct XConfigureEvent;
struct XKeyEvent;
struct XButtonEvent;
struct XMotionEvent;
struct XCrossingEvent;
struct XPropertyEvent;
struct XColormapEvent;
struct XFocusChangeEvent;
struct XClientMessageEvent;
struct XMapEvent;
struct XUnmapEvent;
struct XDestroyWindowEvent;
struct XConfigureRequestEvent;
struct XMapRequestEvent;
struct XSelectionEvent;
struct XSelectionClearEvent;
struct XSelectionRequestEvent;
#endif
#ifndef __YIMP_XUTIL__
#ifdef SHAPE
struct XShapeEvent;
struct XTextProperty;
#endif
#endif

#endif
