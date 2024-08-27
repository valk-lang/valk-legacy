
#include "../all.h"

void stage_props(Build* b, void* payload) {

    usize start = microtime();

    loop(b->units, o) {
        Unit *u = array_get_index(b->units, o);

        if (b->verbose > 2)
            printf("Stage 2 | Scan properties: %s\n", u->nsc->name);

        Parser *p = u->parser;
        Array *classes = u->classes;
        loop(classes, i) {
            Class *class = array_get_index(classes, i);
            p->scope = class->scope;
            *p->chunk = *class->body;
            stage_props_class(p, class, false);
        }
    }

    b->time_parse += microtime() - start;

    stage_classes(b, NULL);
}

void stage_props_class(Parser* p, Class *class, bool is_trait) {

    if (class->is_generic_base)
        return;

    Build *b = p->b;

    if(p->b->verbose > 2){
        printf("# Class properties / functions: '%s'\n", class->name);
    }
    if(class->props->values->length > 0 || class->funcs->values->length > 0){
        die("Called property stage twice on same class (compiler bug)");
    }

    if (!is_trait && class->type == ct_class) {
        // VTABLE
        class_create_vtable(b, class);

        ClassProp *prop = al(b->alc, sizeof(ClassProp));
        prop->act = act_readonly_fc;
        prop->chunk_type = NULL;
        prop->chunk_value = NULL;
        prop->type = type_cache_ptr(b);
        prop->skip_default_value = true;

        map_set(class->props, "_VTABLE", prop);

        // // RC
        // prop = al(b->alc, sizeof(ClassProp));
        // prop->act = act_readonly_fc;
        // prop->chunk_type = NULL;
        // prop->chunk_value = NULL;
        // prop->type = type_cache_uint(b);
        // prop->skip_default_value = true;

        // map_set(class->props, "_RC", prop);
    }

    while(true) {
        char t = tok(p, true, true, true);

        if(t == tok_curly_close)
            break;

        p->parse_last = false;
        p->init_thread = false;
        if (t == tok_hashtag && p->on_newline) {
            t = tok(p, false, false, false);
            if(str_is(p->tkn, "parse_last")) {
                t = tok(p, false, false, true);
                p->parse_last = true;
                tok_expect_newline(p);
                t = tok(p, true, true, true);
            } else if(str_is(p->tkn, "init_thread")) {
                t = tok(p, false, false, true);
                p->init_thread = true;
                tok_expect_newline(p);
                t = tok(p, true, true, true);
            } else {
                cc_parse(p);
                continue;
            }
        }

        char* act_tkn = p->tkn;
        char res[2];
        read_access_type(p, t, res);
        int act = res[0];
        t = res[1];

        char* name = p->tkn;

        int next = tok(p, true, false, true);

        if(next == tok_colon) {
            if(class->type != ct_class && class->type != ct_struct) {
                parse_err(p, -1, "You cannot define properties on this type");
            }
            if(t != tok_id) {
                parse_err(p, -1, "Invalid property name: '%s'", name);
            }
            if(map_contains(class->props, name) || map_contains(class->funcs, name)) {
                parse_err(p, -1, "Property name is already used for another property or function: '%s'", name);
            }
            ClassProp* prop = al(b->alc, sizeof(ClassProp));
            prop->act = act;
            prop->chunk_type = chunk_clone(b->alc, p->chunk);
            prop->chunk_value = NULL;
            prop->skip_default_value = false;
            map_set_force_new(class->props, name, prop);

            prop->type = read_type(p, b->alc, false);

            t = tok(p, true, false, false);
            if(t == tok_bracket_open) {
                tok(p, true, false, true);
                prop->chunk_value = chunk_clone(b->alc, p->chunk);
                skip_body(p);
            }
            continue;
        }

        if(str_is(name, "@use_gc_alloc")) {
            class->use_gc_alloc = true;
            continue;
        }

        if(str_is(name, "use")) {
            // Trait
            if(is_trait) {
                parse_err(p, -1, "You cannot use traits inside other traits");
            }

            name = p->tkn;
            if (next != tok_id) {
                parse_err(p, -1, "Invalid trait name: '%s'", name);
            }
            Id id;
            Idf *idf = idf_by_id(p, p->scope, read_id(p, name, &id), true);
            if (idf->type != idf_trait) {
                parse_err(p, -1, "This is not a trait: '%s'", id.name);
            }

            Scope* scope = p->scope;
            Trait* t = idf->item;
            parser_new_context(&p);

            *p->chunk = *t->chunk;
            p->scope = scope_sub_make(b->alc, sc_default, t->scope);
            p->scope->identifiers = scope->identifiers;

            stage_props_class(p, class, true);

            parser_pop_context(&p);
            continue;
        }

        bool is_static = false;
        bool is_inline = false;
        char* tkn = name;
        if(str_is(tkn, "static")) {
            is_static = true;
            tkn = p->tkn;
            next = tok(p, true, false, true);
        }
        if(str_is(tkn, "inline")) {
            is_inline = true;
            tkn = p->tkn;
            next = tok(p, true, false, true);
        }
        if(!str_is(tkn, "fn")) {
            parse_err(p, -1, "Expected 'fn' here, found '%s' instead", tkn);
        }
        name = p->tkn;
        if (next != tok_id) {
            parse_err(p, -1, "Invalid property name: '%s'", name);
        }
        if (map_contains(class->props, name) || map_contains(class->funcs, name)) {
            parse_err(p, -1, "Function name is already used for another property or function: '%s'", name);
        }

        char export_name[512];
        sprintf(export_name, "%s__%s", class->ir_name, name);

        Func *func = func_make(b->alc, class->unit, p->scope, name, dups(b->alc, export_name));
        func->act = act;
        func->class = class;
        func->is_static = is_static;
        func->is_inline = is_inline;
        func->in_header = class->in_header;
        func->init_thread = p->init_thread;
        func->parse_last = p->parse_last;
        map_set_force_new(class->funcs, name, func);

        parse_handle_func_args(p, func);
    }

    if(p->cc_index > 0) {
        parse_err(p, -1, "Missing #end token");
    }

    Map *props = class->props;

    if (!is_trait) {
        if (class->type == ct_class) {
            // Count & sort gc fields
            int swap = 1;
            loop(props->keys, i) {
                char *name = array_get_index(props->keys, i);
                ClassProp *prop = array_get_index(props->values, i);
                if (type_is_gc(prop->type)) {
                    array_swap(props->values, i, swap);
                    array_swap(props->keys, i, swap);
                    swap++;
                }
            }

            loop(props->keys, i) {
                char *name = array_get_index(props->keys, i);
                ClassProp *prop = array_get_index(props->values, i);
                if (type_is_gc(prop->type)) {
                    class->gc_fields++;
                }
            }
        }

        // Set property indexes for LLVM IR
        int propc = props->values->length;
        // printf("CLASS: %s\n", class->ir_name);
        loop(props->values, i) {
            char *name = array_get_index(props->keys, i);
            ClassProp *prop = array_get_index(props->values, i);
            prop->index = i;
            // printf("PROP: %s | %d\n", name, i);
        }
    }
}
