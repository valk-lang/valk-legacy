
#include "../all.h"

void stage_determine_class_sizes(Build* b, Array* classes);

void stage_2_update_classes(Build* b) {

    if (b->verbose > 2)
        printf("Stage 2 | Update class sizes\n");

    usize start = microtime();

    stage_determine_class_sizes(b, b->classes);

    b->time_parse += microtime() - start;

    Array* fcs = b->fc_by_path->values;
    for(int i = 0; i < fcs->length; i++) {
        stage_add_item(b->stage_2_types, array_get_index(fcs, i));
    }
}

void stage_determine_class_sizes(Build* b, Array* classes) {

    int count = classes->length;
    int changed = 0;

    for (int i = 0; i < classes->length; i++) {
        Class *class = array_get_index(classes, i);
        if(class->size > 0) {
            changed++;
        }
    }

    while (changed < count) {
        for (int i = 0; i < classes->length; i++) {
            Class *class = array_get_index(classes, i);
            if(class->size > 0)
                continue;

            bool done = class_determine_size(b, class);
            if(done) {
                changed++;
            }
        }

        if(changed == 0) {
            build_err(b, "Cannot determine all class sizes because of a circular dependency");
        }
    }
}

