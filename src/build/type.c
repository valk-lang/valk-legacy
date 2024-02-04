
#include "../all.h"

Type* type_make(Allocator* alc, int type) {
    Type* t = al(alc, sizeof(Type));
    t->type = type;
    t->size = 0;
    t->class = NULL;
    t->func_args = NULL;
    t->func_rett = NULL;
    return t;
}

Type* read_type(Fc* fc, Allocator* alc, Scope* scope, bool allow_newline) {
    //
    Build* b = fc->b;

    bool nullable = false;;

    char* tkn = tok(fc, true, allow_newline, true);
    if(str_is(tkn, "?")) {
        nullable = true;
        tkn = tok(fc, false, false, true);
    }

    int t = fc->chunk_parse->token;
    if(t == tok_id) {
        if (str_is(tkn, "void")) {
            return type_make(alc, type_void);
        }

        Idf *idf = read_idf(fc, scope, tkn, true);
        if(idf->type == idf_class) {
            Class* class = idf->item;
            return type_gen_class(alc, class);
        }
    }

    sprintf(b->char_buf, "Invalid type: '%s'", tkn);
    parse_err(fc->chunk_parse, b->char_buf);

    return NULL;
}

Type* type_gen_class(Allocator* alc, Class* class) {
    Type* t = type_make(alc, type_struct);
    t->class = class;
    t->size = class->b->ptr_size;
    return t;
}
Type* type_gen_func(Allocator* alc, Func* func) {
    Type* t = type_make(alc, type_func);
    t->func_rett = func->rett;
    t->func_args = func->args->values;
    t->size = func->b->ptr_size;
    return t;
}
