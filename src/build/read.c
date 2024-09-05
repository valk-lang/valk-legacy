
#include "../all.h"


void read_generic_names(Parser* p, Array* names) {
    Allocator* alc = p->b->alc;

    while (true) {
        int t = tok(p, true, false, true);
        if(t != tok_id){
            parse_err(p, -1, "Invalid generic type name: '%s'", p->tkn);
        }
        if (array_contains(names, p->tkn, arr_find_str)) {
            parse_err(p, -1, "Duplicate generic type name: '%s'", p->tkn);
        }
        array_push(names, p->tkn);
        if (tok_expect_two(p, ",", "]", true, false) == tok_sq_bracket_close)
            break;
    }
}


void read_generic_types(Parser* p, Array* list, int expected_count) {
    Allocator* alc = p->b->alc;

    for (int i = 0; i < expected_count; i++) {
        Type *type = read_type(p, alc, false);
        array_push(list, type);
        if (i + 1 < expected_count) {
            tok_expect(p, ",", true, false);
        } else {
            tok_expect(p, "]", true, false);
            break;
        }
    }
}
