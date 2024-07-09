
#include "../../all.h"

void pt_await_fd(Build* b, Allocator* alc, Parser* p) {

    Scope* scope = p->scope;

    tok_expect(p, "(", false, false);
    Value *fd = read_value(alc, p, true, 0);
    type_check(p, type_gen_valk(alc, b, "FD"), fd->rett);
    tok_expect(p, ",", true, true);
    Value *read = read_value(alc, p, true, 0);
    type_check(p, type_gen_valk(alc, b, "bool"), read->rett);
    tok_expect(p, ",", true, true);
    Value *write = read_value(alc, p, true, 0);
    type_check(p, type_gen_valk(alc, b, "bool"), write->rett);
    tok_expect(p, ")", true, true);

    // Current coro
    Global *g_coro = get_valk_global(b, "core", "current_coro");
    Value *coro = value_make(alc, v_global, g_coro, g_coro->type);
    // Call Coro.await_fd
    Class *coro_class = get_valk_class(b, "core", "Coro");
    Func *f = map_get(coro_class->funcs, "await_fd");
    func_mark_used(p->func, f);
    Value *fptr = vgen_func_ptr(alc, f, NULL);
    Array *args = array_make(alc, 4);
    array_push(args, coro);
    array_push(args, fd);
    array_push(args, read);
    array_push(args, write);
    Value *fcall = vgen_func_call(alc, p, fptr, args);
    array_push(scope->ast, token_make(alc, t_statement, fcall));
}
