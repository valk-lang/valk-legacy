
#include "../all.h"

char cc_parse_cond(Parser* p);
void cc_skip_to_next_cond(Parser* p);

void cc_parse(Parser* p) {
    char t = tok(p, false, false, true);
    char* tkn = p->tkn;
    if(str_is(tkn, "if")) {
        char result = cc_parse_cond(p);
        p->cc_results[p->cc_index++] = result;
        if(result == 1) {
            tok_expect_newline(p);
        } else {
            cc_skip_to_next_cond(p);
        }

    } else if(str_is(tkn, "elif")) {

        int cci = p->cc_index;
        if(cci == 0) {
            parse_err(p, -1, "Unexpected #elif, missing #if in front of it");
        }
        char prev_result = p->cc_results[cci - 1];
        char result = cc_parse_cond(p);
        if(prev_result == 1) {
            cc_skip_to_next_cond(p);
        } else {
            p->cc_results[cci] = result;
            if (result == 1) {
                tok_expect_newline(p);
            } else {
                cc_skip_to_next_cond(p);
            }
        }

    } else if(str_is(tkn, "else")) {

        int cci = p->cc_index;
        if(cci == 0) {
            parse_err(p, -1, "Unexpected #else, missing #if in front of it");
        }
        char prev_result = p->cc_results[cci - 1];
        if(prev_result == 1) {
            cc_skip_to_next_cond(p);
        } else {
            tok_expect_newline(p);
        }

    } else if(str_is(tkn, "end")) {

        int cci = p->cc_index;
        if(cci == 0) {
            parse_err(p, -1, "Unexpected #end, missing #if in front of it");
        }
        p->cc_index--;

    } else {
        parse_err(p, -1, "Expected #if/elif/else/end, found: '%s'", p->tkn);
    }
}

char cc_parse_cond(Parser* p) {
    char t = tok(p, true, false, true);
    char* tkn = p->tkn;
    char result = -1;

    if(t == tok_at_word) {
        if (str_is(p->tkn, "@type_is_gc")) {
            tok_expect(p, "(", false, false);
            Type* type = read_type(p, p->b->alc, false);
            tok_expect(p, ")", true, false);
            result = type_is_gc(type) ? 1 : 0;
        } else if (str_is(p->tkn, "@type_is_signed")) {
            tok_expect(p, "(", false, false);
            Type* type = read_type(p, p->b->alc, false);
            tok_expect(p, ")", true, false);
            result = type->is_signed ? 1 : 0;
        }
    }

    if(result == -1) {
        parse_err(p, -1, "Invalid compile condition value");
    }
    return result;
}

void cc_skip_to_next_cond(Parser* p) {

    int depth = 0;
    Chunk* ch = p->chunk;
    while(true) {
        int before = ch->i;
        char t = tok(p, true, true, true);

        if (t == tok_eof) {
            parse_err(p, -1, "Unexpected end of file, cannot find #end token");
        }

        if (t == tok_hashtag && p->on_newline) {
            tok(p, false, false, true);
            char* tkn = p->tkn;
            if(str_is(tkn, "if")) {
                depth++;
            } else if(str_is(tkn, "end") || str_is(tkn, "else") || str_is(tkn, "elif")) {
                if(depth == 0) {
                    ch->i = before;
                    break;
                }
                depth--;
            }
        }
    }
}