
#ifndef _H_BUILD
#define _H_BUILD

#include "typedefs.h"

int cmd_build(int argc, char *argv[]);
void build_err(Build *b, char *msg);
void parse_err(Chunk *chunk, char *msg);

Pkc *pkc_make(Allocator *alc, Build *b, char *name_suggestion);
void pkc_set_dir(Pkc *pkc, char *dir);
Pkc *pkc_load_pkc(Pkc *pkc, char *name, Chunk *parsing_chunk);
Fc *pkc_load_header(Pkc *pkc, char *fn, Chunk *chunk);

Nsc *nsc_make(Allocator *alc, Pkc *pkc, char *name, char *dir);
Nsc *nsc_load(Pkc *pkc, char *name, bool must_exist, Chunk* chunk);
Nsc *get_volt_nsc(Build *b, char *name);

Fc *fc_make(Nsc *nsc, char *path);

// Chunks
Chunk *chunk_make(Allocator *alc, Build *b, Fc *fc);
Chunk *chunk_clone(Allocator *alc, Chunk *ch);
void chunk_set_content(Chunk *chunk, char *content, int length);
void chunk_lex_start(Chunk *chunk);
void chunk_lex(Chunk *chunk, int err_token_i, int *err_content_i, int *err_line, int *err_col, int *err_col_end);
// Stage functions
void build_set_stages(Build *b);
void stage_add_item(Stage *stage, void *item);
void build_run_stages(Build *b);
// Stages
void stage_1_parse(Fc *fc);
void stage_2_alias(Fc *fc);
void stage_2_props(Fc *fc);
void stage_2_types(Fc *fc);
void stage_3_values(Fc *fc);
void stage_4_ast(Fc *fc);
void stage_4_ir(Fc *fc);
void stage_5_objects(Build *b);
void stage_6_link(Build* b, Array* o_files);
// Sub stages
void stage_props_class(Fc* fc, Class *class);
void stage_types_func(Fc* fc, Func *func);
void stage_types_class(Fc* fc, Class* class);
void read_ast(Fc *fc, Scope *scope, bool single_line);
//

struct Build {
    Allocator *alc;
    Allocator *alc_ast;
    //
    char *cache_dir;
    char *path_out;
    //
    char *os;
    char *arch;
    //
    char *char_buf;
    Str *str_buf;
    usize time_lex;
    usize time_parse;
    usize time_io;
    usize time_ir;
    usize time_llvm;
    usize time_link;
    Stage *stage_1_parse;
    Stage *stage_2_alias;
    Stage *stage_2_props;
    Stage *stage_2_types;
    Stage *stage_3_values;
    Stage *stage_4_ast;
    //
    Map *pkc_by_dir;
    Map *fc_by_path;
    Map *nsc_by_path;
    Array *used_pkc_names;
    Array *pkcs;
    //
    Pkc *pkc_main;
    Pkc *pkc_volt;
    Nsc *nsc_main;
    Func *func_main;
    //
    int ptr_size;
    int export_count;
    int verbose;
    int LOC;
};
struct Fc {
    Build *b;
    char *path;
    char *path_ir;
    char *path_cache;
    //
    Allocator *alc;
    Allocator *alc_ast;
    Nsc *nsc;
    Scope *scope;
    //
    Chunk *content;
    Chunk *chunk_parse;
    Chunk *chunk_parse_prev;
    //
    Array *funcs;
    Array *classes;
    Array *aliasses;
    Array *globals;
    //
    char *hash;
    //
    bool is_header;
    bool ir_changed;
};
struct Nsc {
    char *name;
    char *dir;
    char *path_o;
    Pkc *pkc;
    Scope *scope;
    Array *fcs;
};
struct Pkc {
    Build *b;
    char *dir;
    char *name;
    PkgConfig *config;
    Map *namespaces;
    Map *pkc_by_name;
    Array *header_dirs;
    Map *headers_by_fn;
};
struct Chunk {
    Build *b;
    Fc *fc;
    Allocator *alc;
    Chunk *parent;
    char *tokens;
    char *content;
    int length;
    int i;
    int line;
    int col;
    int scope_end_i;
    char token;
};
struct Stage {
    Array *items;
    void (*func)(void *);
    int i;
};
struct CompileData {
    Build* b;
    Array* ir_files;
    char* path_o;
    //
    void* target_machine;
    void* target_data;
    char* triple;
    char* data_layout;
};

#endif