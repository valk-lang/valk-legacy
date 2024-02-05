
#include "../all.h"

void skip_body(Fc* fc) {
    Chunk* ch = fc->chunk_parse;
    ch->i = ch->scope_end_i;
}

void skip_type(Fc* fc) {
    Chunk* ch = fc->chunk_parse;

    char *tkn = tok(fc, true, true, true);
    if (str_is(tkn, "?") || str_is(tkn, ".")) {
        tkn = tok(fc, false, false, true);
    }
    if(ch->token != tok_id && tkn[0] != ':') {
        sprintf(fc->b->char_buf, "Invalid type syntax, unexpected: '%s'", tkn);
        parse_err(ch, fc->b->char_buf);
    }
    tkn = tok(fc, false, false, true);
    if(tkn[0] == '[') {
        skip_body(fc);
    } else {
        tok_back(fc);
    }
}

void skip_value(Fc *fc) {

    Chunk *chunk = fc->chunk_parse;
    while (true) {

        char* tkn = tok(fc, true, true, true);
        char t = chunk->token;

        if (t == tok_string) {
            continue;
        }
        if (t == tok_char_string) {
            continue;
        }
        if (t == tok_scope_open) {
            skip_body(fc);
            continue;
        }
        if (t == tok_id) {
            continue;
        }
        if (t == tok_number) {
            continue;
        }

        if (str_is(tkn, ":") || str_is(tkn, ".") || str_is(tkn, "<=") || str_is(tkn, ">=") || str_is(tkn, "==") || str_is(tkn, "!=") || str_is(tkn, "&&") || str_is(tkn, "||") || str_is(tkn, "+") || str_is(tkn, "-") || str_is(tkn, "/") || str_is(tkn, "*") || str_is(tkn, "%") || str_is(tkn, "&") || str_is(tkn, "|") || str_is(tkn, "^") || str_is(tkn, "++") || str_is(tkn, "--") || str_is(tkn, "->") || str_is(tkn, "??") || str_is(tkn, "?!") || str_is(tkn, "!!") || str_is(tkn, "?")) {
            continue;
        }

        tok_back(fc);
        break;
    }
}
