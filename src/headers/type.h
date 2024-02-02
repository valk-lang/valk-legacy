
#ifndef _H_TYPE
#define _H_TYPE

#include "typedefs.h"

Type* read_type(Fc* fc, bool sameline);

struct Type {
    int type;
};

#endif
