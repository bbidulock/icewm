#ifndef __YFULL_H
#define __YFULL_H

#include "ykey.h"

#include <X11/Xutil.h>
#define __YIMP_XUTIL__

#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#ifdef CONFIG_SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef CONFIG_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#endif
