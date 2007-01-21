#include "config.h"
#include "mstring.h"

char *ApplicationName = "strtest";

int main() {
    ustring x("foo");
    ustring n(0);

    ustring y = x;

    ustring z = y.remove(1, 1);

    for (int i = 0; i < 10000; i++) {
        z = x.replace(x.length(), 0, y);
    }
    return 0;
}
