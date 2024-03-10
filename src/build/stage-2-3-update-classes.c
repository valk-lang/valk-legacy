
#include "../all.h"

void stage_determine_class_sizes(Build* b, Array* classes);

void stage_2_update_classes(Build* b) {

    if (b->verbose > 2)
        printf("Stage 2 | Update class sizes\n");

    usize start = microtime();

    stage_determine_class_sizes(b, b->classes);

    for (int i = 0; i < b->classes->length; i++) {
        Class *class = array_get_index(b->classes, i);
        class_generate_internals(b->parser, b, class);
    }

    b->time_parse += microtime() - start;

    Array* fcs = b->fc_by_path->values;
    for(int i = 0; i < fcs->length; i++) {
        stage_add_item(b->stage_2_types, array_get_index(fcs, i));
    }
}

void stage_determine_class_sizes(Build* b, Array* classes) {

    int count = classes->length;
    int found = 0;

    for (int i = 0; i < classes->length; i++) {
        Class *class = array_get_index(classes, i);
        if(class->size > -1) {
            found++;
        }
    }

    while (found < count) {
        int changed = 0;

        for (int i = 0; i < classes->length; i++) {
            Class *class = array_get_index(classes, i);
            if(class->size > -1)
                continue;
            
            if(b->verbose > 2) {
                printf("# Try find class size for: %s\n", class->name);
            }

            int size = class_determine_size(b, class);
            if(size > -1) {
                if(b->verbose > 2) {
                    printf("# Class size for '%s' is: %d\n", class->name, size);
                }
                changed++;
            }
        }

        if(changed == 0) {
            build_err(b, "Cannot determine all class sizes because of a circular dependency");
        }
        found += changed;
    }
}

