
#ifndef _H_UNIT
#define _H_UNIT

#include "typedefs.h"


struct Unit {
    Build *b;
    //
    char *path_o;
    char *path_ir;
    char *hash;
    //
    Array *funcs;
    Array *classes;
    Array *aliasses;
    Array *globals;
    //
    bool ir_changed;
    bool contains_main_func;
};

#endif