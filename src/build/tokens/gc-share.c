
#include "../../all.h"

void pt_gc_share(Build* b, Allocator* alc, Parser* p) {

    Scope* scope = p->scope;

    tok_expect(p, "(", false, false);
    Value *v = read_value(alc, p, true, 0);
    Type *rett = v->rett;
    if (!rett->class || (rett->class->type != ct_class && rett->class->type != ct_ptr) || !rett->is_pointer) {
        parse_err(p, -1, "Invalid @gc_share value. It must be a pointer to an instance of a class");
    }
    tok_expect(p, ")", true, true);

    Func *share = get_valk_func(b, "mem", "gc_share");
    func_mark_used(p->func, share);
    Array *args = array_make(alc, 2);
    array_push(args, v);
    Value *on = vgen_func_ptr(alc, share, NULL);
    Value *fcall = vgen_func_call(alc, p, on, args);
    array_push(scope->ast, token_make(alc, t_statement, fcall));
}
