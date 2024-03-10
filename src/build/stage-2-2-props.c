
#include "../all.h"

void stage_props(Unit *u);
void stage_props_class(Unit* u, Class *class);

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
    Array* classes = u->classes;
    for(int i = 0; i < classes->length; i++) {
        Class* class = array_get_index(classes, i);
        stage_props_class(u->b->parser, class);
    }
}

void stage_props_class(Parser* p, Class *class) {

    if (class->is_generic_base)
        return;

    Build *b = p->b;
    *p->chunk = *class->body;

    while(true) {
        int t = tok(p, true, true, true);

        if(t == tok_curly_close)
            break;

        int act = act_public;
        if(t == tok_sub) {
            act = act_private_fc;
            t = tok(p, true, false, true);
        } else if(t == tok_tilde) {
            act = act_readonly_fc;
            t = tok(p, true, false, true);
        }
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
            prop->chunk_type = chunk_clone(b->alc, p->chunk);
            prop->chunk_value = NULL;
            prop->index = class->props->values->length;
            map_set_force_new(class->props, name, prop);

            prop->type = read_type(p, b->alc, class->scope, false);

            t = tok(p, true, false, false);
            if(t == tok_bracket_open) {
                tok(p, true, false, true);
                prop->chunk_value = chunk_clone(b->alc, p->chunk);
                skip_body(p);
            }
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
        char* name = p->tkn;
        if (next != tok_id) {
            parse_err(p, -1, "Invalid property name: '%s'", name);
        }
        if (map_contains(class->props, name) || map_contains(class->funcs, name)) {
            parse_err(p, -1, "Function name is already used for another property or function: '%s'", name);
        }

        char export_name[512];
        sprintf(export_name, "%s__%s", class->ir_name, name);

        Unit* u = class->unit;
        Func *func = func_make(b->alc, u, class->scope, name, dups(b->alc, export_name));
        func->class = class;
        func->is_static = is_static;
        func->is_inline = is_inline;
        array_push(u->funcs, func);
        map_set_force_new(class->funcs, name, func);

        parse_handle_func_args(p, func);
    }
}
