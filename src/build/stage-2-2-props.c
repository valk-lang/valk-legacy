
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
        Map* props = class->props;
        Map* new_props = map_make(b->alc);
        class->props = new_props;
        int index = 0;
        int fields = 0;

        ClassProp* gc_state = class_prop_make(b, type_gen_volt(b->alc, b, "u8"), true);
        gc_state->index = index++;
        ClassProp* gc_sub_state = class_prop_make(b, type_gen_volt(b->alc, b, "u8"), true);
        gc_sub_state->index = index++;
        ClassProp* gc_fields = class_prop_make(b, type_gen_volt(b->alc, b, "u8"), true);
        gc_fields->index = index++;
        ClassProp* gc_in_list = class_prop_make(b, type_gen_volt(b->alc, b, "u8"), true);
        gc_in_list->index = index++;
        ClassProp* gc_uu1 = class_prop_make(b, type_gen_volt(b->alc, b, "u8"), true);
        gc_uu1->index = index++;
        ClassProp* gc_uu2 = class_prop_make(b, type_gen_volt(b->alc, b, "u8"), true);
        gc_uu2->index = index++;
        ClassProp* gc_size = class_prop_make(b, type_gen_volt(b->alc, b, "u16"), true);
        gc_size->index = index++;

        map_set(new_props, "GC_state", gc_state);
        map_set(new_props, "GC_sub_state", gc_sub_state);
        map_set(new_props, "GC_fields", gc_fields);
        map_set(new_props, "GC_in_list", gc_in_list);
        map_set(new_props, "GC_uu1", gc_uu1);
        map_set(new_props, "GC_uu2", gc_uu2);
        map_set(new_props, "GC_size", gc_size);

        // GC properties
        for(int i = 0; i < props->values->length; i++) {
            char* name = array_get_index(props->keys, i);
            ClassProp* prop = array_get_index(props->values, i);
            if(prop->type->class && prop->type->class->type == ct_class) {
                map_set(new_props, name, prop);
                prop->index = index++;
                fields++;
            }
        }

        // Non GC properties
        for(int i = 0; i < props->values->length; i++) {
            char* name = array_get_index(props->keys, i);
            ClassProp* prop = array_get_index(props->values, i);
            if(!prop->type->class || prop->type->class->type != ct_class) {
                prop->index = index++;
                map_set(new_props, name, prop);
            }
        }

        class->gc_fields = fields;

        // Check for vtable functions
        if (map_contains(class->funcs, "_gc_transfer") || map_contains(class->funcs, "_gc_mark") || map_contains(class->funcs, "_gc_free") || map_contains(class->funcs, "_gc_check_moves")) {
            class->gc_vtable_index = ++b->gc_vtables;
            array_push(b->gc_transfer_funcs, map_get(class->funcs, "_gc_transfer"));
            array_push(b->gc_mark_funcs, map_get(class->funcs, "_gc_mark"));
            array_push(b->gc_free_funcs, map_get(class->funcs, "_gc_free"));
            array_push(b->gc_check_moves, map_get(class->funcs, "_gc_check_moves"));
        }
    }
}
