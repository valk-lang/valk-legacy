
#include "../../all.h"

void pt_await_last(Build* b, Allocator* alc, Parser* p) {

    Scope* scope = p->scope;

    Global *g_coro = get_valk_global(b, "core", "current_coro");
    Value *coro = value_make(alc, v_global, g_coro, g_coro->type);
    // Call Coro.await_last
    Class *coro_class = get_valk_class(b, "core", "Coro");
    Func *f = map_get(coro_class->funcs, "await_last");
    func_mark_used(p->func, f);
    Value *fptr = vgen_func_ptr(alc, f, NULL);
    Array *args = array_make(alc, 4);
    array_push(args, coro);
    Value *fcall = vgen_func_call(alc, p, fptr, args);
    array_push(scope->ast, token_make(alc, t_statement, fcall));
}
