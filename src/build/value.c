
#include "../all.h"

Value* value_handle_idf(Allocator *alc, Parser* p, Idf *idf);
Value *value_func_call(Allocator *alc, Parser* p, Value *on);
Value* value_handle_class(Allocator *alc, Parser* p, Class* class);
Value* value_handle_ptrv(Allocator *alc, Parser* p);
Value* value_handle_compare(Allocator *alc, Parser* p, Value *left, Value* right, int op);
Value* pre_calc_float(Allocator* alc, Build* b, Value* n1, Value* n2, int op);
Value* pre_calc_int(Allocator* alc, Build* b, Value* n1, Value* n2, int op);
Value *read_err_value(Allocator* alc, Parser *p, Array *err_names, Array *err_values);

void value_check_act(int act, Fc* fc, Parser* p, char* name);

Value* read_value(Allocator* alc, Parser* p, bool allow_newline, int prio) {
    Build *b = p->b;
    Chunk *chunk = p->chunk;

    char t = tok(p, true, true, true);
    char* tkn = p->tkn;

    Value* v = NULL;

    if (t == tok_at_word) {
        if (str_is(tkn, "@ptrv")) {
            v = value_handle_ptrv(alc, p);
        } else if (str_is(tkn, "@ptr_of")) {
            //
            tok_expect(p, "(", false, false);
            Value *from = read_value(alc, p, true, 0);
            if (!value_is_assignable(from)) {
                parse_err(p, -1, "Value in @ptr_of must be an assign-able value. e.g. a variable");
            }
            tok_expect(p, ")", true, true);
            if (from->type == v_decl) {
                Decl *decl = from->item;
                if (!decl->is_mut)
                    decl->is_mut = true;
            } else if (from->type == v_decl_overwrite) {
                DeclOverwrite *dov = from->item;
                if (!dov->decl->is_mut)
                    dov->decl->is_mut = true;
            }
            v = value_make(alc, v_ptr_of, from, type_gen_valk(alc, b, "ptr"));

        } else if (str_is(tkn, "@ptr_offset")) {
            tok_expect(p, "(", false, false);
            Value *on = read_value(alc, p, true, 0);
            if (!on->rett->is_pointer) {
                parse_err(p, -1, "First argument of '@ptr_offset' must a pointer value");
            }
            tok_expect(p, ",", true, false);
            Value *index = read_value(alc, p, true, 0);
            if (index->rett->type != type_int) {
                parse_err(p, -1, "Second argument of '@ptr_offset' must return an integer value");
            }
            int t = tok(p, true, false, false);
            int size = 1;
            if(t == tok_comma) {
                tok(p, true, false, true);
                Value* s = read_value(alc, p, true, 0);
                if (s->type != v_number || s->rett->type == ct_float) {
                    parse_err(p, -1, "Third argument of '@ptr_offset' must an integer literal. e.g. 1 or sizeof(ptr)");
                }
                VNumber* vn = s->item;
                size = vn->value_int;
                if (size < 1) {
                    parse_err(p, -1, "Third argument of '@ptr_offset' must be larger than 0");
                }
            }
            tok_expect(p, ")", true, true);
            v = vgen_ptr_offset(alc, b, on, index, size);

        } else if (str_is(tkn, "@stack")) {
            tok_expect(p, "(", false, false);
            Type* type = read_type(p, alc, true);
            tok_expect(p, ")", true, true);
            int size = (type->class ? type->class->size : type->size);
            Type* rett = type;
            if(!type->is_pointer) {
                if(type->type == type_static_array) {
                    rett = type_clone(alc, type);
                    rett->is_pointer = true;
                } else {
                    rett = type_cache_ptr(b);
                }
            }
            v = vgen_stack_size(alc, b, size);
            v->rett = rett;

        } else if (str_is(tkn, "@stack_bytes")) {
            tok_expect(p, "(", false, false);
            Value* val = read_value(alc, p , true, 0);
            if(val->rett->type != type_int) {
                parse_err(p, -1, "The first argument for @stack_bytes must be a valid integer value");
            }
            tok_expect(p, ")", true, true);
            v = vgen_stack(alc, b, val);

        } else if (str_is(tkn, "@gc_link")){
            tok_expect(p, "(", false, false);
            Value* on = read_value(alc, p, true, 0);
            tok_expect(p, ",", true, true);
            Value* to = read_value(alc, p, true, 0);
            tok_expect(p, ")", true, true);
            if(type_is_gc(on->rett) && type_is_gc(to->rett)) {
                v = vgen_gc_link(alc, on, to, to->rett);
            } else {
                v = to;
            }
        } else if (str_is(tkn, "@frameptr")) {
            v = value_make(alc, v_frameptr, NULL, type_cache_ptr(b));
        } else if (str_is(tkn, "@stackptr")) {
            v = value_make(alc, v_stackptr, NULL, type_cache_ptr(b));
        } else if (str_is(tkn, "@setjmp")) {
            tok_expect(p, "(", false, false);
            Value* buf = read_value(alc, p, true, 0);
            tok_expect(p, ")", true, true);
            if(!buf->rett->is_pointer) {
                parse_err(p, -1, "Expected a value that return a pointer type");
            }
            v = value_make(alc, v_setjmp, buf, type_cache_i32(b));
        } else if (str_is(tkn, "@longjmp")) {
            tok_expect(p, "(", false, false);
            Value* buf = read_value(alc, p, true, 0);
            tok_expect(p, ")", true, true);
            if(!buf->rett->is_pointer) {
                parse_err(p, -1, "Expected a value that return a pointer type");
            }
            v = value_make(alc, v_longjmp, buf, type_gen_void(alc));
            p->scope->did_return = true;
        } else if (str_is(tkn, "@gc_get_vtable")) {
            tok_expect(p, "(", false, false);
            Value* index = read_value(alc, p, true, 0);
            tok_expect(p, ")", true, true);
            v = value_make(alc, v_gc_get_table, index, type_gen_valk(alc, b, "ptr"));
        } else if (str_is(tkn, "@type_vtable_index")) {
            tok_expect(p, "(", false, false);
            Type *type = read_type(p, alc, true);
            tok_expect(p, ")", true, true);
            v = vgen_int(alc, type->class->gc_vtable_index, type_gen_valk(alc, b, "int"));
        } else if (str_is(tkn, "@class_of")) {
            tok_expect(p, "(", false, false);
            Type *type = read_type(p, alc, true);
            tok_expect(p, ")", true, true);
            Class* class = type->class;
            if(!class) {
                parse_err(p, -1, "Type has no class");
            }
            v = value_handle_class(alc, p, class);
        } else if (str_is(tkn, "@memset")) {
            tok_expect(p, "(", false, false);
            Type *t_u8 = type_cache_u8(b);
            Type *t_uint = type_cache_uint(b);

            Value *on = read_value(alc, p, true, 0);
            Type *rett = on->rett;
            if (!rett->is_pointer) {
                parse_err(p, -1, "@memset first parameter must be a pointer");
            }
            tok_expect(p, ",", true, true);
            Value *length = read_value(alc, p, true, 0);
            length = try_convert(alc, p, p->scope, length, t_uint);
            type_check(p, t_uint, length->rett);
            tok_expect(p, ",", true, true);
            Value *with = read_value(alc, p, true, 0);
            with = try_convert(alc, p, p->scope, with, t_u8);
            type_check(p, t_u8, length->rett);
            tok_expect(p, ")", true, true);

            v = vgen_memset(alc, on, length, with);

        } else if (str_is(tkn, "@memcpy")) {
            tok_expect(p, "(", false, false);
            Value* from = read_value(alc, p, true, 0);
            if(from->rett->type != type_ptr) {
                parse_err(p, -1, "The first argument for '@memset' should be a ptr value");
            }
            tok_expect(p, ",", true, true);
            Value* to = read_value(alc, p, true, 0);
            if(from->rett->type != type_ptr) {
                parse_err(p, -1, "The 2nd argument for '@memset' should be a ptr value");
            }
            tok_expect(p, ",", true, true);
            Value* len = read_value(alc, p, true, 0);
            if(len->rett->type != type_int) {
                parse_err(p, -1, "The 3rd argument for '@memset' should be an integer value");
            }
            tok_expect(p, ")", true, true);

            VMemcpy* mc = al(alc, sizeof(VMemcpy));
            mc->from = from;
            mc->to = to;
            mc->length = len;
            v = value_make(alc, v_memcpy, mc, type_gen_valk(alc, b, "ptr"));

        }
    } else if (t == tok_string) {
        char *body = tkn;
        body = string_replace_backslash_chars(b->alc, body);
        v = vgen_string(alc, p->unit, body);
    } else if (t == tok_char) {

        v = vgen_int(alc, tkn[0], type_gen_valk(alc, b, "u8"));

    } else if (t == tok_not) {

        Value* on = read_value(alc, p, true, 1);
        if (!type_is_bool(on->rett)) {
            parse_err(p, -1, "Value after '!' must be of type 'bool'");
        }
        v = value_make(alc, v_not, on, on->rett);
        // Flip issets if only 1 else clear
        Array* is = on->issets;
        Array* nis = on->not_issets;
        int count = is ? is->length : 0;
        count += nis ? nis->length : 0;
        if (count == 1) {
            v->not_issets = is;
            v->issets = nis;
        }

    } else if (t == tok_bracket_open) {
        Array* values = array_make(alc, 1);
        while(true) {
            v = read_value(alc, p, true, 0);
            array_push(values, v);
            t = tok_expect_two(p, ",", ")", true, true);
            if(t == tok_bracket_close)
                break;
        }
        buffer_values(alc, p, values, false);
        v = vgen_multi(alc, values);
    } else if (t == tok_ltcurly_open) {
        int scope_end_i = p->scope_end_i;
        Array *_prev_values = p->vscope_values;
        Array *values = array_make(alc, 2);
        Scope *_scope = p->scope;
        Scope *vscope = scope_sub_make(alc, sc_vscope, _scope);
        p->scope = vscope;
        p->vscope_values = values;
        read_ast(p, false);
        p->scope = _scope;
        p->vscope_values = _prev_values;
        p->chunk->i = scope_end_i;

        if(!vscope->did_return) {
            parse_err(p, -1, "Value scope must return a value");
        }
        Type* type = vscope_get_result_type(values);
        if(!type) {
            parse_err(p, -1, "Value scope must have atleast 1 return statement");
        }
        // Type check
        loop(values, i) {
            Value* v = array_get_index(values, i);
            type_check(p, type, v->rett);
        }

        v = value_make(alc, v_vscope, vscope, type);

    } else if (t == tok_id) {
        if (str_is(tkn, "sizeof")) {
            tok_expect(p, "(", false, false);
            Type *type = read_type(p, alc, true);
            tok_expect(p, ")", true, true);
            v = vgen_int(alc, type->size, type_gen_valk(alc, b, "int"));

            Type *tcv = p->try_conv;
            bool tcv_int = tcv ? tcv->type == type_int : false;
            if (tcv_int)
                try_convert_number(v, tcv);

        } else if (str_is(tkn, "true") || str_is(tkn, "false")) {
            v = vgen_bool(alc, b, str_is(tkn, "true"));
        } else if (str_is(tkn, "null")) {
            v = value_make(alc, v_null, NULL, type_gen_null(alc, b));
        } else if (str_is(tkn, "atomic")) {
            tok_expect(p, "(", false, false);
            Value* val = read_value(alc, p, true, 0);
            tok_expect(p, ")", true, true);
            if(val->type != v_op) {
                parse_err(p, -1, "Value is not a math operation. e.g. (my_var + 2)");
            }
            VOp* vop = val->item;
            Value* left = vop->left;
            Value* right = vop->right;
            int op = vop->op;
            if(op != op_add && op != op_sub && op != op_mul && op != op_div){
                parse_err(p, -1, "Atomic only allows: + - * /");
            }
            if(!value_is_assignable(left)) {
                parse_err(p, -1, "Left value must be an assignable value. e.g. a variable");
            }
            v = value_make(alc, v_atomic, vop, val->rett);
        } else if (str_is(tkn, "isset")) {
            tok_expect(p, "(", false, false);
            Value* on = read_value(alc, p, true, 0);
            if(on->type != v_decl && on->type != v_decl_overwrite) {
                parse_err(p, -1, "The 'isset' value must be a local variable");
            }
            if(!on->rett->nullable) {
                parse_err(p, -1, "Using 'isset' on a value that cant be null");
            }
            tok_expect(p, ")", true, true);
            v = vgen_isset(alc, b, on);

            if (on->type == v_decl || on->type == v_decl_overwrite) {
                Array *issets = array_make(alc, 4);
                v->issets = issets;
                array_push(issets, on);
            }

        } else if (str_is(tkn, "error_is")) {
            tok_expect(p, "(", false, false);
            Id id;
            read_id(p, NULL, &id);
            Idf *idf = idf_by_id(p, p->scope, &id, true);
            if(idf->type != idf_error) {
                parse_err(p, -1, "Expected an error identifier here");
            }

            Decl* edecl = idf->item;
            Array* err_names = edecl->type->func_info->err_names;
            Array* err_values = edecl->type->func_info->err_values;

            Value* left = value_make(alc, v_decl, edecl, type_gen_number(alc, b, 4, false, false));

            tok_expect(p, ",", true, true);

            Value* right = read_err_value(alc, p, err_names, err_values);
            v = value_handle_compare(alc, p, left, right, op_eq);

            t = tok(p, true, true, false);
            while(t == tok_comma) {
                t = tok(p, true, true, true);
                Value* right = read_err_value(alc, p, err_names, err_values);
                Value* next = value_handle_compare(alc, p, left, right, op_eq);
                v = vgen_and_or(alc, b, v, next, op_or);
                t = tok(p, true, true, false);
            }

            tok_expect(p, ")", true, true);

        } else if (str_is(tkn, "error_value")) {

            tok_expect(p, "(", false, false);
            Id id;
            read_id(p, NULL, &id);
            Idf *idf = idf_by_id(p, p->scope, &id, true);
            if(idf->type != idf_error) {
                parse_err(p, -1, "Expected an error identifier here");
            }
            tok_expect(p, ")", true, true);

            Decl* edecl = idf->item;
            v = value_make(alc, v_decl, edecl, type_gen_number(alc, b, 4, false, false));

        } else if (str_is(tkn, "co")) {

            bool before = p->reading_coro_fcall;
            p->reading_coro_fcall = true;
            Value* on = read_value(alc, p, true, 1);
            p->reading_coro_fcall = before;

            if(on->type != v_func_call) {
                parse_err(p, -1, "Missing function call statement after 'co' token");
            }
            v = coro_generate(alc, p, on);

        } else if (str_is(tkn, "await")) {

            Value* on = read_value(alc, p, true, 1);
            if(on->rett->type != type_promise) {
                parse_err(p, -1, "Using 'await' on a non-promise value");
            }

            v = coro_await(alc, p, on);

        } else if (str_is(tkn, "bit_lz")) {

            tok_expect(p, "(", false, false);
            Value* val = read_value(alc, p, true, 0);
            if(val->rett->type != type_int) {
                parse_err(p, -1, "The first argument for 'bit_lz' should be a integer value");
            }
            tok_expect(p, ")", true, true);

            v = value_make(alc, v_bit_lz, val, val->rett);

        } else if (p->func && p->func->is_test && str_is(tkn, "assert")) {

            tok_expect(p, "(", false, false);
            Value* val = read_value(alc, p, true, 0);
            if(val->rett->type != type_bool) {
                parse_err(p, -1, "The first argument for 'assert' should be a bool value");
            }
            tok_expect(p, ")", true, true);
            // Get test result variable
            Idf *idf = scope_find_idf(p->scope, "VALK_TEST_RESULT", true);
            if(!idf) {
                parse_err(p, -1, "Missing test result variable while parsing 'assert' (compiler bug)");
            }
            Value* result = value_handle_idf(alc, p, idf);
            // 
            Func* test_assert = get_valk_func(b, "core", "test_assert");
            func_mark_used(p->func, test_assert);
            Array* args = array_make(alc, 2);
            array_push(args, val);
            array_push(args, result);
            array_push(args, vgen_string(alc, p->unit, p->chunk->fc ? p->chunk->fc->path : "{generated-code}"));
            array_push(args, vgen_int(alc, p->line, type_gen_valk(alc, b, "uint")));
            Value* fptr = vgen_func_ptr(alc, test_assert, NULL);
            v = vgen_func_call(alc, p, fptr, args);

        } else if (str_is(tkn, "__FILE__")) {
            char* body = p->func->fc->path;
            v = vgen_string(alc, p->unit, body);

        } else if (str_is(tkn, "__DIR__")) {
            char body[VALK_PATH_MAX];
            get_dir_from_path(p->func->fc->path, body);
            v = vgen_string(alc, p->unit, dups(alc, body));

        } else {
            // Identifiers
            Id id;
            read_id(p, tkn, &id);
            Idf *idf = idf_by_id(p, p->scope, &id, true);
            v = value_handle_idf(alc, p, idf);
        }
    } else if (t == tok_number || t == tok_sub) {
        bool negative = false;
        if(t == tok_sub) {
            negative = true;
            t = tok(p, false, false, true);
            if(t != tok_number) {
                parse_err(p, -1, "Invalid negative number syntax: '%s'", p->tkn);
            }
        }
        Type *tcv = p->try_conv;
        bool tcv_unsigned = tcv ? !tcv->is_signed : false;
        bool tcv_float = tcv ? tcv->type == type_float : false;
        bool tcv_int = tcv ? tcv->type == type_int : false;
        bool tcv_ptr = tcv ? tcv->type == type_ptr : false;
        char* num = p->tkn;
        // Check if float
        int before = p->chunk->i;
        char t1 = tok(p, false, false, true);
        char* t1tkn = p->tkn;
        int before2 = p->chunk->i;
        char t2 = tok(p, false, false, true);
        //
        bool is_float = false;
        v_u64 uv = 0;
        char value[64];
        //
        if (t1 == tok_dot && t2 == tok_number) {
            // Float number
            is_float = true;
            char *buf = b->char_buf;
            char* deci = p->tkn;
            sprintf(buf, "%s%s.%s", negative ? "-" : "", num, deci);
            double fv = atof(buf);
            v = vgen_float(alc, fv, type_gen_valk(alc, b, "float"));
            if(tcv_float)
                try_convert_number(v, tcv);
        } else if (t1 == tok_id && t1tkn[0] == 'x') {
            // Hex number
            p->chunk->i = before2;
            strcpy(value, t1tkn + 1);
            if(strlen(value) == 0 || !is_valid_hex_number(value)) {
                parse_err(p, -1, "Invalid hex number syntax: '0x%s'", value);
            }
            uv = hex2uint(value);

        } else if (t1 == tok_id && t1tkn[0] == 'c') {
            // Octal number
            p->chunk->i = before2;
            strcpy(value, t1tkn + 1);
            if(strlen(value) == 0 || !is_valid_octal_number(value)) {
                parse_err(p, -1, "Invalid octal number syntax: '0c%s'", value);
            }
            uv = oct2uint(value);

        } else {
            // Decimal number
            p->chunk->i = before;
            strcpy(value, num);
            uv = atoll(num);
        }

        if(!is_float) {
            if(negative && uv > 0x7FFFFFFFFFFFFFFF) {
                parse_err(p, -1, "Number too large: '%s'", value);
            }
            v_i64 iv = ((v_i64)uv) * (negative ? -1 : 1);
            Type* type = tcv_int ? tcv : NULL;
            if(type && !number_fits_type(iv, type)) {
                type = NULL;
            }
            if(!type) {
                if(uv > 0x7FFFFFFFFFFFFFFF) {
                    type = type_gen_valk(alc, b, "uint");
                } else {
                    type = type_gen_valk(alc, b, "int");
                }
            }

            v = vgen_int(alc, iv, type);
        }

    } else if(t == tok_plusplus || t == tok_subsub) {
        bool incr = t == tok_plusplus;
        if(tok_next_is_whitespace(p)) {
            parse_err(p, -1, "Missing value after '%s' (do not use whitespace)", incr ? "++" : "--");
        }
        Value* on = read_value(alc, p, false, 1);
        if(on->rett->type != type_int) {
            parse_err(p, -1, "Invalid value after '%s' (not an integer value)", incr ? "++" : "--");
        }
        if(!value_is_assignable(on)) {
            parse_err(p, -1, "Invalid value after '%s' (non-assign-able value)", incr ? "++" : "--");
        }
        value_is_mutable(on);
        v = vgen_incr(alc, b, on, incr, true);
    }

    if(!v) {
        parse_err(p, -1, "Unknown value: '%s'", tkn);
    }

    ///////////////////////
    // TRAILING CHARS
    ///////////////////////

    if(v->rett && v->rett->type == type_multi) {
        return v;
    }

    t = tok(p, false, false, false);
    while(t == tok_dot || t == tok_bracket_open || t == tok_plusplus || t == tok_subsub) {
        tok(p, false, false, true);
        Type* rett = v->rett;
        if(t == tok_dot) {
            Class *class = rett->class;
            if (!class) {
                parse_err(p, -1, "Unexpected '.'");
            }
            if (rett->nullable) {
                parse_err(p, -1, "You cannot use '.' on a null-able value");
            }

            t = tok(p, false, false, true);
            char* prop_name = p->tkn;
            ClassProp* prop = map_get(class->props, prop_name);
            if(prop) {
                // Property
                value_check_act(prop->act, class->fc, p, "property");
                v = vgen_class_pa(alc, v, prop);
            } else {
                // Check functions
                Func* func = map_get(class->funcs, prop_name);
                if (!func) {
                    parse_err(p, -1, "Class '%s' has no property/function named: '%s'", class->name, prop_name);
                }
                func_mark_used(p->func, func);
                value_check_act(func->act, class->fc, p, "function");
                if(func->is_static) {
                    parse_err(p, -1, "Accessing a static class in a non-static way: '%s.%s'\n", class->name, prop_name);
                }
                v = vgen_func_ptr(alc, func, v);
            }
            t = tok(p, false, false, false);
            continue;
        }
        if(t == tok_bracket_open) {
            v = value_func_call(alc, p, v);
            t = tok(p, false, false, false);
            continue;
        }
        if(t == tok_plusplus || t == tok_subsub) {
            bool incr = t == tok_plusplus;
            if (v->rett->type != type_int) {
                parse_err(p, -1, "Invalid value before '%s' (not an integer value)", incr ? "++" : "--");
            }
            if (!value_is_assignable(v)) {
                parse_err(p, -1, "Invalid value before '%s' (non-assign-able value)", incr ? "++" : "--");
            }
            value_is_mutable(v);
            v = vgen_incr(alc, b, v, incr, false);
            t = tok(p, false, false, false);
            continue;
        }
        break;
    }

    ///////////////////////
    // TRAILING WORDS & OPS
    ///////////////////////

    t = tok(p, true, true, false);

    while (t == tok_at_word && str_is(p->tkn, "@as")) {
        tok(p, true, true, true);

        if (type_is_void(v->rett)) {
            parse_err(p, -1, "Left side of '@as' must return a value");
        }

        Type *type = read_type(p, alc, true);
        v = vgen_cast(alc, v, type);

        t = tok(p, true, true, false);
    }

    if (prio == 0 || prio > 10) {
        while (t == tok_mul || t == tok_div || t == tok_mod) {
            tok(p, true, true, true);
            int op;
            if (t == tok_mul)
                op = op_mul;
            else if (t == tok_div)
                op = op_div;
            else if (t == tok_mod)
                op = op_mod;
            else
                break;

            Type *tcv_prev = p->try_conv;
            Type *tcv = v->rett;
            p->try_conv = tcv;
            Value *right = read_value(alc, p, true, 10);
            p->try_conv = tcv_prev;

            v = value_handle_op(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 20) {
        while (t == tok_plus || t == tok_sub) {
            tok(p, true, true, true);
            int op;
            if (t == tok_plus)
                op = op_add;
            else if (t == tok_sub)
                op = op_sub;
            else
                break;

            Type *tcv_prev = p->try_conv;
            Type *tcv = v->rett;
            p->try_conv = tcv;
            Value *right = read_value(alc, p, true, 20);
            p->try_conv = tcv_prev;

            v = value_handle_op(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 25) {
        while (t == tok_shl || t == tok_shr) {
            tok(p, true, true, true);
            int op;
            if (t == tok_shl)
                op = op_shl;
            else if (t == tok_shr)
                op = op_shr;
            else
                break;

            Type *tcv_prev = p->try_conv;
            Type *tcv = v->rett;
            p->try_conv = tcv;
            Value *right = read_value(alc, p, true, 25);
            p->try_conv = tcv_prev;

            v = value_handle_op(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 30) {
        while (t == tok_bit_and || t == tok_bit_or || t == tok_bit_xor) {
            tok(p, true, true, true);
            int op;
            if(t == tok_bit_and)
                op = op_bit_and;
            else if(t == tok_bit_or)
                op = op_bit_or;
            else if(t == tok_bit_xor)
                op = op_bit_xor;
            else
                break;

            Type *tcv_prev = p->try_conv;
            Type *tcv = v->rett;
            p->try_conv = tcv;
            Value *right = read_value(alc, p, true, 30);
            p->try_conv = tcv_prev;

            v = value_handle_op(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 35) {
        while (t >= tok_eqeq && t <= tok_not_eq) {
            tok(p, true, true, true);
            int op;
            if(t == tok_eqeq)
                op = op_eq;
            else if(t == tok_not_eq)
                op = op_ne;
            else if(t == tok_lte)
                op = op_lte;
            else if(t == tok_gte)
                op = op_gte;
            else if(t == tok_lt)
                op = op_lt;
            else if(t == tok_gt)
                op = op_gt;
            else
                break;

            Type *tcv_prev = p->try_conv;
            Type *tcv = v->rett;
            p->try_conv = tcv;
            Value *right = read_value(alc, p, true, 35);
            p->try_conv = tcv_prev;

            v = value_handle_compare(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 40) {
        while (t == tok_and || t == tok_or) {
            tok(p, true, true, true);

            if (v->rett->type != type_bool) {
                parse_err(p, -1, "Left side must return a bool");
            }

            int op = op_and;
            if (t == tok_or) {
                op = op_or;
            }

            Type *tcv_prev = p->try_conv;
            Type *tcv = v->rett;
            p->try_conv = tcv;
            Value *right = read_value(alc, p, true, 40);
            p->try_conv = tcv_prev;

            if (right->rett->type != type_bool) {
                parse_err(p, -1, "Right side must return a bool");
            }

            v = vgen_and_or(alc, b, v, right, op);

            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 50) {
        // ? ... : ...
        while (t == tok_qmark) {
            tok(p, true, true, true);

            int start = p->chunk->i;

            if (v->rett->type != type_bool) {
                parse_err(p, -1, "Left side of '?' must be of type bool");
            }

            // True
            Scope* scope = p->scope;
            Scope* sv1 = scope_sub_make(alc, sc_default, scope);
            scope_apply_issets(alc, sv1, v->issets);

            p->scope = sv1;
            Value *v1 = read_value(alc, p, true, 0);

            // False
            tok_expect(p, ":", true, true);

            Scope* sv2 = scope_sub_make(alc, sc_default, scope);
            scope_apply_issets(alc, sv2, v->not_issets);
            p->scope = sv2;

            Type *tcv_prev = p->try_conv;
            Type *tcv = v1->rett;
            p->try_conv = tcv;
            Value *v2 = read_value(alc, p, true, 51);
            p->try_conv = tcv_prev;

            //
            p->scope = scope;

            Type *type = type_merge(alc, v1->rett, v2->rett);
            if (!type) {
                char t1[512];
                char t2[512];
                type_to_str(v1->rett, t1);
                type_to_str(v2->rett, t2);
                parse_err(p, start, "Types '%s' and '%s' are not compatible", t1, t2);
            }

            v = vgen_this_or_that(alc, v, v1, v2, type);

            t = tok(p, true, true, false);
        }
    }


    if (prio == 0 || prio > 60) {
        while (t == tok_qqmark) {
            tok(p, true, true, true);
            if(!v->rett->nullable) {
                parse_err(p, -1, "Left side value of '\?\?' can never be 'null'");
            }
            Value *right = read_value(alc, p, true, 60);
            type_check(p, v->rett, right->rett);

            v = vgen_null_alt_value(alc, v, right);

            t = tok(p, true, true, false);
        }
    }

    return v;
}

Value* value_handle_idf(Allocator *alc, Parser* p, Idf *idf) {
    Build *b = p->b;
    Chunk *chunk = p->chunk;

    int type = idf->type;

    if (type == idf_decl) {
        Decl* decl = idf->item;
        return value_make(alc, v_decl, decl, decl->type);
    }
    if (type == idf_decl_overwrite) {
        DeclOverwrite* dov = idf->item;
        return value_make(alc, v_decl_overwrite, dov, dov->type);
    }
    if (type == idf_global) {
        Global* g = idf->item;
        if(g->type->class && p->func) {
            array_push(p->func->used_classes, g->type->class);
        }
        value_check_act(g->act, g->fc, p, "global");
        return value_make(alc, v_global, g, g->type);
    }
    if (type == idf_scope) {
        Scope* sub = idf->item;
        tok_expect(p, ":", false, false);
        char t = tok(p, false, false, true);

        Id id;
        read_id(p, p->tkn, &id);
        Idf *idf_sub = idf_by_id(p, sub, &id, true);
        return value_handle_idf(alc, p, idf_sub);
    }
    if (type == idf_func) {
        Func* func = idf->item;
        func_mark_used(p->func, func);
        value_check_act(func->act, func->fc, p, "function");
        return vgen_func_ptr(alc, func, NULL);
    }
    if (type == idf_class) {
        Class* class = idf->item;
        value_check_act(class->act, class->fc, p, "struct");
        return value_handle_class(alc, p, class);
    }
    if (type == idf_value_alias) {
        ValueAlias* va = idf->item;
        value_check_act(va->act, va->fc, p, "value-alias");
        Chunk ch;
        ch = *p->chunk;
        *p->chunk = *va->chunk;
        Scope *scope = p->scope;
        p->scope = va->scope;
        Value* val = read_value(alc, p, true, 0);
        p->scope = scope;
        *p->chunk = ch;
        return val;
    }
    if (type == idf_macro) {
        Macro* m = idf->item;
        if(!m->is_value) {
            parse_err(p, -1, "You cannot use this macro as a value");
        }
        return macro_read_value(alc, m, p);
    }
    if (type == idf_macro_item) {
        MacroItem* mi = idf->item;
        tok_expect(p, ".", false, false);
        char t = tok(p, false, false, true);
        Idf* sub = map_get(mi->identifiers, p->tkn);
        if(!sub)
            parse_err(p, -1, "Unknown macro item property: '%s'", p->tkn);
        return value_handle_idf(alc, p, sub);
    }
    if (type == idf_value) {
        Value* v = al(alc, sizeof(Value));
        Value* ori = idf->item;
        *v = *ori;
        return v;
    }

    parse_err(p, -1, "Identifier cannot be used as a value: %d", idf->type);
    return NULL;
}

Value *value_func_call(Allocator *alc, Parser* p, Value *on) {
    Type* ont = on->rett;
    if (ont->type != type_func) {
        parse_err(p, -1, "Function call on non-function type");
    }
    
    Build* b = p->b;
    TypeFuncInfo* fi = ont->func_info;
    Array* func_args = fi->args;
    Array* func_default_values = fi->default_values;
    Type *rett = fi->rett;

    if (!func_args) {
        parse_err(p, -1, "Function pointer value is missing function type information (compiler bug)\n");
    }
    if(on->type == v_func_ptr) {
        VFuncPtr* item = on->item;
        array_push(p->func->called_functions, item->func);
    }

    Array* args = array_make(alc, func_args->length + 1);
    int arg_i = 0;
    if(on->type == v_func_ptr) {
        VFuncPtr* fptr = on->item;
        if(fptr->first_arg) {
            arg_i++;
            array_push(args, fptr->first_arg);
        }
    }
    int offset = arg_i;
    bool inf_args = fi->inf_args;
    bool contains_gc_args = false;

    // Read argument values
    if (tok(p, true, true, false) == tok_bracket_close) {
        tok(p, true, true, true);
    } else {
        while (true) {
            Type *arg_type = array_get_index(func_args, arg_i++);
            if (!arg_type && !inf_args) {
                parse_err(p, -1, "Too many arguments");
            }

            Type *tcv_prev = p->try_conv;
            Type *tcv = arg_type;
            p->try_conv = tcv;
            Value *arg = read_value(alc, p, true, 0);
            p->try_conv = tcv_prev;

            if(arg_type)
                arg = try_convert(alc, p, p->scope, arg, arg_type);

            char t = tok(p, true, true, true);
            if (t == tok_at_word && str_is(p->tkn, "@autocast") && arg_type) {
                char* reason;
                if(!type_compat(arg_type, arg->rett, &reason)) {
                    arg = vgen_cast(alc, arg, arg_type);
                }
                t = tok(p, true, true, true);
            }

            if (arg_type)
                type_check(p, arg_type, arg->rett);

            array_push(args, arg);

            if (t == tok_comma)
                continue;
            if (t == tok_bracket_close)
                break;
            parse_err(p, -1, "Unexpected token in function arguments: '%s'\n", p->tkn);
        }
    }

    if(args->length > func_args->length && !inf_args) {
        parse_err(p, -1, "Too many arguments. Expected: %d, Found: %d\n", func_args->length - offset, args->length - offset);
    }

    // Add default values
    if (func_default_values) {
        int index = args->length;
        while (index < func_args->length) {
            Type *arg_type = array_get_index(func_args, index);
            Chunk *default_value_ch = array_get_index(func_default_values, index);
            if(!arg_type || !default_value_ch)
                break;
            Chunk ch;
            ch = *p->chunk;
            *p->chunk = *default_value_ch;
            Scope* scope = p->scope;
            p->scope = default_value_ch->fc->scope;
            //
            Type *tcv_prev = p->try_conv;
            Type *tcv = arg_type;
            p->try_conv = tcv;
            Value *arg = read_value(alc, p, true, 0);
            p->try_conv = tcv_prev;
            //
            arg = try_convert(alc, p, scope, arg, arg_type);
            type_check(p, arg_type, arg->rett);
            //
            p->scope = scope;
            *p->chunk = ch;

            array_push(args, arg);
            index++;
        }
    }

    if(args->length < func_args->length) {
        parse_err(p, -1, "Missing arguments. Expected: %d, Found: %d\n", func_args->length - offset, args->length - offset);
    }

    if(on->rett->func_info->will_exit) {
        p->scope->did_return = true;
    }

    Value* retv = vgen_func_call(alc, p, on, args);

    if (!p->reading_coro_fcall) {
        TypeFuncInfo *fi = ont->func_info;
        if (fi->err_names && fi->err_names->length > 0) {
            retv = read_err_handler(alc, p, retv, fi);
        }
    }

    return retv;
}

Value* value_handle_class(Allocator *alc, Parser* p, Class* class) {
    Build* b = p->b;
    Chunk* ch = p->chunk;

    if(class->is_generic_base) {
        tok_expect(p, "[", false, false);
        int count = class->generic_names->length;
        Array* generic_types = array_make(alc, count + 1);
        for (int i = 0; i < count; i++) {
            Type* type = read_type(p, alc, false);
            array_push(generic_types, type);
            if(i + 1 < count) {
                tok_expect(p, ",", true, false);
            } else {
                tok_expect(p, "]", true, false);
                break;
            }
        }
        class = get_generic_class(p, class, generic_types);
    }
    if (p->func)
        array_push(p->func->used_classes, class);

    char t = tok(p, false, false, false);
    if(t == tok_dot) {
        // Static functions
        tok(p, false, false, true);
        t = tok(p, false, false, true);
        char* name = p->tkn;
        Func* func = map_get(class->funcs, name);
        func_mark_used(p->func, func);
        if(!func) {
            parse_err(p, -1, "Class '%s' has no function named: '%s'", class->name, name);
        }
        return vgen_func_ptr(alc, func, NULL);
    }
    // Class init
    t = tok(p, true, false, false);
    if(t != tok_curly_open) {
        return NULL;
    }
    t = tok(p, true, false, true);
    int propc = class->props->values->length;
    Map* values = map_make(alc);
    // Read values
    t = tok(p, true, true, true);
    while(t != tok_curly_close) {
        char* name = p->tkn;
        ClassProp* prop = map_get(class->props, name);
        if(!prop) {
            parse_err(p, -1, "Class '%s' has no property named: '%s'", class->name, name);
        }
        if(map_contains(values, name)) {
            parse_err(p, -1, "Setting same property twice: '%s'", name);
        }
        tok_expect(p, ":", true, false);

        Type *tcv_prev = p->try_conv;
        Type *tcv = prop->type;
        p->try_conv = tcv;
        Value *val = read_value(alc, p, true, 0);
        p->try_conv = tcv_prev;

        val = try_convert(alc, p, p->scope, val, prop->type);

        type_check(p, prop->type, val->rett);
        map_set_force_new(values, name, val);
        t = tok(p, true, true, true);
        if(t == tok_comma) {
            t = tok(p, true, true, true);
        }
    }
    // Default values
    Array* props = class->props->values;
    for(int i = 0; i < propc; i++) {
        ClassProp* prop = array_get_index(props, i);
        char* name = array_get_index(class->props->keys, i);
        if(!map_contains(values, name)) {
            if(prop->skip_default_value)
                continue;
            if(prop->type->type == type_static_array)
                continue;
            if(!prop->chunk_value) {
                parse_err(p, -1, "Missing property value for: '%s'", name);
            }

            Value* val = read_value_from_other_chunk(p, alc, prop->chunk_value, class->scope, prop->type);

            map_set_force_new(values, name, val);
        }
    }

    VClassInit* ci = al(alc, sizeof(VClassInit));
    ci->prop_values = values;

    if (class->type == ct_class) {
        ci->item = vgen_call_pool_alloc(alc, p, b, class);
        p->func->can_create_objects = true;
    } else {
        ci->item = vgen_call_alloc(alc, p, class->size, class);
    }

    Value* init = value_make(alc, v_class_init, ci, type_gen_class(alc, class));
    buffer_values(alc, p, values->values, false);

    if(class->type == ct_class && p->scope->ast) {
        p->scope->gc_check = true;
    }

    return init;
}

Value* value_handle_ptrv(Allocator *alc, Parser* p) {
    tok_expect(p, "(", false, false);
    // On
    Value *on = read_value(alc, p, true, 0);
    if (!on->rett->is_pointer) {
        parse_err(p, -1, "First argument of '@ptrv' must be a value of type 'ptr'");
    }
    // Type
    tok_expect(p, ",", true, true);
    Type *type = read_type(p, alc, true);
    if (!type || type->type == type_void) {
        parse_err(p, -1, "You cannot use 'void' type in @ptrv");
    }
    // Index
    char t = tok(p, true, true, false);
    Value *index = NULL;
    if(t == tok_comma) {
        tok(p, true, true, true);
        index = read_value(alc, p, true, 0);
        if (index->rett->type != type_int) {
            parse_err(p, -1, "@ptrv index must be of type integer");
        }
        Build* b = p->b;
        if (index->rett->size < b->ptr_size && index->rett->is_signed == false) {
            // Increase index type size
            index = vgen_cast(alc, index, type_gen_number(alc, b, b->ptr_size, false, true));
        }
    }
    tok_expect(p, ")", true, true);

    return vgen_ptrv(alc, p->b, on, type, index);
}

Value* value_handle_op(Allocator *alc, Parser* p, Value *left, Value* right, int op) {
    // Check type
    Build* b = p->b;
    Type* lt = left->rett;
    Type* rt = right->rett;

    if (op == op_add) {
        Class* str_class = get_valk_class(b, "type", "String");
        // Try make both string if neither are nullable
        if (!lt->nullable && !rt->nullable) {
            if (lt->class == str_class || rt->class == str_class) {
                if (lt->class != str_class) {
                    left = try_convert(alc, p, p->scope, left, rt);
                    lt = left->rett;
                } else {
                    right = try_convert(alc, p, p->scope, right, lt);
                    rt = right->rett;
                }
            }
        }
    }

    // _add
    if (op == op_add && !lt->nullable) {
        Func *add = lt->class ? map_get(lt->class->funcs, "_add") : NULL;
        if (add && add->is_static == false && add->arg_types->length == 2) {
            func_mark_used(p->func, add);
            Type *arg_type = array_get_index(add->arg_types, 1);
            right = try_convert(alc, p, p->scope, right, arg_type);
            Array *args = array_make(alc, 2);
            array_push(args, left);
            array_push(args, right);
            type_check(p, arg_type, right->rett);
            Value *on = vgen_func_ptr(alc, add, NULL);
            return vgen_func_call(alc, p, on, args);
        }
    }

    //
    if(!lt->class || !lt->class->allow_math) {
        char t1[512];
        char t2[512];
        type_to_str(left->rett, t1);
        type_to_str(right->rett, t2);
        parse_err(p, -1, "You cannot use the '%s' operator between these types: %s <-> %s", op_to_str(op), t1, t2);
    }
    bool is_ptr = false;
    bool is_signed = lt->is_signed || rt->is_signed;
    if(lt->type == type_ptr) {
        is_ptr = true;
        left = vgen_cast(alc, left, type_gen_valk(alc, b, is_signed ? "int" : "uint"));
    }
    if(rt->type == type_ptr) {
        is_ptr = true;
        right = vgen_cast(alc, right, type_gen_valk(alc, b, is_signed ? "int" : "uint"));
    }

    bool has_number_suggestion = p->try_conv && (p->try_conv->type == type_int || p->try_conv->type == type_float);

if(!has_number_suggestion) {
    if (left->type == v_number && right->type != v_number) {
        try_convert_number(left, right->rett);
    } else if (right->type == v_number && left->type != v_number) {
        try_convert_number(right, left->rett);
    } else if (left->type == v_number && right->type == v_number) {
        // Pre-calculate
        bool is_float = left->rett->type == type_float || right->rett->type == type_float;
        Value *combo;
        if (is_float) {
            // combo = pre_calc_float(alc, fc->b, left, right, op);
        } else {
            combo = pre_calc_int(alc, b, left, right, op);
        }
        if (combo)
            return combo;
    }
}

    // Try match types
    match_value_types(alc, b, &left, &right);

    Type* t1 = left->rett;
    Type* t2 = right->rett;
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)){
        char t1b[256];
        char t2b[256];
        parse_err(p, -1, "Operator values are not compatible: %s %s %s", type_to_str(lt, t1b), op_to_str(op), type_to_str(rt, t2b));
    }

    Value* v = vgen_op(alc, op, left, right, t1);

    if(is_ptr) {
        v = vgen_cast(alc, v, type_gen_valk(alc, b, "ptr"));
    }

    return v;
}

Value* pre_calc_float(Allocator* alc, Build* b, Value* n1, Value* n2, int op) {
    VNumber* vn1 = n1->item;
    VNumber* vn2 = n2->item;
    double v1 = n1->rett->type == type_int ? (double) vn1->value_int : vn1->value_float;
    double v2 = n2->rett->type == type_int ? (double) vn2->value_int : vn2->value_float;
    if(op == op_add) {
        v1 = v1 + v2;
    } else if(op == op_sub) {
        v1 = v1 - v2;
    } else if(op == op_mul) {
        v1 = v1 * v2;
    } else if(op == op_div) {
        v1 = v1 / v2;
    } else {
        return NULL;
    }
    int size = max_num(n1->rett->size, n2->rett->size);
    return vgen_float(alc, v1, type_gen_number(alc, b, size, true, true));
}
Value* pre_calc_int(Allocator* alc, Build* b, Value* n1, Value* n2, int op) {
    VNumber* vn1 = n1->item;
    VNumber* vn2 = n2->item;
    int v1 = vn1->value_int;
    int v2 = vn2->value_int;
    if(op == op_add) {
        v1 = v1 + v2;
    } else if(op == op_sub) {
        v1 = v1 - v2;
    } else if(op == op_mul) {
        v1 = v1 * v2;
    } else if(op == op_div) {
        v1 = v1 / v2;
    } else {
        return NULL;
    }
    int size = max_num(n1->rett->size, n2->rett->size);
    return vgen_int(alc, v1, type_gen_number(alc, b, size, false, n1->rett->is_signed || n2->rett->is_signed));
}

Value* value_handle_compare(Allocator *alc, Parser* p, Value *left, Value* right, int op) {
    Build* b = p->b;
    Type* lt = left->rett;
    Type* rt = right->rett;

    if (!lt->nullable) {
        if (op == op_eq || op == op_ne) {
            Func *eq = lt->class ? map_get(lt->class->funcs, "_eq") : NULL;
            if (eq && eq->is_static == false && eq->arg_types->length == 2) {
                func_mark_used(p->func, eq);
                Type *arg_type = array_get_index(eq->arg_types, 1);
                Array *args = array_make(alc, 2);
                array_push(args, left);
                array_push(args, right);
                type_check(p, arg_type, right->rett);
                Value *on = vgen_func_ptr(alc, eq, NULL);
                Value *result = vgen_func_call(alc, p, on, args);
                if(op == op_ne) {
                    result = value_make(alc, v_not, result, result->rett);
                }
                return result;
            }
        }
    }

    bool is_ptr = lt->is_pointer || rt->is_pointer;
    if(is_ptr) {
        // Pointers
        if(lt->type == type_int) {
            left = vgen_cast(alc, left, type_gen_valk(alc, b, "ptr"));
        }
        if(rt->type == type_int) {
            right = vgen_cast(alc, right, type_gen_valk(alc, b, "ptr"));
        }
    } else {
        // Numbers
        bool is_bool = lt->type == type_bool || rt->type == type_bool;
        if (!is_bool) {
            if (left->type == v_number && right->type != v_number) {
                try_convert_number(left, rt);
            } else if (right->type == v_number && left->type != v_number) {
                try_convert_number(right, lt);
            }
            match_value_types(alc, b, &left, &right);
        }
    }

    Type *t1 = left->rett;
    Type* t2 = right->rett;
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)){
        char t1b[256];
        char t2b[256];
        parse_err(p, -1, "Operator values are not compatible: %s <-> %s", type_to_str(lt, t1b), type_to_str(rt, t2b));
    }

    return vgen_comp(alc, op, left, right, type_gen_valk(alc, b, "bool"));
}

bool value_is_assignable(Value *v) {
    int vt = v->type;
    if(vt == v_ir_cached) {
        VIRCached* vc = v->item;
        return value_is_assignable(vc->value);
    }
    return vt == v_decl || vt == v_decl_overwrite || vt == v_class_pa || vt == v_ptrv || vt == v_global; 
}

void match_value_types(Allocator* alc, Build* b, Value** v1_, Value** v2_) {
    //
    Value* v1 = *v1_;
    Value* v2 = *v2_;
    Type* t1 = v1->rett;
    Type* t2 = v2->rett;
    bool is_signed = t1->is_signed || t2->is_signed;
    bool is_float = t1->type == type_float || t2->type == type_float;
    int size = is_float ? max_num(t1->size, t2->size) : (max_num(t1->is_signed != is_signed ? t1->size * 2 : t1->size, t2->is_signed != is_signed ? t2->size * 2 : t2->size));
    Type* type = type_gen_number(alc, b, size, is_float, is_signed);
    if(!type)
        return;
    if(t1->type != type->type || t1->is_signed != type->is_signed || t1->size != type->size) {
        *v1_ = vgen_cast(alc, v1, type);
    }
    if(t2->type != type->type || t2->is_signed != type->is_signed || t2->size != type->size) {
        *v2_ = vgen_cast(alc, v2, type);
    }
}

void value_is_mutable(Value* v) {
    if (v->type == v_decl) {
        Decl *decl = v->item;
        decl->is_mut = true;
    }
    if (v->type == v_decl_overwrite) {
        DeclOverwrite* dov = v->item;
        dov->decl->is_mut = true;
    }
}

Value* try_convert(Allocator* alc, Parser* p, Scope* scope, Value* val, Type* type) {
    Build* b = p->b;

    if(!val->rett || !type)
        return val;

    Class* str_class = get_valk_class(b, "type", "String");
    Class* from_class = val->rett->class;
    if(type->class == str_class && from_class != str_class && from_class != NULL && !val->rett->nullable) {
        // To string
        Func *to_str = map_get(from_class->funcs, "_string");
        if (to_str) {
            func_mark_used(p->func, to_str);
            Array *args = array_make(alc, 2);
            array_push(args, val);
            Value *on = vgen_func_ptr(alc, to_str, NULL);
            return vgen_func_call(alc, p, on, args);
        }
    }


    // If number literal, change the type
    if(val->type == v_number){
        if (type->type == type_int || type->type == type_float) {
            try_convert_number(val, type);
        } else if (val->rett->type == type_int && type->type == type_ptr) {
            val = vgen_cast(alc, val, type);
        }
    }

    // If number value, cast if appropriate
    if(val->type != v_number){
        if (val->rett->type == type_int && type->type == type_float) {
            // We can always convert to float
            val->rett = type_clone(alc, type);
        } else if (val->rett->type == type_float && type->type == type_float) {
            // f32 -> f64
            if(val->rett->size < type->size) {
                val = vgen_cast(alc, val, type);
            }
        } else if (val->rett->type == type_int && type->type == type_int) {
            if(val->rett->is_signed && !type->is_signed) {
                // Not possible
            } else {
                int from_size = val->rett->size;
                int to_size = type->size;
                if(type->is_signed && !val->rett->is_signed) {
                    from_size *= 2;
                }
                if (from_size <= to_size) {
                    val = vgen_cast(alc, val, type);
                }
            }
        }
    }

    // If to ptr, just change value return type to ptr
    Class* ptr = get_valk_class(b, "type", "ptr");
    if (val->rett->class != ptr && type->class == ptr) {
        bool nullable = val->rett->nullable;
        val->rett = type_gen_class(alc, ptr);
        val->rett->nullable = nullable;
    }
    return val;
}

bool try_convert_number(Value* val, Type* to_type) {
    int tto = to_type->type;
    if(val->type != v_number || (tto != type_int && tto != type_float))
        return false;

    VNumber *number = val->item;
    if (tto == type_int && val->rett->type != type_float) {
        if(number_fits_type(number->value_int, to_type)) {
            val->rett = to_type;
            return true;
        }
    } else if (tto == type_float) {
        if(val->rett->type == type_int) {
            // int -> float
            number->value_float = (double) number->value_int;
            val->rett = to_type;
            return true;
        }
    }
    return false;
}

bool value_needs_gc_buffer(Value* val) {
    if(type_is_gc(val->rett)) {
        if(val->type == v_decl || val->type == v_decl_overwrite || val->type == v_string || val->type == v_null) {
            return false;
        }
        return true;
    }
    return false;
}

void value_check_act(int act, Fc* fc, Parser* p, char* name) {
    if (act == act_public)
        return;
    Fc* cfc = NULL;
    Parser* sp = p;
    while(sp && !cfc) {
        cfc = sp->chunk->fc;
        sp = sp->prev;
    }
    if(!cfc)
        return;
    if(cfc->ignore_access_types)
        return;
    if(act == act_private_fc) {
        if(fc != cfc) {
            parse_err(p, -1, "You cannot access this %s from this file", name);
        }
    } else if(act == act_private_nsc) {
        if(fc->nsc != cfc->nsc) {
            parse_err(p, -1, "You cannot access this %s from this namespace", name);
        }
    } else if(act == act_private_pkc) {
        if(fc->nsc->pkc != cfc->nsc->pkc) {
            parse_err(p, -1, "You cannot access this %s from this package", name);
        }
    }
}

Value *read_err_value(Allocator* alc, Parser *p, Array *err_names, Array *err_values) {
    tok_expect(p, "!", true, true);
    char t = tok(p, false, false, true);
    if (t != tok_id) {
        parse_err(p, -1, "Invalid error name syntax");
    }

    char *err_name = p->tkn;
    int index = array_find(err_names, err_name, arr_find_str);
    if (index == -1) {
        Str *buf = p->b->str_buf;
        str_clear(buf);
        loop(err_names, i) {
            char *name = array_get_index(err_names, i);
            if (i > 0) {
                str_append_chars(buf, ", ");
            }
            str_append_chars(buf, "!");
            str_append_chars(buf, name);
        }
        char *options = str_to_chars(alc, buf);
        parse_err(p, -1, "Error can never be '%s', possible errors: %s", err_name, options);
    }
    unsigned int errv = array_get_index_u32(err_values, index);
    return vgen_int(alc, (v_i64)errv, type_gen_number(alc, p->b, 4, false, false));
}

Value *read_err_handler(Allocator* alc, Parser *p, Value* on, TypeFuncInfo *fi) {

    Type* frett = fi->rett;
    bool void_rett = type_is_void(frett);
    bool ignore = false;
    char t = tok(p, true, false, true);
    char *name = NULL;

    // on = vgen_ir_cached(alc, on);

    ErrorHandler* errh = al(alc, sizeof(ErrorHandler));
    errh->type = 0;
    errh->on = on;
    errh->err_scope = NULL;
    errh->err_value = NULL;
    errh->err_decl = NULL;
    errh->phi_s = NULL;

    Value *retv = value_make(alc, v_errh, errh, on->rett);
    retv->extra_values = on->extra_values;

    if(type_is_void(frett) && str_is(p->tkn, "_")) {
        return retv;
    }

    if (t == tok_id) {
        name = p->tkn;
        t = tok(p, true, false, true);
    }
    if (!str_is(p->tkn, "!") && !str_is(p->tkn, "?")) {
        parse_err(p, -1, "Expected one the following error handler tokens: ! or ? (Found: '%s')", p->tkn);
    }

    if (t == tok_not) {

        t = tok(p, true, true, false);

        Chunk *chunk_end = NULL;
        int scope_end_i = -1;
        if (t == tok_curly_open) {
            tok(p, true, true, true);
            scope_end_i = p->scope_end_i;
        }
        bool single = scope_end_i == -1;

        Scope *scope = p->scope;
        Scope *err_scope = scope_sub_make(alc, sc_default, scope);
        errh->err_scope = err_scope;

        if (name) {
            // Error identifier
            Decl *decl = decl_make(alc, p->func, name, type_gen_error(alc, fi->err_names, fi->err_values), false);
            Idf *idf = idf_make(alc, idf_error, decl);
            scope_set_idf(err_scope, name, idf, p);
            scope_add_decl(alc, err_scope, decl);
            errh->err_decl = decl;
        }

        p->scope = err_scope;
        read_ast(p, single);
        p->scope = scope;
        if (!single)
            p->chunk->i = scope_end_i;

        if (!void_rett && !err_scope->did_return) {
            parse_err(p, -1, "Expected scope to contain one of the following tokens: return, throw, break, continue");
        }

    } else if (t == tok_qmark) {
        if (void_rett) {
            parse_err(p, -1, "You cannot provide an alternative value for a function that doesnt return a value");
        }

        Scope *scope = p->scope;
        Scope *sub_scope = scope_sub_make(alc, sc_default, p->scope);
        p->scope = sub_scope;

        if (name) {
            // Error identifier
            Decl *decl = decl_make(alc, p->func, name, type_gen_error(alc, fi->err_names, fi->err_values), false);
            Idf *idf = idf_make(alc, idf_error, decl);
            scope_set_idf(sub_scope, name, idf, p);
            scope_add_decl(alc, sub_scope, decl);
            errh->err_decl = decl;
        }

        Type *tcv_prev = p->try_conv;
        Type *tcv = on->rett;
        p->try_conv = tcv;
        Value *errv = read_value(alc, p, false, 0);
        p->try_conv = tcv_prev;
        // errv = try_convert(alc, p, sub_scope, errv, frett);

        p->scope = scope;

        errh->err_value = errv;

        Array* left = all_values(alc, on);
        Array* right = all_values(alc, errv);

        Array* phi_s = array_make(alc, 1);
        loop(left, i) {
            if(i == left->length || i == right->length)
                break;
            Value* l = array_get_index(left, i);
            Value* r = array_get_index(right, i);

            // Mix nullable
            if(r->rett->nullable) {
                Type *t = type_clone(alc, l->rett);
                t->nullable = true;
                l->rett = t;
            }

            r = try_convert(alc, p, scope, r, l->rett);
            type_check(p, l->rett, r->rett);

            r->rett = l->rett;
            array_push(phi_s, vgen_phi(alc, l, r));
        }

        errh->phi_s = phi_s;

        retv = vgen_this_but_return_that(alc, retv, array_get_index(phi_s, 0));
        if(phi_s->length > 1) {
            retv->extra_values = array_make(alc, phi_s->length - 1);
            loop(phi_s, i) {
                if(i == 0)
                    continue;
                array_push(retv->extra_values, array_get_index(phi_s, i));
            }
        }
    }

    return retv;
}

void value_enable_cached(VIRCached* v) {
    v->used = true;
}

Array* all_values(Allocator*alc, Value* v) {
    Array* res = array_make(alc, 2);
    array_push(res, v);
    if(v->extra_values) {
        loop(v->extra_values, i) {
            Value* ex = array_get_index(v->extra_values, i);
            array_push(res, ex);
        }
    }
    return res;
}
