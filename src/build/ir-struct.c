
#include "../all.h"

void ir_define_struct(IR *ir, Class* class) {
    if (!array_contains(ir->declared_classes, class, arr_find_adr)) {
        char name[256];
        strcpy(name, "\%struct.");
        strcpy(name + 8, class->ir_name);

        array_push(ir->declared_classes, class);

        Str *code = str_make(ir->alc, 800);
        str_add(code, name);
        str_flat(code, " = type ");
        str_add(code, class->packed ? "<{ " : "{ ");

        for (int i = 0; i < class->props->keys->length; i++) {
            str_preserve(code, 400);
            ClassProp *prop = array_get_index(class->props->values, i);
            if (i > 0) {
                str_flat(code, ", ");
            }
            str_add(code, ir_type(ir, prop->type));
        }
        str_add(code, class->packed ? " }>\n" : " }\n");

        str_append(ir->code_struct, code);
    }
}