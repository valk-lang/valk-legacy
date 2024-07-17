
#include "../all.h"

void stage_cache(Build *b, void *payload) {

    Array* units = b->units;
    loop(units, i) {
        Unit* u = array_get_index(units, i);
        u->changed = unit_intern_changed(u) || unit_extern_changed(u);
    }

    stage_ast(b, NULL);
}
