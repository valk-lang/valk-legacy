
#include "../all.h"

Stage* stage_make(Allocator* alc, void (*func)(void*)) {
    Stage* s = al(alc, sizeof(Stage));
    s->items = array_make(alc, 20);
    s->func = func;
    s->i = 0;
    return s;
}

void build_set_stages(Build* b) {
    Allocator *alc = b->alc;
    b->stage_1_parse = stage_make(alc, (void(*)(void*))stage_1_parse);
    b->stage_2_alias = stage_make(alc, (void(*)(void*))stage_2_alias);
    b->stage_2_props = stage_make(alc, (void(*)(void*))stage_2_props);
    b->stage_2_class_sizes = stage_make(alc, (void(*)(void*))stage_2_update_classes);
    b->stage_2_types = stage_make(alc, (void(*)(void*))stage_2_types);
    b->stage_3_values = stage_make(alc, (void(*)(void*))stage_3_values);
    b->stage_3_gen = stage_make(alc, (void(*)(void*))stage_3_gen);
    b->stage_4_ast = stage_make(alc, (void(*)(void*))stage_4_ast);

    stage_add_item(b->stage_2_class_sizes, b);
    stage_add_item(b->stage_3_gen, b);
}

void stage_add_item(Stage* stage, void* item) {
    array_push(stage->items, item);
}
void* stage_get_item(Stage* stage) {
    if(stage->i >= stage->items->length)
        return NULL;
    return array_get_index(stage->items, stage->i++);
}

void build_run_stages(Build* b) {
    if(b->verbose > 2)
        printf("# Run build stages\n");

    b->parser_started = false;

    Allocator *alc = b->alc;
    Array* stages = array_make(alc, 10);
    array_push(stages, b->stage_1_parse);
    array_push(stages, b->stage_2_alias);
    array_push(stages, b->stage_2_props);
    array_push(stages, b->stage_2_class_sizes);
    array_push(stages, b->stage_2_types);
    array_push(stages, b->stage_3_values);
    array_push(stages, b->stage_3_gen);
    array_push(stages, b->stage_4_ast);

    for (int i = 0; i < stages->length; i++) {
        // if (b->verbose > 2)
        //     printf("# Run build stage: %d\n", i);
        bool did_work = false;
        Stage *stage = array_get_index(stages, i);
        void *item = stage_get_item(stage);
        while(item) {
            stage->func(item);
            item = stage_get_item(stage);
            did_work = true;
        }
        if (did_work) {
            i = -1;
        }
    }
}
