
#include "../all.h"

Value* coro_generate(Allocator* alc, Parser* p, Value* vfcall) {
    Build* b = p->b;
    Unit* u = p->unit;
    Scope* scope = p->scope;

    VFuncCall *fcall = vfcall->item;
    TypeFuncInfo *fi = fcall->on->rett->func_info;
    Array *types = fi->args;
    Type *rett = fi->rett;

    bool has_rett = !type_is_void(rett);
    bool has_arg = false;
    bool has_gc_arg = false;
    for(int i = 0; i < types->length; i++) {
        Type* type = array_get_index(types, i); 
        if(type_is_gc(type)) {
            has_gc_arg = true;
        } else {
            has_arg = true;
        }
    }
    bool has_args = has_arg || has_gc_arg;

    // Generate handler function
    Scope* parent = p->func->scope->parent;
    char name[256];
    sprintf(name, "VALK_CORO_%d", b->coro_count++);
    Func *func = func_make(b->alc, u, parent, dups(b->alc, name), NULL);

    Str* code = b->str_buf;
    str_clear(code);

    Idf *idf = idf_make(b->alc, idf_class, get_valk_class(b, "core", "Coro2"));
    scope_set_idf(func->scope, "CORO_CLASS", idf, NULL);
    idf = idf_make(b->alc, idf_func, get_valk_func(b, "core", "setjmp"));
    scope_set_idf(func->scope, "SETJMP", idf, NULL);

    // Coro start function code
    str_flat(code, "(coro: CORO_CLASS) {\n");
    if(has_arg) {
        str_flat(code, "let args = coro.stack\n");
    }
    if(has_gc_arg) {
        str_flat(code, "let gc_args = coro.gc_stack\n");
    }
    // Load args
    for(int i = 0; i < types->length; i++) {
        Type* type = array_get_index(types, i);
        char nr[32];
        char tnr[33];
        sprintf(nr, "%d", i);
        sprintf(tnr, "T%d", i);
        Idf *idf = idf_make(alc, idf_type, type);
        scope_set_idf(func->scope, tnr, idf, NULL);
        str_flat(code, "let arg");
        str_add(code, nr);
        str_flat(code, " = @ptrv(");
        str_add(code, type_is_gc(type) ? "gc_args" : "args");
        str_flat(code, ", ");
        str_add(code, tnr);
        str_flat(code, ", 0)\n");
        str_add(code, type_is_gc(type) ? "gc_args" : "args");
        str_flat(code, " += sizeof(");
        str_add(code, tnr);
        str_flat(code, ")\n");
    }
    // Call handler
    if(has_rett) {
        str_flat(code, "let res = ");
    }
    str_flat(code, "coro.handler(");
    // Args
    for(int i = 0; i < types->length; i++) {
        Type* type = array_get_index(types, i);
        char argname[36];
        sprintf(argname, "arg%d", i);
        if(i > 0)
            str_flat(code, ", ");
        str_add(code, argname);
    }
    str_flat(code, ")\n");
    // Finish
    if(has_rett) {
        str_flat(code, "@ptrv(coro.result, RETT, 0) = res\n");
    }
    str_flat(code, "coro.complete()\n");
    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    // Create new parser
    parser_new_context(&p);

    *p->chunk = *chunk;
    p->scope = parent;
    parse_handle_func_args(p, func);
    stage_types_func(p, func);

    // Return parser
    parser_pop_context(&p);

    array_push(p->func->used_functions, func);

    ///////////////////////
    // Generate init coro
    ///////////////////////

    str_clear(code);

    Scope* sub = scope_sub_make(alc, sc_default, scope);

    idf = idf_make(alc, idf_class, get_valk_class(b, "core", "Coro2"));
    scope_set_idf(sub, "CORO_CLASS", idf, NULL);
    idf = idf_make(alc, idf_value, fcall->on);
    scope_set_idf(sub, "HANDLER", idf, NULL);
    idf = idf_make(alc, idf_value, vgen_func_ptr(alc, func, NULL));
    scope_set_idf(sub, "START_FUNC", idf, NULL);

    // Coro start function code
    str_flat(code, "<{\n");
    str_flat(code, "let coro = CORO_CLASS.new(HANDLER, START_FUNC, true)\n");
    if(has_arg) {
        str_flat(code, "let args = coro.stack_adr\n");
    }
    if(has_gc_arg) {
        str_flat(code, "let gc_args = coro.gc_stack_adr\n");
    }
    // Load args
    Array* values = fcall->args;
    for(int i = 0; i < values->length; i++) {
        Value* val = array_get_index(types, i);
        Type* type = val->rett;

        char nr[32];
        char tnr[33];
        sprintf(nr, "%d", i);
        sprintf(tnr, "ARG_T_%d", i);
        idf = idf_make(alc, idf_type, type);
        scope_set_idf(sub, tnr, idf, NULL);

        sprintf(tnr, "ARG_V_%d", i);
        idf = idf_make(alc, idf_value, val);
        scope_set_idf(sub, tnr, idf, NULL);

        str_flat(code, "@ptrv(");
        str_add(code, type_is_gc(type) ? "gc_args" : "args");
        str_flat(code, ", ARG_T_");
        str_add(code, nr);
        str_flat(code, ", 0) = ARG_V_");
        str_add(code, nr);
        str_flat(code, "\n");
        str_add(code, type_is_gc(type) ? "gc_args" : "args");
        str_flat(code, " += sizeof(ARG_T_");
        str_add(code, nr);
        str_flat(code, ")\n");
    }
    // Update adr
    if(has_arg) {
        str_flat(code, "coro.stack_adr = args\n");
    }
    if(has_gc_arg) {
        str_flat(code, "coro.gc_stack_adr = gc_args\n");
    }
    str_flat(code, "return coro\n");
    str_flat(code, "}\n");

    content = str_to_chars(b->alc, code);
    chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    // Create new parser
    parser_new_context(&p);

    *p->chunk = *chunk;
    p->scope = sub;
    Value* v = read_value(alc, p, true, 0);

    // Return parser
    parser_pop_context(&p);

    return v;
}
