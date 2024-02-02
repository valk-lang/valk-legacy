
#ifndef _H_BUILD
#define _H_BUILD

#include "typedefs.h"

int cmd_build(int argc, char *argv[]);
void build_err(Build* b, char* msg);
void parse_err(Build *b, Chunk *chunk, char *msg);

Pkc* pkc_make(Allocator* alc, Build* b, char* name_suggestion);
void pkc_set_dir(Pkc* pkc, char* dir);
Pkc* pkc_load_pkc(Pkc* pkc, char* name, Chunk* parsing_chunk);

Nsc* nsc_make(Allocator* alc, Pkc* pkc, char* name, char* dir);
Nsc* nsc_try_load(Pkc* pkc, char* name);

int fc_make(Allocator* alc, char* path);

struct Build {
    Allocator* alc;
    Map* pkc_by_dir;
    Array* used_pkc_names;
    char* char_buf;
};
struct Fc {
    Build* b;
    char* path;
    Allocator* alc;
    Allocator* alc_ast;
    Nsc* nsc;
    //
    Chunk* content;
    Chunk* chunk_parse;
    //
    bool is_header;
};
struct Nsc {
    char* name;
    char* dir;
    Pkc* pkc;
};
struct Pkc {
    Build* b;
    char* dir;
    char* name;
    Map* namespaces;
    Map* pkc_by_name;
    bool is_main;
};
struct Chunk {
    Fc* fc;
    Chunk* parent;
    char* tokens;
    char* content;
    int length;
    int i;
    int line;
    int col;
};

#endif