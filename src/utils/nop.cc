#include "config.h"
#include "yapp.h"
#include "prefs.h"
#include "yconfig.h"
#include "sysdep.h"
//#include <unistd.h>

void nop(int argc, char **argv) {
    YApplication app("nop", &argc, &argv);
    if (argc != 1)
        sleep(60); // for testing memory usage
}

int main(int argc, char **argv) {
    struct timeval start, end, diff;
    gettimeofday(&start, 0);

    nop(argc, argv);

    gettimeofday(&end, 0);
    timersub(&end, &start, &diff);
    fprintf(stderr, "nop in %ld.%06ld\n", diff.tv_sec, diff.tv_usec);

    return 0;
}
