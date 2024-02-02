
#include "../all.h"

void stage_parse(Fc *fc);

void stage_1_parse(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 1 | Parse: %s\n", fc->path);

    stage_parse(fc);

    stage_add_item(b->stage_2_alias, fc);
}

void stage_parse(Fc *fc) {
}
