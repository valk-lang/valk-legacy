
#include "../all.h"

void stage_props(Unit *u);

void stage_2_props(Unit* u) {
    Build* b = u->b;

    if (b->verbose > 2)
        printf("Stage 2 | Scan properties: %s\n", u->nsc->name);

    usize start = microtime();
    stage_props(u);
    b->time_parse += microtime() - start;

    stage_add_item(b->stage_2_types, u);
}

void stage_props(Unit *u) {
    Parser* p = u->parser;
    //
    Array* classes = u->classes;
    for(int i = 0; i < classes->length; i++) {
        Class* class = array_get_index(classes, i);
        p->scope = class->scope;
        *p->chunk = *class->body;
        stage_props_class(p, class, false);
    }
    //
}

void stage_props_class(Parser* p, Class *class, bool is_trait) {

    if (class->is_generic_base)
        return;

    if(p->b->verbose > 2){
        printf("# Class properties / functions: '%s'\n", class->name);
    }
    if(class->props->values->length > 0 || class->funcs->values->length > 0){
        die("Called property stage twice on same class (compiler bug)");
    }

    Build *b = p->b;

    while(true) {
        char t = tok(p, true, true, true);

        if(t == tok_curly_close)
            break;

        p->parse_last = false;
        if (t == tok_hashtag && p->on_newline) {
            t = tok(p, false, false, false);
            if(str_is(p->tkn, "parse_last")) {
                t = tok(p, false, false, true);
                p->parse_last = true;
                tok_expect_newline(p);
                t = tok(p, true, true, true);
            } else {
                cc_parse(p);
                continue;
            }
        }

        int act = act_public;
        if (t == tok_sub) {
            act = act_private_fc;
        } else if (t == tok_subsub) {
            act = act_private_nsc;
        } else if (t == tok_triple_sub) {
            act = act_private_pkc;
        } else if (t == tok_tilde) {
            act = act_readonly_fc;
        } else if (t == tok_tilde2) {
            act = act_readonly_nsc;
        } else if (t == tok_tilde3) {
            act = act_readonly_pkc;
        }

        char* name = p->tkn;
        char* act_tkn;

        if(act != act_public) {
            t = tok(p, true, false, true);
            act_tkn = name;
            name = p->tkn;
        }

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
            prop->index = class->props->values->length;
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
        map_set_force_new(class->funcs, name, func);

        parse_handle_func_args(p, func);
    }

    if(p->cc_index > 0) {
        parse_err(p, -1, "Missing #end token");
    }
}
