
#include "../all.h"

void stage_2_pools(Build* b) {

    if (b->verbose > 2)
        printf("Stage 2 | Generate class pool types\n");

    b->stage_1_done = true;

    Array *classes = b->classes;
    for(int i = 0; i < classes->length; i++) {
        Class* class = array_get_index(classes, i);
        generate_class_pool(class->unit->parser, class);
    }
}