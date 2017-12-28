#include "config.h"
#include "ref.h"
#include <stddef.h>

// dummy allocation arena
class null_arena {
public:
    void* allocate(size_t);
};

// place anything at NULL
void* null_arena::allocate(size_t) {
    return (void *) 0;
}

// arena for null_ref
null_arena a_null_arena;

// placement new for null_arena
void* operator new(size_t size, null_arena& arena) {
    return arena.allocate(size);
}

// an inaccessible class
class null_ref {
public:
    // pointer to null_ref
    static null_ref* a_null_ptr;
private:
    // cannot instantiate
    null_ref() { }
    // cannot copy
    null_ref(const null_ref&);
};

// allocate a null_ref at NULL
null_ref* null_ref::a_null_ptr(new (a_null_arena) null_ref);

// null references a null_ref at NULL
null_ref& null(*null_ref::a_null_ptr);

// reference count became zero
void refcounted::__destroy() {
    delete this;
}

// vim: set sw=4 ts=4 et:
