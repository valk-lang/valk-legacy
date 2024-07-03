
#include "../all.h"

void ir_gen_final(IR* ir) {
    //
    Unit* u = ir->unit;
    Build* b = u->b;
    Str* code = b->str_buf;
    str_clear(code);

    // Structs & Globals
    str_append(code, ir->code_struct);
    str_append_chars(code, "\n");
    str_append(code, ir->code_global);
    str_append_chars(code, "\n\n");

    u->ir_start = str_to_chars(b->alc, code);
    str_clear(code);

    // Extern
    str_append(code, ir->code_extern);
    str_append_chars(code, "\n");

    str_append_chars(code, "declare ptr @llvm.frameaddress(i32) nocallback nofree nosync nounwind readnone willreturn\n");
    str_append_chars(code, "declare ptr @llvm.stacksave() nounwind\n");
    str_append_chars(code, "declare i32 @llvm.eh.sjlj.setjmp(ptr) nounwind returns_twice\n");
    str_append_chars(code, "declare void @llvm.eh.sjlj.longjmp(ptr) nounwind\n");
    str_append_chars(code, "declare void @_setjmp(ptr) nounwind\n");
    str_append_chars(code, "declare void @longjmp(ptr, i32) nounwind\n");
    str_append_chars(code, "declare void @llvm.memset.inline.p0.p0.i64(ptr, i8, i64, i1)\n");
    str_append_chars(code, "declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)\n");
    str_append_chars(code, "\n");

    for(int i = 8; i <= 64; i *= 2) {
        char type[8];
        sprintf(type, "i%d", i);
        str_flat(code, "declare ");
        str_add(code, type);
        str_flat(code, " @llvm.ctlz.");
        str_add(code, type);
        str_flat(code, " (");
        str_add(code, type);
        str_flat(code, ", i1)\n");
    }

    // Attrs
    loop(ir->attrs, i) {
        str_append_chars(code, array_get_index(ir->attrs, i));
        str_append_chars(code, "\n");
    }
    str_append(code, ir->code_attr);
    str_append_chars(code, "\n");

    u->ir_end = str_to_chars(u->b->alc, code);
}
