#include "config.h"
#include "yxlib.h"
#include "ypointer.h"
#include "yapp.h"

#include <X11/cursorfont.h>

static YPointer *create(int shape) {
    return new YPointer(XCreateFontCursor(app->display(), shape));
}

#define XGET(w, x) \
    static YPointer *s##w = 0; \
    YPointer *YPointer::w() { \
    if (s##w == 0) s##w = create(x); \
    return s##w; \
    }

XGET(left, XC_left_ptr)
XGET(right, XC_right_ptr)
XGET(move, XC_fleur)
XGET(sb_left_arrow, XC_sb_left_arrow)
XGET(sb_right_arrow, XC_sb_right_arrow)
XGET(resize_top_left, XC_top_left_corner);
XGET(resize_top_right, XC_top_right_corner);
XGET(resize_bottom_left, XC_bottom_left_corner);
XGET(resize_bottom_right, XC_bottom_right_corner);
XGET(resize_left, XC_left_side);
XGET(resize_right, XC_right_side);
XGET(resize_top, XC_top_side);
XGET(resize_bottom, XC_bottom_side);
