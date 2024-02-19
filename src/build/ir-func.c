
#include "../all.h"

void ir_gen_func(IR *ir, IRFunc *func);

void ir_gen_functions(IR* ir) {
    //
    Array* funcs = ir->fc->funcs;

    for (int i = 0; i < funcs->length; i++) {
        Func* vfunc = array_get_index(funcs, i);
        IRFunc* func = al(ir->alc, sizeof(IRFunc));
        func->ir = ir;
        func->func = vfunc;
        //
        func->blocks = array_make(ir->alc, 20);
        //
        func->stack_save_vn = NULL;
        func->di_scope = NULL;
        func->gc_stack = NULL;
        func->gc_stack_adr = NULL;
        func->gc_stack_adr_val = NULL;
        //
        func->var_count = 0;
        func->gc_count = 0;

        IRBlock* start = ir_block_make(ir, func, "start_");
        IRBlock* code = ir_block_make(ir, func, "code_");

        func->block_start = start;
        func->block_code = code;

        array_push(ir->funcs, func);

        ir_gen_func(ir, func);
    }
}

void ir_gen_func(IR *ir, IRFunc *func) {
    Allocator* alc = ir->alc;
    Build* b = ir->b;
    Func* vfunc = func->func;
    int gc_count = func_get_reserve_count(vfunc);
    func->gc_count = gc_count;

    Scope* gc_reserve = NULL;
    if(gc_count > 0) {
        Map *idfs = map_make(alc);
        Value *amount = vgen_int(alc, gc_count, type_gen_number(alc, b, b->ptr_size, false, false));
        Idf *idf = idf_make(alc, idf_value, amount);
        map_set(idfs, "amount", idf);
        Fc* fc = ir->fc;
        gc_reserve = gen_snippet_ast(alc, ir->fc, get_volt_snippet(ir->b, "mem", "reserve"), idfs, vfunc->scope);
    }

    // Arg vars
    // Array *args = vfunc->args->values;
    // for (int i = 0; i < args->length; i++) {
    //     FuncArg *arg = array_get_index(args, i);
    //     Decl* decl = arg->decl;
    //     char* var = ir_var(func);
    //     decl->ir_var = var;
    //     // if(decl->is_mut && !decl->is_gc) {
    //     //     decl->ir_store_var = ir_alloca(ir, func, decl->type);
    //     // }
    // }

    // Decls
    Scope* scope = vfunc->scope;
    Array* decls = scope->decls;
    for (int i = 0; i < decls->length; i++) {
        Decl* decl = array_get_index(decls, i);
        if(decl->is_arg) {
            char *var = ir_var(func);
            decl->ir_var = var;
        }
        if(decl->is_mut && !decl->is_gc) {
            decl->ir_store_var = ir_alloca(ir, func, decl->type);
        }
    }

    ir->func = func;
    ir->block = func->block_code;

    if(ir->b->verbose > 2)
        printf("> Func IR: %s\n", vfunc->export_name);

    // Start GC
    if(func->func == ir->b->func_main) {
        // Gc
        Func* gc_start = get_volt_class_func(b, "mem", "GcManager", "init");
        ir_func_call(ir, ir_func_ptr(ir, gc_start), NULL, ir_type(ir, gc_start->rett), 0, 0);
        Func* stack_new = get_volt_class_func(b, "mem", "Stack", "init");
        char* stack_ob = ir_func_call(ir, ir_func_ptr(ir, stack_new), NULL, ir_type(ir, stack_new->rett), 0, 0);
    }

    // Set decl IR values
    int gc_index = 0;
    char* stack_adr = NULL;

    // GC reserve
    if(gc_reserve) {
        ir_write_ast(ir, gc_reserve);
        Idf *idf = map_get(gc_reserve->identifiers, "STACK_ADR");
        Value *vc = idf->item;
        stack_adr = ir_value(ir, scope, vc);
    }
    // if(gc_reserve) {
    //     Global* g = get_volt_global(ir->b, "mem", "stack");
    //     char *stack_ = ir_global(ir, g);
    //     char *stack = ir_load(ir, g->type, stack_);
    //     Class *class = get_volt_class(ir->b, "mem", "Stack");
    //     ClassProp *prop = map_get(class->props, "stack_adr");
    //     char *pa = ir_class_pa(ir, class, stack, prop);
    //     stack_adr = ir_load(ir, prop->type, pa);
    // }

    for (int i = 0; i < decls->length; i++) {
        Decl* decl = array_get_index(decls, i);
        Str *code = ir->block->code;
        if(decl->is_gc) {
            str_preserve(ir->block->code, 256);

            char lindex[10];
            itoa(gc_index, lindex, 10);
            gc_index += 2;
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = getelementptr inbounds ptr, ptr ");
            str_add(code, stack_adr);
            str_flat(code, ", i32 ");
            str_add(code, lindex);
            str_flat(code, "\n");

            decl->ir_store_var = var;
        }
        if (decl->is_arg && decl->is_mut) {
            ir_store(ir, decl->ir_store_var, decl->ir_var, ir_type(ir, decl->type), decl->type->size);

            // Check if moved
            // if(decl->is_gc) {
            //     Type* type_u8 = type_gen_volt(ir->alc, ir->b, "u8");
            //     char* state_ = ir_ptrv(ir, decl->ir_var, "i8", 0);
            //     char* state = ir_load(ir, type_u8, state_);
            //     char* op = ir_op(ir, scope, op_bit_and, state, "1", type_u8);
            //     char* comp = ir_compare(ir, op_eq, op, "1", "i8", false, false);
            //     IRBlock *block_if = ir_block_make(ir, ir->func, "if_moved_");
            //     IRBlock *after = ir_block_make(ir, ir->func, "if_moved_after_");
            //     ir_cond_jump(ir, comp, block_if, after);

            //     ir->block = block_if;
            //     Type* type_ptr = type_gen_volt(ir->alc, ir->b, "ptr");
            //     char* new_ = ir_ptrv(ir, decl->ir_var, "ptr", 0);
            //     char* new = ir_load(ir, type_ptr, new_);
            //     ir_store(ir, decl->ir_store_var, new, ir_type(ir, decl->type), decl->type->size);
            //     ir_jump(ir, after);

            //     ir->block = after;
            // }
        }
    }

    // Store arg values
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

void ir_func_definition(Str* code, IR* ir, Func *vfunc, bool is_extern) {

    Array *args = vfunc->args->values;
    int argc = args->length;

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
            str_add(code, arg->decl->ir_var);
        }
    }
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
    if (func->fc == ir->fc)
        return;
    if(!array_contains(ir->declared_funcs, func, arr_find_adr)) {
        Str *code = ir->code_extern;
        ir_func_definition(code, ir, func, true);
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

char *ir_alloca_by_size(IR *ir, IRFunc* func, char* size) {
    IRBlock *block = func->block_start;
    Str *code = block->code;

    char *var = ir_var(func);
    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = alloca i8, i32 ");
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
    if(func->gc_count > 0) {
        Allocator* alc = ir->alc;
        Build* b = ir->b;
        Func* vfunc = func->func;
        if(type && type_is_gc(vfunc->rett)) {
            Map *idfs = map_make(alc);
            Value *retv = value_make(alc, v_ir_value, value, vfunc->rett);
            Idf *idf = idf_make(alc, idf_value, retv);
            map_set(idfs, "retv", idf);

            Scope* pop = gen_snippet_ast(alc, ir->fc, get_volt_snippet(ir->b, "mem", "pop_return"), idfs, vfunc->scope);
            ir_write_ast(ir, pop);
        } else {
            Scope* pop = gen_snippet_ast(alc, ir->fc, get_volt_snippet(ir->b, "mem", "pop_no_return"), map_make(alc), vfunc->scope);
            ir_write_ast(ir, pop);
        }
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
