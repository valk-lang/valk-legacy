
#include "../all.h"

void macro_parse_pattern(Allocator* alc, MacroPattern* pat, char* close_tkn, Parser* p);

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

    tok_expect(p, "{", true, true);
    m->body = chunk_clone(alc, p->chunk);
    skip_body(p);
}

void macro_parse_pattern(Allocator* alc, MacroPattern* pat, char* close_tkn, Parser* p) {

    Array* items = pat->items;

    while (true) {
        char t = tok(p, true, true, true);
        char* tkn = p->tkn;
        if (str_is(tkn, close_tkn)) {
            return;
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

            tok_expect(p, ",", true, true);

            MacroPattern *pat2 = al(alc, sizeof(MacroPattern));
            pat2->open_tkn = NULL;
            pat2->close_tkn = NULL;
            pat2->items = array_make(alc, 4);
            pat2->names_used = array_make(alc, 4);

            rep->pattern = pat2;

            macro_parse_pattern(alc, pat2, ")", p);

        } else {
            item->type = pat_tkn;
            item->item = p->tkn;
        }
        array_push(items, item);
    }
}