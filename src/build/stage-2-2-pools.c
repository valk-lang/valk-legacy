
#include "../all.h"

void stage_pools(Build* b, void* payload) {

    if (b->verbose > 2)
        printf("Stage 2 | Generate class pool types\n");
    
    // TODO: delete stage

    stage_props(b, NULL);
}