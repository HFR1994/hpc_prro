#include "utils/timer.h"
#include <time.h>
#include <sys/stat.h>

static struct timespec start_time;
static int started = 0; // single-threaded per-process

void start_timer() {
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    started = 1;
}

double get_elapsed_time(void) {
    if (!started) {
        start_timer();
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    const double seconds = (double) (now.tv_sec - start_time.tv_sec) + (double) (now.tv_nsec - start_time.tv_nsec) / 1e9;
    return seconds;
}
