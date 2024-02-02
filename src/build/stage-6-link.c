
#include "../all.h"

void stage_6_link(Build* b) {

    if (b->verbose > 2)
        printf("Stage 6 | Link .o files to executable\n");

    usize start = microtime();

    b->time_link += microtime() - start;
}
