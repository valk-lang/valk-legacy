
#include "../all.h"

void ir_gen_func(IR *ir, IRFunc *func);
// char* ir_decl_store_var(IR* ir, IRFunc* func, Decl* decl);
void ir_write_async_func_start(IR *ir, IRFunc *func);
void ir_init_decls(IR *ir, IRFunc *func);

void ir_gen_ir_for_func(IR *ir, Func *vfunc) {

    Build* b = ir->b;
    Unit* u = ir->unit;

    if (b->verbose > 2)
        printf("Stage 4 | Generate function IR: %s\n", vfunc->export_name);

    IRFunc *func = al(ir->alc, sizeof(IRFunc));
    func->ir = ir;
    func->func = vfunc;
    //
    func->blocks = array_make(ir->alc, 20);
    //
    func->stack_save_vn = NULL;
    func->di_scope = NULL;
    // func->gc_stack = NULL;
    // func->gc_stack_adr = NULL;
    // func->gc_stack_adr_val = NULL;
    //
    func->var_count = 0;
    func->gc_count = 0;
    func->rett_refs = vfunc->multi_rett ? array_make(ir->alc, 4) : NULL;
    // Coro
    // func->var_coro = NULL;
    // func->var_alloca_stack = NULL;
    // func->var_stack_adr = NULL;
    // func->var_g_stack = NULL;
    // func->var_g_stack_adr = NULL;
    // func->var_g_stack_adr_ref = NULL;

    IRBlock *start = ir_block_make(ir, func, "start_");
    IRBlock *code_block = ir_block_make(ir, func, "code_");

    func->block_start = start;
    func->block_code = code_block;

    array_push(ir->funcs, func);

    // Generate IR
    ir_gen_func(ir, func);

    // Collect final IR
    Str* code = b->str_buf;
    str_clear(code);
    ir_func_definition(code, ir, func->func, false, func->rett_refs);
    // Blocks
    if (func->block_code->code->length > 0) {
        for (int o = 0; o < func->blocks->length; o++) {
            str_preserve(code, 1000);
            IRBlock *block = array_get_index(func->blocks, o);
            str_preserve(code, block->code->length + 100);
            str_add(code, block->name);
            str_flat(code, ":\n");
            str_append(code, block->code);
        }
    }
    //
    str_flat(code, "}\n\n");

    IRFuncIR *irf = al(b->alc, sizeof(IRFuncIR));
    irf->func = func->func;
    irf->ir = str_to_chars(b->alc, code);
    array_push(u->func_irs, irf);
}

void ir_gen_func(IR *ir, IRFunc *func) {
    Allocator* alc = ir->alc;
    Build* b = ir->b;
    Func* vfunc = func->func;

    ir->func = func;
    ir->block = func->block_code;

    Scope* init = vfunc->ast_stack_init;
    if(init)
        ir_write_ast(ir, init);

    // Load stack refs
    // if (vfunc->alloca_size > 0)
    //     func->var_alloca_stack = ir_alloca_by_size(ir, func, "i32", ir_int(ir, func->func->alloca_size));
    // if (vfunc->gc_decl_count > 0) {
    //     VIRCached* c = vfunc->v_cache_stack_pos->item;
    //     func->var_stack_adr = c->ir_value;
    // }
    ir_init_decls(ir, func);

    // Store arg values
    // Array *args = vfunc->args->values;
    // for (int i = 0; i < args->length; i++) {
    //     FuncArg *arg = array_get_index(args, i);
    //     Decl* decl = arg->decl;
    //     if(decl->is_mut) {
    //         // Store passed argument in storage var
    //         ir_store_old(ir, decl->type, decl->ir_store_var, decl->ir_var);
    //     }
    // }

    // AST
    ir_write_ast(ir, vfunc->scope);
    //
    if (!vfunc->scope->did_return) {
        ir_func_return(ir, NULL, "void");
    }

    // Jump from start to code block
    ir->block = func->block_start;
    ir_jump(ir, func->block_code);

    ir->func = NULL;
    ir->block = NULL;
}

void ir_func_definition(Str* code, IR* ir, Func *vfunc, bool is_extern, Array* rett_refs) {

    Array *args = vfunc->args->values;
    int argc = args->length;

    str_preserve(code, 1000);

    if (is_extern) {
        str_flat(code, "declare ");
    } else {
        str_flat(code, "define dso_local ");
    }
    str_add(code, ir_type(ir, vfunc->rett));
    str_flat(code, " @");
    str_add(code, vfunc->export_name);
    str_flat(code, "(");

    // Args
    for (int i = 0; i < argc; i++) {
        FuncArg *arg = array_get_index(args, i);
        str_preserve(code, 1000);
        if (i > 0)
            str_flat(code, ", ");
        str_add(code, ir_type(ir, arg->type));
        str_flat(code, " noundef");

        Type *type = arg->type;
        bool notnull = type->type == type_struct && type->is_pointer && type->nullable == false;
        if (notnull)
            str_flat(code, " nonnull");

        if(!is_extern) {
            str_flat(code, " ");
            str_add(code, ir_arg_nr(ir, arg->decl->arg_nr));
        }
    }
    // Return value references
    Array *retts = vfunc->rett_types;
    for (int i = 1; i < retts->length; i++) {
        str_preserve(code, 1000);
        if (i > 1 || argc > 0)
            str_flat(code, ", ");
        str_flat(code, "ptr noundef");
        if (!is_extern) {
            str_flat(code, " ");
            str_add(code, array_get_index(rett_refs, i - 1));
        }
    }
    //
    if (is_extern) {
        str_flat(code, ")\n");
    }else {
        str_flat(code, ")");
        if (vfunc->is_inline)
            str_flat(code, " alwaysinline");

        str_flat(code, " {\n");
    }
}

void ir_define_ext_func(IR* ir, Func* func) {
    if (!func)
        return;
    if (func->unit == ir->unit)
        return;
    if(!array_contains(ir->declared_funcs, func, arr_find_adr)) {
        Str *code = ir->code_extern;
        ir_func_definition(code, ir, func, true, NULL);
        array_push(ir->declared_funcs, func);
    }
}

char *ir_alloca(IR *ir, IRFunc* func, Type *type) {
    IRBlock *block = func->block_start;
    Str *code = block->code;

    str_preserve(code, 200);

    char bytes[20];

    char *var = ir_var(func);
    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = alloca ");
    str_add(code, ir_type(ir, type));
    str_flat(code, ", align ");
    str_add(code, ir_type_align(ir, type, bytes));
    str_flat(code, "\n");

    return var;
}

char *ir_alloca_by_size(IR *ir, IRFunc* func, char* type, char* size) {
    IRBlock *block = func->block_start;
    Str *code = block->code;

    char *var = ir_var(func);
    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = alloca i8, ");
    str_add(code, type);
    str_flat(code, " ");
    str_add(code, size);
    str_flat(code, ", align 8\n");

    return var;
}

void ir_func_return_nothing(IR* ir) {
    Func* func = ir->func->func;
    if(!func->rett || type_is_void(func->rett)) {
        ir_func_return(ir, NULL, "void");
    } else {
        if(func->rett->is_pointer) {
            ir_func_return(ir, "ptr", "null");
        } else {
            char* type = ir_type(ir, func->rett);
            ir_func_return(ir, type, "0");
        }
    }
}

void ir_func_return(IR* ir, char* type, char* value) {

    IRFunc* func = ir->func;
    Func* vfunc = func->func;
    if(vfunc->ast_end) {
        ir_write_ast(ir, vfunc->ast_end);
    }
    
    Str* code = ir->block->code;
    str_flat(code, "  ret ");
    if(type) {
        str_add(code, type);
        str_flat(code, " ");
    }
    str_add(code, value);
    str_flat(code, "\n");
}

// char* ir_decl_store_var(IR* ir, IRFunc* func, Decl* decl) {
    // if(decl->is_gc) {
    //     char* stack = ir_value(ir, func->func->v_cache_stack_pos);
    //     return ir_ptr_offset(ir, stack, ir_int(ir, decl->gc_offset), "i32", ir->b->ptr_size);
    // }
    // char* stack = func->var_alloca_stack;
    // return ir_ptr_offset(ir, stack, ir_int(ir, decl->offset), "i8", 1);
// }

void ir_init_decls(IR *ir, IRFunc *func) {
    Func* vfunc = func->func;

    Scope* scope = vfunc->scope;
    Array* decls = scope->decls;
    char *stack = ir_value(ir, func->func->v_cache_stack_pos);

    for (int i = 0; i < decls->length; i++) {
        Decl* decl = array_get_index(decls, i);
        if(decl->is_gc) {
            char* offset = ir_ptr_offset(ir, stack, ir_int(ir, decl->gc_offset), "i32", ir->b->ptr_size);
            // ir_decl_set(ir, decl, offset);
            decl->custom_ir_name = offset;
        } else {
            if(decl->is_mut) {
                decl->custom_ir_name = ir_alloca(ir, func, decl->type);
                // ir_decl_set(ir, decl, ir_alloca(ir, func, decl->type));
            }
        }
        if(decl->is_arg) {
            char *arg = ir_arg_nr(ir, decl->arg_nr);
            if(decl->is_mut) {
                ir_store(ir, ir_decl_var(ir, decl), arg, ir_type(ir, decl->type), decl->type->size);
            } else {
                decl->custom_ir_name = arg;
                // ir_decl_set(ir, decl, arg);
            }
        }
    }

    // Return value references
    Array *retts = vfunc->rett_types;
    for (int i = 1; i < retts->length; i++) {
        char *var = ir_var(func);
        array_push(func->rett_refs, var);
    }

    // Array *args = vfunc->args->values;
    // Str* code = ir->block->code;
    // for (int i = 0; i < args->length; i++) {
    //     FuncArg *arg = array_get_index(args, i);
    //     Decl* decl = arg->decl;

    //     char* var = ir_var(func);
    //     decl->ir_var = var;
    //     if(decl->is_mut) {
    //         decl->ir_store_var = ir_decl_var(ir, decl);
    //     }
    // }

    // // Return value references
    // Array *retts = vfunc->rett_types;
    // for (int i = 1; i < retts->length; i++) {
    //     char *var = ir_var(func);
    //     array_push(func->rett_refs, var);
    // }

    // // Decls
    // Scope* scope = vfunc->scope;
    // Array* decls = scope->decls;
    // for (int i = 0; i < decls->length; i++) {
    //     Decl* decl = array_get_index(decls, i);
    //     if(decl->is_mut || decl->is_gc) {
    //         decl->ir_store_var = ir_decl_store_var(ir, func, decl);
    //     }
    // }
}
