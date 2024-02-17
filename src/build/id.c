
#include "../all.h"

Idf* idf_make(Allocator* alc, int type, void* item) {
    Idf* idf = al(alc, sizeof(Idf));
    idf->type = type;
    idf->item = item;
    return idf;
}
Decl* decl_make(Allocator* alc, Type* type, bool is_arg) {
    bool is_gc = type_is_gc(type);
    Decl* d = al(alc, sizeof(Decl));
    d->type = type;
    d->ir_var = NULL;
    d->ir_store_var = NULL;
    d->is_mut = false || is_gc;
    d->is_gc = is_gc;
    d->is_arg = is_arg;
    return d;
}

char* gen_export_name(Nsc* nsc, char* suffix) {
    char name[512];
    Pkc* pkc = nsc->pkc;
    Build* b = pkc->b;
    sprintf(name, "%s__%s__%s_%d", pkc->name, nsc->name, suffix, b->export_count++);
    return dups(b->alc, name);
}

Id* read_id(Fc* fc, char* first_part, Id* buf) {

    char* ns = NULL;
    char* name = first_part;
    Build* b = fc->b;

    if(tok_id_next(fc) == tok_char && tok_read_byte(fc, 1) == ':') {
        ns = name;
        tok(fc, false, false, true);
        name = tok(fc, false, false, true);
        if(!is_valid_varname(name)) {
            sprintf(b->char_buf, "Invalid name syntax");
            parse_err(fc->chunk_parse, b->char_buf);
        }
    }

    buf->name = name;
    buf->ns = ns;

    return buf;
}

Idf* idf_by_id(Fc* fc, Scope* scope, Id* id, bool must_exist) {
    //
    Build* b = fc->b;
    char* ns = id->ns;
    char* name = id->name;

    if(ns) {
        Idf* idf = scope_find_idf(fc->scope, ns, false);
        if(!idf || idf->type != idf_scope) {
            sprintf(b->char_buf, "Unknown namespace: '%s' (You can import namespaces using the 'use' keyword)", ns);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        scope = idf->item;
    }

    Idf* idf = scope_find_idf(scope, name, true);
    if(!idf && !ns) {
        if(str_is(name, "string") || str_is(name, "ptr") || str_is(name, "bool") || str_is(name, "int") || str_is(name, "uint") || str_is(name, "i32") || str_is(name, "u32") || str_is(name, "u8") || str_is(name, "Array")) {
            Nsc* ns = get_volt_nsc(fc->b, "type");
            idf = scope_find_idf(ns->scope, name, true);
        }
        if(str_is(name, "print") || str_is(name, "println")) {
            Nsc* ns = get_volt_nsc(fc->b, "io");
            idf = scope_find_idf(ns->scope, name, true);
        }
    }

    if(!idf && must_exist) {
        if(ns) sprintf(b->char_buf, "Unknown identifier: '%s:%s'", ns, name);
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
            if(scope->prio_idf_scope) {
                idf = scope_find_idf(scope->prio_idf_scope, name, true);
                if(idf)
                    return idf;
            }
            scope = scope->parent;
            continue;
        }
        return idf;
    }
    return NULL;
}
Idf* scope_find_type_idf(Scope* scope, char* name, bool recursive) {
    while(scope) {
        Idf* idf = scope->type_identifiers ? map_get(scope->type_identifiers, name) : NULL;
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

Idf* get_volt_idf(Build* b, char* ns, char* name, bool must_exist) {
    Nsc* nsc = get_volt_nsc(b, ns);
    if(!nsc) {
        if(!must_exist)
            return NULL;
        printf("Namespace: '%s'\n", ns);
        build_err(b, "Volt namespace not found (compiler bug)");
    }
    Idf* idf = scope_find_idf(nsc->scope, name, false);
    if(!idf && must_exist) {
        printf("Identifier: '%s:%s'\n", ns, name);
        build_err(b, "Volt identifier not found (compiler bug)");
    }
    return idf;
}

Func *get_volt_func(Build *b, char *namespace, char *name) {
    Idf* idf = get_volt_idf(b, namespace, name, true);
    if(idf->type == idf_func) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Volt identifier was found but is not a function (compiler bug)");
}
Func *get_volt_class_func(Build *b, char *namespace, char *class_name, char* fn) {
    Idf* idf = get_volt_idf(b, namespace, class_name, true);
    if(idf->type == idf_class) {
        Class* class = idf->item;
        Func* func = map_get(class->funcs, fn);
        if(func)
            return func;
    }
    printf("Identifier: '%s:%s->%s'\n", namespace, class_name, fn);
    build_err(b, "Volt identifier was found but is not a function (compiler bug)");
}

Global *get_volt_global(Build *b, char *namespace, char *name) {
    Idf* idf = get_volt_idf(b, namespace, name, true);
    if(idf->type == idf_global) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Volt identifier was found but is not a global variable (compiler bug)");
}

Snippet *get_volt_snippet(Build *b, char *namespace, char *name) {
    Idf* idf = get_volt_idf(b, namespace, name, true);
    if(idf->type == idf_snippet) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Volt identifier was found but is not a snippet (compiler bug)");
}