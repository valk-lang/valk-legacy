
#include "../all.h"

Value* value_handle_idf(Fc *fc, Scope *scope, Idf *idf);
Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on);


Value* read_value(Fc* fc, Scope* scope, bool allow_newline, int prio) {
    Allocator *alc = fc->alc_ast;
    Build *b = fc->b;
    Chunk *chunk = fc->chunk_parse;

    char *tkn = tok(fc, true, true, true);
    int t = chunk->token;

    Value* v = NULL;

    if (t == tok_id) {
        Idf *idf = read_idf(fc, scope, tkn, true);
        v = value_handle_idf(fc, scope, idf);
    }

    if(!v) {
        sprintf(b->char_buf, "Unknown value: '%s'", tkn);
        parse_err(chunk, b->char_buf);
    }

    t = tok_id_next(fc);
    while(t == tok_char || t == tok_scope_open) {
        Type* rett = v->rett;
        char ch = tok_read_byte(fc, t == tok_scope_open ? (sizeof(int) + 1) : 1);
        if(ch == '.') {
            tok(fc, false, false, true);

            Class *class = rett->class;
            if (!class) {
                parse_err(chunk, "Unexpected '.'");
            }

            char* prop_name = tok(fc, false, false, true);
            ClassProp* prop = map_get(class->props, prop_name);
            if(prop) {
                // Property
                die("TODO: vgen-class-pa");
            } else {
                // Check functions
                Func* func = map_get(class->funcs, prop_name);
                if (!func) {
                    sprintf(b->char_buf, "Unknown class property/function: '%s'", tkn);
                    parse_err(chunk, b->char_buf);
                }
                die("TODO: vgen-class-func");
            }
            t = tok_id_next(fc);
            continue;
        }
        if(ch == '(') {
            tok(fc, false, false, true);
            v = value_func_call(alc, fc, scope, v);
            t = tok_id_next(fc);
            continue;
        }
        break;
    }

    return v;
}

Value* value_handle_idf(Fc *fc, Scope *scope, Idf *idf) {
    Build *b = fc->b;
    Allocator *alc = fc->alc_ast;
    Chunk *chunk = fc->chunk_parse;

    int type = idf->type;

    if (type == idf_scope) {
        Scope* sub = idf->item;
        tok_expect(fc, ".", false, false);
        char *tkn = tok(fc, false, false, true);
        Idf *idf_sub = read_idf(fc, sub, tkn, true);
        return value_handle_idf(fc, scope, idf_sub);
    }
    if (type == idf_func) {
        Func* func = idf->item;
        return vgen_func_ptr(alc, func, NULL);
    }

    sprintf(b->char_buf, "This identifier cannot be used inside a function. (identifier-type:%d)", idf->type);
    parse_err(chunk, b->char_buf);
    return NULL;
}

Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on) {
    Type* ont = on->rett;
    if (ont->type != type_func) {
        parse_err(fc->chunk_parse, "Function call on non-function type");
    }
    
    Build* b = fc->b;
    Array* arg_types = ont->func_args;
    Type *rett = ont->func_rett;

    if (!arg_types || !rett) {
        sprintf(b->char_buf, "Function pointer value is missing function type information (compiler bug)\n");
        parse_err(fc->chunk_parse, b->char_buf);
    }

    Array* args = array_make(alc, arg_types->length + 1);
    int arg_i = 0;
    if(on->type == v_func_ptr) {
        VFuncPtr* fptr = on->item;
        if(fptr->first_arg) {
            arg_i++;
            array_push(args, fptr->first_arg);
        }
    }
    int offset = arg_i;

    while(arg_i < arg_types->length) {
        Type* arg_type = array_get_index(arg_types, arg_i++);
        Value* arg = read_value(fc, scope, true, 0);
        type_check(fc->chunk_parse, arg_type, arg->rett);
        array_push(args, arg);
        char* tkn = tok(fc, true, true, true);
        if(str_is(tkn, ","))
            continue;
        if(str_is(tkn, ")"))
            break;
        sprintf(b->char_buf, "Unexpected token in function arguments: '%s'\n", tkn);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    if(args->length > arg_types->length) {
        sprintf(b->char_buf, "Too many arguments. Expected: %d, Found: %d\n", arg_types->length - offset, args->length - offset);
        parse_err(fc->chunk_parse, b->char_buf);
    }
    if(args->length < arg_types->length) {
        sprintf(b->char_buf, "Missing arguments. Expected: %d, Found: %d\n", arg_types->length - offset, args->length - offset);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    return vgen_func_call(alc, on, args);
}

bool value_is_assignable(Value *v) {
    return v->type == v_decl || v->type == v_class_pa || v->type == v_ptrv || v->type == v_global;
}
