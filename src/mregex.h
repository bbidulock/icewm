#ifndef SRC_MSTRINGEX_H_
#define SRC_MSTRINGEX_H_

#include <regex.h>
#include "ypointer.h"
#include "mstring.h"
#include "base.h"

// helper class to keep precompiled regex on stack
class mregex {
    // XXX: actually, can use a union for them
    union {
        regex_t preg;
        const char* mCompError;
    };
    int execFlags;
    int stateFlags;
    friend class mstring;
public:
    mregex(const char* regex, const char* flags = nullptr);
    ~mregex();
    void reset();
    mregex(const mregex&) =delete;
    mregex& operator=(const mregex&) =delete;
    // a basic check the existence of the matched pattern
    bool matchIn(const char*) const;
    mslice match(const char *s) const;
};


#endif /* SRC_MSTRINGEX_H_ */
