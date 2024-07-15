
#include "../all.h"

void stage_unroll(Build *b, Func *func) {
    Scope* scope = func->scope;
    Allocator* alc = func->ast_alc;
    Block* block = block_make(alc, "start");
    unroll_scope(alc, b, func, block, scope);
}

void unroll_scope(Allocator*alc, Build *b, Func *func, Block* block, Scope* scope) {
    Array* scope_ast = scope->ast;

    if(scope->type == type_func) {
        unroll_scope_start(alc, b, func, block, scope);
    }

    loop(scope_ast, i) {
        Token* t = array_get_index(scope_ast, i);
    }
}

void unroll_scope_start(Allocator*alc, Build *b, Func *func, Block* block, Scope* scope) {
}

void unroll_value(Allocator*alc, Build *b, Func *func, Block* block, Value* value) {
}
