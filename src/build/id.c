
#include "../all.h"

Idf* idf_make(Allocator* alc, int type, void* item) {
    Idf* idf = al(alc, sizeof(Idf));
    idf->type = type;
    idf->item = item;
    return idf;
}
Decl* decl_make(Allocator* alc, char* name, Type* type, bool is_arg) {
    bool is_gc = !is_arg && type_is_gc(type);
    Decl* d = al(alc, sizeof(Decl));
    d->name = name;
    d->type = type;
    d->ir_var = NULL;
    d->ir_store_var = NULL;
    d->is_mut = false;
    d->is_gc = is_gc;
    d->is_arg = is_arg;
    return d;
}

char* gen_export_name(Nsc* nsc, char* suffix) {
    char name[512];
    Pkc* pkc = nsc->pkc;
    Build* b = pkc->b;
    if(nsc == b->nsc_main) {
        sprintf(name, "%s", suffix);
    } else {
        sprintf(name, "%s__%s__%s_%d", pkc->name, nsc->name, suffix, nsc->unit->export_count++);
    }
    return dups(b->alc, name);
}

Id *read_id(Parser *p, char *first_part, Id *buf) {

    char* ns = NULL;
    char* name = first_part;
    Build* b = p->b;

    if(first_part == NULL) {
        char t = tok(p, true, true, true);
        if(t != tok_id) {
            parse_err(p, -1, "Invalid identifier name syntax");
        }
        name = p->tkn;
    }

    char t = tok(p, false, false, false);
    if(t == tok_colon) {
        tok(p, false, false, true);
        t = tok(p, false, false, true);
        if(t != tok_id) {
            parse_err(p, -1, "Invalid identifier name syntax");
        }
        ns = name;
        name = p->tkn;
    }

    buf->name = name;
    buf->ns = ns;

    return buf;
}

Idf* idf_by_id(Parser* p, Scope* scope, Id* id, bool must_exist) {
    //
    Build* b = p->b;
    char* ns = id->ns;
    char* name = id->name;

    if(ns) {
        Idf* idf = scope_find_idf(scope, ns, true);
        if(!idf || idf->type != idf_scope) {
            parse_err(p, -1, "Unknown namespace: '%s' (try adding 'use %s' to your file)", ns, ns);
        }
        scope = idf->item;
    }

    Idf* idf = scope_find_idf(scope, name, true);
    if(!idf && !ns) {
        if(name[0] == 'u' && str_is(name, "uint")){
            char* name = get_number_type_name(b, b->ptr_size, false, false);
            Nsc* ns = get_valk_nsc(p->b, "type");
            idf = scope_find_idf(ns->scope, name, true);
        } else if(name[0] == 'i' && str_is(name, "int")){
            char* name = get_number_type_name(b, b->ptr_size, false, true);
            Nsc* ns = get_valk_nsc(p->b, "type");
            idf = scope_find_idf(ns->scope, name, true);
        } else if(name[0] == 'f' && str_is(name, "float")){
            char* name = get_number_type_name(b, b->ptr_size, true, false);
            Nsc* ns = get_valk_nsc(p->b, "type");
            idf = scope_find_idf(ns->scope, name, true);
        } else if(str_is(name, "String") || str_is(name, "cstring") || str_is(name, "ptr") || str_is(name, "bool") || str_is(name, "f32") || str_is(name, "f64") || str_is(name, "i64") || str_is(name, "u64") || str_is(name, "i32") || str_is(name, "u32") || str_is(name, "u16") || str_is(name, "i16") || str_is(name, "u8") || str_is(name, "i8") || str_is(name, "Array") || str_is(name, "Map") || str_is(name, "array") || str_is(name, "map")) {
            Nsc* ns = get_valk_nsc(p->b, "type");
            idf = scope_find_idf(ns->scope, name, true);
        } else if(str_is(name, "print") || str_is(name, "println") || str_is(name, "FD")) {
            Nsc* ns = get_valk_nsc(p->b, "io");
            idf = scope_find_idf(ns->scope, name, true);
        } else if(str_is(name, "panic") || str_is(name, "exit")) {
            Nsc* ns = get_valk_nsc(p->b, "os");
            idf = scope_find_idf(ns->scope, name, true);
        }
    }

    if(!idf && must_exist) {
        if(ns) parse_err(p, -1, "Unknown identifier: '%s:%s'", ns, name);
        else parse_err(p, -1, "Unknown identifier: '%s'", name);
    }

    return idf;
}

Idf* scope_find_idf(Scope* scope, char* name, bool recursive) {
    while(scope) {
        Idf* idf = map_get(scope->identifiers, name);
        if(idf)
            return idf;
        if(!recursive)
            break;
        scope = scope->idf_parent;
    }
    return NULL;
}
bool scope_delete_idf_by_value(Scope* scope, void* item, bool recursive) {
    while(scope) {
        Array* idfs = scope->identifiers->values;
        for(int i = 0; i < idfs->length; i++) {
            Idf* idf = array_get_index(idfs, i);
            if(idf->item == item) {
                array_set_index(scope->identifiers->values, i, NULL);
                return true;
            }
        }
        if (!recursive)
            break;
        scope = scope->idf_parent;
    }
    return false;
}


Idf* get_valk_idf(Build* b, char* ns, char* name, bool must_exist) {
    Nsc* nsc = get_valk_nsc(b, ns);
    if(!nsc) {
        if(!must_exist)
            return NULL;
        printf("Namespace: '%s'\n", ns);
        build_err(b, "Valk namespace not found (compiler bug)");
    }
    Idf* idf = scope_find_idf(nsc->scope, name, false);
    if(!idf && must_exist) {
        printf("Identifier: '%s:%s'\n", ns, name);
        build_err(b, "Valk identifier not found (compiler bug)");
    }
    return idf;
}

Func *get_valk_func(Build *b, char *namespace, char *name) {
    Idf* idf = get_valk_idf(b, namespace, name, true);
    if(idf->type == idf_func) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Valk identifier was found but is not a function (compiler bug)");
    return NULL;
}
Func *get_valk_class_func(Build *b, char *namespace, char *class_name, char* fn) {
    Idf* idf = get_valk_idf(b, namespace, class_name, true);
    if(idf->type == idf_class) {
        Class* class = idf->item;
        Func* func = map_get(class->funcs, fn);
        if(func)
            return func;
    }
    printf("Identifier: '%s:%s->%s'\n", namespace, class_name, fn);
    build_err(b, "Valk identifier was found but is not a function (compiler bug)");
    return NULL;
}
Class *get_valk_class(Build *b, char *namespace, char *name) {
    Idf* idf = get_valk_idf(b, namespace, name, true);
    if(idf->type == idf_class) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Valk identifier was found but is not a class (compiler bug)");
    return NULL;
}

Global *get_valk_global(Build *b, char *namespace, char *name) {
    Idf* idf = get_valk_idf(b, namespace, name, true);
    if(idf->type == idf_global) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Valk identifier was found but is not a global variable (compiler bug)");
    return NULL;
}

ValueAlias *get_valk_value_alias(Build *b, char *namespace, char *name) {
    Idf* idf = get_valk_idf(b, namespace, name, true);
    if(idf->type == idf_value_alias) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Valk identifier was found but is not a 'value alias' (compiler bug)");
    return NULL;
}

Snippet *get_valk_snippet(Build *b, char *namespace, char *name) {
    Idf* idf = get_valk_idf(b, namespace, name, true);
    if(idf->type == idf_snippet) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Valk identifier was found but is not a snippet (compiler bug)");
    return NULL;
}