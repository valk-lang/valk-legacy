
#include "../all.h"

void stage_props(Fc *fc);

void stage_2_props(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 2 | Scan properties: %s\n", fc->path);

    stage_props(fc);

    stage_add_item(b->stage_2_types, fc);
}

void stage_props(Fc *fc) {
}
