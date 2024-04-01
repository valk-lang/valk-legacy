
#include "../all.h"

char cc_parse_cond(Parser* p, int prio);
void cc_skip_to_next_cond(Parser* p);
void cc_skip_loop(Parser* p);

CCLoop* cc_init_loop(Allocator* alc, Array* items) {
    CCLoop* cl = al(alc, sizeof(CCLoop));
    cl->items = items;
    cl->start = NULL;
    cl->idf1 = NULL;
    cl->idf2 = NULL;
    cl->idf3 = NULL;
    cl->idf4 = NULL;
    cl->idf_type = 0;
    cl->length = items->length;
    cl->index = 0;
    return cl;
}


void cc_parse(Parser* p) {
    Unit* u = p->unit;
    Build* b = u->b;
    char t = tok(p, false, false, true);
    char* tkn = p->tkn;
    if(str_is(tkn, "if")) {
        char result = cc_parse_cond(p, 0);
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
        char result = cc_parse_cond(p, 0);
        if(prev_result == 1) {
            cc_skip_to_next_cond(p);
        } else {
            p->cc_results[cci - 1] = result;
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

    } else if(str_is(tkn, "loop_globals")) {
        CCLoop* cl = cc_init_loop(b->alc, b->globals);
        cl->idf_type = idf_global;
        array_set_index(p->cc_loops, p->cc_loop_index++, cl);
        tok_expect(p, "as", true, false);
        // Global idf
        char t = tok(p, true, false, true);
        if(t != tok_id)
            parse_err(p, -1, "Invalid identifier name");
        char* idf_name1 = p->tkn;
        // // Name idf
        // tok_expect(p, ",", true, false);
        // t = tok(p, true, false, true);
        // if(t != tok_id)
        //     parse_err(p, -1, "Invalid identifier name");
        // cl->idf_name2 = p->tkn;
        // Type idf
        tok_expect(p, ",", true, false);
        t = tok(p, true, false, true);
        if(t != tok_id)
            parse_err(p, -1, "Invalid identifier name");
        char* idf_name3 = p->tkn;
        tok_expect_newline(p);
        //
        cl->start = chunk_clone(b->alc_ast, p->chunk);
        //
        if(cl->index == cl->length) {
            // No items, skip
            cc_skip_loop(p);
        } else {
            Global* g = array_get_index(cl->items, cl->index++);
            cl->idf1 = idf_make(b->alc_ast, idf_global, g);
            cl->idf3 = idf_make(b->alc_ast, idf_type, g->type);
            scope_set_idf(p->scope, idf_name1, cl->idf1, p);
            // scope_set_idf(p->scope, cl->idf_name2, idf_make(b->alc_ast, idf_chars, g->name), p);
            scope_set_idf(p->scope, idf_name3, cl->idf3, p);
        }

    } else if(str_is(tkn, "endloop")) {
        if(p->cc_loop_index == 0) {
            parse_err(p, -1, "Using '#endloop' while not being inside a loop");
        }
        CCLoop* cl = array_get_index(p->cc_loops, p->cc_loop_index - 1);
        if(cl->index < cl->length) {
            if(cl->idf_type == idf_global) {
                Global* g = array_get_index(cl->items, cl->index++);
                cl->idf1->item = g;
                cl->idf3->item = g->type;
            } else {
                parse_err(p, -1, "Unhandled '#endloop' type (compiler bug)");
            }
            *p->chunk = *cl->start;
        }

    } else {
        parse_err(p, -1, "Expected #if/elif/else/end, found: '%s'", p->tkn);
    }
}

char cc_parse_cond(Parser* p, int prio) {
    char t = tok(p, true, false, true);
    char* tkn = p->tkn;
    char result = -1;

    Build *b = p->b;

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
        } else if (str_is(p->tkn, "@type_is_void")) {
            tok_expect(p, "(", false, false);
            Type* type = read_type(p, p->b->alc, false);
            tok_expect(p, ")", true, false);
            result = type_is_void(type) ? 1 : 0;
        } else if (str_is(p->tkn, "@global_is_shared")) {
            tok_expect(p, "(", false, false);
            Id id;
            read_id(p, NULL, &id);
            Idf *idf = idf_by_id(p, p->scope, &id, true);
            if(idf->type != idf_global) {
                parse_err(p, -1, "This identifier does not represent a global");
            }
            tok_expect(p, ")", true, false);
            Global* g = idf->item;
            result = g->is_shared ? 1 : 0;
        }
    } else if(t == tok_id) {
        char* name = p->tkn;
        Map* defs = b->cc_defs;
        char* value = map_get(defs, name);
        if(!value)
            parse_err(p, -1, "Unknown compile condition variable: '%s'", name);
        result = str_is(value, "0") ? 0 : 1;

        t = tok(p, true, false, false);
        if(t == tok_eqeq || t == tok_not_eq) {
            bool not_eq = t == tok_not_eq;
            t = tok(p, true, false, true);
            t = tok(p, true, false, true);
            if(t == tok_id) {
                result = str_is(value, p->tkn) ? 1 : 0;
            } else if(t == tok_number) {
                result = str_is(value, p->tkn) ? 1 : 0;
            } else {
                parse_err(p, -1, "Invalid right-side compile condition value: '%s'", p->tkn);
            }
            if(not_eq) {
                // Inverse result
                result = result ? 0 : 1;
            }
        }
    } else if(t == tok_not) {
        result = cc_parse_cond(p, 1);
        result = result == 1 ? 0 : 1;
    }

    if(result == -1) {
        parse_err(p, -1, "Invalid compile condition value");
    }

    if (prio == 0 || prio >= 10) {
        t = tok(p, true, false, false);
        if (t == tok_and) {
            t = tok(p, true, false, true);
            result = result && cc_parse_cond(p, 10);
        } else if (t == tok_or) {
            t = tok(p, true, false, true);
            result = result && cc_parse_cond(p, 10);
        }
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
            } else if(str_is(tkn, "end") || str_is(tkn, "elif") || str_is(tkn, "else")) {
                if(depth == 0) {
                    ch->i = before;
                    break;
                }
                if (str_is(tkn, "end"))
                    depth--;
            }
        }
    }
}
void cc_skip_loop(Parser* p) {

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
            if(str_is(tkn, "loop_globals")) {
                depth++;
            } else if(str_is(tkn, "endloop")) {
                if(depth == 0) {
                    ch->i = before;
                    break;
                }
                depth--;
            }
        }
    }
}