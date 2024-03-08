
#include "../all.h"

void stage_ast(Fc *fc);

void stage_4_ast(Fc *fc) {
    if (fc->is_header)
        return;

    Build *b = fc->b;

    if (b->verbose > 2)
        printf("Stage 4 | Parse AST: %s\n", fc->path);

    fc->alc_ast = b->alc_ast;

    if(fc->contains_main_func)
        return;

    usize start = microtime();
    stage_ast(fc);
    b->time_parse += microtime() - start;

    stage_4_ir(fc);
}

void stage_4_ast_main(Fc *fc) {

    Build *b = fc->b;

    usize start = microtime();
    stage_ast(fc);
    b->time_parse += microtime() - start;

    stage_4_ir(fc);
}

void stage_ast(Fc *fc) {
    Array *funcs = fc->funcs;
    for (int i = 0; i < funcs->length; i++) {
        Func *func = array_get_index(funcs, i);
        *fc->chunk_parse = *func->chunk_body;
        read_ast(fc, func->scope, false);
    }
}

void read_ast(Fc *fc, Scope *scope, bool single_line) {
    Allocator *alc = fc->alc_ast;
    Build *b = fc->b;
    Chunk *chunk = fc->chunk_parse;

    if (!scope->ast)
        scope->ast = array_make(alc, 50);

    bool first = false;

    while (true) {

        if (single_line && first)
            return;

        char *tkn = tok(fc, true, true, true);
        int t = chunk->token;

        if (tkn[0] == ';')
            continue;
        if (t == tok_scope_close)
            break;

        first = true;
        //

        if (t == tok_id) {
            if (str_is(tkn, "let")){
                char* name = tok(fc, true, false, true);
                if(!is_valid_varname(tkn)) {
                    sprintf(b->char_buf, "Invalid variable name: '%s'", name);
                    parse_err(fc->chunk_parse, b->char_buf);
                }
                char* tkn = tok(fc, true, true, true);
                Type* type = NULL;
                if(str_is(tkn, ":")) {
                    type = read_type(fc, alc, scope, true);
                    tkn = tok(fc, true, true, true);
                }
                if(!str_is(tkn, "=")) {
                    sprintf(b->char_buf, "Expected '=' here, found: '%s'", tkn);
                    parse_err(fc->chunk_parse, b->char_buf);
                }

                Value* val = read_value(alc, fc, scope, true, 0);
                if(type) {
                    val = try_convert(alc, b, val, type);
                    type_check(fc->chunk_parse, type, val->rett);
                } else {
                    type = val->rett;
                }

                Decl* decl = decl_make(alc, type, false);
                Idf *idf = idf_make(b->alc, idf_decl, decl);
                scope_set_idf(scope, name, idf, fc);

                array_push(scope->ast, tgen_declare(alc, scope, decl, val));
                continue;
            }
            if (str_is(tkn, "if")){
                token_if(alc, fc, scope);
                continue;
            }
            if (str_is(tkn, "while")){
                token_while(alc, fc, scope);
                continue;
            }
            if (str_is(tkn, "break") || str_is(tkn, "continue")){
                if(!scope->loop_scope) {
                    sprintf(fc->b->char_buf, "Using 'break' without being inside a loop");
                    parse_err(fc->chunk_parse, fc->b->char_buf);
                }
                array_push(scope->ast, token_make(alc, str_is(tkn, "break") ? t_break : t_continue, scope->loop_scope));
                scope->did_return = true;
                if(!scope->chunk_end) {
                    sprintf(fc->b->char_buf, "Missing scope end position (compiler bug)");
                    parse_err(fc->chunk_parse, fc->b->char_buf);
                }
                *fc->chunk_parse = *scope->chunk_end;
                break;
            }
            if (str_is(tkn, "return")){
                Value* val = NULL;
                if(scope->rett && !type_is_void(scope->rett)) {
                    val = read_value(alc, fc, scope, false, 0);
                    val = try_convert(alc, b, val, scope->rett);
                    type_check(fc->chunk_parse, scope->rett, val->rett);
                } else {
                    char* tkn = tok(fc, true, false, true);
                    if(tkn[0] != 0 && !str_is(tkn, "}")) {
                        sprintf(fc->b->char_buf, "Return statement should not return a value if the function has a 'void' return type");
                        parse_err(fc->chunk_parse, fc->b->char_buf);
                    }
                }
                array_push(scope->ast, tgen_return(alc, val));
                scope->did_return = true;
                if(!scope->chunk_end) {
                    sprintf(fc->b->char_buf, "Missing scope end position (compiler bug)");
                    parse_err(fc->chunk_parse, fc->b->char_buf);
                }
                *fc->chunk_parse = *scope->chunk_end;
                break;
            }
            if (str_is(tkn, "throw")){
                char* name = tok(fc, true, false, true);
                if(!is_valid_varname(tkn)) {
                    sprintf(b->char_buf, "Invalid error name: '%s'", name);
                    parse_err(fc->chunk_parse, b->char_buf);
                }
                Scope* fscope = scope_get_func(scope, true, fc);
                FuncError* err = NULL;
                if (fscope->func->errors) {
                    err = map_get(fscope->func->errors, name);
                }
                if(!err) {
                    sprintf(b->char_buf, "Function has no error defined named: '%s'", name);
                    parse_err(fc->chunk_parse, b->char_buf);
                }

                array_push(scope->ast, tgen_throw(alc, b, fc, err, name));
                scope->did_return = true;
                continue;
            }
        }
        if (t == tok_at_word) {
            if (str_is(tkn, "@cache_value")){
                tok_expect(fc, "(", false, false);
                Value* v = read_value(alc, fc, scope, true, 0);
                tok_expect(fc, ")", true, true);
                tok_expect(fc, "as", true, false);
                char* name = tok(fc, true, false, true);
                if(!is_valid_varname(tkn)) {
                    sprintf(b->char_buf, "Invalid variable name: '%s'", name);
                    parse_err(fc->chunk_parse, b->char_buf);
                }
                Value* vc = vgen_ir_cached(alc, v);
                Idf* idf = idf_make(alc, idf_cached_value, vc);
                scope_set_idf(scope, name, idf, fc);
                array_push(scope->ast, token_make(alc, t_statement, vc));
                continue;
            }
            if (str_is(tkn, "@snippet")){
                tok_expect(fc, "(", false, false);
                char* name = tok(fc, true, false, true);

                Id id;
                Idf* idf = idf_by_id(fc, scope, read_id(fc, name, &id), true);

                if(idf->type != idf_snippet) {
                    sprintf(b->char_buf, "Invalid snippet name: '%s'", name);
                    parse_err(fc->chunk_parse, b->char_buf);
                }
                Snippet* snip = idf->item;
                Array *args = snip->args;
                Map *idfs = map_make(alc);
                for(int i = 0; i < args->length; i++) {
                    tok_expect(fc, ",", true, true);
                    SnipArg* arg = array_get_index(args, i);
                    if(arg->type == snip_value) {
                        Value* v = read_value(alc, fc, scope, true, 0);
                        Idf* idf = idf_make(alc, idf_value, v);
                        map_set(idfs, arg->name, idf);
                    } else {
                        die("TODO: snippet pass types");
                    }
                }
                tok_expect(fc, ")", true, true);

                Scope* sub = scope_sub_make(alc, sc_default, scope, NULL);
                sub->prio_idf_scope = snip->fc_scope;
                sub->identifiers = idfs;

                Chunk ch;
                ch = *fc->chunk_parse;
                *fc->chunk_parse = *snip->chunk;
                read_ast(fc, sub, false);
                *fc->chunk_parse = ch;
                continue;
            }
        }

        tok_back(fc);

        Value* left = read_value(alc, fc, scope, true, 0);

        tok_skip_whitespace(fc);
        t = tok_id_next(fc);
        if((t == tok_op1 || t == tok_op2)) {
            tkn = tok(fc, true, true, true);
            if(str_in(tkn, ",=,+=,-=,*=,/=,")) {
                if (!value_is_assignable(left)) {
                    parse_err(chunk, "Cannot assign to left side");
                }
                value_is_mutable(left);

                Value *right = read_value(alc, fc, scope, true, 0);
                if(type_is_void(right->rett)) {
                    parse_err(chunk, "Trying to assign a void value");
                }

                int op = op_eq;
                if(str_is(tkn, "=")){
                } else if(str_is(tkn, "+=")){
                    op = op_add;
                } else if(str_is(tkn, "-=")){
                    op = op_sub;
                } else if(str_is(tkn, "*=")){
                    op = op_mul;
                } else if(str_is(tkn, "/=")){
                    op = op_div;
                }
                if(op != op_eq) {
                    right = value_handle_op(alc, fc, scope, left, right, op);
                }

                right = try_convert(alc, b, right, left->rett);
                type_check(chunk, left->rett, right->rett);

                array_push(scope->ast, tgen_assign(alc, left, right));
                continue;
            }
            tok_back(fc);
        }

        //
        array_push(scope->ast, token_make(alc, t_statement, left));
    }

    if(scope->must_return && !scope->did_return) {
        sprintf(b->char_buf, "Missing return statement");
        parse_err(fc->chunk_parse, b->char_buf);
    }
}
