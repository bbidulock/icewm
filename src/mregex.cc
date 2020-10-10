#include "mregex.h"
#include "ref.h"

#include <regex.h>

enum STATE_FLAGS {
    STATE_NONE = 0,
    STATE_COMPILED = 1,
    STATE_ERROR = 2
};

mslice mregex::match(const char *s) const {
    if (stateFlags == STATE_ERROR) {
        warn("match regcomp: %s", mCompError);
        return null;
    }
    regmatch_t pos;
    return regexec(&preg, s, 1, &pos, execFlags) != 0 ?
            mslice() : mslice(s + pos.rm_so, size_t(pos.rm_eo - pos.rm_so));
}

mregex::mregex(const char *regex, const char *flags) :
        execFlags(0), stateFlags(0) {
    auto compFlags = REG_EXTENDED;
    for (int i = 0; flags && flags[i]; ++i) {
        switch (flags[i]) {
        case 'i':
            compFlags |= REG_ICASE;
            break;
        case 'n':
            compFlags |= REG_NEWLINE;
            break;
        case 'B':
            execFlags |= REG_NOTBOL;
            break;
        case 'E':
            execFlags |= REG_NOTEOL;
            break;
        }
    }
    int comp = regcomp(&preg, regex, compFlags);
    if (0 == comp) {
        stateFlags = STATE_COMPILED;
    } else {
        stateFlags = STATE_ERROR;
        auto sResult = new char[200];
        regerror(comp, &preg, sResult, 200);
        sResult[199] = 0x0;
        regfree(&preg);
        mCompError = sResult;
    }
}

void mregex::reset() {
    switch(stateFlags)
    {
    case STATE_NONE: break;
    case STATE_COMPILED: regfree(&preg); break;
    case STATE_ERROR: delete [] mCompError; break;
    }
    mCompError = nullptr;
    stateFlags = STATE_NONE;
}
mregex::~mregex() {
    reset();
}

bool mregex::matchIn(const char *s) const {
    if (STATE_ERROR == stateFlags) {
        warn("match regcomp: %s", mCompError);
        return false;
    }
    regmatch_t pos;
    return 0 == regexec(&preg, s, 1, &pos, execFlags);
}
