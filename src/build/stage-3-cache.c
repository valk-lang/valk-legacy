
#include "../all.h"

void stage_cache(Build *b, void *payload) {

    Array* units = b->units;
    loop(units, i) {
        Unit* u = array_get_index(units, i);
        unit_gen_dep_hash(u);
        u->changed = unit_intern_changed(u) || unit_extern_changed(u);
        if(u->nsc == b->nsc_main)
            u->changed = true;
        if(b->verbose > 2 && u->changed) {
            printf("Stage 3 | Changed: %s\n", u->path_o);
            loop(u->nsc_deps, i) {
                Nsc* nsc = array_get_index(u->nsc_deps, i);
                printf("- Dependency: %s\n", nsc->name);
            }
        }
    }

    stage_ast(b, NULL);
}
