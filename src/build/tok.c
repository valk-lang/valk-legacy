
#include "../all.h"

char* tok(Fc* fc, bool allow_space, bool allow_newline, bool update) {
    Chunk* ch = fc->chunk_parse;
    Chunk* prev = fc->chunk_parse_prev;
    if(update)
        *prev = *ch;
    return chunk_tok(ch, allow_space, allow_newline, update);
}
void tok_back(Fc* fc) {
    Chunk* ch = fc->chunk_parse;
    Chunk* prev = fc->chunk_parse_prev;
    *ch = *prev;
}
void tok_expect(Fc* fc, char* expect, bool allow_space, bool allow_newline) {
    char* tkn = tok(fc, allow_space, allow_newline, true);
    if(!tok_is(tkn, expect)) {
        sprintf(fc->b->char_buf, "Expected '%s' here", expect);
        parse_err(fc->chunk_parse, fc->b->char_buf);
    }
}

char* chunk_tok(Chunk* chunk, bool allow_space, bool allow_newline, bool update) {
    int x = chunk->i;
    char* res = chunk_read(chunk, &x);
    char t = chunk->token;
    if(t == tok_space) {
        if(!allow_space) {
            chunk->token = tok_none;
            return "";
        }
        res = chunk_read(chunk, &x);
        t = chunk->token;
    }
    if(t == tok_newline) {
        if(!allow_space || !allow_newline){
            chunk->token = tok_none;
            return "";
        }
        res = chunk_read(chunk, &x);
    }
    if(update) {
        chunk->i = x;
    }
    return res;
}

char* chunk_read(Chunk* chunk, int *i_ref) {
    //
    int i = i_ref ? *i_ref : chunk->i;
    char* tokens = chunk->tokens;
    char t = tokens[i++];
    if(t == tok_pos) {
        int line = *(int*)(&tokens[i]);
        i += sizeof(int);
        int col = *(int*)(&tokens[i]);
        i += sizeof(int);
        chunk->line = line;
        chunk->col = col;
        t = tokens[i++];
    }
    if(t == tok_scope_open) {
        chunk->scope_end_i = *(int*)(&tokens[i]);
        i += sizeof(int);
    }
    char* tchars = (char*)(&tokens[i]);
    if(t != tok_eof && i_ref) {
        while(tokens[i++] != 0) {
        }
        *i_ref = i;
    }
    chunk->token = t;
    return tchars;
}

bool tok_is(char* tkn, char* comp) {
    int i = 0;
    while(true) {
        const char ch = tkn[i];
        if(ch != comp[i++])
            return false;
        if(ch == 0)
            break;
    }
    return true;
}
