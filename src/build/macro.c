
#include "../all.h"

void macro_parse_pattern(Allocator* alc, MacroPattern* pat, char* close_tkn, Parser* p);
void macro_read_pattern(Allocator *alc, Parser *p, MacroPattern *pat, Map* identifiers);

void macro_parse(Allocator* alc, Macro* m, Parser* p) {

    Array* patterns = m->patterns;
    while (true) {
        char t = tok(p, true, true, true);
        char *open_tkn = p->tkn;
        char *close_tkn = NULL;
        if (t == tok_bracket_open) {
            close_tkn = ")";
        } else if (t == tok_sq_bracket_open) {
            close_tkn = "]";
        } else if (t == tok_curly_open) {
            close_tkn = "}";
        } else if (t == tok_colon) {
            break;
        } else {
            parse_err(p, -1, "Expected tokens: '[' '(' '{' or ':' , but found '%s'", p->tkn);
        }

        MacroPattern *pat = al(alc, sizeof(MacroPattern));
        pat->open_tkn = open_tkn;
        pat->close_tkn = close_tkn;
        pat->items = array_make(alc, 4);
        pat->names_used = array_make(alc, 4);

        macro_parse_pattern(alc, pat, close_tkn, p);

        array_push(patterns, pat);
    }

    m->body = chunk_clone(alc, p->chunk);

    char t = tok_expect_two(p, "{", "<{", true, true);
    m->is_value = t == tok_ltcurly_open;
    if(!m->is_value)
        *m->body = *p->chunk;
    skip_body(p);
}

void macro_parse_pattern(Allocator* alc, MacroPattern* pat, char* close_tkn, Parser* p) {

    Array* items = pat->items;
    bool has_repeat = false;

    while (true) {
        char t = tok(p, true, true, true);
        char* tkn = p->tkn;
        if (str_is(tkn, close_tkn)) {
            return;
        }
        if(has_repeat) {
            parse_err(p, -1, "You cannot define more pattern items after a 'repeat'");
        }
        MacroPatternItem* item = al(alc, sizeof(MacroPatternItem));
        item->item = NULL;
        item->name = NULL;
        if(str_is(tkn, "T")) {
            item->type = pat_type;
            tok_expect(p, ":", false, false);
            t = tok(p, false, false, true);
            if(t != tok_id)
                parse_err(p, -1, "Invalid macro pattern item identifier name: '%s'", p->tkn);
            char* name = p->tkn;
            if(array_contains(pat->names_used, name, arr_find_str))
                parse_err(p, -1, "Duplicate macro pattern item name used: '%s'", p->tkn);
            array_push(pat->names_used, name);
            item->name = name;
        } else if(str_is(tkn, "V")) {
            item->type = pat_value;
            tok_expect(p, ":", false, false);
            t = tok(p, false, false, true);
            if(t != tok_id)
                parse_err(p, -1, "Invalid macro pattern item identifier name: '%s'", p->tkn);
            char* name = p->tkn;
            if(array_contains(pat->names_used, name, arr_find_str))
                parse_err(p, -1, "Duplicate macro pattern item name used: '%s'", p->tkn);
            array_push(pat->names_used, name);
            item->name = name;
        } else if(str_is(tkn, "repeat")) {

            MacroRepeat *rep = al(alc, sizeof(MacroRepeat));
            item->type = pat_repeat;
            item->item = rep;

            tok_expect(p, "(", true, true);
            t = tok(p, true, true, true);
            if(t != tok_id)
                parse_err(p, -1, "Invalid macro pattern item identifier name: '%s'", p->tkn);
            char* name = p->tkn;
            if(array_contains(pat->names_used, name, arr_find_str))
                parse_err(p, -1, "Duplicate macro pattern item name used: '%s'", p->tkn);
            tok_expect(p, ",", true, true);
            t = tok(p, true, true, true);
            if(t != tok_char)
                parse_err(p, -1, "Invalid repeat delimiter, expected a character between single quotes");
            rep->delimiter = p->tkn;
            item->name = name;

            tok_expect(p, ",", true, true);

            MacroPattern *pat2 = al(alc, sizeof(MacroPattern));
            pat2->open_tkn = NULL;
            pat2->close_tkn = NULL;
            pat2->items = array_make(alc, 4);
            pat2->names_used = array_make(alc, 4);

            rep->pattern = pat2;

            macro_parse_pattern(alc, pat2, ")", p);

            has_repeat = true;

        } else {
            item->type = pat_tkn;
            item->item = p->tkn;
        }
        array_push(items, item);
    }
}

Value* macro_read_value(Allocator* alc, Macro* m, Parser* p) {
    //
    Array* patterns = m->patterns;
    Scope* scope = p->scope;
    Scope* sub = scope_sub_make(alc, sc_default, p->scope);

    loop(patterns, i) {
        MacroPattern* pat = array_get_index(patterns, i);
        macro_read_pattern(alc, p, pat, sub->identifiers);
    }

    p->scope = sub;
    parser_new_context(&p);

    *p->chunk = *m->body;
    Value* res = read_value(alc, p, true, 0);

    parser_pop_context(&p);
    p->scope = scope;

    return res;
}

void macro_read_ast(Allocator* alc, Macro* m, Parser* p) {
    //
    Array* patterns = m->patterns;
    Scope* scope = p->scope;
    Scope* sub = scope_sub_make(alc, sc_default, p->scope);

    loop(patterns, i) {
        MacroPattern* pat = array_get_index(patterns, i);
        macro_read_pattern(alc, p, pat, sub->identifiers);
    }

    p->scope = sub;
    parser_new_context(&p);

    *p->chunk = *m->body;
    read_ast(p, false);

    parser_pop_context(&p);
    p->scope = scope;

    if (sub->did_return)
        scope->did_return = true;
    array_push(scope->ast, token_make(alc, t_ast_scope, sub));
}

void macro_read_pattern(Allocator *alc, Parser *p, MacroPattern *pat, Map* identifiers) {

    if (pat->open_tkn) {
        tok_expect(p, pat->open_tkn, true, true);
    }

    Array *items = pat->items;
    loop(items, o) {
        MacroPatternItem *item = array_get_index(items, o);
        if (item->type == pat_type) {
            Type *type = read_type(p, alc, true);
            map_set(identifiers, item->name, idf_make(alc, idf_type, type));
        } else if (item->type == pat_value) {
            Value *val = read_value(alc, p, true, 0);
            map_set(identifiers, item->name, idf_make(alc, idf_value, val));
        } else if (item->type == pat_tkn) {
            tok_expect(p, item->item, true, true);
        } else if (item->type == pat_repeat) {

            MacroRepeat *rep = item->item;
            Array* rep_items = array_make(alc, 4);

            while (true) {
                char t = tok(p, true, true, false);
                if (pat->close_tkn && str_is(p->tkn, pat->close_tkn)) {
                    break;
                }
                if(rep_items->length > 0 && rep->delimiter && rep->delimiter[0] != 0) {
                    tok_expect(p, rep->delimiter, true, true);
                }

                Map *identifiers = map_make(alc);
                MacroItem *mi = al(alc, sizeof(MacroItem));
                mi->identifiers = identifiers;
                macro_read_pattern(alc, p, rep->pattern, identifiers);
                array_push(rep_items, mi);
            }

            map_set(identifiers, item->name, idf_make(alc, idf_macro_items, rep_items));
        }
    }

    if (pat->close_tkn) {
        tok_expect(p, pat->close_tkn, true, true);
    }
}
