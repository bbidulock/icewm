#include "config.h"
#include "yatom.h"
#include "yapp.h"

#if 0
long XAtom::handle() {
    if (atom == 0) {
        atom = XInternAtom(app->display(), name, False);
    }
    return atom;
}
#endif
