
#include "../all.h"

void build_set_stages(Build* b) {
    stage_add(b, stage_prio_alias, stage_alias, NULL);
    // stage_add(b, stage_prio_pools, stage_pools, NULL);
    // stage_add(b, stage_prio_props, stage_props, NULL);
    // stage_add(b, stage_prio_class_sizes, stage_classes, NULL);
    // stage_add(b, stage_prio_types, stage_types, NULL);
    // stage_add(b, stage_prio_gen_main, stage_gen, NULL);
    // stage_add(b, stage_prio_ast, stage_ast, NULL);
}

void stage_add(Build* b, int prio, void(*func)(Build*, void*), void* payload) {
    Stage* stage = al(b->alc, sizeof(Stage));
    stage->prio = prio;
    stage->payload = payload;
    stage->func = func;
    stage->done = false;
    array_push(b->stages, stage);
}

void build_run_stages(Build* b) {
    if(b->verbose > 2)
        printf("# Run build stages\n");

    b->parser_started = false;

    Allocator *alc = b->alc;
    Array* stages = b->stages;

    int s = 0;

    while (true) {
        bool did_work = false;
        for(int i = 0; i < stages->length; i++) {
            // if (b->verbose > 2)
            //     printf("# Run build stage: %d\n", i);
            Stage *stage = array_get_index(stages, i);
            if(stage->prio != s || stage->done)
                continue;
            did_work = true;
            stage->func(b, stage->payload);
            stage->done = true;
        }
        if (did_work) {
            s = 0;
            continue;
        }
        s++;
        if (s > stage_last) break;
    }
}
