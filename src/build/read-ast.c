
#include "../all.h"

void loop_defer(Allocator* alc, Parser* p);

void read_ast(Parser *p, bool single_line) {
    //
    Build *b = p->b;
    Allocator *alc = b->alc_ast;
    Scope *scope = p->scope;

    if (!scope->ast)
        scope->ast = array_make(alc, 50);

    int scope_type = scope->type;
    bool is_func_scope = scope_type == sc_func;
    if (is_func_scope) {
        ast_func_start(alc, p);
    }

    bool first = false;

    while (true) {

        if (single_line && first)
            return;

        int before_i = p->chunk->i;
        char t = tok(p, true, true, true);
        char *tkn = p->tkn;

        if (tkn[0] == ';')
            continue;
        if (t == tok_curly_close)
            break;

        first = true;
        //
        if (t == tok_hashtag && p->on_newline) {
            cc_parse(p);
            continue;
        }

        if (scope->did_return)
            continue;

        if (t == tok_id) {
            if (str_is(tkn, "let")) {
                pt_let(b, alc, p);
                continue;
            }
            if (str_is(tkn, "if")) {
                pt_if(b, alc, p);
                continue;
            }
            if (str_is(tkn, "while")) {
                pt_while(b, alc, p);
                continue;
            }
            if (str_is(tkn, "break") || str_is(tkn, "continue")) {
                if (!p->loop_scope) {
                    parse_err(p, -1, "Using 'break' without being inside a loop");
                }
                loop_defer(alc, p);
                array_push(scope->ast, token_make(alc, str_is(tkn, "break") ? t_break : t_continue, p->loop_scope));
                scope->did_return = true;
                continue;
            }
            if (str_is(tkn, "return")) {
                pt_return(b, alc, p);
                continue;
            }
            if (str_is(tkn, "throw")) {
                pt_throw(b, alc, p);
                continue;
            }
            if (str_is(tkn, "each")) {
                pt_each(b, alc, p);
                continue;
            }
            if (str_is(tkn, "await_fd")) {
                pt_await_fd(b, alc, p);
                continue;

            } else if (str_is(tkn, "await_last")) {
                pt_await_last(b, alc, p);
                continue;
            }
            // Check if macro
            Idf *idf = scope_find_idf(scope, tkn, true);
            if (idf && idf->type == idf_macro) {
                Macro *m = idf->item;
                macro_read_ast(alc, m, p);
                continue;
            }
        }
        if (t == tok_at_word) {
            if (str_is(tkn, "@gc_share")) {
                pt_gc_share(b, alc, p);
                continue;
            }
        }
        if (t == tok_curly_open) {
            Scope *sub = scope_sub_make(alc, sc_default, scope);
            p->scope = sub;
            read_ast(p, false);
            p->scope = scope;
            if (sub->did_return)
                scope->did_return = true;
            array_push(scope->ast, token_make(alc, t_ast_scope, sub));
            continue;
        }

        p->chunk->i = before_i;

        Value *left = read_value(alc, p, true, 0);

        t = tok(p, true, false, false);
        if (t >= tok_eq && t <= tok_div_eq) {
            pt_assign(b, alc, p, left, t);
            continue;
        }

        //
        array_push(scope->ast, token_make(alc, t_statement, left));
        //
        if (scope->did_return && p->func) {
            // Value contains an exit function call
            array_push(scope->ast, token_make(alc, t_return, NULL));
        }
    }

    if (scope->must_return && !scope->did_return) {
        parse_err(p, -1, "Missing return statement");
    }

    if (scope->gc_check) {
        p->func->calls_gc_check = true;
    }

    if (scope_type == sc_loop && !scope->did_return) {
        loop_defer(alc, p);
    }

    if (is_func_scope) {
        ast_func_end(alc, p);
    }
}

void loop_defer(Allocator *alc, Parser *p) {
    Scope *ast_scope = p->scope;
    Scope *loop_scope = p->loop_scope;
    if (!loop_scope || !loop_scope->has_gc_decls) {
        return;
    }
    Build *b = p->b;
    Array *decls = loop_scope->decls;
    loop(decls, i) {
        Decl *decl = array_get_index(decls, i);
        if (!decl->is_gc)
            continue;
        array_push(ast_scope->ast, tgen_assign(alc, vgen_decl(alc, decl), vgen_null(alc, b)));
    }

    Scope *gcscope = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "run_gc_check"), map_make(alc), ast_scope);
    Token *t = token_make(alc, t_ast_scope, gcscope);
    array_push(ast_scope->ast, t);
}
