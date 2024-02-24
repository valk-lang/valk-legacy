
#include "../all.h"

void stage_props(Fc *fc);
void stage_props_class(Fc* fc, Class *class);

void stage_2_props(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 2 | Scan properties: %s\n", fc->path);

    usize start = microtime();
    stage_props(fc);
    b->time_parse += microtime() - start;

    stage_add_item(b->stage_2_types, fc);
}

void stage_props(Fc *fc) {
    Array* classes = fc->classes;
    for(int i = 0; i < classes->length; i++) {
        Class* class = array_get_index(classes, i);
        stage_props_class(fc, class);
    }
}

void stage_props_class(Fc* fc, Class *class) {

    if (class->is_generic_base)
        return;

    Build *b = fc->b;
    *fc->chunk_parse = *class->body;

    while(true) {
        char *tkn = tok(fc, true, true, true);

        int t = fc->chunk_parse->token;
        if(t == tok_scope_close)
            break;

        int act = act_public;
        if(str_is(tkn, "-")) {
            act = act_private_fc;
            tkn = tok(fc, true, false, true);
        } else if(str_is(tkn, "~")) {
            act = act_readonly_fc;
            tkn = tok(fc, true, false, true);
        }
        t = fc->chunk_parse->token;
        char* next = tok(fc, true, false, true);

        if(str_is(next, ":")) {
            char* name = tkn;
            if(class->type != ct_class && class->type != ct_struct) {
                sprintf(b->char_buf, "You cannot define properties on this type");
                parse_err(fc->chunk_parse, b->char_buf);
            }
            if(t != tok_id) {
                sprintf(b->char_buf, "Invalid property name: '%s'", name);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            if(map_contains(class->props, name) || map_contains(class->funcs, name)) {
                sprintf(b->char_buf, "Property name is already used for another property or function: '%s'", name);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            ClassProp* prop = al(b->alc, sizeof(ClassProp));
            prop->chunk_type = chunk_clone(b->alc, fc->chunk_parse);
            prop->chunk_value = NULL;
            prop->index = class->props->values->length;
            map_set_force_new(class->props, name, prop);

            prop->type = read_type(fc, fc->alc, class->scope, false);

            tok_skip_whitespace(fc);
            if(tok_read_byte(fc, 0) == tok_scope_open && tok_read_byte(fc, 1 + sizeof(int)) == '(') {
                tkn = tok(fc, true, true, true);
                prop->chunk_value = chunk_clone(b->alc, fc->chunk_parse);
                skip_body(fc);
            }
            continue;
        }

        bool is_static = false;
        bool is_inline = false;
        if(str_is(tkn, "static")) {
            is_static = true;
            tkn = next;
            next = tok(fc, true, false, true);
        }
        if(str_is(tkn, "inline")) {
            is_inline = true;
            tkn = next;
            next = tok(fc, true, false, true);
        }
        if(!str_is(tkn, "fn")) {
            sprintf(b->char_buf, "Expected 'fn' here, found '%s' instead", tkn);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        char* name = next;
        t = fc->chunk_parse->token;
        if (t != tok_id) {
            sprintf(b->char_buf, "Invalid property name: '%s'", name);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        if (map_contains(class->props, name) || map_contains(class->funcs, name)) {
            sprintf(b->char_buf, "Function name is already used for another property or function: '%s'", name);
            parse_err(fc->chunk_parse, b->char_buf);
        }

        Func *func = func_make(b->alc, fc, class->scope, name, NULL);
        func->class = class;
        func->is_static = is_static;
        func->is_inline = is_inline;
        array_push(fc->funcs, func);
        map_set_force_new(class->funcs, name, func);

        parse_handle_func_args(fc, func);
    }

    // Add GC properties + re-order previous properties
    if(class->type == ct_class) {
    }
}
