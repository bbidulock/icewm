/*
 *  IceWM - Generic command line parser
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/03/12: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - initial version
 */
#include "config.h"
#include "ycmdline.h"

#include "base.h"
#include "ascii.h"
#include "intl.h"

#include <string.h>

int YCommandLine::parse() {
    int rc(0);

    for (int n = 1, apos = 0; !rc && n < argc; ) {
        const char * const *arg = argv + n;

        if (**arg == '-') {
            char const * value = NULL;
            char const option = getArgument(*arg, value);

            rc = setOption(*arg, option, value);

            if (option != '\0')
                eatArgument(n);
            else
                ++n;
        } else
            rc = setArgument(apos++, argv[n++]);
    }

    return rc;
}

int YCommandLine::setOption(char const * arg, char, char const *) {
    warn(_("Unrecognized option: %s\n"), arg);
    return 2;
}

int YCommandLine::setArgument(int /*pos*/, char const * val) {
    warn(_("Unrecognized argument: %s\n"), val);
    return 2;
}

char const * YCommandLine::getValue(char const * const & arg,
                                    char const * vptr) {
    if (vptr) { // ------------------------------------- eat leading garbage ---
        if (*vptr == '=') ++vptr;
        while (ASCII::isSpaceOrTab(*vptr)) ++vptr;
    } else { // ------------------------- value assumed in the next argument ---
        int idx = &arg - static_cast<char const* const*>(argv) + 1;

        if (idx < argc) {
            vptr = argv[idx];
            eatArgument(idx);
        } else
            warn(_("Argument required for %s switch"), arg);
    }

    return vptr;
}

void YCommandLine::eatArgument(int idx) {
    if (idx < argc)
        ::memmove(argv + idx, argv + idx + 1, sizeof(*argv) * (--argc - idx));
}
