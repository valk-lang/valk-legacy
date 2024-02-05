
#include "../all.h"

void stage_values(Fc *fc);

void stage_3_values(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 3 | Scan values: %s\n", fc->path);

    usize start = microtime();
    stage_values(fc);
    b->time_parse += microtime() - start;

    stage_add_item(b->stage_4_ast, fc);
}

void stage_values(Fc *fc) {
}
