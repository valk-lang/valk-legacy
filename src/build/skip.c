
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

