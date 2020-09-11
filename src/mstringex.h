#ifndef SRC_MSTRINGEX_H_
#define SRC_MSTRINGEX_H_

#include <regex.h>
#include <functional>
#include "ypointer.h"
#include "mstring.h"
#include "base.h"

// some code parts with heavier dependencies that are optional to mstring:
// STL-complieant hash code for unordered containers
// cached regular expressions

namespace std {
template<>
struct hash<mstring> {
    std::size_t operator()(const mstring &c) const {
        return strhash(c);
    }
};
}
// helper class to keep precompiled regex on stack
class precompiled_regex {
    // XXX: actually, can use a union for them
    union {
        regex_t preg;
        const char* mCompError;
    };
    int execFlags;
    int stateFlags;
    friend class mstring;
public:
    precompiled_regex(const char* regex, const char* flags = nullptr);
    ~precompiled_regex();
    precompiled_regex(const precompiled_regex&) =delete;
    precompiled_regex& operator=(const precompiled_regex&) =delete;
    // a basic check the existence of the matched pattern
    bool matchIn(const char*) const;
};


#endif /* SRC_MSTRINGEX_H_ */
