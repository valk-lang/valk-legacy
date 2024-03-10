
#include "../all.h"

Token *token_make(Allocator *alc, int type, void *item) {
    Token *t = al(alc, sizeof(Token));
    t->type = type;
    t->item = item;
    return t;
}

void token_if(Allocator* alc, Fc* fc, Scope* scope) {
    Build* b = fc->b;

    Value *cond = read_value(alc, fc, scope, false, 0);
    if (!type_is_bool(cond->rett)) {
        char buf[256];
        sprintf(b->char_buf, "if condition value must return a bool type, but found: '%s'", type_to_str(cond->rett, buf));
        parse_err(fc->chunk_parse, b->char_buf);
    }

    char* tkn = tok(fc, true, true, true);
    int scope_end_i = 0;
    bool single = false;
    Chunk* chunk_end = NULL;
    if (str_is(tkn, "{")) {
        chunk_end = chunk_clone(alc, fc->chunk_parse);
        chunk_end->i = chunk_end->scope_end_i;
    } else if (str_is(tkn, ":")) {
        single = true;
    } else {
        parse_err(p, -1, "Expected '{' or ':' after the if-condition value, but found: '%s'", tkn)
    }

    Scope *scope_if = scope_sub_make(alc, sc_default, scope, chunk_end);
    Scope *scope_else = scope_sub_make(alc, sc_default, scope, NULL);

    read_ast(fc, scope_if, single);

    tkn = tok(fc, true, true, true);
    if(str_is(tkn, "else")) {
        tkn = tok(fc, true, true, true);
        if(str_is(tkn, "if")) {
            // else if
            token_if(alc, fc, scope_else);
        } else {
            // else
            bool single = false;
            if (str_is(tkn, "{")) {
                chunk_end = chunk_clone(alc, fc->chunk_parse);
                chunk_end->i = chunk_end->scope_end_i;
                scope_else->chunk_end = chunk_end;
            } else if (str_is(tkn, ":")) {
                single = true;
            } else {
                parse_err(p, -1, "Expected '{' or ':' after 'else', but found: '%s'", tkn)
            }
            read_ast(fc, scope_else, single);
        }
    } else {
        tok_back(fc);
    }

    if (!scope->ast)
        scope->ast = array_make(alc, 20);
    array_push(scope->ast, tgen_if(alc, cond, scope_if, scope_else));
}

void token_while(Allocator* alc, Fc* fc, Scope* scope) {
    Build* b = fc->b;

    Value *cond = read_value(alc, fc, scope, false, 0);
    if (!type_is_bool(cond->rett)) {
        char buf[256];
        sprintf(b->char_buf, "if condition value must return a bool type, but found: '%s'", type_to_str(cond->rett, buf));
        parse_err(fc->chunk_parse, b->char_buf);
    }

    char* tkn = tok(fc, true, true, true);
    bool single = false;

    Chunk* chunk_end = NULL;
    if (str_is(tkn, "{")) {
        chunk_end = chunk_clone(alc, fc->chunk_parse);
        chunk_end->i = chunk_end->scope_end_i;
    } else if (str_is(tkn, ":")) {
        single = true;
    } else {
        parse_err(p, -1, "Expected '{' or ':' after the if-condition value, but found: '%s'", tkn)
    }

    Scope *scope_while = scope_sub_make(alc, sc_loop, scope, chunk_end);
    scope_while->loop_scope = scope_while;

    read_ast(fc, scope_while, single);

    // Gc reserve
    if(scope_while->decls) {
        Array* decls = scope_while->decls;
        int gc_count = decls->length;
        Scope* wrap = scope_sub_make(alc, sc_default, scope, chunk_end);

        // Reserve
        Map *idfs = map_make(alc);
        Value *amount = vgen_int(alc, gc_count, type_gen_number(alc, b, b->ptr_size, false, false));
        Idf *idf = idf_make(alc, idf_value, amount);
        map_set(idfs, "amount", idf);
        Scope *gc_reserve = gen_snippet_ast(alc, fc, get_volt_snippet(b, "mem", "reserve"), idfs, scope_while);

        array_shift(scope_while->ast, token_make(alc, t_ast_scope, gc_reserve));

        // Pop
        if (!scope_while->did_return) {
            Scope *pop = gen_snippet_ast(alc, fc, get_volt_snippet(b, "mem", "pop_no_return"), map_make(alc), scope_while);
            array_push(scope_while->ast, token_make(alc, t_ast_scope, pop));
        }
    }

    //
    array_push(scope->ast, tgen_while(alc, cond, scope_while));
}