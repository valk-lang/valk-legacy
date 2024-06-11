
#include "../all.h"

void skip_body(Parser* p) {
    p->chunk->i = p->scope_end_i;
}

void skip_type(Parser* p) {
    Chunk* ch = p->chunk;

    char t = tok(p, true, true, true);
    if(t == tok_bracket_open) {
        tok(p, false, false, true);
        skip_body(p);
        return;
    }
    if (t == tok_at_word && str_is(p->tkn, "@ignu")) {
        tok_expect(p, "(", false, false);
        skip_body(p);
        return;
    }
    if (t == tok_qmark || t == tok_dot) {
        t = tok(p, false, false, true);
    }
    if(t != tok_id) {
        parse_err(p, -1, "Invalid type syntax, unexpected: '%s'", p->tkn);
    }
    t = tok(p, false, false, false);
    if(t == tok_colon) {
        tok(p, false, false, true);
        t = tok(p, false, false, true);
        if (t != tok_id) {
            parse_err(p, -1, "Invalid type syntax, unexpected: '%s'", p->tkn);
        }
        t = tok(p, false, false, false);
    }
    if(t == tok_sq_bracket_open) {
        tok(p, false, false, true);
        skip_body(p);
    }
}

void skip_id(Parser* p) {
    char t = tok(p, true, true, true);
    if(t != tok_id) {
        parse_err(p, -1, "Invalid identifier syntax, unexpected: '%s'", p->tkn);
    }
    t = tok(p, false, false, false);
    if(t == tok_colon) {
        tok(p, false, false, true);
        t = tok(p, false, false, true);
        if (t != tok_id) {
            parse_err(p, -1, "Invalid identifier syntax, unexpected: '%s'", p->tkn);
        }
    }
}
