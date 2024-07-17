
#ifndef _H_UNIT
#define _H_UNIT

#include "typedefs.h"

void unit_validate(Unit *u, Parser *p);
void unit_load_cache(Unit* u);
void validate_class(Parser *p, Class* class);
// Cache
int unit_mod_time(Unit* u);
void unit_set_mod_time(Unit* u, int time);
int unit_filecount(Unit* u);
void unit_set_filecount(Unit* u, int time);
bool unit_intern_changed(Unit* u);
bool unit_extern_changed(Unit* u);
void unit_update_cache(Unit* u);
void unit_gen_dep_hash(Unit* u);

struct Unit {
    Build *b;
    Nsc *nsc;
    //
    char *path_o;
    char *path_ir;
    char *path_cache;
    char *hash;
    char *unique_hash;
    //
    char *ir_start;
    char *ir_end;
    Array *func_irs;
    IR* ir;
    //
    Parser *parser;
    //
    Array *funcs;
    Array *classes;
    Array *aliasses;
    Array *globals;
    Array *tests;
    // Pools
    Array *pool_parsers;
    //
    cJSON *cache;
    Array *nsc_deps;
    char *nsc_deps_hash;
    int c_modtime;
    int c_filecount;
    //
    int id;
    int string_count;
    int export_count;
    //
    bool ir_changed;
    bool is_main;
    bool changed;
};

#endif