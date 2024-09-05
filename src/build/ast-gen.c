
#include "../all.h"

void func_ast_finalize(Allocator* alc, Parser* p) {
    Build* b = p->b;
    Scope* scope = p->scope;
    Func* func = p->func;
    Scope *start = func->ast_start;

    // Count gc decls
    int gc_decl_count = 0;
    int alloca_size = 0;
    Array *decls = scope->decls;
    loop(decls, i) {
        Decl *decl = array_get_index(decls, i);
        if (!decl->is_gc) {
            if (decl->is_mut) {
                decl->offset = alloca_size;
                alloca_size += type_get_size(b, decl->type);
                alloca_size += ((b->ptr_size - (alloca_size % b->ptr_size)) % b->ptr_size);
            }
            continue;
        }
        if (decl->is_mut || !decl->is_arg) {
            decl->offset = gc_decl_count++;
        }
    }
    func->alloca_size = alloca_size;
    func->gc_decl_count = gc_decl_count;
}