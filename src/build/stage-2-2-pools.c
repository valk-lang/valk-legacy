
#include "../all.h"

void stage_pools(Build* b, void* payload) {

    if (b->verbose > 2)
        printf("Stage 2 | Generate class pool types\n");

    Array *classes = b->classes;
    loop(classes, i) {
        Class* class = array_get_index(classes, i);
        generate_class_pool(class->unit->parser, class);
    }

    stage_props(b, NULL);
}