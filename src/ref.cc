#include "config.h"

#include "ref.h"

void refcounted::__destroy() {
    delete this;
}
