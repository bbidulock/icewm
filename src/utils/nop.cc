#include "config.h"
#include "yapp.h"
#include "prefs.h"
#include "yconfig.h"
#include <unistd.h>

int main(int argc, char **argv) {
    YApplication app("nop", &argc, &argv);
    if (argc != 1)
        sleep(60); // for testing memory usage
    return 0;
}
