#include "config.h"

#include "ref.h"

class null_ref* null_ref_ptr;
class null_ref& null(*null_ref_ptr);

void refcounted::__destroy() {
    delete this;
}
