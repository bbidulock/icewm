#include "config.h"
#include "ykeyevent.h"
#include "yxkeydef.h"

bool YKeyEvent::isModifierKey() const {
    if (key == XK_Control_L || key == XK_Control_R ||
        key == XK_Alt_L     || key == XK_Alt_R     ||
        key == XK_Meta_L    || key == XK_Meta_R    ||
        key == XK_Super_L   || key == XK_Super_R   ||
        key == XK_Hyper_L   || key == XK_Hyper_R   ||
        key == XK_Shift_L   || key == XK_Shift_R)
        return true;
    return false;
}
