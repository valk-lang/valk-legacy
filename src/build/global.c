
#include "../all.h"

Global* global_make(Allocator* alc, Unit* u, Fc* fc, int act, char* name, bool is_shared, bool is_const) {
    Global* g = al(alc, sizeof(Global));
    g->act = act;
    g->fc = fc;
    g->unit = u;
    g->name = name;
    g->export_name = gen_export_name(u->nsc, name);
    g->type = NULL;
    g->value = NULL;
    g->chunk_type = NULL;
    g->chunk_value = NULL;
    g->declared_scope = NULL;
    g->is_shared = is_shared;
    g->is_used = true;
    g->is_const = is_const;

    return g;
}
