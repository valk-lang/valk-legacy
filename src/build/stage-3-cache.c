
#include "../all.h"

void stage_cache(Build *b, void *payload) {

    Array* units = b->units;
    loop(units, i) {
        Unit* u = array_get_index(units, i);
        unit_check_cache(u);

        if (u->nsc == b->nsc_main)
            u->changed = true;
        // if (u->nsc == b->nsc_generated)
        //     u->changed = true;

        if(b->verbose > 2 && u->changed) {
            printf("Stage 3 | Changed: %s\n", u->path_o);
            loop(u->nsc_deps, i) {
                Nsc* nsc = array_get_index(u->nsc_deps, i);
                printf("- Dependency: %s\n", nsc->name);
            }
        }
    }

    // Generate vtable indexes
    loop(units, i) {
        Unit* u = array_get_index(units, i);
        Array* classes = u->classes;
        loop(classes, o) {
            Class* class = array_get_index(classes, o);
            class_assign_vtable_index(class);
        }
    }

    stage_ast(b, NULL);
}
