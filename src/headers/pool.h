
#ifndef _H_POOL
#define _H_POOL

#include "typedefs.h"

Parser* pool_get_parser(Unit* u);
void pool_return_parser(Unit* u, Parser* p);

#endif