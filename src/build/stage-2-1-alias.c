
#include "../all.h"

void stage_2_alias(Unit* u) {
    Build* b = u->b;

    if (b->verbose > 2)
        printf("Stage 2 | Aliasses: %s\n", u->nsc->name);

    Parser* p = u->parser;

    Array* als = u->aliasses;
    for(int i = 0; i < als->length; i++) {
        Alias* a = array_get_index(als, i);
        *p->chunk = *a->chunk;

        char t = tok(p, true, false, true);
        if (t != tok_id) {
            parse_err(p, -1, "Invalid identifier name: '%s'", p->tkn);
        }

        Id id;
        read_id(p, p->tkn, &id);
        Idf *idf = idf_by_id(p, a->scope, &id, true);

        scope_set_idf(u->nsc->scope, a->name, idf, p);

        a->idf = idf;
    }

    stage_add_item(b->stage_2_props, u);
}

