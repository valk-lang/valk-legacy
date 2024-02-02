
#include "../all.h"

Type* type_make(Allocator* alc, int type) {
    Type* t = al(alc, sizeof(Type));
    t->type = type;
    return t;
}

Type* read_type(Fc* fc, bool allow_newline) {
    //
    Build* b = fc->b;

    char* tkn = tok(fc, true, allow_newline, true);
    if(str_is(tkn, "void")) {
        return type_make(fc->alc, type_void);
    }

    sprintf(b->char_buf, "Invalid type: '%s'", tkn);
    parse_err(fc->chunk_parse, b->char_buf);

    return NULL;
}
