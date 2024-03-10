
#include "../all.h"

void skip_body(Parser* p) {
    p->chunk->i = p->scope_end_i;
}

void skip_type(Parser* p) {
    Chunk* ch = p->chunk;

    int t = tok(p, true, true, true);
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

// void skip_value(Fc *fc) {

//     Chunk *chunk = fc->chunk_parse;

//     char *tkn = tok(fc, true, true, true);
//     char t = chunk->token;

//     if (t == tok_string) {
//     } else if (t == tok_char_string) {
//     } else if (t == tok_scope_open && str_is(tkn, "(")) {
//         skip_body(fc);
//     } else if (t == tok_id) {
//         // Full identifier
//         if (tok_id_next(fc) == tok_char && tok_read_byte(fc, 1) == ':') {
//             tkn = tok(fc, false, false, true);
//             tkn = tok(fc, false, false, true);
//         }
//         // Class init
//         tkn = tok(fc, true, true, true);
//         if(str_is(tkn, "{")) {
//             skip_body(fc);
//         } else {
//             tok_back(fc);
//         }
//     } else if (t == tok_number) {
//         // Number type hints
//         if (tok_id_next(fc) == tok_char && tok_read_byte(fc, 1) == '#') {
//             skip_type(fc);
//         }
//     } else if (str_is(tkn, "--") || str_is(tkn, "++")) {
//         skip_value(fc);
//         return;
//     } else {
//         tok_back(fc);
//         return;
//     }

//     tkn = tok(fc, false, false, true);
//     while(chunk->token != tok_none) {
//         if (str_is(tkn, ".")) {
//             skip_value(fc);
//             return;
//         }
//         if (str_is(tkn, "(") || str_is(tkn, "[")) {
//             skip_body(fc);
//             tkn = tok(fc, false, false, true);
//             continue;
//         }
//         if (str_is(tkn, "--") || str_is(tkn, "++")) {
//             tkn = tok(fc, false, false, true);
//             continue;
//         }
//         tok_back(fc);
//         break;
//     }

//     tkn = tok(fc, true, true, true);
//     while(str_is(tkn, "@as")) {
//         skip_type(fc);
//         tkn = tok(fc, true, true, true);
//         t = chunk->token;
//     }
//     if(str_is(tkn, "?")) {
//         skip_value(fc);
//         tok_expect(fc, ":", true, true);
//         skip_value(fc);
//         return;
//     }

//     if (str_is(tkn, "<=") || str_is(tkn, ">=") || str_is(tkn, "==") || str_is(tkn, "!=") || str_is(tkn, "&&") || str_is(tkn, "||") || str_is(tkn, "+") || str_is(tkn, "-") || str_is(tkn, "/") || str_is(tkn, "*") || str_is(tkn, "%") || str_is(tkn, "&") || str_is(tkn, "|") || str_is(tkn, "^") || str_is(tkn, "??") || str_is(tkn, "?!")) {
//         skip_value(fc);
//         return;
//     }

//     tok_back(fc);
// }
