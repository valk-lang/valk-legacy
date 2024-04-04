
#ifndef H_MACRO
#define H_MACRO

#include "typedefs.h"

void macro_parse(Allocator* alc, Macro* m, Parser* p);
Value* macro_read_value(Allocator* alc, Macro* m, Parser* p);

struct Macro {
    Array* patterns;
    Chunk* body;
};
struct MacroPattern {
    char* open_tkn;
    char* close_tkn;
    Array* items;
    Array* names_used;
};
struct MacroPatternItem {
    void* item;
    char* name;
    int type;
};
struct MacroRepeat {
    char* delimiter;
    MacroPattern* pattern;
};
struct MacroItem {
    Map* identifiers;
};

enum MACRO_PATTERNS {
    pat_type,
    pat_value,
    pat_tkn,
    pat_repeat,
};

#endif
