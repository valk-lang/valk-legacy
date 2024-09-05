
#include "../all.h"

Parser* parser_make(Allocator* alc, Unit* u) {
    Parser* p = al(alc, sizeof(Parser));
    p->b = u->b;
    p->unit = u;
    p->tkn = NULL;
    p->try_conv = NULL;

    p->prev = NULL;
    p->chunk = chunk_make(alc, u->b, NULL);

    p->func = NULL;
    p->scope = NULL;
    p->loop_scope = NULL;
    p->vscope_values = NULL;
    p->cc_loops = array_make(alc, 10);

    p->line = 0;
    p->col = 0;
    p->scope_end_i = 0;
    p->cc_index = 0;
    p->cc_loop_index = 0;

    p->in_header = false;
    p->on_newline = false;
    p->reading_coro_fcall = false;
    p->parse_last = false;
    p->init_thread = false;
    p->allow_multi_type = false;

    return p;
}

void parser_new_context(Parser** ref) {
    Parser* p = *ref;
    Parser *p2 = pool_get_parser(p->unit);

    // Copy everything except chunk
    Chunk* ch = p2->chunk;
    *p2 = *p;
    p2->chunk = ch;
    p2->prev = p;

    // Set variable
    *ref = p2;
}
void parser_pop_context(Parser** ref) {
    Parser* p2 = *ref;
    Parser* p = p2->prev;
    *ref = p;
}

Value *read_value_from_other_chunk(Parser *p, Allocator* alc, Chunk *chunk, Scope *idf_scope, Type* check_type) {

    Scope *sub = scope_sub_make(alc, sc_default, p->scope);
    if (idf_scope)
        sub->idf_parent = idf_scope;

    parser_new_context(&p);

    *p->chunk = *chunk;
    p->scope = sub;
    Value *val = read_value(alc, p, true, 0);

    if (check_type && val->type != v_undefined) {
        val = try_convert(alc, p, p->scope, val, check_type);
        type_check(p, check_type, val->rett);
    }

    parser_pop_context(&p);

    return val;
}

void read_access_type(Parser *p, char t, char* res) {
    char act = act_public;
    if (t == tok_sub) {
        act = act_private_fc;
        t = tok(p, false, false, false);
        if (t == tok_id) {
            t = tok(p, false, false, true);
            if (str_is(p->tkn, "ns")) {
                act = act_private_nsc;
            } else if (str_is(p->tkn, "pkg")) {
                act = act_private_pkc;
            } else {
                parse_err(p, -1, "Invalid access type '-%s', valid options: '-', '-ns' or '-pkg'", p->tkn);
            }
        }
        t = tok(p, true, false, true);
    } else if (t == tok_tilde) {
        act = act_readonly_fc;
        t = tok(p, false, false, false);
        if (t == tok_id) {
            t = tok(p, false, false, true);
            if (str_is(p->tkn, "ns")) {
                act = act_readonly_nsc;
            } else if (str_is(p->tkn, "pkg")) {
                act = act_readonly_pkc;
            } else {
                parse_err(p, -1, "Invalid access type '~%s', valid options: '~', '~ns' or '~pkg'", p->tkn);
            }
        }
        t = tok(p, true, false, true);
    }
    res[0] = act;
    res[1] = t;
}
