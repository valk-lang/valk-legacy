
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
    sprintf(name, "%s__%s__%s_%d", pkc->name, nsc->name, suffix, b->export_count++);
    return dups(b->alc, name);
}

Id *read_id(Parser *p, char *first_part, Id *buf) {

    char* ns = NULL;
    char* name = first_part;
    Build* b = p->b;

    char t = tok(p, false, false, false);
    if(t == tok_colon) {
        tok(p, false, false, true);
        t = tok(p, false, false, true);
        if(t != tok_id) {
            parse_err(p, -1, "Invalid name syntax");
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
        if(str_is(name, "String") || str_is(name, "cstring") || str_is(name, "ptr") || str_is(name, "bool") || str_is(name, "int") || str_is(name, "uint") || str_is(name, "i32") || str_is(name, "u32") || str_is(name, "u16") || str_is(name, "u8") || str_is(name, "Array") || str_is(name, "Map")) {
            Nsc* ns = get_volt_nsc(p->b, "type");
            idf = scope_find_idf(ns->scope, name, true);
        }
        if(str_is(name, "print") || str_is(name, "println")) {
            Nsc* ns = get_volt_nsc(p->b, "io");
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
        if(!idf) {
            if(!recursive)
                break;
            scope = scope->idf_parent;
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
Class *get_volt_class(Build *b, char *namespace, char *name) {
    Idf* idf = get_volt_idf(b, namespace, name, true);
    if(idf->type == idf_class) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Volt identifier was found but is not a class (compiler bug)");
}

Global *get_volt_global(Build *b, char *namespace, char *name) {
    Idf* idf = get_volt_idf(b, namespace, name, true);
    if(idf->type == idf_global) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Volt identifier was found but is not a global variable (compiler bug)");
}

ValueAlias *get_volt_value_alias(Build *b, char *namespace, char *name) {
    Idf* idf = get_volt_idf(b, namespace, name, true);
    if(idf->type == idf_value_alias) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Volt identifier was found but is not a 'value alias' (compiler bug)");
}

Snippet *get_volt_snippet(Build *b, char *namespace, char *name) {
    Idf* idf = get_volt_idf(b, namespace, name, true);
    if(idf->type == idf_snippet) {
        return idf->item;
    }
    printf("Identifier: '%s:%s'\n", namespace, name);
    build_err(b, "Volt identifier was found but is not a snippet (compiler bug)");
}