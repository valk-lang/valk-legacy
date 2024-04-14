
#include "../all.h"

char* ir_await(IR* ir, Scope* scope, VAwait* aw) {
    Func* vfunc = ir->func->func;
    // Add suspend point
    // x = coro.done
    // if x == false -> jump suspend block
    // else -> result block

    // Run/save the on value
    if(!aw->on_decl) {
        char* on = ir_value(ir, scope, aw->on);
        ir_store(ir, aw->decl->ir_store_var, on, "ptr", ir->b->ptr_size);
    }
    // Resume block
    IRBlock* resume = aw->block;
    ir_jump(ir, resume);
    ir->block = resume;

    Class* coro_class = get_valk_class(ir->b, "core", "Coro");
    char* coro = ir_load(ir, type_gen_valk(ir->alc, ir->b, "ptr"), aw->decl->ir_store_var);
    // Check if coro is done
    char *is_done = ir_load(ir, type_gen_valk(ir->alc, ir->b, "bool"), ir_class_pa(ir, coro_class, coro, map_get(coro_class->props, "done")));
    IRBlock *suspend = ir_block_make(ir, ir->func, "coro_suspend_");
    IRBlock *done = ir_block_make(ir, ir->func, "coro_done_");
    ir_cond_jump(ir, is_done, done, suspend);

    // If suspend
    ir->block = suspend;
    ir_store(ir, ir_class_pa(ir, coro_class, coro, map_get(coro_class->props, "resume_index")), ir_int(ir, aw->suspend_index), "i32", 4);
    ir_func_return(ir, "ptr", ir->func->var_coro);

    // If done
    // Load result
    ir->block = done;
    char* result_ref = ir_class_pa(ir, coro_class, coro, map_get(coro_class->props, "result"));
    char* result = ir_load(ir, aw->rett, result_ref);
    return result;
}

void ir_coro_return(IR* ir, Value* value) {
    IRFunc* func = ir->func;
    Class* coro_class = get_valk_class(ir->b, "core", "Coro");

    char *coro = func->var_coro;

    // Set return value if any
    if(value) {
        char *result_ref = ir_class_pa(ir, coro_class, coro, map_get(coro_class->props, "result"));
        char* result = ir_value(ir, func->func->scope, value);
        Type *rett = func->func->rett;
        ir_store(ir, result_ref, result, ir_type(ir, rett), rett->size);
    }

    char *done_ref = ir_class_pa(ir, coro_class, coro, map_get(coro_class->props, "done"));
    ir_store(ir, done_ref, "true", "i1", 1);

    ir_func_return(ir, "ptr", coro);
}
