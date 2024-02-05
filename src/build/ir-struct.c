
#include "../all.h"

void ir_define_struct(IR *ir, Class* class) {
    if (!array_contains(ir->declared_classes, class, arr_find_adr)) {
        char name[256];
        sprintf(name, "%%struct.%s", class->ir_name);

        array_push(ir->declared_classes, class);

        Str *code = str_make(ir->alc, 1000);
        str_append_chars(code, name);
        str_append_chars(code, " = type ");
        str_append_chars(code, class->packed ? "<{ " : "{ ");

        for (int i = 0; i < class->props->keys->length; i++) {
            ClassProp *prop = array_get_index(class->props->values, i);
            if (i > 0) {
                str_append_chars(code, ", ");
            }
            str_append_chars(code, ir_type(ir, prop->type));
        }
        str_append_chars(code, class->packed ? " }>\n" : " }\n");

        str_append(ir->code_struct, code);
    }
}