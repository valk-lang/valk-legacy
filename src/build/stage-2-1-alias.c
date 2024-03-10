
#include "../all.h"

void stage_2_alias(Unit* u) {
    Build* b = u->b;

    // TODO

    // if (b->verbose > 2)
    //     printf("Stage 2 | Aliasses: %s\n", fc->path);

    // stage_alias(fc);

    stage_add_item(b->stage_2_props, u);
}

