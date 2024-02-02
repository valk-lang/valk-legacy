
#include "../all.h"

Idf* idf_make(Allocator* alc, int type, void* item) {
    Idf* idf = al(alc, sizeof(Idf));
    idf->type = type;
    idf->item = item;
    return idf;
}
Decl* decl_make(Allocator* alc, Type* type, bool is_arg) {
    Decl* d = al(alc, sizeof(Decl));
    d->type = type;
    d->ir_var = NULL;
    d->is_arg = is_arg;
    d->is_mut = false;
    return d;
}

char* gen_export_name(Nsc* nsc, char* suffix) {
    char name[512];
    Pkc* pkc = nsc->pkc;
    Build* b = pkc->b;
    sprintf(name, "%s__%s__%s_%d", pkc->name, nsc->name, suffix, b->export_count++);
    return dups(b->alc, name);
}


Idf* read_idf(Fc* fc, Scope* scope, char* first_part, bool must_exist) {
    //
    char* nsc = NULL;
    char* name = first_part;
    Build* b = fc->b;

    if(tok_id_next(fc) == ':') {
        nsc = name;
        tok(fc, false, false, true);
        name = tok(fc, false, false, true);
        if(!is_valid_varname(name)) {
            sprintf(b->char_buf, "Missing string closing tag '\"', compiler reached end of file");
            parse_err(fc->chunk_parse, b->char_buf);
        }
    }

    if(nsc) {
        Idf* idf = scope_find_idf(fc->scope, nsc, false);
        if(!idf || idf->type != idf_nsc) {
            sprintf(b->char_buf, "Unknown namespace name/alias: '%s'", nsc);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        Nsc* ns = idf->item;
        scope = ns->scope;
    }

    Idf* idf = scope_find_idf(scope, name, true);
    if(!idf && must_exist) {
        if(nsc) sprintf(b->char_buf, "Unknown identifier: '%s:%s'", nsc, name);
        else sprintf(b->char_buf, "Unknown identifier: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    return idf;
}

Idf* scope_find_idf(Scope* scope, char* name, bool recursive) {
    while(scope) {
        Idf* idf = map_get(scope->identifiers, name);
        if(!idf) {
            if(!recursive)
                break;
            scope = scope->parent;
            continue;
        }
        return idf;
    }
    return NULL;
}
