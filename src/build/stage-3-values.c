
#include "../all.h"

void stage_values(Parser* p, Unit* u);

void stage_3_values(Unit* u) {
    Build* b = u->b;
    Parser* p = u->parser;

    if (b->verbose > 2)
        printf("Stage 3 | Scan values: %s\n", u->nsc->name);

    usize start = microtime();
    stage_values(p, u);
    b->time_parse += microtime() - start;

    stage_add_item(b->stage_4_ast, u);
}

void stage_values(Parser* p, Unit* u) {
    Build *b = p->b;

    Array* globals = u->globals;
    for(int i = 0; i < globals->length; i++) {
        Global* g = array_get_index(globals, i);

        if(!g->chunk_value) {
            if(!g->type->is_pointer)
                continue;
            if(!g->type->nullable && !g->type->ignore_null) {
                char buf[256];
                parse_err(p, -1, "Globals with a non-null type require a default value (type: %s)", type_to_str(g->type, buf));
            }
            g->value = value_make(b->alc, v_null, NULL, g->type);
            continue;
        }

        *p->chunk = *g->chunk_value;
        p->scope = g->declared_scope;
        g->value = read_value(b->alc, p, true, 0);

        type_check(p, g->type, g->value->rett);
    }
}
