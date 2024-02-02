
#include "../all.h"

void stage_5_objects(Build* b) {

    if (b->verbose > 2)
        printf("Stage 5 | Generate .o files\n");

    usize start = microtime();

    b->time_llvm += microtime() - start;
}
