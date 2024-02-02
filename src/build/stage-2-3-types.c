
#include "../all.h"

void stage_types(Fc *fc);

void stage_2_types(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 2 | Scan types: %s\n", fc->path);

    stage_types(fc);

    stage_add_item(b->stage_3_values, fc);
}

void stage_types(Fc *fc) {
}
