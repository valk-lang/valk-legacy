
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

    char *tkn = tok(fc, true, true, true);
    char t = chunk->token;

    if (t == tok_string) {
    } else if (t == tok_char_string) {
    } else if (t == tok_scope_open && str_is(tkn, "(")) {
        skip_body(fc);
    } else if (t == tok_id) {
        // Full identifier
        if (tok_id_next(fc) == tok_char && tok_read_byte(fc, 1) == ':') {
            tkn = tok(fc, false, false, true);
            tkn = tok(fc, false, false, true);
        }
        // Class init
        tkn = tok(fc, true, true, true);
        if(str_is(tkn, "{")) {
            skip_body(fc);
        } else {
            tok_back(fc);
        }
    } else if (t == tok_number) {
        // Number type hints
        if (tok_id_next(fc) == tok_char && tok_read_byte(fc, 1) == '#') {
            skip_type(fc);
        }
    } else if (str_is(tkn, "--") || str_is(tkn, "++")) {
        skip_value(fc);
        return;
    } else {
        tok_back(fc);
        return;
    }

    tkn = tok(fc, false, false, true);
    while(chunk->token != tok_none) {
        if (str_is(tkn, ".")) {
            skip_value(fc);
            return;
        }
        if (str_is(tkn, "(") || str_is(tkn, "[")) {
            skip_body(fc);
            tkn = tok(fc, false, false, true);
            continue;
        }
        if (str_is(tkn, "--") || str_is(tkn, "++")) {
            tkn = tok(fc, false, false, true);
            continue;
        }
        tok_back(fc);
        break;
    }

    tkn = tok(fc, true, true, true);
    while(str_is(tkn, "@as")) {
        skip_type(fc);
        tkn = tok(fc, true, true, true);
        t = chunk->token;
    }
    if(str_is(tkn, "?")) {
        skip_value(fc);
        tok_expect(fc, ":", true, true);
        skip_value(fc);
        return;
    }

    if (str_is(tkn, "<=") || str_is(tkn, ">=") || str_is(tkn, "==") || str_is(tkn, "!=") || str_is(tkn, "&&") || str_is(tkn, "||") || str_is(tkn, "+") || str_is(tkn, "-") || str_is(tkn, "/") || str_is(tkn, "*") || str_is(tkn, "%") || str_is(tkn, "&") || str_is(tkn, "|") || str_is(tkn, "^") || str_is(tkn, "??") || str_is(tkn, "?!")) {
        skip_value(fc);
        return;
    }

    tok_back(fc);
}
