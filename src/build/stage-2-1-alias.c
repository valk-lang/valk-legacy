
#include "../all.h"

void stage_alias(Fc *fc);

void stage_2_alias(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 2 | Aliasses: %s\n", fc->path);

    stage_alias(fc);

    stage_add_item(b->stage_2_props, fc);
}

void stage_alias(Fc *fc) {
}
