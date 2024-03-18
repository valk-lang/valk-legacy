
#include "../all.h"

char cc_parse_cond(Parser* p);
void cc_skip_to_next_cond(Parser* p);

void cc_parse(Parser* p) {
    char t = tok(p, false, false, true);
    if(t != tok_id) {
        parse_err(p, -1, "Expected #if/elif/else/end, found: '%s'", p->tkn);
    }
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
}

void cc_skip_to_next_cond(Parser* p) {
}